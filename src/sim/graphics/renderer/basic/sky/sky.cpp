#include "sky.h"
#include "constants.h"
#include "sim/graphics/base/pipeline/render_pass.h"
#include "sim/graphics/base/pipeline/pipeline.h"
#include "sim/graphics/base/pipeline/descriptors.h"
#include "sim/graphics/compiledShaders/basic/quad_vert.h"
#include "sim/graphics/compiledShaders/basic/sky/computeTransmittance_frag.h"

namespace sim::graphics::renderer::basic {

namespace {
constexpr int kLambdaMin = 360;
constexpr int kLambdaMax = 830;

double CieColorMatchingFunctionTableValue(double wavelength, int column) {
  if(wavelength <= kLambdaMin || wavelength >= kLambdaMax) { return 0.0; }
  double u = (wavelength - kLambdaMin) / 5.0;
  int row = static_cast<int>(std::floor(u));
  assert(row >= 0 && row + 1 < 95);
  assert(
    CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * row] <= wavelength &&
    CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * (row + 1)] >= wavelength);
  u -= row;
  return CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * row + column] * (1.0 - u) +
         CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * (row + 1) + column] * u;
}

double Interpolate(
  const std::vector<double> &wavelengths, const std::vector<double> &wavelength_function,
  double wavelength) {
  assert(wavelength_function.size() == wavelengths.size());
  if(wavelength < wavelengths[0]) { return wavelength_function[0]; }
  for(unsigned int i = 0; i < wavelengths.size() - 1; ++i) {
    if(wavelength < wavelengths[i + 1]) {
      double u = (wavelength - wavelengths[i]) / (wavelengths[i + 1] - wavelengths[i]);
      return wavelength_function[i] * (1.0 - u) + wavelength_function[i + 1] * u;
    }
  }
  return wavelength_function[wavelength_function.size() - 1];
}

/*
<p>We can then implement a utility function to compute the "spectral radiance to
luminance" conversion constants (see Section 14.3 in <a
href="https://arxiv.org/pdf/1612.04336.pdf">A Qualitative and Quantitative
Evaluation of 8 Clear Sky Models</a> for their definitions):
*/

// The returned constants are in lumen.nm / watt.
void ComputeSpectralRadianceToLuminanceFactors(
  const std::vector<double> &wavelengths, const std::vector<double> &solar_irradiance,
  double lambda_power, double *k_r, double *k_g, double *k_b) {
  *k_r = 0.0;
  *k_g = 0.0;
  *k_b = 0.0;
  double solar_r = Interpolate(wavelengths, solar_irradiance, SkyModel::kLambdaR);
  double solar_g = Interpolate(wavelengths, solar_irradiance, SkyModel::kLambdaG);
  double solar_b = Interpolate(wavelengths, solar_irradiance, SkyModel::kLambdaB);
  int dlambda = 1;
  for(int lambda = kLambdaMin; lambda < kLambdaMax; lambda += dlambda) {
    double x_bar = CieColorMatchingFunctionTableValue(lambda, 1);
    double y_bar = CieColorMatchingFunctionTableValue(lambda, 2);
    double z_bar = CieColorMatchingFunctionTableValue(lambda, 3);
    const double *xyz2srgb = XYZ_TO_SRGB;
    double r_bar = xyz2srgb[0] * x_bar + xyz2srgb[1] * y_bar + xyz2srgb[2] * z_bar;
    double g_bar = xyz2srgb[3] * x_bar + xyz2srgb[4] * y_bar + xyz2srgb[5] * z_bar;
    double b_bar = xyz2srgb[6] * x_bar + xyz2srgb[7] * y_bar + xyz2srgb[8] * z_bar;
    double irradiance = Interpolate(wavelengths, solar_irradiance, lambda);
    *k_r += r_bar * irradiance / solar_r * pow(lambda / SkyModel::kLambdaR, lambda_power);
    *k_g += g_bar * irradiance / solar_g * pow(lambda / SkyModel::kLambdaG, lambda_power);
    *k_b += b_bar * irradiance / solar_b * pow(lambda / SkyModel::kLambdaB, lambda_power);
  }
  *k_r *= MAX_LUMINOUS_EFFICACY * dlambda;
  *k_g *= MAX_LUMINOUS_EFFICACY * dlambda;
  *k_b *= MAX_LUMINOUS_EFFICACY * dlambda;
}

}

SkyModel::SkyModel(
  Device &device, const std::vector<double> &wavelengths,
  const std::vector<double> &solar_irradiance, double sun_angular_radius,
  double bottom_radius, double top_radius,
  const std::vector<DensityProfileLayer> &rayleigh_density,
  const std::vector<double> &rayleigh_scattering,
  const std::vector<DensityProfileLayer> &mie_density,
  const std::vector<double> &mie_scattering, const std::vector<double> &mie_extinction,
  double mie_phase_function_g, const std::vector<DensityProfileLayer> &absorption_density,
  const std::vector<double> &absorption_extinction,
  const std::vector<double> &ground_albedo, double max_sun_zenith_angle,
  double length_unit_in_meters, unsigned int num_precomputed_wavelengths,
  bool half_precision)
  : device{device},
    num_precomputed_wavelengths_(num_precomputed_wavelengths),
    half_precision_(half_precision) {
  // Compute the values for the SKY_RADIANCE_TO_LUMINANCE constant. In theory
  // this should be 1 in precomputed illuminance mode (because the precomputed
  // textures already contain illuminance values). In practice, however, storing
  // true illuminance values in half precision textures yields artefacts
  // (because the values are too large), so we store illuminance values divided
  // by MAX_LUMINOUS_EFFICACY instead. This is why, in precomputed illuminance
  // mode, we set SKY_RADIANCE_TO_LUMINANCE to MAX_LUMINOUS_EFFICACY.
  bool precompute_illuminance = num_precomputed_wavelengths > 3;
  double sky_k_r, sky_k_g, sky_k_b;
  if(precompute_illuminance) {
    sky_k_r = sky_k_g = sky_k_b = MAX_LUMINOUS_EFFICACY;
  } else {
    ComputeSpectralRadianceToLuminanceFactors(
      wavelengths, solar_irradiance, -3 /* lambda_power */, &sky_k_r, &sky_k_g, &sky_k_b);
  }
  // Compute the values for the SUN_RADIANCE_TO_LUMINANCE constant.
  double sun_k_r, sun_k_g, sun_k_b;
  ComputeSpectralRadianceToLuminanceFactors(
    wavelengths, solar_irradiance, 0 /* lambda_power */, &sun_k_r, &sun_k_g, &sun_k_b);

  transmittance_texture_ = u<Texture2D>(
    device, TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT,
    vk::Format::eR32G32B32A32Sfloat);
  scattering_texture_ = u<Texture3D>(
    device, SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH,
    half_precision ? vk::Format::eR16G16B16A16Sfloat : vk::Format::eR32G32B32A32Sfloat);
  irradiance_texture_ = u<Texture2D>(
    device, IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT,
    vk::Format::eR32G32B32A32Sfloat);
}

void SkyModel::Init(unsigned int num_scattering_orders) {
  auto delta_irradiance_texture = u<Texture2D>(
    device, TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT,
    vk::Format::eR32G32B32A32Sfloat);
  auto delta_rayleigh_scattering_texture = u<Texture3D>(
    device, SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH,
    half_precision_ ? vk::Format::eR16G16B16A16Sfloat : vk::Format::eR32G32B32A32Sfloat);
  auto delta_mie_scattering_texture = u<Texture3D>(
    device, SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH,
    half_precision_ ? vk::Format::eR16G16B16A16Sfloat : vk::Format::eR32G32B32A32Sfloat);
  auto delta_scattering_density_texture = u<Texture3D>(
    device, SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH,
    half_precision_ ? vk::Format::eR16G16B16A16Sfloat : vk::Format::eR32G32B32A32Sfloat);
  Texture3D &delta_multiple_scattering_texture = *delta_rayleigh_scattering_texture;

  if(num_precomputed_wavelengths_ <= 3) {
    vec3 lambdas{kLambdaR, kLambdaG, kLambdaB};
    mat3 luminance_from_radiance{1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0};
    Precompute(
      *delta_irradiance_texture, *delta_rayleigh_scattering_texture,
      *delta_mie_scattering_texture, *delta_scattering_density_texture,
      delta_multiple_scattering_texture, lambdas, luminance_from_radiance,
      false /* blend */, num_scattering_orders);
  } else {
    constexpr double _kLambdaMin = 360.0;
    constexpr double _kLambdaMax = 830.0;
    int num_iterations = (int(num_precomputed_wavelengths_) + 2) / 3;
    double dlambda = (_kLambdaMax - _kLambdaMin) / (3 * num_iterations);
    for(int i = 0; i < num_iterations; ++i) {
      vec3 lambdas{_kLambdaMin + (3 * i + 0.5) * dlambda,
                   _kLambdaMin + (3 * i + 1.5) * dlambda,
                   _kLambdaMin + (3 * i + 2.5) * dlambda};
      auto coeff = [dlambda](double lambda, int component) {
        // Note that we don't include MAX_LUMINOUS_EFFICACY here, to avoid
        // artefacts due to too large values when using half precision on GPU.
        // We add this term back in kAtmosphereShader, via
        // SKY_SPECTRAL_RADIANCE_TO_LUMINANCE (see also the comments in the
        // Model constructor).
        double x = CieColorMatchingFunctionTableValue(lambda, 1);
        double y = CieColorMatchingFunctionTableValue(lambda, 2);
        double z = CieColorMatchingFunctionTableValue(lambda, 3);
        return static_cast<float>(
          (XYZ_TO_SRGB[component * 3] * x + XYZ_TO_SRGB[component * 3 + 1] * y +
           XYZ_TO_SRGB[component * 3 + 2] * z) *
          dlambda);
      };
      mat3 luminance_from_radiance{
        coeff(lambdas[0], 0), coeff(lambdas[1], 0), coeff(lambdas[2], 0),
        coeff(lambdas[0], 1), coeff(lambdas[1], 1), coeff(lambdas[2], 1),
        coeff(lambdas[0], 2), coeff(lambdas[1], 2), coeff(lambdas[2], 2)};
      Precompute(
        *delta_irradiance_texture, *delta_rayleigh_scattering_texture,
        *delta_mie_scattering_texture, *delta_scattering_density_texture,
        delta_multiple_scattering_texture, lambdas, luminance_from_radiance,
        i > 0 /* blend */, num_scattering_orders);
    }
  }
}

void SkyModel::ConvertSpectrumToLinearSrgb(
  const std::vector<double> &wavelengths, const std::vector<double> &spectrum, double *r,
  double *g, double *b) {}

void SkyModel::Precompute(
  Texture2D &delta_irradiance_texture, Texture3D &delta_rayleigh_scattering_texture,
  Texture3D &delta_mie_scattering_texture, Texture3D &delta_scattering_density_texture,
  Texture3D &delta_multiple_scattering_texture, const SkyModel::vec3 &lambdas,
  const SkyModel::mat3 &luminance_from_radiance, bool blend,
  unsigned int num_scattering_orders) {

  computeTransmittance(*transmittance_texture_);
  computeDirectIrradiance(blend, delta_irradiance_texture, *irradiance_texture_);
  computeSingleScattering();
  computeScatteringDensity();
  computeIndirectIrradiance();
  computeMultipleScattering();
}

}
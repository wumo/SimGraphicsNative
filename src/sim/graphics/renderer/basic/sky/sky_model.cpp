#include "sky_model.h"
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

using imageUsage = vk::ImageUsageFlagBits;
using imageCreate = vk::ImageCreateFlagBits;
using address = vk::SamplerAddressMode;
using loadOp = vk::AttachmentLoadOp;
using storeOp = vk::AttachmentStoreOp;
using layout = vk::ImageLayout;
using bindpoint = vk::PipelineBindPoint;
using stage = vk::PipelineStageFlagBits;
using access = vk::AccessFlagBits;
using flag = vk::DescriptorBindingFlagBitsEXT;
using shader = vk::ShaderStageFlagBits;
using aspect = vk::ImageAspectFlagBits;

uPtr<Texture> newTexture2D(
  Device &device, uint32_t width, uint32_t height, vk::Format format) {
  auto texture = u<Texture>(
    device, vk::ImageCreateInfo{{},
                                vk::ImageType::e2D,
                                format,
                                {width, height, 1U},
                                1,
                                1,
                                vk::SampleCountFlagBits::e1,
                                vk::ImageTiling::eOptimal,
                                imageUsage::eSampled | imageUsage::eStorage});
  texture->createImageView(
    device.getDevice(), vk::ImageViewType::e2D, vk::ImageAspectFlagBits::eColor);
  SamplerMaker maker{};
  maker.addressModeU(vk::SamplerAddressMode::eClampToEdge)
    .addressModeV(vk::SamplerAddressMode::eClampToEdge)
    .addressModeW(vk::SamplerAddressMode::eClampToEdge);
  texture->setSampler(maker.createUnique(device.getDevice()));
  return texture;
}

uPtr<Texture> newTexture3D(
  Device &device, uint32_t width, uint32_t height, uint32_t depth, vk::Format format) {
  auto texture = u<Texture>(
    device, vk::ImageCreateInfo{{},
                                vk::ImageType::e3D,
                                format,
                                {width, height, depth},
                                1,
                                1,
                                vk::SampleCountFlagBits::e1,
                                vk::ImageTiling::eOptimal,
                                imageUsage::eSampled | imageUsage::eStorage});
  texture->createImageView(
    device.getDevice(), vk::ImageViewType::e3D, vk::ImageAspectFlagBits::eColor);
  SamplerMaker maker{};
  maker.addressModeU(vk::SamplerAddressMode::eClampToEdge)
    .addressModeV(vk::SamplerAddressMode::eClampToEdge)
    .addressModeW(vk::SamplerAddressMode::eClampToEdge);
  texture->setSampler(maker.createUnique(device.getDevice()));
  return texture;
}
}

SkyModel::SkyModel(
  Device &device, DebugMarker &debugMarker, const std::vector<double> &wavelengths,
  const std::vector<double> &solar_irradiance, double sun_angular_radius,
  double bottom_radius, double top_radius,
  const std::vector<DensityProfileLayer> &rayleigh_density,
  const std::vector<double> &rayleigh_scattering,
  const std::vector<DensityProfileLayer> &mie_density,
  const std::vector<double> &mie_scattering, const std::vector<double> &mie_extinction,
  double mie_phase_function_g, const std::vector<DensityProfileLayer> &absorption_density,
  const std::vector<double> &absorption_extinction,
  const std::vector<double> &ground_albedo, double max_sun_zenith_angle,
  float length_unit_in_meters, unsigned int num_precomputed_wavelengths,
  bool half_precision)
  : device{device},
    debugMarker{debugMarker},
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

  ubo.transmittance_texture_width = TRANSMITTANCE_TEXTURE_WIDTH;
  ubo.transmittance_texture_height = TRANSMITTANCE_TEXTURE_HEIGHT;
  ubo.scattering_texture_r_size = SCATTERING_TEXTURE_R_SIZE;
  ubo.scattering_texture_mu_size = SCATTERING_TEXTURE_MU_SIZE;
  ubo.scattering_texture_mu_s_size = SCATTERING_TEXTURE_MU_S_SIZE;
  ubo.scattering_texture_nu_size = SCATTERING_TEXTURE_NU_SIZE;
  ubo.irradiance_texture_width = IRRADIANCE_TEXTURE_WIDTH;
  ubo.irradiance_texture_height = IRRADIANCE_TEXTURE_HEIGHT;

  ubo.sky_spectral_radiance_to_luminance = {sky_k_r, sky_k_g, sky_k_b, 0.0};
  ubo.sun_spectral_radiance_to_luminance = {sun_k_r, sun_k_g, sun_k_b, 0.0};

  auto spectrum =
    [&wavelengths](const std::vector<double> &v, const glm::vec3 &lambdas, double scale) {
      double r = Interpolate(wavelengths, v, lambdas[0]) * scale;
      double g = Interpolate(wavelengths, v, lambdas[1]) * scale;
      double b = Interpolate(wavelengths, v, lambdas[2]) * scale;
      return glm::vec3{r, g, b};
    };

  auto density_layer = [length_unit_in_meters](const DensityProfileLayer &layer) {
    return DensityProfileLayer{layer.width / length_unit_in_meters, layer.exp_term,
                               layer.exp_scale * length_unit_in_meters,
                               layer.linear_term * length_unit_in_meters,
                               layer.constant_term};
  };

  auto density_profile = [density_layer](std::vector<DensityProfileLayer> layers) {
    constexpr int kLayerCount = 2;
    while(layers.size() < kLayerCount) {
      layers.insert(layers.begin(), DensityProfileLayer());
    }
    DensityProfile result{};
    for(int i = 0; i < kLayerCount; ++i) {
      result.layers[i] = density_layer(layers[i]);
    }
    return result;
  };

  calcAtmosphereParams = [=](const glm::vec3 &lambdas) {
    return AtmosphereParameters{
      spectrum(solar_irradiance, lambdas, 1.0),
      float(sun_angular_radius),
      spectrum(rayleigh_scattering, lambdas, length_unit_in_meters),
      float(bottom_radius / length_unit_in_meters),
      spectrum(mie_scattering, lambdas, length_unit_in_meters),
      float(top_radius / length_unit_in_meters),
      spectrum(mie_extinction, lambdas, length_unit_in_meters),
      float(mie_phase_function_g),
      spectrum(absorption_extinction, lambdas, length_unit_in_meters),
      float(std::cos(max_sun_zenith_angle)),
      spectrum(ground_albedo, lambdas, 1.0),
      0,
      density_profile(rayleigh_density),
      density_profile(mie_density),
      density_profile(absorption_density),
    };
  };

  uboBuffer = u<HostUniformBuffer>(device.allocator(), ubo);

  layerBuffer = u<HostUniformBuffer>(device.allocator(), sizeof(int));
  LFRUniformBuffer = u<HostUniformBuffer>(device.allocator(), sizeof(glm::mat3));

  transmittance_texture_ = newTexture2D(
    device, TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT,
    vk::Format::eR32G32B32A32Sfloat);

  scattering_texture_ = newTexture3D(
    device, SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH,
    half_precision ? vk::Format::eR16G16B16A16Sfloat : vk::Format::eR32G32B32A32Sfloat);

  irradiance_texture_ = newTexture2D(
    device, IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT,
    vk::Format::eR32G32B32A32Sfloat);

  debugMarker.name(layerBuffer->buffer(), "layerBuffer");
  debugMarker.name(LFRUniformBuffer->buffer(), "LFRUniformBuffer");
  debugMarker.name(uboBuffer->buffer(), "AtmosphereUniform");
  debugMarker.name(transmittance_texture_->image(), "transmittance_texture_");
  debugMarker.name(scattering_texture_->image(), "scattering_texture_");
  debugMarker.name(irradiance_texture_->image(), "irradiance_texture_");
}

void SkyModel::Init(unsigned int num_scattering_orders) {

  delta_irradiance_texture = newTexture2D(
    device, IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT,
    vk::Format::eR32G32B32A32Sfloat);

  delta_rayleigh_scattering_texture = newTexture3D(
    device, SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH,
    half_precision_ ? vk::Format::eR16G16B16A16Sfloat : vk::Format::eR32G32B32A32Sfloat);

  delta_mie_scattering_texture = newTexture3D(
    device, SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH,
    half_precision_ ? vk::Format::eR16G16B16A16Sfloat : vk::Format::eR32G32B32A32Sfloat);

  delta_scattering_density_texture = newTexture3D(
    device, SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH,
    half_precision_ ? vk::Format::eR16G16B16A16Sfloat : vk::Format::eR32G32B32A32Sfloat);

  Texture &delta_multiple_scattering_texture = *delta_rayleigh_scattering_texture;

  debugMarker.name(delta_irradiance_texture->image(), "delta_irradiance_texture");
  debugMarker.name(
    delta_rayleigh_scattering_texture->image(), "delta_rayleigh_scattering_texture");
  debugMarker.name(delta_mie_scattering_texture->image(), "delta_mie_scattering_texture");
  debugMarker.name(
    delta_scattering_density_texture->image(), "delta_scattering_density_texture");

  if(num_precomputed_wavelengths_ <= 3) {
    glm::vec3 lambdas{kLambdaR, kLambdaG, kLambdaB};
    glm::mat3 luminance_from_radiance{1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0};
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
      glm::vec3 lambdas{_kLambdaMin + (3 * i + 0.5) * dlambda,
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
      glm::mat3 luminance_from_radiance{
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
  Texture &deltaIrradianceTexture, Texture &deltaRayleighScatteringTexture,
  Texture &deltaMieScatteringTexture, Texture &deltaScatteringDensityTexture,
  Texture &deltaMultipleScatteringTexture, const glm::vec3 &lambdas,
  const glm::mat3 &luminance_from_radiance, bool blend,
  unsigned int num_scattering_orders) {

  ubo.atmosphere = calcAtmosphereParams(lambdas);
  uboBuffer->updateSingle(ubo);

  computeTransmittance(*transmittance_texture_);

  computeDirectIrradiance(
    blend, deltaIrradianceTexture, *irradiance_texture_, *transmittance_texture_);

  LFRUniformBuffer->updateSingle(luminance_from_radiance);
  computeSingleScattering(
    blend, deltaRayleighScatteringTexture, deltaMieScatteringTexture,
    *scattering_texture_, *transmittance_texture_);
  //  computeScatteringDensity();
  //  computeIndirectIrradiance();
  //  computeMultipleScattering();
}

}
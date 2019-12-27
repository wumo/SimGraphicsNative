#pragma once
#include "sim/graphics/base/device.h"
#include "sim/graphics/base/debug_marker.h"
#include "sim/graphics/base/resource/images.h"
#include "sim/graphics/base/resource/buffers.h"

namespace sim::graphics::renderer::basic {
// An atmosphere layer of width 'width' (in m), and whose density is defined as
//   'exp_term' * exp('exp_scale' * h) + 'linear_term' * h + 'constant_term',
// clamped to [0,1], and where h is the altitude (in m). 'exp_term' and
// 'constant_term' are unitless, while 'exp_scale' and 'linear_term' are in
// m^-1.
class DensityProfileLayer {
public:
  DensityProfileLayer(): DensityProfileLayer(0.0, 0.0, 0.0, 0.0, 0.0) {}
  DensityProfileLayer(
    float width, float exp_term, float exp_scale, float linear_term, float constant_term)
    : width(width),
      exp_term(exp_term),
      exp_scale(exp_scale),
      linear_term(linear_term),
      constant_term(constant_term) {}
  float width;
  float exp_term;
  float exp_scale;
  float linear_term;
  float constant_term;
  float pad[3]{0};
};

struct DensityProfile {
  DensityProfileLayer layers[2];
};

using IrradianceSpectrum = glm::vec3;
using ScatteringSpectrum = glm::vec3;
using DimensionlessSpectrum = glm::vec3;

using Angle = float;
using Length = float;
struct AtmosphereParameters {
  // The solar irradiance at the top of the atmosphere.
  IrradianceSpectrum solar_irradiance;
  // The sun's angular radius. Warning: the implementation uses approximations
  // that are valid only if this angle is smaller than 0.1 radians.
  Angle sun_angular_radius;
  // The scattering coefficient of air molecules at the altitude where their
  // density is maximum (usually the bottom of the atmosphere), as a function of
  // wavelength. The scattering coefficient at altitude h is equal to
  // 'rayleigh_scattering' times 'rayleigh_density' at this altitude.
  ScatteringSpectrum rayleigh_scattering;
  // The distance between the planet center and the bottom of the atmosphere.
  Length bottom_radius;
  // The scattering coefficient of aerosols at the altitude where their density
  // is maximum (usually the bottom of the atmosphere), as a function of
  // wavelength. The scattering coefficient at altitude h is equal to
  // 'mie_scattering' times 'mie_density' at this altitude.
  ScatteringSpectrum mie_scattering;
  // The distance between the planet center and the top of the atmosphere.
  Length top_radius;
  // The extinction coefficient of aerosols at the altitude where their density
  // is maximum (usually the bottom of the atmosphere), as a function of
  // wavelength. The extinction coefficient at altitude h is equal to
  // 'mie_extinction' times 'mie_density' at this altitude.
  ScatteringSpectrum mie_extinction;
  // The asymetry parameter for the Cornette-Shanks phase function for the
  // aerosols.
  float mie_phase_function_g;
  // The extinction coefficient of molecules that absorb light (e.g. ozone) at
  // the altitude where their density is maximum, as a function of wavelength.
  // The extinction coefficient at altitude h is equal to
  // 'absorption_extinction' times 'absorption_density' at this altitude.
  ScatteringSpectrum absorption_extinction;
  // The cosine of the maximum Sun zenith angle for which atmospheric scattering
  // must be precomputed (for maximum precision, use the smallest Sun zenith
  // angle yielding negligible sky light radiance values. For instance, for the
  // Earth case, 102 degrees is a good choice - yielding mu_s_min = -0.2).
  float mu_s_min;
  // The average albedo of the ground.
  DimensionlessSpectrum ground_albedo;
  float pad;
  // The density profile of air molecules, i.e. a function from altitude to
  // dimensionless values between 0 (null density) and 1 (maximum density).
  DensityProfile rayleigh_density;
  // The density profile of aerosols, i.e. a function from altitude to
  // dimensionless values between 0 (null density) and 1 (maximum density).
  DensityProfile mie_density;
  // The density profile of air molecules that absorb light (e.g. ozone), i.e.
  // a function from altitude to dimensionless values between 0 (null density)
  // and 1 (maximum density).
  DensityProfile absorption_density;
};

class SkyModel {

private:
  struct AtmosphereUniform {
    int transmittance_texture_width;
    int transmittance_texture_height;
    int scattering_texture_r_size;
    int scattering_texture_mu_size;
    int scattering_texture_mu_s_size;
    int scattering_texture_nu_size;
    int irradiance_texture_width;
    int irradiance_texture_height;
    glm::vec4 sky_spectral_radiance_to_luminance;
    glm::vec4 sun_spectral_radiance_to_luminance;
    AtmosphereParameters atmosphere;
  };

  struct SunUniform {
    glm::vec4 white_point{};
    glm::vec4 earth_center;
    glm::vec4 sun_direction;
    glm::vec2 sun_size;
    float exposure;
  };

public:
  SkyModel(
    Device &device, DebugMarker &debugMarker,
    // The wavelength values, in nanometers, and sorted in increasing order, for
    // which the solar_irradiance, rayleigh_scattering, mie_scattering,
    // mie_extinction and ground_albedo samples are provided. If your shaders
    // use luminance values (as opposed to radiance values, see above), use a
    // large number of wavelengths (e.g. between 15 and 50) to get accurate
    // results (this number of wavelengths has absolutely no impact on the
    // shader performance).
    const std::vector<double> &wavelengths,
    // The solar irradiance at the top of the atmosphere, in W/m^2/nm. This
    // vector must have the same size as the wavelengths parameter.
    const std::vector<double> &solar_irradiance,
    // The sun's angular radius, in radians. Warning: the implementation uses
    // approximations that are valid only if this value is smaller than 0.1.
    double sun_angular_radius,
    // The distance between the planet center and the bottom of the atmosphere,
    // in m.
    double bottom_radius,
    // The distance between the planet center and the top of the atmosphere,
    // in m.
    double top_radius,
    // The density profile of air molecules, i.e. a function from altitude to
    // dimensionless values between 0 (null density) and 1 (maximum density).
    // Layers must be sorted from bottom to top. The width of the last layer is
    // ignored, i.e. it always extend to the top atmosphere boundary. At most 2
    // layers can be specified.
    const std::vector<DensityProfileLayer> &rayleigh_density,
    // The scattering coefficient of air molecules at the altitude where their
    // density is maximum (usually the bottom of the atmosphere), as a function
    // of wavelength, in m^-1. The scattering coefficient at altitude h is equal
    // to 'rayleigh_scattering' times 'rayleigh_density' at this altitude. This
    // vector must have the same size as the wavelengths parameter.
    const std::vector<double> &rayleigh_scattering,
    // The density profile of aerosols, i.e. a function from altitude to
    // dimensionless values between 0 (null density) and 1 (maximum density).
    // Layers must be sorted from bottom to top. The width of the last layer is
    // ignored, i.e. it always extend to the top atmosphere boundary. At most 2
    // layers can be specified.
    const std::vector<DensityProfileLayer> &mie_density,
    // The scattering coefficient of aerosols at the altitude where their
    // density is maximum (usually the bottom of the atmosphere), as a function
    // of wavelength, in m^-1. The scattering coefficient at altitude h is equal
    // to 'mie_scattering' times 'mie_density' at this altitude. This vector
    // must have the same size as the wavelengths parameter.
    const std::vector<double> &mie_scattering,
    // The extinction coefficient of aerosols at the altitude where their
    // density is maximum (usually the bottom of the atmosphere), as a function
    // of wavelength, in m^-1. The extinction coefficient at altitude h is equal
    // to 'mie_extinction' times 'mie_density' at this altitude. This vector
    // must have the same size as the wavelengths parameter.
    const std::vector<double> &mie_extinction,
    // The asymetry parameter for the Cornette-Shanks phase function for the
    // aerosols.
    double mie_phase_function_g,
    // The density profile of air molecules that absorb light (e.g. ozone), i.e.
    // a function from altitude to dimensionless values between 0 (null density)
    // and 1 (maximum density). Layers must be sorted from bottom to top. The
    // width of the last layer is ignored, i.e. it always extend to the top
    // atmosphere boundary. At most 2 layers can be specified.
    const std::vector<DensityProfileLayer> &absorption_density,
    // The extinction coefficient of molecules that absorb light (e.g. ozone) at
    // the altitude where their density is maximum, as a function of wavelength,
    // in m^-1. The extinction coefficient at altitude h is equal to
    // 'absorption_extinction' times 'absorption_density' at this altitude. This
    // vector must have the same size as the wavelengths parameter.
    const std::vector<double> &absorption_extinction,
    // The average albedo of the ground, as a function of wavelength. This
    // vector must have the same size as the wavelengths parameter.
    const std::vector<double> &ground_albedo,
    // The maximum Sun zenith angle for which atmospheric scattering must be
    // precomputed, in radians (for maximum precision, use the smallest Sun
    // zenith angle yielding negligible sky light radiance values. For instance,
    // for the Earth case, 102 degrees is a good choice for most cases (120
    // degrees is necessary for very high exposure values).
    double max_sun_zenith_angle,
    // The length unit used in your shaders and meshes. This is the length unit
    // which must be used when calling the atmosphere model shader functions.
    float length_unit_in_meters,
    // The number of wavelengths for which atmospheric scattering must be
    // precomputed (the temporary GPU memory used during precomputations, and
    // the GPU memory used by the precomputed results, is independent of this
    // number, but the <i>precomputation time is directly proportional to this
    // number</i>):
    // - if this number is less than or equal to 3, scattering is precomputed
    // for 3 wavelengths, and stored as irradiance values. Then both the
    // radiance-based and the luminance-based API functions are provided (see
    // the above note).
    // - otherwise, scattering is precomputed for this number of wavelengths
    // (rounded up to a multiple of 3), integrated with the CIE color matching
    // functions, and stored as illuminance values. Then only the
    // luminance-based API functions are provided (see the above note).
    unsigned int num_precomputed_wavelengths, float exposure_scale);

  void Init(unsigned int num_scattering_orders = 4);

  // Utility method to convert a function of the wavelength to linear sRGB.
  // 'wavelengths' and 'spectrum' must have the same size. The integral of
  // 'spectrum' times each CIE_2_DEG_COLOR_MATCHING_FUNCTIONS (and times
  // MAX_LUMINOUS_EFFICACY) is computed to get XYZ values, which are then
  // converted to linear sRGB with the XYZ_TO_SRGB matrix.
  static void ConvertSpectrumToLinearSrgb(
    const std::vector<double> &wavelengths, const std::vector<double> &spectrum,
    double *r, double *g, double *b);

  static constexpr double kLambdaR = 680.0;
  static constexpr double kLambdaG = 550.0;
  static constexpr double kLambdaB = 440.0;

  HostUniformBuffer &atmosphereUBO();
  HostUniformBuffer &sunUBO();
  Texture &transmittanceTexture();
  Texture &scatteringTexture();
  Texture &irradianceTexture();

private:
  void Precompute(
    const glm::vec3 &lambdas, const glm::mat4 &luminance_from_radiance, bool cumulate,
    unsigned int num_scattering_orders);
  void computeTransmittance(Texture &transmittanceTexture);
  void computeDirectIrradiance(
    Texture &deltaIrradianceTexture, Texture &irradianceTexture,
    Texture &transmittanceTexture);
  void computeSingleScattering(
    Texture &deltaRayleighScatteringTexture, Texture &deltaMieScatteringTexture,
    Texture &scatteringTexture, Texture &transmittanceTexture);
  void computeScatteringDensity(
    Texture &deltaScatteringDensityTexture, Texture &transmittanceTexture,
    Texture &deltaRayleighScatteringTexture, Texture &deltaMieScatteringTexture,
    Texture &deltaMultipleScatteringTexture, Texture &deltaIrradianceTexture);
  void computeIndirectIrradiance(
    Texture &deltaIrradianceTexture, Texture &irradianceTexture,
    Texture &deltaRayleighScatteringTexture, Texture &deltaMieScatteringTexture,
    Texture &deltaMultipleScatteringTexture);
  void computeMultipleScattering(
    Texture &deltaMultipleScatteringTexture, Texture &scatteringTexture,
    Texture &transmittanceTexture, Texture &deltaScatteringDensityTexture);

private:
  Device &device;
  DebugMarker &debugMarker;

  unsigned int num_precomputed_wavelengths_;
  //  bool rgb_format_supported_;
  //  std::function<std::string(const vec3 &)> glsl_header_factory_;
  uPtr<Texture> transmittance_texture_;
  uPtr<Texture> scattering_texture_;
  uPtr<Texture> irradiance_texture_;

  uPtr<Texture> delta_irradiance_texture;
  uPtr<Texture> delta_rayleigh_scattering_texture, delta_mie_scattering_texture,
    delta_scattering_density_texture;

  uPtr<HostUniformBuffer> _atmosphereUBO;
  AtmosphereUniform atmosphere;
  uPtr<HostUniformBuffer> _sunUBO;
  SunUniform sun;

  bool do_white_balance_{true};
  float view_distance_meters_{9000.0};
  float view_zenith_angle_radians_{1.47};
  float view_azimuth_angle_radians_{-0.1};
  float sun_zenith_angle_radians_{1.3};
  float sun_azimuth_angle_radians_{2.9};
  float exposure_{10.0};

  std::function<AtmosphereParameters(const glm::vec3 &)> calcAtmosphereParams;

  uPtr<HostUniformBuffer> cumulateUBO;
  uPtr<HostUniformBuffer> LFRUBO;
  uPtr<HostUniformBuffer> ScatterOrderUBO;
};
}
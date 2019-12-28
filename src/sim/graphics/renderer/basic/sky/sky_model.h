#pragma once
#include "sim/graphics/base/device.h"
#include "sim/graphics/base/debug_marker.h"
#include "sim/graphics/base/resource/images.h"
#include "sim/graphics/base/resource/buffers.h"
#include "sim/graphics/base/pipeline/pipeline.h"
#include "sim/graphics/base/pipeline/descriptors.h"

namespace sim::graphics::renderer::basic {
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
  IrradianceSpectrum solar_irradiance;
  Angle sun_angular_radius;
  ScatteringSpectrum rayleigh_scattering;
  Length bottom_radius;
  ScatteringSpectrum mie_scattering;
  Length top_radius;
  ScatteringSpectrum mie_extinction;
  float mie_phase_function_g;
  ScatteringSpectrum absorption_extinction;
  float mu_s_min;
  DimensionlessSpectrum ground_albedo;
  float pad;
  DensityProfile rayleigh_density;
  DensityProfile mie_density;
  DensityProfile absorption_density;
};

class SkyModel {
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
    Device &device, DebugMarker &debugMarker, const std::vector<double> &wavelengths,
    const std::vector<double> &solar_irradiance, double sun_angular_radius,
    double bottom_radius, double top_radius,
    const std::vector<DensityProfileLayer> &rayleigh_density,
    const std::vector<double> &rayleigh_scattering,
    const std::vector<DensityProfileLayer> &mie_density,
    const std::vector<double> &mie_scattering, const std::vector<double> &mie_extinction,
    double mie_phase_function_g,
    const std::vector<DensityProfileLayer> &absorption_density,
    const std::vector<double> &absorption_extinction,
    const std::vector<double> &ground_albedo, double max_sun_zenith_angle,
    float length_unit_in_meters, unsigned int num_precomputed_wavelengths,
    float exposure_scale);

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
  void updateSunPosition(float sun_zenith_angle_radians, float sun_azimuth_angle_radians);
  Texture &transmittanceTexture();
  Texture &scatteringTexture();
  Texture &irradianceTexture();

private:
  void Precompute(
    const glm::vec3 &lambdas, const glm::mat4 &luminance_from_radiance, bool cumulate,
    unsigned int num_scattering_orders);

  void createComputePipelines();

  using ComputeCMD = std::function<void(vk::CommandBuffer cb)>;
  ComputeCMD createTransmittance(Texture &transmittanceTexture);
  ComputeCMD createDirectIrradiance(
    Texture &deltaIrradianceTexture, Texture &irradianceTexture,
    Texture &transmittanceTexture);
  ComputeCMD createSingleScattering(
    Texture &deltaRayleighScatteringTexture, Texture &deltaMieScatteringTexture,
    Texture &scatteringTexture, Texture &transmittanceTexture);
  ComputeCMD createScatteringDensity(
    Texture &deltaScatteringDensityTexture, Texture &transmittanceTexture,
    Texture &deltaRayleighScatteringTexture, Texture &deltaMieScatteringTexture,
    Texture &deltaMultipleScatteringTexture, Texture &deltaIrradianceTexture);
  ComputeCMD createIndirectIrradiance(
    Texture &deltaIrradianceTexture, Texture &irradianceTexture,
    Texture &deltaRayleighScatteringTexture, Texture &deltaMieScatteringTexture,
    Texture &deltaMultipleScatteringTexture);
  ComputeCMD createMultipleScattering(
    Texture &deltaMultipleScatteringTexture, Texture &scatteringTexture,
    Texture &transmittanceTexture, Texture &deltaScatteringDensityTexture);

private:
  Device &device;
  DebugMarker &debugMarker;

  unsigned int num_precomputed_wavelengths_;

  uPtr<Texture> transmittance_texture_;
  uPtr<Texture> scattering_texture_;
  uPtr<Texture> irradiance_texture_;

  uPtr<Texture> delta_irradiance_texture;
  uPtr<Texture> delta_rayleigh_scattering_texture, delta_mie_scattering_texture,
    delta_scattering_density_texture;

  uPtr<HostUniformBuffer> _atmosphereUBO;
  uPtr<HostUniformBuffer> _sunUBO;

  bool do_white_balance_{true};
  float sun_zenith_angle_radians_{1.3};
  float sun_azimuth_angle_radians_{2.9};
  float exposure_{10.0};

  std::function<AtmosphereParameters(const glm::vec3 &)> calcAtmosphereParams;

  uPtr<HostUniformBuffer> cumulateUBO;
  uPtr<HostUniformBuffer> LFRUBO;
  uPtr<HostUniformBuffer> ScatterOrderUBO;

  using shader = vk::ShaderStageFlagBits;

  vk::UniqueDescriptorPool descriptorPool;

  struct TransmittanceSetDef: DescriptorSetDef {
    __uniform__(atmosphere, shader::eCompute);
    __storageImage__(transmittance, shader::eCompute);
  } transmittanceSetDef;
  struct TransmittanceLayoutDef: PipelineLayoutDef {
    __set__(set, TransmittanceSetDef);
  } transmittanceLayoutDef;
  vk::DescriptorSet transmittanceSet;
  vk::UniquePipeline transmittancePipeline;
  ComputeCMD computeTransmittanceCMD;

  struct DirectIrradianceSetDef: DescriptorSetDef {
    __uniform__(atmosphere, shader::eCompute);
    __uniform__(cumulate, shader::eCompute);
    __sampler__(transmittance, shader::eCompute);
    __storageImage__(delta_irradiance, shader::eCompute);
    __storageImage__(irradiance, shader::eCompute);
  } directIrradianceSetDef;
  struct DirectIrradianceLayoutDef: PipelineLayoutDef {
    __set__(set, DirectIrradianceSetDef);
  } directIrradianceLayoutDef;
  vk::DescriptorSet directIrradianceSet;
  vk::UniquePipeline directIrradiancePipeline;
  ComputeCMD computeDirectIrradianceCMD;

  struct SingleScatteringSetDef: DescriptorSetDef {
    __uniform__(atmosphere, shader::eCompute);
    __uniform__(cumulate, shader::eCompute);
    __uniform__(luminance_from_radiance, shader::eCompute);
    __sampler__(transmittance, shader::eCompute);
    __storageImage__(delta_rayleigh, shader::eCompute);
    __storageImage__(delta_mie, shader::eCompute);
    __storageImage__(scattering, shader::eCompute);
  } singleScatteringSetDef;
  struct SingleScatteringLayoutDef: PipelineLayoutDef {
    __set__(set, SingleScatteringSetDef);
  } singleScatteringLayoutDef;
  vk::DescriptorSet singleScatteringSet;
  vk::UniquePipeline singleScatteringPipeline;
  ComputeCMD computeSingleScatteringCMD;

  struct ScatteringDensitySetDef: DescriptorSetDef {
    __uniform__(atmosphere, shader::eCompute);
    __uniform__(scattering_order, shader::eCompute);
    __sampler__(transmittance, shader::eCompute);
    __sampler__(delta_rayleigh, shader::eCompute);
    __sampler__(delta_mie, shader::eCompute);
    __sampler__(multpli_scattering, shader::eCompute);
    __sampler__(irradiance, shader::eCompute);
    __storageImage__(scattering_density, shader::eCompute);
  } scatteringDensitySetDef;
  struct ScatteringDensityLayoutDef: PipelineLayoutDef {
    __set__(set, ScatteringDensitySetDef);
  } scatteringDensityLayoutDef;
  vk::DescriptorSet scatteringDensitySet;
  vk::UniquePipeline scatteringDensityPipeline;
  ComputeCMD computeScatteringDensityCMD;

  struct IndirectIrradianceSetDef: DescriptorSetDef {
    __uniform__(atmosphere, shader::eCompute);
    __uniform__(luminance_from_radiance, shader::eCompute);
    __uniform__(scattering_order, shader::eCompute);
    __sampler__(delta_rayleigh, shader::eCompute);
    __sampler__(delta_mie, shader::eCompute);
    __sampler__(multpli_scattering, shader::eCompute);
    __storageImage__(delta_irradiance, shader::eCompute);
    __storageImage__(irradiance, shader::eCompute);
  } indirectIrradianceSetDef;
  struct IndirectIrradianceLayoutDef: PipelineLayoutDef {
    __set__(set, IndirectIrradianceSetDef);
  } indirectIrradianceLayoutDef;
  vk::DescriptorSet indirectIrradianceSet;
  vk::UniquePipeline indirectIrradiancePipeline;
  ComputeCMD computeIndirectIrradianceCMD;

  struct MultipleScatteringSetDef: DescriptorSetDef {
    __uniform__(atmosphere, shader::eCompute);
    __uniform__(luminance_from_radiance, shader::eCompute);
    __sampler__(transmittance, shader::eCompute);
    __sampler__(scattering_density, shader::eCompute);
    __storageImage__(delta_multiple_scattering, shader::eCompute);
    __storageImage__(scattering, shader::eCompute);
  } multipleScatteringSetDef;
  struct MultipleScatteringLayoutDef: PipelineLayoutDef {
    __set__(set, MultipleScatteringSetDef);
  } multipleScatteringLayoutDef;
  vk::DescriptorSet multipleScatteringSet;
  vk::UniquePipeline multipleScatteringPipeline;
  ComputeCMD computeMultipleScatteringCMD;
};
}
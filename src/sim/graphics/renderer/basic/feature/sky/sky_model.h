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

  HostUniformBuffer &atmosphereUBO();
  HostUniformBuffer &sunUBO();
  void updateSunPosition(glm::vec3 sunDirection);
  void updateEarthCenter(glm::vec3 earthCenter);
  Texture &transmittanceTexture();
  Texture &scatteringTexture();
  Texture &irradianceTexture();

private:
  void createDescriptors();

  void createTransmittanceSets();
  void recordTransmittanceCMD(vk::CommandBuffer cb);

  void createDirectIrradianceSets();
  void recordDirectIrradianceCMD(vk::CommandBuffer cb, vk::Bool32 cumulate);

  void createSingleScatteringSets();
  void recordSingleScatteringCMD(
    vk::CommandBuffer cb, const glm::mat4 &luminance_from_radiance, vk::Bool32 cumulate);

  void createScatteringDensitySets();
  void recordScatteringDensityCMD(vk::CommandBuffer cb, int32_t scatteringOrder);

  void createIndirectIrradianceSets();
  void recordIndirectIrradianceCMD(
    vk::CommandBuffer cb, const glm::mat4 &luminance_from_radiance,
    int32_t scatteringOrder);

  void createMultipleScatteringSets();
  void recordMultipleScatteringCMD(
    vk::CommandBuffer cb, const glm::mat4 &luminance_from_radiance);
  void compute(uint32_t num_scattering_orders);
  void precompute(
    vk::CommandBuffer cb, const glm::vec3 &lambdas,
    const glm::mat4 &luminance_from_radiance, bool cumulate,
    unsigned int num_scattering_orders);

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
  double bottom_radius;
  float length_unit_in_meters;
  glm::vec3 sunDirection_{0, 1, 0};
  float exposure_{10.0};

  std::function<AtmosphereParameters(const glm::vec3 &)> calcAtmosphereParams;

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

  struct DirectIrradianceSetDef: DescriptorSetDef {
    __uniform__(atmosphere, shader::eCompute);
    __sampler__(transmittance, shader::eCompute);
    __storageImage__(delta_irradiance, shader::eCompute);
    __storageImage__(irradiance, shader::eCompute);
  } directIrradianceSetDef;
  struct DirectIrradianceLayoutDef: PipelineLayoutDef {
    __push_constant__(constant, shader::eCompute, vk::Bool32);
    __set__(set, DirectIrradianceSetDef);
  } directIrradianceLayoutDef;
  vk::DescriptorSet directIrradianceSet;
  vk::UniquePipeline directIrradiancePipeline;

  struct SingleScatteringSetDef: DescriptorSetDef {
    __uniform__(atmosphere, shader::eCompute);
    __sampler__(transmittance, shader::eCompute);
    __storageImage__(delta_rayleigh, shader::eCompute);
    __storageImage__(delta_mie, shader::eCompute);
    __storageImage__(scattering, shader::eCompute);
  } singleScatteringSetDef;
  struct CumulateLFUConstant {
    glm::mat4 luminance_from_radiance;
    vk::Bool32 cumulate;
  };
  struct SingleScatteringLayoutDef: PipelineLayoutDef {
    __push_constant__(constant, shader::eCompute, CumulateLFUConstant);
    __set__(set, SingleScatteringSetDef);
  } singleScatteringLayoutDef;
  vk::DescriptorSet singleScatteringSet;
  vk::UniquePipeline singleScatteringPipeline;

  struct ScatteringDensitySetDef: DescriptorSetDef {
    __uniform__(atmosphere, shader::eCompute);
    __sampler__(transmittance, shader::eCompute);
    __sampler__(delta_rayleigh, shader::eCompute);
    __sampler__(delta_mie, shader::eCompute);
    __sampler__(multpli_scattering, shader::eCompute);
    __sampler__(irradiance, shader::eCompute);
    __storageImage__(scattering_density, shader::eCompute);
  } scatteringDensitySetDef;
  struct ScatteringDensityLayoutDef: PipelineLayoutDef {
    __push_constant__(constant, shader::eCompute, int32_t);
    __set__(set, ScatteringDensitySetDef);
  } scatteringDensityLayoutDef;
  vk::DescriptorSet scatteringDensitySet;
  vk::UniquePipeline scatteringDensityPipeline;

  struct IndirectIrradianceSetDef: DescriptorSetDef {
    __uniform__(atmosphere, shader::eCompute);
    __sampler__(delta_rayleigh, shader::eCompute);
    __sampler__(delta_mie, shader::eCompute);
    __sampler__(multpli_scattering, shader::eCompute);
    __storageImage__(delta_irradiance, shader::eCompute);
    __storageImage__(irradiance, shader::eCompute);
  } indirectIrradianceSetDef;
  struct ScatteringOrderLFUConstant {
    glm::mat4 luminance_from_radiance;
    int32_t scattering_order;
  };
  struct IndirectIrradianceLayoutDef: PipelineLayoutDef {
    __push_constant__(constant, shader::eCompute, ScatteringOrderLFUConstant);
    __set__(set, IndirectIrradianceSetDef);
  } indirectIrradianceLayoutDef;
  vk::DescriptorSet indirectIrradianceSet;
  vk::UniquePipeline indirectIrradiancePipeline;

  struct MultipleScatteringSetDef: DescriptorSetDef {
    __uniform__(atmosphere, shader::eCompute);
    __sampler__(transmittance, shader::eCompute);
    __sampler__(scattering_density, shader::eCompute);
    __storageImage__(delta_multiple_scattering, shader::eCompute);
    __storageImage__(scattering, shader::eCompute);
  } multipleScatteringSetDef;
  struct MultipleScatteringLayoutDef: PipelineLayoutDef {
    __push_constant__(constant, shader::eCompute, glm::mat4);
    __set__(set, MultipleScatteringSetDef);
  } multipleScatteringLayoutDef;
  vk::DescriptorSet multipleScatteringSet;
  vk::UniquePipeline multipleScatteringPipeline;
};
}
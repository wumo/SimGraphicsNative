#pragma once
#include "sky_model.h"

namespace sim::graphics::renderer::basic {

class BasicSceneManager;

class SkyManager {
public:
  explicit SkyManager(BasicSceneManager &mm);

  void enable();
  void setSunPosition(float sun_zenith_angle_radians, float sun_azimuth_angle_radians);

  bool enabled() const;

  SkyModel &model();

  void updateModel();

private:
  friend class BasicSceneManager;

  struct SkySetDef: DescriptorSetDef {
    __uniform__(atmosphere, vk::ShaderStageFlagBits::eFragment);
    __uniform__(sun, vk::ShaderStageFlagBits::eFragment);
    __sampler__(transmittance, vk::ShaderStageFlagBits::eFragment);
    __sampler__(scattering, vk::ShaderStageFlagBits::eFragment);
    __sampler__(irradiance, vk::ShaderStageFlagBits::eFragment);
  } skySetDef;
  vk::DescriptorSet skySet;

private:
  void createDescriptorSets(vk::DescriptorPool descriptorPool);

private:
  BasicSceneManager &mm;
  Device &device;
  DebugMarker &debugMarker;
  uPtr<SkyModel> _model;
};
}
#pragma once
#include "sky_model.h"

namespace sim::graphics::renderer::basic {

class BasicSceneManager;

class SkyManager {
public:
  explicit SkyManager(BasicSceneManager &mm);

  void init(
    double kLengthUnitInMeters = 1000.f, double kSunAngularRadius = 0.00935 / 2.0);
  void setSunDirection(glm::vec3 sunDirection);
  void setEarthCenter(glm::vec3 earthCenter);
  double earthRadius() const;
  bool enabled() const;
  double sunAngularRadius() const;
  double lengthUnitInMeters() const;
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

  double kSunAngularRadius_{0.00935 / 2.0};
  double kSunSolidAngle_{glm::pi<double>() * kSunAngularRadius_ * kSunAngularRadius_};
  double kLengthUnitInMeters_{1000.0};
  double kBottomRadius_ = 6360000.0;
  glm::vec3 earthCenter_{0, -kBottomRadius_ / kLengthUnitInMeters_, 0};
};
}
#pragma once
#include "sim/graphics/base/device.h"
#include "sim/graphics/base/debug_marker.h"
#include "sim/graphics/base/resource/images.h"
#include "sim/graphics/base/resource/buffers.h"
#include "sim/graphics/base/pipeline/pipeline.h"
#include "sim/graphics/base/pipeline/descriptors.h"
#include "../model/basic_model.h"

namespace sim::graphics::renderer::basic {
class BasicSceneManager;
class OceanManager {
public:
  explicit OceanManager(BasicSceneManager &mm);

  Ptr<ModelInstance> newField(float patchSize = 500.f, int N = 128);

  void updateWind(glm::vec2 windDirection, float windSpeed);

private:
  void createDescriptorSets(vk::DescriptorPool descriptorPool);
  bool enabled();

  glm::vec2 hTiled_0(glm::vec2 k);
  float phillipsSpectrum(glm::vec2 k);
  void initOceanData();

  void compute(vk::CommandBuffer computeCB, uint32_t imageIndex, float elapsedDuration);

private:
  friend class BasicSceneManager;

  const float g = 9.8f;
  const float PI = glm::pi<float>();

  using shader = vk::ShaderStageFlagBits;
  struct OceanDescriptorSet: DescriptorSetDef {
    __buffer__(bitReversal, shader::eCompute);
    __buffer__(datum, shader::eCompute);
    __buffer__(positions, shader::eCompute);
    __buffer__(normals, shader::eCompute);
  } oceanSetDef;

  struct OceanConstant {
    int32_t positionOffset{};
    int32_t normalOffset{};
    int32_t dataOffset{};
    float patchSize{500.f};
    float choppyScale{-1.f};
    float timeScale{5.f};
    float time{0.f};
  } oceanConstant;

  struct ComputeMeshLayoutDef: PipelineLayoutDef {
    __push_constant__(constant, shader::eCompute, OceanConstant);
    __set__(set, OceanDescriptorSet);
  } oceanLayoutDef;

  struct Ocean {
    glm::vec2 h0, ht, hDx, hDz, slopeX, slopeZ;
  };

private:
  BasicSceneManager &mm;
  Device &device;
  DebugMarker &debugMarker;

  vk::DescriptorSet oceanSet;

  vk::UniquePipeline pingPipe, pongPipe;

  uPtr<HostStorageBuffer> bitReversalBuffer;
  uPtr<HostStorageBuffer> datumBuffers;

  Ptr<Primitive> seaPrimitive;

  bool initialized{false};
  int32_t N{128};
  uint32_t lx{128}, ly{1};

  float windDx{0.8f}, windDy{0.6f};
  float windSpeed{60.f};
  float waveAmplitude{10.f};
};
}

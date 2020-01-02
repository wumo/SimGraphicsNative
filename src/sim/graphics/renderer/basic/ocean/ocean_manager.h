#pragma once
#include "sim/graphics/base/device.h"
#include "sim/graphics/base/debug_marker.h"
#include "sim/graphics/base/resource/images.h"
#include "sim/graphics/base/resource/buffers.h"
#include "sim/graphics/base/pipeline/pipeline.h"
#include "sim/graphics/base/pipeline/descriptors.h"
#include "../model/model.h"

namespace sim::graphics::renderer::basic {
class BasicSceneManager;
class OceanManager {
public:
  explicit OceanManager(BasicSceneManager &mm);

  void enable();
  bool enabled();
  void newField(const AABB &aabb, uint32_t numVertexX, uint32_t numVertexY);

private:
  glm::vec2 hTiled_0(glm::vec2 k);
  float phillipsSpectrum(glm::vec2 k);
  void initOceanData();

private:
  friend class BasicSceneManager;

  const float g = 9.8f;
  const float PI = glm::pi<float>();

  using shader = vk::ShaderStageFlagBits;
  struct OceanDescriptorSet: DescriptorSetDef {
    __uniform__(cam, shader::eCompute);
    __buffer__(bitReversal, shader::eCompute);
    __buffer__(datum, shader::eCompute);
    __buffer__(positions, shader::eCompute);
    __buffer__(normals, shader::eCompute);
  } oceanSetDef;

  struct OceanConstant {
    uint32_t positionOffset{};
    uint32_t normalOffset{};
    int32_t N{128};
    float windSpeed{60.f};
    float waveAmplitude{10.f};
    float choppyScale{-1.f};
    float timeScale{0.01f};
    float windDx{0.8f}, windDy{0.6f};
    float ampFactor{1.f};
    float time{0.f};
  } oceanConstant;

  struct ComputeMeshLayoutDef: PipelineLayoutDef {
    __push_constant__(constant, shader::eCompute, OceanConstant);
    __set__(set, OceanDescriptorSet);
  } oceanLayoutDef;

  struct WaveUniform {
    int meshOffset{};
    float patchSize{500.f};
    float windSpeed{60.f};
    float waveAmplitude{10.f};
    float choppyScale{-1.f};
    float timeScale{0.01f};
    float windDx{0.8f}, windDy{0.6f};
    float ampFactor{1.f};
    float time{0.f};
  } waveConfig;

  struct Ocean {
    glm::vec2 h0, ht, hDx, hDz, slopeX, slopeZ;
  };

private:
  bool initialized{false};

  vk::DescriptorSet computeSet;
  uPtr<HostUniformBuffer> uboWave;
  uPtr<HostStorageBuffer> bitReversalBuffer;

  std::vector<uPtr<HostStorageBuffer>> camBuffers;
  std::vector<uPtr<HostStorageBuffer>> datumBuffers;

  BasicSceneManager &mm;
  Device &device;
};
}

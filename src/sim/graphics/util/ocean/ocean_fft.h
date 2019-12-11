#pragma once

#include "sim/graphics/base/vulkan_base.h"
#include "sim/graphics/resource/buffers.h"
#include "sim/graphics/pipeline/descriptors.h"

namespace sim { namespace graphics {
class OceanFFT {
  using shader = vk::ShaderStageFlagBits;

public:
  OceanFFT();

private:
  struct WaveConstant {
    uint32_t lx{128}, ly{1};
    int32_t N{128};
  } waveConstant{};

  int N{128};

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

  struct ComputeDescriptorSet: DescriptorSetDef {
    __uniform__(wave, shader::eCompute);
    __buffer__(bitReversal, shader::eCompute);
    __buffer__(datum, shader::eCompute);
    __buffer__(vertices, shader::eCompute);
    __uniform__(viewProj, shader::eCompute);
  } computeDef;

  vk::DescriptorSet computeSet;
  uPtr<HostUniformBuffer> uboWave;
  uPtr<HostStorageBuffer> bitReversalBuffer, datumBuffer;
  uPtr<HostIndirectBuffer> computeDispatchBuffer;
  Pipe computePing, computePong;

  const float g = 9.8f;
  const float PI = glm::pi<float>();

  glm::vec2 hTiled_0(glm::vec2 k);
  float phillipsSpectrum(glm::vec2 k);
  void initOceanData();

  void createComputePipelines();
  void fillComputeCmdBuffer(const vk::CommandBuffer &cb, uint32_t i);
};
}}
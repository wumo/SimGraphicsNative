#include "ocean_fft.h"

namespace sim::graphics {
using bindpoint = vk::PipelineBindPoint;
using stage = vk::PipelineStageFlagBits;
using access = vk::AccessFlagBits;

glm::vec2 OceanFFT::hTiled_0(glm::vec2 k) {
  float phillips = (k.x == 0 && k.y == 0) ? 0 : sqrt(phillipsSpectrum(k));
  return {guassian() * phillips / sqrt(2), guassian() * phillips / sqrt(2)};
}
float OceanFFT::phillipsSpectrum(glm::vec2 k) {
  float L = waveConfig.windSpeed * waveConfig.windSpeed / g;
  float damping = 0.001f;
  float l = L * damping;
  float sqrK = dot(k, k);
  glm::vec2 windDir{waveConfig.windDx, waveConfig.windDy};
  float cosK = dot(k, windDir);
  float phillips = waveConfig.waveAmplitude * exp(-1 / (sqrK * L * L)) /
                   (sqrK * sqrK * sqrK) * (cosK * cosK);
  if(cosK < 0) phillips *= 0.07f;
  return phillips * exp(-sqrK * l * l);
}
void OceanFFT::initOceanData() {
  {
    auto ptr = datumBuffer->ptr<Ocean>();
    for(auto row = 0; row < N; ++row) {
      glm::vec2 k;
      k.y = (-N / 2.f + row) * 2 * PI / waveConfig.patchSize;
      for(auto column = 0; column < N; ++column) {
        k.x = (-N / 2.f + column) * 2 * PI / waveConfig.patchSize;
        glm::vec2 phi = hTiled_0(k);
        ptr[row * N + column].h0 = {phi.x, phi.y};
      }
    }
  }
  {
    auto rev = u<int32_t[]>(N);
    auto bit = int32_t(log2f(float(N)));
    for(auto i = 0; i < N; ++i)
      rev[i] = (rev[i >> 1] >> 1) | ((i & 1) << (bit - 1));
    bitReversalBuffer->update(rev.get(), N * sizeof(int32_t));
  }
}

void OceanFFT::createComputePipelines() {
  //  uboWave = u<HostUniformBuffer>(*allocator, waveConfig);
  //  datumBuffer = u<HostStorageBuffer>(*allocator, N * N * sizeof(Ocean));
  //  bitReversalBuffer = u<HostStorageBuffer>(*allocator, N * sizeof(int32_t));
  //  initOceanData();
  //  vk::DispatchIndirectCommand dispatch;
  //  dispatch.x = N / waveConstant.lx;
  //  dispatch.y = N / waveConstant.ly;
  //  dispatch.z = 5;
  //  computeDispatchBuffer = u<HostIndirectBuffer>(*allocator, dispatch);
  //  debugMarker.name(*uboWave, "ubo.wave");
  //  debugMarker.name(*computeDispatchBuffer, "computeDispatchBuffer");
  //  debugMarker.name(*datumBuffer, "datumBuffer");
  //  debugMarker.name(*bitReversalBuffer, "bitReversalBuffer");
  //
  //  computeDef.init(vkDevice);
  //  computeSet = computeDef.createSet(descriptorPool);
  //  computeDef.wave(uboWave->buffer());
  //  computeDef.bitReversal(bitReversalBuffer->buffer());
  //  computeDef.datum(datumBuffer->buffer());
  //  computeDef.vertices(scene->Buffer.vertices);
  //  computeDef.viewProj(*ubo.viewProj);
  //  computeDef.update(computeSet);
  //
  //  computePing.pipelineLayout = PipelineLayoutMaker()
  //                                 .descriptorSetLayout(*computeDef.descriptorSetLayout)
  //                                 .createUnique(vkDevice);
  //  computePong.pipelineLayout = PipelineLayoutMaker()
  //                                 .descriptorSetLayout(*computeDef.descriptorSetLayout)
  //                                 .createUnique(vkDevice);
  //
  //  {
  //    //compute pipeline
  //    waveConstant.N = N;
  //    ComputePipelineMaker pipelineMaker;
  //    using sEntry = vk::SpecializationMapEntry;
  //    std::array<sEntry, 3> constants = {
  //      sEntry{0, offsetOf(WaveConstant, lx), sizeof(uint32_t)},
  //      sEntry{1, offsetOf(WaveConstant, ly), sizeof(uint32_t)},
  //      sEntry{2, offsetOf(WaveConstant, N), sizeof(int32_t)}};
  //    vk::SpecializationInfo constInfo;
  //    constInfo.mapEntryCount = constants.size();
  //    constInfo.pMapEntries = constants.data();
  //    constInfo.dataSize = sizeof(waveConstant);
  //    constInfo.pData = &waveConstant;
  //    {
  //      ShaderModule comp{vkDevice,
  //                        "./assets/public/sim/shaders/ocean/wave-fft-ping.comp.spv"s};
  //      pipelineMaker.shader(comp, &constInfo);
  //      computePing.pipeline =
  //        pipelineMaker.createUnique(vkDevice, *pipelineCache, *computePing.pipelineLayout);
  //      debugMarker.name(*computePing.pipeline, "computePing pipeline");
  //    }
  //    {
  //      ShaderModule comp{vkDevice,
  //                        "./assets/public/sim/shaders/ocean/wave-fft-pong.comp.spv"s};
  //      pipelineMaker.shader(comp, &constInfo);
  //      computePong.pipeline =
  //        pipelineMaker.createUnique(vkDevice, *pipelineCache, *computePong.pipelineLayout);
  //      debugMarker.name(*computePong.pipeline, "computePong pipeline");
  //    }
  //  }
}
void OceanFFT::fillComputeCmdBuffer(const vk::CommandBuffer &cb, uint32_t i) {
  //  {
  //    vk::BufferMemoryBarrier barrier{access::eIndirectCommandRead,
  //                                    access::eShaderWrite,
  //                                    VK_QUEUE_FAMILY_IGNORED,
  //                                    VK_QUEUE_FAMILY_IGNORED,
  //                                    *scene->Buffer.vertices,
  //                                    0,
  //                                    VK_WHOLE_SIZE};
  //    cb.pipelineBarrier(
  //      stage::eDrawIndirect, stage::eComputeShader, {}, nullptr, barrier, nullptr);
  //  }
  cb.bindPipeline(bindpoint::eCompute, *computePing.pipeline);
  cb.bindDescriptorSets(
    bindpoint::eCompute, *computePing.pipelineLayout, 0, computeSet, nullptr);
  cb.dispatchIndirect(computeDispatchBuffer->buffer(), 0);
  {
    vk::BufferMemoryBarrier barrier{access::eShaderWrite,
                                    access::eShaderRead,
                                    VK_QUEUE_FAMILY_IGNORED,
                                    VK_QUEUE_FAMILY_IGNORED,
                                    datumBuffer->buffer(),
                                    0,
                                    VK_WHOLE_SIZE};
    cb.pipelineBarrier(
      stage::eComputeShader, stage::eComputeShader, {}, nullptr, barrier, nullptr);
  }
  cb.bindPipeline(bindpoint::eCompute, *computePong.pipeline);
  cb.dispatchIndirect(computeDispatchBuffer->buffer(), 0);

  {
    //    vk::BufferMemoryBarrier barrier{access::eShaderWrite,
    //                                    access::eIndirectCommandRead,
    //                                    VK_QUEUE_FAMILY_IGNORED,
    //                                    VK_QUEUE_FAMILY_IGNORED,
    //                                    *scene->Buffer.vertices,
    //                                    0,
    //                                    VK_WHOLE_SIZE};
    //    cb.pipelineBarrier(
    //      stage::eComputeShader, stage::eDrawIndirect, {}, nullptr, barrier, nullptr);
  }
}
}
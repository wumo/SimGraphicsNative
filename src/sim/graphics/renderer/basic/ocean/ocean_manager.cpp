#include "ocean_manager.h"
#include "../basic_scene_manager.h"
#include "sim/graphics/compiledShaders/ocean/wave_fft_ping_comp.h"
#include "sim/graphics/compiledShaders/ocean/wave_fft_pong_comp.h"

namespace sim::graphics::renderer::basic {
using bindpoint = vk::PipelineBindPoint;
using stage = vk::PipelineStageFlagBits;
using access = vk::AccessFlagBits;

OceanManager::OceanManager(BasicSceneManager &mm)
  : mm(mm), device{mm.device()}, debugMarker{mm.debugMarker()} {
  oceanSetDef.init(device.getDevice());
  oceanLayoutDef.set(oceanSetDef);
  oceanLayoutDef.init(device.getDevice());
}

void OceanManager::createDescriptorSets(vk::DescriptorPool descriptorPool) {
  oceanSet = oceanSetDef.createSet(descriptorPool);
}

bool OceanManager::enabled() { return initialized; }

Ptr<ModelInstance> OceanManager::newField(float patchSize, int N) {
  initialized = true;

  oceanConstant.patchSize = patchSize;
  this->N = N;
  datumBuffers = u<HostStorageBuffer>(
    device.allocator(), mm.config().numFrame * N * N * sizeof(Ocean));

  auto rev = u<int32_t[]>(N);
  auto bit = int32_t(log2f(float(N)));
  for(auto i = 0; i < N; ++i)
    rev[i] = (rev[i >> 1] >> 1) | ((i & 1) << (bit - 1));
  bitReversalBuffer = u<HostStorageBuffer>(device.allocator(), N * sizeof(int32_t));
  bitReversalBuffer->updateRaw(rev.get(), N * sizeof(int32_t));

  debugMarker.name(bitReversalBuffer->buffer(), "bitReversalBuffer");
  debugMarker.name(datumBuffers->buffer(), "datumBuffers");

  initOceanData();

  oceanSetDef.bitReversal(bitReversalBuffer->buffer());
  oceanSetDef.datum(datumBuffers->buffer());
  oceanSetDef.positions(mm.Buffer.position->buffer());
  oceanSetDef.normals(mm.Buffer.normal->buffer());
  oceanSetDef.update(oceanSet);

  seaPrimitive =
    mm.newPrimitive(PrimitiveBuilder(mm)
                      .grid(N - 1, N - 1)
                      .newPrimitive(PrimitiveTopology::Triangles, DynamicType::Dynamic));

  auto seaMat = mm.newMaterial(MaterialType::eTranslucent);
  //  seaMat->setColorFactor({39 / 255.f, 93 / 255.f, 121 / 255.f, 0.9f});
  seaMat->setColorFactor({39 / 255.f, 93 / 255.f, 121 / 255.f, 0.8f});
  seaMat->setPbrFactor({0, 0.5, 0.3, 0});
  auto seaMesh = mm.newMesh(seaPrimitive, seaMat);
  auto seaNode = mm.newNode();
  Node::addMesh(seaNode, seaMesh);
  auto seaModel = mm.newModel({seaNode});
  auto sea = mm.newModelInstance(seaModel);

  SpecializationMaker sp{};
  auto spInfo = sp.entry<uint32_t>(lx).entry<uint32_t>(ly).entry<uint32_t>(N).create();

  {
    ComputePipelineMaker pipelineMaker{device.getDevice()};
    pipelineMaker.shader(wave_fft_ping_comp, __ArraySize__(wave_fft_ping_comp), &spInfo);

    pingPipe = pipelineMaker.createUnique(nullptr, *oceanLayoutDef.pipelineLayout);
  }
  {
    ComputePipelineMaker pipelineMaker{device.getDevice()};
    pipelineMaker.shader(wave_fft_pong_comp, __ArraySize__(wave_fft_pong_comp), &spInfo);

    pongPipe = pipelineMaker.createUnique(nullptr, *oceanLayoutDef.pipelineLayout);
  }
  return sea;
}

void OceanManager::updateWind(glm::vec2 windDirection, float windSpeed) {
  windDx_ = windDirection.x;
  windDy_ = windDirection.y;
  windSpeed_ = windSpeed;

  initOceanData();
}

void OceanManager::updateWaveAmplitude(float waveAmplitude) {
  waveAmplitude_ = waveAmplitude;

  initOceanData();
}

float OceanManager::phillipsSpectrum(glm::vec2 k) {
  float L = windSpeed_ * windSpeed_ / g;
  float damping = 0.001f;
  float l = L * damping;
  float sqrK = dot(k, k);
  glm::vec2 windDir{windDx_, windDy_};
  float cosK = dot(k, windDir);
  float phillips =
    waveAmplitude_ * glm::exp(-1 / (sqrK * L * L)) / (sqrK * sqrK * sqrK) * (cosK * cosK);
  if(cosK < 0) phillips *= 0.07f;
  return phillips * glm::exp(-sqrK * l * l);
}

glm::vec2 OceanManager::hTiled_0(glm::vec2 k) {
  float phillips = (k.x == 0 && k.y == 0) ? 0 : glm::sqrt(phillipsSpectrum(k));
  return {guassian() * phillips / sqrt(2), guassian() * phillips / glm::sqrt(2)};
}

void OceanManager::initOceanData() {
  for(int i = 0; i < mm.config().numFrame; ++i) {
    auto ptr = datumBuffers->ptr<Ocean>() + i * N * N;
    for(auto row = 0; row < N; ++row) {
      glm::vec2 k;
      k.y = (float(-N) / 2.f + row) * 2 * PI / oceanConstant.patchSize;
      for(auto column = 0; column < N; ++column) {
        k.x = (float(-N) / 2.f + column) * 2 * PI / oceanConstant.patchSize;
        glm::vec2 phi = hTiled_0(k);
        ptr[row * N + column].h0 = {phi.x, phi.y};
      }
    }
  }
}

void OceanManager::compute(
  vk::CommandBuffer cb, uint32_t imageIndex, float elapsedDuration) {
  static float time = 0;
  time += elapsedDuration;

  cb.bindDescriptorSets(
    bindpoint::eCompute, *oceanLayoutDef.pipelineLayout, oceanLayoutDef.set.set(),
    oceanSet, nullptr);

  auto dataSize = N * N;
  auto offset = imageIndex * dataSize;

  auto &positionRange = seaPrimitive->position();
  auto &normalRange = seaPrimitive->normal();

  oceanConstant.positionOffset =
    positionRange.offset + imageIndex * positionRange.size / mm.config().numFrame;
  oceanConstant.normalOffset =
    normalRange.offset + imageIndex * normalRange.size / mm.config().numFrame;
  oceanConstant.dataOffset = 0;
  //  oceanConstant.dataOffset = offset;
  oceanConstant.time = time;

  debugMarker.begin(cb, toString("compute wave mesh ", imageIndex).c_str());
  cb.bindPipeline(bindpoint::eCompute, *pingPipe);
  cb.pushConstants<OceanConstant>(
    *oceanLayoutDef.pipelineLayout, shader::eCompute, 0, oceanConstant);
  cb.dispatch(N / lx, N / ly, 5);
  debugMarker.end(cb);
  {
    //    vk::BufferMemoryBarrier barrier{access::eShaderWrite,    access::eShaderRead,
    //                                    VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
    //                                    datumBuffers->buffer(),  offset * sizeof(Ocean),
    //                                    dataSize * sizeof(Ocean)};
    vk::BufferMemoryBarrier barrier{access::eShaderWrite,
                                    access::eShaderRead,
                                    VK_QUEUE_FAMILY_IGNORED,
                                    VK_QUEUE_FAMILY_IGNORED,
                                    datumBuffers->buffer(),
                                    0,
                                    VK_WHOLE_SIZE};
    cb.pipelineBarrier(
      stage::eComputeShader, stage::eComputeShader, {}, nullptr, barrier, nullptr);
  }
  debugMarker.begin(cb, toString("compute wave mesh ", imageIndex).c_str());
  cb.bindPipeline(bindpoint::eCompute, *pongPipe);
  cb.pushConstants<OceanConstant>(
    *oceanLayoutDef.pipelineLayout, shader::eCompute, 0, oceanConstant);
  cb.dispatch(N / lx, N / ly, 5);
  debugMarker.end(cb);
}
}
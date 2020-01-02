#include "ocean_manager.h"
#include "../basic_scene_manager.h"
namespace sim::graphics::renderer::basic {

OceanManager::OceanManager(BasicSceneManager &mm): mm(mm), device{mm.device()} {}

void OceanManager::enable() {
  int N = oceanConstant.N;
  datumBuffers.clear();
  for(int i = 0; i < mm.config().numFrame; ++i)
    datumBuffers.push_back(
      u<HostStorageBuffer>(device.allocator(), N * N * sizeof(Ocean)));

  auto rev = u<int32_t[]>(N);
  auto bit = int32_t(log2f(float(N)));
  for(auto i = 0; i < N; ++i)
    rev[i] = (rev[i >> 1] >> 1) | ((i & 1) << (bit - 1));
  bitReversalBuffer = u<HostStorageBuffer>(device.allocator(), N * sizeof(int32_t));
  bitReversalBuffer->updateRaw(rev.get(), N * sizeof(int32_t));

  initialized = true;
}

bool OceanManager::enabled() { return initialized; }

void OceanManager::newField(const AABB &aabb, uint32_t numVertexX, uint32_t numVertexY) {

  initOceanData();
}

float OceanManager::phillipsSpectrum(glm::vec2 k) {
  float L = waveConfig.windSpeed * waveConfig.windSpeed / g;
  float damping = 0.001f;
  float l = L * damping;
  float sqrK = dot(k, k);
  glm::vec2 windDir{waveConfig.windDx, waveConfig.windDy};
  float cosK = dot(k, windDir);
  float phillips = waveConfig.waveAmplitude * glm::exp(-1 / (sqrK * L * L)) /
                   (sqrK * sqrK * sqrK) * (cosK * cosK);
  if(cosK < 0) phillips *= 0.07f;
  return phillips * glm::exp(-sqrK * l * l);
}

glm::vec2 OceanManager::hTiled_0(glm::vec2 k) {
  float phillips = (k.x == 0 && k.y == 0) ? 0 : glm::sqrt(phillipsSpectrum(k));
  return {guassian() * phillips / sqrt(2), guassian() * phillips / glm::sqrt(2)};
}

void OceanManager::initOceanData() {
  int N = oceanConstant.N;
  for(int i = 0; i < mm.config().numFrame; ++i) {
    datumBuffers.push_back(
      u<HostStorageBuffer>(device.allocator(), N * N * sizeof(Ocean)));
    auto ptr = datumBuffers.back()->ptr<Ocean>();
    for(auto row = 0; row < N; ++row) {
      glm::vec2 k;
      k.y = (float(-N) / 2.f + row) * 2 * PI / waveConfig.patchSize;
      for(auto column = 0; column < N; ++column) {
        k.x = (float(-N) / 2.f + column) * 2 * PI / waveConfig.patchSize;
        glm::vec2 phi = hTiled_0(k);
        ptr[row * N + column].h0 = {phi.x, phi.y};
      }
    }
  }
  {}
}

}
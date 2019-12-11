#pragma once
#include "sim/graphics/renderer/default/model/default_model_manager.h"
#include <unordered_map>
#include "sim/graphics/base/debug_marker.h"

namespace sim::graphics::renderer::defaultRenderer {
using namespace sim::graphics;

class DeferredModel: public DefaultModelManager {
  friend class Deferred;

public:
  explicit DeferredModel(
    Device &context, DebugMarker &debugMarker, const ModelConfig &config = {},
    bool enableIBL = false);
  void loadEnvironmentMap(const std::string &imagePath);

private:
  enum class EnvMap { Irradiance, PreFiltered };

  void loadSkybox();
  void generateEnvMap();
  void generateEnvMap(EnvMap envMap);
  void generateBRDFLUT();

  void updateInstances() override;
  void generateIBLTextures();

  void printSceneInfo();

private:
  DebugMarker &debugMarker;
  uPtr<HostIndirectBuffer> drawCMDs, drawSkyboxCMD;
  Range tri, debugLine, translucent;

  std::vector<uint32_t> idxMesh;

  const bool enableIBL;
  struct {
    uint32_t environment{-1u}, brdfLUT{-1u}, irradiance{-1u}, prefiltered{-1u};
    bool genEnvMap{true}, genBRDFLut{true};

    struct {
      uint32_t prefilteredCubeMipLevels{1};
      float scaleIBLAmbient{0.5f};
    } _shader_envMapInfo;

    uPtr<HostUniformBuffer> envMapInfoBuffer;
  } IBL;
};
}

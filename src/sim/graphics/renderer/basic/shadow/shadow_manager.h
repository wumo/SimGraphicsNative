#pragma once
#include "sim/graphics/base/device.h"
#include "sim/graphics/base/debug_marker.h"
#include "sim/graphics/base/resource/images.h"
#include "sim/graphics/base/resource/buffers.h"
#include "sim/graphics/base/pipeline/pipeline.h"
#include "sim/graphics/base/pipeline/descriptors.h"
#include "../model/shadow.h"

namespace sim::graphics::renderer::basic {
class BasicSceneManager;

class ShadowManager {
public:
  explicit ShadowManager(BasicSceneManager &mm);

  void init();

private:
  void createShadowMap();

private:
  BasicSceneManager &mm;
  Device &device;
  DebugMarker &debugMarker;

  struct ShadowSettings {
    bool SnapCascades = true;
    bool StabilizeExtents = true;
    bool EqualizeExtents = true;
    bool SearchBestCascade = true;
    bool FilterAcrossCascades = true;
    int Resolution = 2048;
    float PartitioningFactor = 0.95f;
    vk::Format Format = vk::Format::eD16Unorm;
    ShadowMode iShadowMode{ShadowMode::PCF};

    bool Is32BitFilterableFmt = true;
  } ShadowSettings;

  struct DistributeCascadeInfo {
    /// Pointer to camera view matrix, must not be null.
    const glm::mat4 *pCameraView = nullptr;

    /// Pointer to camera world matrix.
    const glm::mat4 *pCameraWorld = nullptr;

    /// Pointer to camera projection matrix, must not be null.
    const glm::mat4 *pCameraProj = nullptr;

    /// Pointer to light direction, must not be null.
    const glm::vec3 *pLightDir = nullptr;

    /// Whether to snap cascades to texels in light view space
    bool SnapCascades = true;

    /// Wether to stabilize cascade extents in light view space
    bool StabilizeExtents = true;

    /// Wether to use same extents for X and Y axis. Enabled automatically if StabilizeExtents == true
    bool EqualizeExtents = true;

    /// Cascade partitioning factor that defines the ratio between fully linear (0.0) and
    /// fully logarithmic (1.0) partitioning.
    float fPartitioningFactor = 0.95f;

    /// Callback that allows the application to adjust z range of every cascade.
    /// The callback is also called with cascade value -1 to adjust that entire camera range.
    std::function<void(int, float &, float &)> AdjustCascadeRange;
  };

  struct CascadeTransforms {
    glm::mat4 Proj;
    glm::mat4 WorldToLightProjSpace;
  };

  LightAttribs lightAttribs;
  uPtr<HostUniformBuffer> lightAttribsUBO_;

  uPtr<Texture> shadowMap, filterableShadowMap, intermediateMap;

  std::vector<vk::UniqueImageView> shadowMapDSVs;

  vk::DescriptorSet shadowSet;
  vk::UniquePipeline shadowPipeline;
};
}
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
  void createConversionTechs(vk::Format format);

private:
  BasicSceneManager &mm;
  Device &device;
  DebugMarker &debugMarker;

  struct ShadowSettings {
    bool snapCascades = true;
    bool stabilizeExtents = true;
    bool equalizeExtents = true;
    bool searchBestCascade = true;
    bool filterAcrossCascades = true;
    int resolution = 2048;
    float partitioningFactor = 0.95f;
    vk::Format format = vk::Format::eD16Unorm;
    ShadowMode shadowMode{ShadowMode::PCF};

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

  using shader = vk::ShaderStageFlagBits;
  struct ShadowConversionsDescriptorSet: DescriptorSetDef {
    __uniform__(ubo, shader::eFragment);
    __sampler__(shadowMap, shader::eFragment);
  } shadowSetDef;

  struct ShadowMapLayoutDef: PipelineLayoutDef {
    __set__(set, ShadowConversionsDescriptorSet);
  } shadowLayoutDef;

  LightAttribs lightAttribs;
  uPtr<HostUniformBuffer> lightAttribsUBO_;

  uPtr<Texture> shadowMap;
  std::vector<vk::UniqueImageView> shadowMapDSVs;
  uPtr<Texture> filterableShadowMap;
  std::vector<vk::UniqueImageView> filterableShadowMapRTVs;
  uPtr<Texture> intermediateMap;

  uPtr<HostUniformBuffer> conversionAttribsUBO;
  std::vector<CascadeTransforms> cascadeTransforms;

  struct ShadowConversionTechnique {
    vk::UniquePipeline pipeline;
    vk::DescriptorSet set;
  };
  std::array<ShadowConversionTechnique, uint32_t(ShadowMode::EVSM4) + 1> conversionTech;
  ShadowConversionTechnique blurVertTech;
};
}
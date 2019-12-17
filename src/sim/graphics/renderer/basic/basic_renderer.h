#pragma once
#include "sim/graphics/base/vulkan_base.h"
#include "sim/graphics/base/resource/buffers.h"
#include "sim/graphics/base/resource/images.h"
#include "basic_model_manager.h"

namespace sim::graphics::renderer::basic {

class BasicRenderer: public VulkanBase {
  friend class BasicModelManager;

public:
  explicit BasicRenderer(
    const Config &config = {}, const ModelConfig &modelConfig = {},
    const DebugConfig &debugConfig = {});

  ~BasicRenderer() override = default;

  BasicModelManager &modelManager() const;

protected:
  void createQueryPool();
  void createModelManager();

  void createRenderPass();
  void createPipelines();
  void createOpaquePipeline(const vk::PipelineLayout &pipelineLayout);
  void createDeferredPipeline(const vk::PipelineLayout &pipelineLayout);
  void createTranslucentPipeline(const vk::PipelineLayout &pipelineLayout);
  void createTerrainPipeline(const vk::PipelineLayout &pipelineLayout);

  void recreateResources();

  void resize() override;

  void updateFrame(
    std::function<void(uint32_t, float)> &updater, uint32_t imageIndex,
    float elapsedDuration) override;

protected:
  const ModelConfig modelConfig;

  const vk::Device &vkDevice;
  vk::SampleCountFlagBits sampleCount{vk::SampleCountFlagBits ::e1};
  bool enableSampleShading{true};
  float minSampleShading{1};

  uPtr<BasicModelManager> mm{};
  vk::UniquePipelineCache pipelineCache;
  vk::UniqueQueryPool queryPool;
  std::vector<uint64_t> pipelineStats;
  std::vector<std::string> pipelineStatNames;

  vk::UniqueRenderPass renderPass;

  std::vector<vk::UniqueFramebuffer> framebuffers{};

  struct {
    uint32_t gBuffer, deferred, translucent, combine;
  } Subpasses{};

  struct {
    vk::UniquePipeline opaqueTri, opaqueLine, opaqueTriWireframe;
    vk::UniquePipeline deferred, deferredIBL;
    vk::UniquePipeline transTri, transLine;
    vk::UniquePipeline terrain, terrainWireframe;
  } Pipelines;

  struct {
    uPtr<StorageAttachmentImage> offscreenImage;
    uPtr<ColorInputAttachmentImage> position, normal, albedo, pbr, emissive, translucent;
    uPtr<DepthStencilImage> depth;
  } attachments;
};
}

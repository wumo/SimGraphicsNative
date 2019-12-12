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
  void createModelManager();
  void createDirectPipeline();
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
  vk::UniqueRenderPass renderPass;
  std::vector<vk::UniqueFramebuffer> framebuffers{};

  struct Pipe {
    vk::UniquePipeline pipeline;
    uint32_t subpass;
  } opaqueTri, opaqueLine, deferred, deferredIBL, transTri, transLine;
  Pipe opaqueTriWireframe;

  struct {
    uPtr<StorageAttachmentImage> offscreenImage;
    uPtr<ColorInputAttachmentImage> position, normal, albedo, pbr, emissive;
    uPtr<DepthStencilImage> depth;
  } attachments;
};
}

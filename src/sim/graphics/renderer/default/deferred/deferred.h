#pragma once
#include "sim/graphics/renderer/default/renderer.h"
#include "deferred_model.h"
//#include "sim/graphics/renderpass/gui_pass.h"

namespace sim::graphics::renderer::defaultRenderer {
using namespace sim::graphics;

struct DeferredConfig: Config {
  bool enableImageBasedLighting{false};
  bool displayWireFrame{false};
};

class Deferred: public Renderer {
  using flag = vk::DescriptorBindingFlagBitsEXT;
  using shader = vk::ShaderStageFlagBits;

public:
  explicit Deferred(
    const DeferredConfig &config = {}, const ModelConfig &modelLimit = {},
    const DebugConfig &debugConfig = {});

  DeferredModel &modelManager() const;
  void updateModels() override;

private:
  void recreateResources();
  void resize() override;

  void createModelManger();

  void createDeferredRenderPass();
  void createDescriptorSets();
  void createDeferredPipeline();

  void render(
    std::function<void(uint32_t, float)> &updater, uint32_t imageIndex,
    float elapsedDuration) override;

protected:
  const DeferredConfig config;
  vk::SampleCountFlagBits sampleCount{vk::SampleCountFlagBits ::e1};
  bool enableSampleShading{true};
  float minSampleShading{1};

  uPtr<DeferredModel> mm;

  struct {
    uPtr<ColorInputAttachmentImage> translucent, position, normal, albedo, pbr, emissive;
    uPtr<DepthStencilImage> depth;
  } attachments;

  Pass depthPre, gBuffer, gBufferWireframe, lighting, shading, debugLines, translucent;

  vk::UniquePipelineCache pipelineCache;
  vk::UniqueRenderPass renderPass;
  std::vector<vk::UniqueFramebuffer> framebuffers;
  vk::UniqueDescriptorPool descriptorPool;

//  uPtr<GuiPass> gui;

  float normalLength{1.f};

  struct GBufferDescriptorSet: DescriptorSetDef {
    __uniform__(cam, shader::eVertex);
    __buffer__(transforms, shader::eVertex);
    __buffer__(instances, shader::eVertex | shader::eGeometry);
    __buffer__(materials, shader::eFragment);

    __samplers__(
      texture,
      flag::eVariableDescriptorCount | flag::ePartiallyBound |
        flag::eUpdateUnusedWhilePending | flag::eUpdateAfterBind,
      shader::eFragment);
  } gBufferDef;
  vk::DescriptorSet gBufferSet;

  struct LightingDescriptorSet: DescriptorSetDef {
    __uniform__(cam, shader::eVertex);

    __input__(position, shader::eFragment);
    __input__(normal, shader::eFragment);
    __input__(albedo, shader::eFragment);
    __input__(pbr, shader::eFragment);
    __input__(emissive, shader::eFragment);

    __buffer__(lights, shader::eFragment);
  } deferredDef;
  vk::DescriptorSet deferredSet;

  struct IBLDescriptorSet: DescriptorSetDef {
    __sampler__(irradiance, shader::eFragment);
    __sampler__(prefiltered, shader::eFragment);
    __sampler__(brdfLUT, shader::eFragment);
    __uniform__(envMap, shader::eFragment);
  } iblDef;
  vk::DescriptorSet iblSet;
};

}

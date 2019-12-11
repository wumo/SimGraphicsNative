#pragma once
#include "sim/graphics/renderer/default/renderer.h"
#include "sim/graphics/renderer/default/model/camera.h"
#include "sim/graphics/renderer/default/model/light.h"
//#include "sim/graphics/renderpass/gui_pass.h"
#include "raytracing_model.h"
#include "raytracing_pipeline.h"

namespace sim::graphics::renderer::defaultRenderer {
struct RayTracingConfig: Config {
  bool debug{false};
  bool displayWireFrame{false};
  uint32_t maxRecursion{3};
  uint32_t maxNumAccelerationStructure = 1;
};
class RayTracing: public Renderer {
  using flag = vk::DescriptorBindingFlagBitsEXT;
  using shader = vk::ShaderStageFlagBits;

public:
  explicit RayTracing(
    const RayTracingConfig &config = {}, const ModelConfig &modelLimit = {},
    const DebugConfig &debugConfig = {});
  ModelManager &modelManager() const;
  void updateModels() override;

private:
  void createSceneManager();

  void createDescriptorPool();
  void createCommonSet();

  void recreateResources();
  void resize() override;

  void createRayTracingPipeline();
  void createRayTracingDescriptorSets();
  void updateRayTracingSets();

  void createDeferredRenderPass();
  void createDeferredSets();
  void createDeferredPipeline();
  void resizeDeferred();
  void recreateDeferredFrameBuffer();
  void drawDeferred(uint32_t imageIndex);

  void render(
    std::function<void(uint32_t, float)> &updater, uint32_t imageIndex,
    float elapsedDuration) override;
  void traceRays(const vk::CommandBuffer &cb);

private:
  const RayTracingConfig config;

  uPtr<RayTracingModel> mm;
  vk::UniqueDescriptorPool descriptorPool;

  struct CommonSet: DescriptorSetDef {
    __uniform__(
      cam,
      shader::eVertex | shader::eFragment | shader::eRaygenNV | shader::eClosestHitNV);
    __buffer__(lights, shader::eClosestHitNV | shader::eFragment);
    __buffer__(transforms, shader::eVertex);
    __buffer__(instances, shader::eClosestHitNV | shader::eVertex);
    __buffer__(materials, shader::eClosestHitNV | shader::eFragment);

    __samplers__(
      texture,
      flag::eVariableDescriptorCount | flag::ePartiallyBound |
        flag::eUpdateUnusedWhilePending | flag::eUpdateAfterBind,
      shader::eFragment | shader::eClosestHitNV);
  } commonSetDef;
  vk::DescriptorSet commonSet;

  struct RayTracingDescriptorSet: DescriptorSetDef {
    __accelerationStructure__(as, shader::eRaygenNV | shader::eClosestHitNV);
    __storageImage__(offscreen, shader::eRaygenNV);
    __storageImage__(depth, shader::eRaygenNV);

    __buffer__(vertices, shader::eClosestHitNV);
    __buffer__(indices, shader::eClosestHitNV);
    __buffer__(meshes, shader::eClosestHitNV);
  };

  struct {
    vk::PhysicalDeviceRayTracingPropertiesNV properties;
    RayTracingDescriptorSet def;
    vk::DescriptorSet set;

    uPtr<StorageAttachmentImage> depth;
    struct {
      vk::UniquePipelineLayout pipelineLayout;
      UniqueRayTracingPipeline pipeline;
      ShaderBindingTable sbt;
    } pipe;
    vk::UniquePipelineCache pipeCache;
  } rayTracing;

  struct DeferredAttachmentSet: DescriptorSetDef {
    __input__(position, shader::eFragment);
    __input__(normal, shader::eFragment);
    __input__(albedo, shader::eFragment);
    __input__(pbr, shader::eFragment);
    __samplers__(depth, flag::eUpdateAfterBind, shader::eFragment);
  };

  struct {
    uPtr<ColorInputAttachmentImage> position, normal, albedo, pbr;
    uPtr<DepthStencilImage> depth;
    DeferredAttachmentSet inputsDef;
    vk::DescriptorSet inputsSet;
    vk::UniqueRenderPass pass;
    std::vector<vk::UniqueFramebuffer> frameBuffers;
    vk::UniquePipelineCache pipeCache;
    Pass gBuffer, gBufferLine, wireFrame, deferred;
  } debug;

//  uPtr<GuiPass> gui;
};

}
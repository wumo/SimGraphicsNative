#include "raytracing.h"

#include <memory>
#include <utility>
#include "sim/graphics/base/vulkan_base.h"
#include "raytracing_pipeline.h"

namespace sim::graphics::renderer::defaultRenderer {
using namespace std::string_literals;
using layout = vk::ImageLayout;
using stage = vk::PipelineStageFlagBits;
using access = vk::AccessFlagBits;
using bindpoint = vk::PipelineBindPoint;
using descriptor = vk::DescriptorType;
using ASType = vk::AccelerationStructureTypeNV;
using memReqType = vk::AccelerationStructureMemoryRequirementsTypeNV;
using buildASFlag = vk::BuildAccelerationStructureFlagBitsNV;

RayTracing::RayTracing(
  const RayTracingConfig &config, const ModelConfig &modelLimit,
  const DebugConfig &debugConfig)
  : Renderer{config, modelLimit, FeatureConfig{FeatureConfig::RayTracing}, debugConfig},
    config{config} {
  createSceneManager();

  createDescriptorPool();
  createCommonSet();

  createRayTracingPipeline();
  if(config.debug) createDeferredPipeline();
  //  if(config.gui)
  //    gui = u<GuiPass>(window, *vkInstance, *device, *swapchain, *descriptorPool);

  recreateResources();
}

ModelManager &RayTracing::modelManager() const { return *mm; }

void RayTracing::createSceneManager() {
  mm = u<RayTracingModel>(*device, modelLimit);

  for(auto &img: mm->Image.textures)
    debugMarker.name(img.image(), "image");
  debugMarker.name(mm->Buffer.instancess->buffer(), "triangleInstances");
  debugMarker.name(mm->Buffer.vertices->buffer(), "vertices");
  debugMarker.name(mm->Buffer.indices->buffer(), "indices");
  debugMarker.name(mm->Buffer.meshes->buffer(), "meshes");
  debugMarker.name(mm->Buffer.materials->buffer(), "materials");
  debugMarker.name(mm->Buffer.transforms->buffer(), "transforms");
}

void RayTracing::createDescriptorPool() {
  std::vector<vk::DescriptorPoolSize> poolSizes{
    {vk::DescriptorType::eUniformBuffer, modelLimit.maxNumUniformBuffer},
    {vk::DescriptorType::eCombinedImageSampler, modelLimit.maxNumCombinedImageSampler +
                                                  (config.gui ? 1 : 0) +
                                                  (config.debug ? 1 : 0)},
    {vk::DescriptorType::eInputAttachment, modelLimit.maxNumInputAttachment},
    {vk::DescriptorType::eStorageBuffer, modelLimit.maxNumStorageBuffer},
    {vk::DescriptorType::eAccelerationStructureNV, config.maxNumAccelerationStructure},
    {vk::DescriptorType::eStorageImage, modelLimit.maxNumStorageImage}};
  vk::DescriptorPoolCreateInfo descriptorPoolInfo{
    vk::DescriptorPoolCreateFlagBits::eUpdateAfterBindEXT, modelLimit.maxNumSet,
    uint32_t(poolSizes.size()), poolSizes.data()};
  descriptorPool = vkDevice.createDescriptorPoolUnique(descriptorPoolInfo);
}

void RayTracing::createCommonSet() {
  commonSetDef.texture.descriptorCount() = modelLimit.maxNumCombinedImageSampler;
  commonSetDef.init(vkDevice);
  commonSet = commonSetDef.createSet(*descriptorPool);
  commonSetDef.cam(camBuffer->buffer());
  commonSetDef.lights(lightsBuffer->buffer());
  commonSetDef.transforms(mm->Buffer.transforms->buffer());
  commonSetDef.instances(mm->Buffer.instancess->buffer());
  commonSetDef.materials(mm->Buffer.materials->buffer());
  commonSetDef.update(commonSet);
}

void RayTracing::createRayTracingDescriptorSets() {
  rayTracing.def.init(vkDevice);
  rayTracing.set = rayTracing.def.createSet(*descriptorPool);
  rayTracing.def.vertices(mm->Buffer.vertices->buffer());
  rayTracing.def.indices(mm->Buffer.indices->buffer());
  rayTracing.def.meshes(mm->Buffer.meshes->buffer());
  rayTracing.def.update(rayTracing.set);
}

void RayTracing::createRayTracingPipeline() {
  createRayTracingDescriptorSets();

  rayTracing.pipe.pipelineLayout =
    PipelineLayoutMaker()
      .descriptorSetLayouts(
        {*rayTracing.def.descriptorSetLayout, *commonSetDef.descriptorSetLayout})
      .createUnique(vkDevice);
  rayTracing.pipeCache = vkDevice.createPipelineCacheUnique({});
  rayTracing.properties = device->getRayTracingProperties();
}

void RayTracing::updateRayTracingSets() {
  rayTracing.depth = u<StorageAttachmentImage>(
    *device, extent.width, extent.height, vk::Format::eR32Sfloat);
  rayTracing.depth->setSampler(SamplerMaker{}.createUnique(vkDevice));
  debugMarker.name(rayTracing.depth->image(), "raytracing depth");
  rayTracing.def.offscreen(offscreenImage->imageView());
  rayTracing.def.depth(rayTracing.depth->imageView());
  rayTracing.def.update(rayTracing.set);
}

void RayTracing::recreateResources() {
  updateRayTracingSets();

  if(config.debug) resizeDeferred();
  //  if(config.gui) gui->resizeGui(extent);
}

void RayTracing::resize() {
  Renderer::resize();

  recreateResources();
}
}
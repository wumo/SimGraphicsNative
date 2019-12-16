#include <random>
#include "sim/graphics/renderer/default/model/default_model_manager.h"
#include "sim/graphics/base/pipeline/pipeline.h"
#include "sim/graphics/base/pipeline/descriptors.h"
#include "sim/graphics/base/pipeline/render_pass.h"
#include "sim/graphics/compiledShaders/raster/gbuffer_vert.h"
#include "sim/graphics/compiledShaders/raster/gbuffer_frag.h"
#include "sim/graphics/compiledShaders/raster/deferred_vert.h"
#include "sim/graphics/compiledShaders/raster/deferred_frag.h"
#include "sim/graphics/compiledShaders/raster/deferred_ms_frag.h"
#include "sim/graphics/compiledShaders/raster/deferred_ibl_frag.h"
#include "sim/graphics/compiledShaders/raster/deferred_ms_ibl_frag.h"
#include "sim/graphics/compiledShaders/raster/debug_vert.h"
#include "sim/graphics/compiledShaders/raster/translucent_frag.h"
#include "deferred.h"
#include "deferred_model.h"

namespace sim::graphics::renderer::defaultRenderer {
using namespace std::string_literals;
using namespace glm;
using layout = vk::ImageLayout;
using stage = vk::PipelineStageFlagBits;
using access = vk::AccessFlagBits;
using bindpoint = vk::PipelineBindPoint;
using descriptor = vk::DescriptorType;

Deferred::Deferred(
  const DeferredConfig &config, const ModelConfig &modelLimit,
  const DebugConfig &debugConfig)
  : Renderer{config, modelLimit, FeatureConfig{FeatureConfig::DescriptorIndexing},
             debugConfig},
    config{config} {
  sampleCount = static_cast<vk::SampleCountFlagBits>(config.sampleCount);

  createModelManger();

  createDescriptorSets();

  createDeferredPipeline();

  //  if(config.gui)
  //    gui = u<GuiPass>(window, *vkInstance, *device, *swapchain, *descriptorPool);

  recreateResources();
}

DeferredModel &Deferred::modelManager() const { return *mm; }

void Deferred::createModelManger() {
  mm =
    u<DeferredModel>(*device, debugMarker, modelLimit, config.enableImageBasedLighting);

  debugMarker.name(mm->Buffer.vertices->buffer(), "vertices");
  debugMarker.name(mm->Buffer.indices->buffer(), "indices");
  debugMarker.name(mm->Buffer.transforms->buffer(), "transforms");
  debugMarker.name(mm->Buffer.materials->buffer(), "materials");
  debugMarker.name(mm->Buffer.instancess->buffer(), "instances");
  debugMarker.name(mm->Image.textures[0].image(), "pixel texture");

  debugMarker.name(mm->drawCMDs->buffer(), "draw commands");

  debugMarker.name(camBuffer->buffer(), "ubo.cam");
  debugMarker.name(lightsBuffer->buffer(), "ubo.lights");
}

void Deferred::createDeferredRenderPass() {
  RenderPassMaker maker;
  auto color = maker.attachment(swapchain->getImageFormat())
                 .samples(sampleCount)
                 .loadOp(vk::AttachmentLoadOp::eClear)
                 .storeOp(vk::AttachmentStoreOp::eStore)
                 .stencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                 .stencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                 .initialLayout(layout::eUndefined)
                 .finalLayout(layout::eColorAttachmentOptimal)
                 .index();
  auto position = maker.attachment(vk::Format::eR32G32B32A32Sfloat)
                    .storeOp(vk::AttachmentStoreOp::eDontCare)
                    .finalLayout(layout::eColorAttachmentOptimal)
                    .index();
  auto normal = maker.attachment(vk::Format::eR32G32B32A32Sfloat).index();
  auto albedo = maker.attachment(vk::Format::eR8G8B8A8Unorm).index();
  auto pbr = maker.attachment(vk::Format::eR32G32Sfloat).index();
  auto emissive = maker.attachment(vk::Format::eR8G8B8A8Unorm).index();
  auto depth = maker.attachment(vk::Format::eD24UnormS8Uint)
                 .finalLayout(layout::eDepthStencilAttachmentOptimal)
                 .index();
  auto translucentAttach = maker.attachment(vk::Format::eR8G8B8A8Unorm)
                             .finalLayout(layout::eColorAttachmentOptimal)
                             .index();
  auto backbuffer = maker.attachment(swapchain->getImageFormat())
                      .samples(vk::SampleCountFlagBits::e1)
                      .loadOp(vk::AttachmentLoadOp::eClear)
                      .storeOp(vk::AttachmentStoreOp::eStore)
                      .finalLayout(layout::eColorAttachmentOptimal)
                      .index();
  auto shadingOutput = config.sampleCount > 1 ? color : backbuffer;
  gBuffer.subpass = maker.subpass(bindpoint::eGraphics)
                      .color(position)
                      .color(normal)
                      .color(albedo)
                      .color(pbr)
                      .color(emissive)
                      .depthStencil(depth)
                      .index();
  shading.subpass = maker.subpass(bindpoint::eGraphics)
                      .color(shadingOutput)
                      .input(position)
                      .input(normal)
                      .input(albedo)
                      .input(pbr)
                      .input(emissive)
                      .preserve(depth)
                      .index();

  auto &spMaker =
    maker.subpass(bindpoint::eGraphics).color(shadingOutput).depthStencil(depth);

  if(config.sampleCount > 1)
    spMaker.resolve(backbuffer); //resolve using tiled on chip memory.

  translucent.subpass = spMaker.index();

  // A dependency to reset the layout of both attachments.
  maker.dependency(VK_SUBPASS_EXTERNAL, gBuffer.subpass)
    .srcStageMask(stage::eBottomOfPipe)
    .dstStageMask(stage::eColorAttachmentOutput)
    .srcAccessMask(access::eMemoryRead)
    .dstAccessMask(access::eColorAttachmentWrite)
    .dependencyFlags(vk::DependencyFlagBits::eByRegion);
  maker.dependency(gBuffer.subpass, shading.subpass)
    .srcStageMask(stage::eColorAttachmentOutput)
    .dstStageMask(stage::eFragmentShader)
    .srcAccessMask(access::eColorAttachmentWrite)
    .dstAccessMask(access::eInputAttachmentRead)
    .dependencyFlags(vk::DependencyFlagBits::eByRegion);
  maker.dependency(shading.subpass, translucent.subpass)
    .srcStageMask(stage::eColorAttachmentOutput)
    .dstStageMask(stage::eEarlyFragmentTests | stage::eLateFragmentTests)
    .srcAccessMask(access::eColorAttachmentWrite)
    .dstAccessMask(access::eDepthStencilAttachmentRead)
    .dependencyFlags(vk::DependencyFlagBits::eByRegion);
  maker.dependency(translucent.subpass, VK_SUBPASS_EXTERNAL)
    .srcStageMask(stage::eColorAttachmentOutput)
    .dstStageMask(stage::eBottomOfPipe)
    .srcAccessMask(access::eColorAttachmentWrite)
    .dstAccessMask(access::eMemoryRead)
    .dependencyFlags(vk::DependencyFlagBits::eByRegion);

  renderPass = maker.createUnique(vkDevice);
}

void Deferred::createDescriptorSets() {
  auto numSampler = modelLimit.maxNumCombinedImageSampler;
  if(config.gui) numSampler += 1;
  if(config.enableImageBasedLighting) numSampler += 3;

  std::vector<vk::DescriptorPoolSize> poolSizes{
    {vk::DescriptorType::eUniformBuffer, modelLimit.maxNumUniformBuffer},
    {vk::DescriptorType::eCombinedImageSampler, numSampler},
    {vk::DescriptorType::eInputAttachment, modelLimit.maxNumInputAttachment},
    {vk::DescriptorType::eStorageBuffer, modelLimit.maxNumStorageBuffer},
  };
  vk::DescriptorPoolCreateInfo descriptorPoolInfo{
    vk::DescriptorPoolCreateFlagBits::eUpdateAfterBindEXT, modelLimit.maxNumSet,
    (uint32_t)poolSizes.size(), poolSizes.data()};
  descriptorPool = vkDevice.createDescriptorPoolUnique(descriptorPoolInfo);

  gBufferDef.texture.descriptorCount() = uint32_t(modelLimit.maxNumCombinedImageSampler);

  gBufferDef.init(vkDevice);
  gBufferSet = gBufferDef.createSet(*descriptorPool);
  gBufferDef.cam(camBuffer->buffer());
  gBufferDef.transforms(mm->Buffer.transforms->buffer());
  gBufferDef.instances(mm->Buffer.instancess->buffer());
  gBufferDef.materials(mm->Buffer.materials->buffer());
  gBufferDef.update(gBufferSet);

  deferredDef.init(vkDevice);
  deferredSet = deferredDef.createSet(*descriptorPool);
  deferredDef.cam(camBuffer->buffer());
  deferredDef.lights(lightsBuffer->buffer());
  deferredDef.update(deferredSet);

  if(config.enableImageBasedLighting) {
    iblDef.init(vkDevice);
    iblSet = iblDef.createSet(*descriptorPool);
    iblDef.envMap(mm->IBL.envMapInfoBuffer->buffer());
    iblDef.brdfLUT(mm->Image.textures[mm->IBL.brdfLUT]);
    iblDef.irradiance(mm->Image.cubeTextures[mm->IBL.irradiance]);
    iblDef.prefiltered(mm->Image.cubeTextures[mm->IBL.prefiltered]);
    iblDef.update(iblSet);
    debugMarker.name(iblSet, "IBL descriptor set");
  }

  debugMarker.name(gBufferSet, "gbuffer descriptor set");
  debugMarker.name(deferredSet, "deferred descriptor set");
}

void Deferred::createDeferredPipeline() {
  createDeferredRenderPass();

  gBuffer.pipelineLayout = PipelineLayoutMaker()
                             .descriptorSetLayout(*gBufferDef.descriptorSetLayout)
                             .createUnique(vkDevice);
  {
    PipelineLayoutMaker pipelineLayoutMaker;
    pipelineLayoutMaker.descriptorSetLayout(*deferredDef.descriptorSetLayout);
    if(config.enableImageBasedLighting)
      pipelineLayoutMaker.descriptorSetLayout(*iblDef.descriptorSetLayout);
    shading.pipelineLayout = pipelineLayoutMaker.createUnique(vkDevice);
  }
  pipelineCache = vkDevice.createPipelineCacheUnique({});
  { // gbuffer pipeline
    GraphicsPipelineMaker pipelineMaker{vkDevice, extent.width, extent.height};
    pipelineMaker.subpass(gBuffer.subpass)
      .vertexBinding(0, Vertex::stride(), Vertex::attributes(0, 0))
      .cullMode(vk::CullModeFlagBits::eBack)
      .frontFace(vk::FrontFace::eCounterClockwise)
      .depthTestEnable(true)
      .depthCompareOp(vk::CompareOp::eLessOrEqual)
      .dynamicState(vk::DynamicState::eViewport)
      .dynamicState(vk::DynamicState::eScissor)
      .rasterizationSamples(sampleCount)
      .sampleShadingEnable(enableSampleShading)
      .minSampleShading(minSampleShading);

    pipelineMaker.blendColorAttachment(false);
    pipelineMaker.blendColorAttachment(false);
    pipelineMaker.blendColorAttachment(false);
    pipelineMaker.blendColorAttachment(false);
    pipelineMaker.blendColorAttachment(false);

    pipelineMaker.shader(shader::eVertex, gbuffer_vert, __ArraySize__(gbuffer_vert))
      .shader(shader::eFragment, gbuffer_frag, __ArraySize__(gbuffer_frag));
    gBuffer.pipeline = pipelineMaker.createUnique(
      vkDevice, *pipelineCache, *gBuffer.pipelineLayout, *renderPass);
    debugMarker.name(*gBuffer.pipeline, "gbuffer triangle pipeline");

    pipelineMaker.topology(vk::PrimitiveTopology::eLineList)
      .cullMode(vk::CullModeFlagBits::eNone)
      .lineWidth(mm->config.lineWidth);
    debugLines.pipeline = pipelineMaker.createUnique(
      vkDevice, *pipelineCache, *gBuffer.pipelineLayout, *renderPass);
    debugMarker.name(*debugLines.pipeline, "debug line pipeline");

    if(config.displayWireFrame) {
      // wireframe pipeline
      pipelineMaker.topology(vk::PrimitiveTopology::eTriangleList)
        .cullMode(vk::CullModeFlagBits::eNone)
        .polygonMode(vk::PolygonMode::eLine)
        .lineWidth(1.f);
      gBufferWireframe.pipeline = pipelineMaker.createUnique(
        vkDevice, *pipelineCache, *gBuffer.pipelineLayout, *renderPass);
      debugMarker.name(*gBufferWireframe.pipeline, "wireframe pipeline");
    }
  }
  { // shading pipeline
    GraphicsPipelineMaker pipelineMaker{vkDevice, extent.width, extent.height};
    pipelineMaker.subpass(shading.subpass)
      .cullMode(vk::CullModeFlagBits::eNone)
      .frontFace(vk::FrontFace::eClockwise)
      .depthTestEnable(false)
      .dynamicState(vk::DynamicState::eViewport)
      .dynamicState(vk::DynamicState::eScissor)
      .rasterizationSamples(sampleCount)
      .sampleShadingEnable(enableSampleShading)
      .minSampleShading(minSampleShading)
      .blendColorAttachment(false);

    SpecializationMaker sp;
    auto spInfo = sp.entry(config.sampleCount).entry(config.displayWireFrame).create();
    pipelineMaker.shader(shader::eVertex, deferred_vert, __ArraySize__(deferred_vert));
    if(config.enableImageBasedLighting) {
      if(config.sampleCount > 1)
        pipelineMaker.shader(
          shader::eFragment, deferred_ms_ibl_frag, __ArraySize__(deferred_ms_ibl_frag),
          &spInfo);
      else
        pipelineMaker.shader(
          shader::eFragment, deferred_ibl_frag, __ArraySize__(deferred_ibl_frag),
          &spInfo);
    } else {
      if(config.sampleCount > 1)
        pipelineMaker.shader(
          shader::eFragment, deferred_ms_frag, __ArraySize__(deferred_ms_frag), &spInfo);
      else
        pipelineMaker.shader(
          shader::eFragment, deferred_frag, __ArraySize__(deferred_frag), &spInfo);
    }
    shading.pipeline = pipelineMaker.createUnique(
      vkDevice, *pipelineCache, *shading.pipelineLayout, *renderPass);
    debugMarker.name(*shading.pipeline, "deferred pipeline");
  }
  { // translucent pipeline
    GraphicsPipelineMaker pipelineMaker{vkDevice, extent.width, extent.height};
    pipelineMaker.subpass(translucent.subpass)
      .vertexBinding(0, Vertex::stride(), Vertex::attributes(0, 0))
      .cullMode(vk::CullModeFlagBits::eNone)
      .frontFace(vk::FrontFace::eCounterClockwise)
      .depthTestEnable(true)
      .depthCompareOp(vk::CompareOp::eLessOrEqual)
      .depthWriteEnable(false)
      .dynamicState(vk::DynamicState::eViewport)
      .dynamicState(vk::DynamicState::eScissor)
      .rasterizationSamples(sampleCount)
      .sampleShadingEnable(enableSampleShading)
      .minSampleShading(minSampleShading);

    using flag = vk::ColorComponentFlagBits;
    pipelineMaker.blendColorAttachment(true)
      .srcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
      .dstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
      .colorBlendOp(vk::BlendOp::eAdd)
      .srcAlphaBlendFactor(vk::BlendFactor::eOne)
      .dstAlphaBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
      .alphaBlendOp(vk::BlendOp::eAdd)
      .colorWriteMask(flag::eR | flag::eG | flag::eB | flag::eA);

    pipelineMaker.shader(shader::eVertex, debug_vert, __ArraySize__(debug_vert));
    pipelineMaker.shader(
      shader::eFragment, translucent_frag, __ArraySize__(translucent_frag));

    translucent.pipeline = pipelineMaker.createUnique(
      vkDevice, *pipelineCache, *gBuffer.pipelineLayout, *renderPass);
    debugMarker.name(*translucent.pipeline, "translucent pipeline");
  }
}

void Deferred::recreateResources() {
  attachments.position = u<ColorInputAttachmentImage>(
    *device, extent.width, extent.height, vk::Format::eR32G32B32A32Sfloat, sampleCount);
  attachments.normal = u<ColorInputAttachmentImage>(
    *device, extent.width, extent.height, vk::Format::eR32G32B32A32Sfloat, sampleCount);
  attachments.albedo = u<ColorInputAttachmentImage>(
    *device, extent.width, extent.height, vk::Format::eR8G8B8A8Unorm, sampleCount);
  attachments.pbr = u<ColorInputAttachmentImage>(
    *device, extent.width, extent.height, vk::Format::eR32G32Sfloat, sampleCount);
  attachments.emissive = u<ColorInputAttachmentImage>(
    *device, extent.width, extent.height, vk::Format::eR8G8B8A8Unorm, sampleCount);
  attachments.depth = u<DepthStencilImage>(
    *device, extent.width, extent.height, vk::Format::eD24UnormS8Uint, sampleCount);
  attachments.translucent = u<ColorInputAttachmentImage>(
    *device, extent.width, extent.height, vk::Format::eR8G8B8A8Unorm, sampleCount);

  debugMarker.name(attachments.translucent->image(), "translucent attchment");
  debugMarker.name(attachments.position->image(), "position attchment");
  debugMarker.name(attachments.normal->image(), "normal attchment");
  debugMarker.name(attachments.albedo->image(), "albedo attchment");
  debugMarker.name(attachments.pbr->image(), "pbr attchment");
  debugMarker.name(attachments.emissive->image(), "pbr attchment");
  debugMarker.name(attachments.depth->image(), "depth attchment");

  deferredDef.position(attachments.position->imageView());
  deferredDef.normal(attachments.normal->imageView());
  deferredDef.albedo(attachments.albedo->imageView());
  deferredDef.pbr(attachments.pbr->imageView());
  deferredDef.emissive(attachments.emissive->imageView());
  deferredDef.update(deferredSet);

  std::vector<vk::ImageView> _attachments = {offscreenImage->imageView(),
                                             attachments.position->imageView(),
                                             attachments.normal->imageView(),
                                             attachments.albedo->imageView(),
                                             attachments.pbr->imageView(),
                                             attachments.emissive->imageView(),
                                             attachments.depth->imageView(),
                                             attachments.translucent->imageView(),
                                             {}};
  vk::FramebufferCreateInfo info{{},
                                 *renderPass,
                                 uint32_t(_attachments.size()),
                                 _attachments.data(),
                                 extent.width,
                                 extent.height,
                                 1};

  framebuffers.resize(swapchain->getImageCount());
  for(auto i = 0u; i < swapchain->getImageCount(); i++) {
    _attachments.back() = *swapchain->getImageViews()[i];
    framebuffers[i] = vkDevice.createFramebufferUnique(info);
  }

  //  if(config.gui) gui->resizeGui(extent);
}

void Deferred::resize() {
  Renderer::resize();
  recreateResources();
}
}

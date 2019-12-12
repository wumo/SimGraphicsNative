#include "basic_renderer.h"
#include "sim/graphics/base/pipeline/render_pass.h"
#include "sim/graphics/base/pipeline/pipeline.h"
#include "sim/graphics/compiledShaders/basic/basic_vert.h"
#include "sim/graphics/compiledShaders/basic/gbuffer_frag.h"
#include "sim/graphics/compiledShaders/basic/quad_vert.h"
#include "sim/graphics/compiledShaders/basic/deferred_frag.h"
#include "sim/graphics/compiledShaders/basic/deferred_ms_frag.h"
#include "sim/graphics/compiledShaders/basic/deferred_ibl_frag.h"
#include "sim/graphics/compiledShaders/basic/deferred_ibl_ms_frag.h"
#include "sim/graphics/compiledShaders/basic/translucent_frag.h"

namespace sim::graphics::renderer::basic {
using layout = vk::ImageLayout;
using shader = vk::ShaderStageFlagBits;
using stage = vk::PipelineStageFlagBits;
using access = vk::AccessFlagBits;
using bindpoint = vk::PipelineBindPoint;
using descriptor = vk::DescriptorType;

BasicRenderer::BasicRenderer(
  const Config &config, const ModelConfig &modelConfig, const DebugConfig &debugConfig)
  : VulkanBase{config, {}, debugConfig},
    modelConfig{modelConfig},
    vkDevice{device->getDevice()} {
  sampleCount = static_cast<vk::SampleCountFlagBits>(config.sampleCount);

  createModelManager();
  createDirectPipeline();
  recreateResources();
}

BasicModelManager &BasicRenderer::modelManager() const { return *mm; }

void BasicRenderer::createModelManager() { mm = u<BasicModelManager>(*this); }

void BasicRenderer::createDirectPipeline() {
  RenderPassMaker maker;
  auto presentImage = maker.attachment(swapchain->getImageFormat())
                        .samples(vk::SampleCountFlagBits::e1)
                        .loadOp(vk::AttachmentLoadOp::eClear)
                        .storeOp(vk::AttachmentStoreOp::eStore)
                        .stencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                        .stencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                        .initialLayout(layout::eUndefined)
                        .finalLayout(layout::eColorAttachmentOptimal)
                        .index();
  auto color = maker.attachment(swapchain->getImageFormat()).samples(sampleCount).index();
  auto position = maker.attachment(vk::Format::eR32G32B32A32Sfloat)
                    .storeOp(vk::AttachmentStoreOp::eDontCare)
                    .finalLayout(layout::eColorAttachmentOptimal)
                    .index();
  auto normal = maker.attachment(vk::Format::eR32G32B32A32Sfloat).index();
  auto albedo = maker.attachment(vk::Format::eR8G8B8A8Unorm).index();
  auto pbr = maker.attachment(vk::Format::eR8G8B8A8Unorm).index();
  auto emissive = maker.attachment(vk::Format::eR8G8B8A8Unorm).index();
  auto depth = maker.attachment(vk::Format::eD24UnormS8Uint)
                 .storeOp(vk::AttachmentStoreOp::eDontCare)
                 .finalLayout(layout::eDepthStencilAttachmentOptimal)
                 .index();
  auto shadingOutput = config.sampleCount > 1 ? color : presentImage;
  opaqueTri.subpass = maker.subpass(bindpoint::eGraphics)
                        .color(position)
                        .color(normal)
                        .color(albedo)
                        .color(pbr)
                        .color(emissive)
                        .depthStencil(depth)
                        .index();
  deferred.subpass = maker.subpass(bindpoint::eGraphics)
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
    spMaker.resolve(presentImage); //resolve using tiled on chip memory.
  transTri.subpass = spMaker.index();

  maker.dependency(VK_SUBPASS_EXTERNAL, opaqueTri.subpass)
    .srcStageMask(stage::eBottomOfPipe)
    .dstStageMask(stage::eColorAttachmentOutput)
    .srcAccessMask(access::eMemoryRead)
    .dstAccessMask(access::eColorAttachmentRead | access::eColorAttachmentWrite)
    .dependencyFlags(vk::DependencyFlagBits::eByRegion);
  maker.dependency(opaqueTri.subpass, deferred.subpass)
    .srcStageMask(stage::eColorAttachmentOutput)
    .dstStageMask(stage::eFragmentShader)
    .srcAccessMask(access::eColorAttachmentWrite)
    .dstAccessMask(access::eInputAttachmentRead)
    .dependencyFlags(vk::DependencyFlagBits::eByRegion);
  maker.dependency(deferred.subpass, transTri.subpass)
    .srcStageMask(stage::eColorAttachmentOutput)
    .dstStageMask(stage::eEarlyFragmentTests | stage::eLateFragmentTests)
    .srcAccessMask(access::eColorAttachmentWrite)
    .dstAccessMask(access::eDepthStencilAttachmentRead)
    .dependencyFlags(vk::DependencyFlagBits::eByRegion);
  maker.dependency(transTri.subpass, VK_SUBPASS_EXTERNAL)
    .srcStageMask(stage::eColorAttachmentOutput)
    .dstStageMask(stage::eBottomOfPipe)
    .srcAccessMask(access::eColorAttachmentRead | access::eColorAttachmentWrite)
    .dstAccessMask(access::eMemoryRead)
    .dependencyFlags(vk::DependencyFlagBits::eByRegion);
  renderPass = maker.createUnique(vkDevice);

  auto pipelineLayout = *mm->basicLayout.pipelineLayout;
  pipelineCache = vkDevice.createPipelineCacheUnique({});

  { // Pipeline
    GraphicsPipelineMaker pipelineMaker{vkDevice, extent.width, extent.height};
    pipelineMaker.subpass(opaqueTri.subpass)
      .vertexBinding(0, Vertex::stride(), Vertex::attributes(0, 0))
      .topology(vk::PrimitiveTopology::eTriangleList)
      .polygonMode(vk::PolygonMode::eFill)
      .cullMode(vk::CullModeFlagBits::eBack)
      .frontFace(vk::FrontFace::eCounterClockwise)
      .depthTestEnable(true)
      .depthWriteEnable(true)
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

    pipelineMaker.shader(shader::eVertex, basic_vert, __ArraySize__(basic_vert));
    SpecializationMaker sp;
    auto spInfo = sp.entry(modelConfig.maxNumTexture).create();
    pipelineMaker.shader(
      shader::eFragment, gbuffer_frag, __ArraySize__(gbuffer_frag), &spInfo);
    opaqueTri.pipeline =
      pipelineMaker.createUnique(vkDevice, *pipelineCache, pipelineLayout, *renderPass);
    debugMarker.name(*opaqueTri.pipeline, "opaque triangle pipeline");

    pipelineMaker.polygonMode(vk::PolygonMode::eLine);

    opaqueTriWireframe.pipeline =
      pipelineMaker.createUnique(vkDevice, *pipelineCache, pipelineLayout, *renderPass);

    pipelineMaker.topology(vk::PrimitiveTopology::eLineList)
      .polygonMode(vk::PolygonMode::eFill)
      .cullMode(vk::CullModeFlagBits::eNone)
      .lineWidth(1.f);
    opaqueLine.pipeline =
      pipelineMaker.createUnique(vkDevice, *pipelineCache, pipelineLayout, *renderPass);
    debugMarker.name(*opaqueLine.pipeline, "opaque line pipeline");
  }
  { // shading pipeline
    GraphicsPipelineMaker pipelineMaker{vkDevice, extent.width, extent.height};
    pipelineMaker.subpass(deferred.subpass)
      .cullMode(vk::CullModeFlagBits::eNone)
      .frontFace(vk::FrontFace::eClockwise)
      .depthTestEnable(false)
      .dynamicState(vk::DynamicState::eViewport)
      .dynamicState(vk::DynamicState::eScissor)
      .rasterizationSamples(sampleCount)
      .sampleShadingEnable(enableSampleShading)
      .minSampleShading(minSampleShading)
      .blendColorAttachment(false);

    pipelineMaker.shader(shader::eVertex, quad_vert, __ArraySize__(quad_vert));
    if(config.sampleCount > 1) {
      pipelineMaker.shader(
        shader::eFragment, deferred_ms_frag, __ArraySize__(deferred_ms_frag));
    } else {
      pipelineMaker.shader(
        shader::eFragment, deferred_frag, __ArraySize__(deferred_frag));
    }
    deferred.pipeline =
      pipelineMaker.createUnique(vkDevice, *pipelineCache, pipelineLayout, *renderPass);
    debugMarker.name(*deferred.pipeline, "deferred pipeline");
  }

  { // shading pipeline for IBL
    GraphicsPipelineMaker pipelineMaker{vkDevice, extent.width, extent.height};
    pipelineMaker.subpass(deferred.subpass)
      .cullMode(vk::CullModeFlagBits::eNone)
      .frontFace(vk::FrontFace::eClockwise)
      .depthTestEnable(false)
      .dynamicState(vk::DynamicState::eViewport)
      .dynamicState(vk::DynamicState::eScissor)
      .rasterizationSamples(sampleCount)
      .sampleShadingEnable(enableSampleShading)
      .minSampleShading(minSampleShading)
      .blendColorAttachment(false);

    pipelineMaker.shader(shader::eVertex, quad_vert, __ArraySize__(quad_vert));
    if(config.sampleCount > 1)
      pipelineMaker.shader(
        shader::eFragment, deferred_ibl_ms_frag, __ArraySize__(deferred_ibl_ms_frag));
    else
      pipelineMaker.shader(
        shader::eFragment, deferred_ibl_frag, __ArraySize__(deferred_ibl_frag));
    deferredIBL.pipeline =
      pipelineMaker.createUnique(vkDevice, *pipelineCache, pipelineLayout, *renderPass);
    debugMarker.name(*deferredIBL.pipeline, "deferred IBL pipeline");
  }

  { // translucent pipeline
    GraphicsPipelineMaker pipelineMaker{vkDevice, extent.width, extent.height};
    pipelineMaker.subpass(transTri.subpass)
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

    pipelineMaker.shader(shader::eVertex, basic_vert, __ArraySize__(basic_vert));
    pipelineMaker.shader(
      shader::eFragment, translucent_frag, __ArraySize__(translucent_frag));

    transTri.pipeline =
      pipelineMaker.createUnique(vkDevice, *pipelineCache, pipelineLayout, *renderPass);
    debugMarker.name(*transTri.pipeline, "translucent tri pipeline");

    pipelineMaker.topology(vk::PrimitiveTopology::eLineList)
      .cullMode(vk::CullModeFlagBits::eNone)
      .lineWidth(1.f);
    transLine.pipeline =
      pipelineMaker.createUnique(vkDevice, *pipelineCache, pipelineLayout, *renderPass);
    debugMarker.name(*transLine.pipeline, "translucent line pipeline");
  }
}

void BasicRenderer::recreateResources() {
  attachments.offscreenImage = u<StorageAttachmentImage>(
    *device, extent.width, extent.height, swapchain->getImageFormat(), sampleCount);
  attachments.position = u<ColorInputAttachmentImage>(
    *device, extent.width, extent.height, vk::Format::eR32G32B32A32Sfloat, sampleCount);
  attachments.normal = u<ColorInputAttachmentImage>(
    *device, extent.width, extent.height, vk::Format::eR32G32B32A32Sfloat, sampleCount);
  attachments.albedo = u<ColorInputAttachmentImage>(
    *device, extent.width, extent.height, vk::Format::eR8G8B8A8Unorm, sampleCount);
  attachments.pbr = u<ColorInputAttachmentImage>(
    *device, extent.width, extent.height, vk::Format::eR8G8B8A8Unorm, sampleCount);
  attachments.emissive = u<ColorInputAttachmentImage>(
    *device, extent.width, extent.height, vk::Format::eR8G8B8A8Unorm, sampleCount);
  attachments.depth = u<DepthStencilImage>(
    *device, extent.width, extent.height, vk::Format::eD24UnormS8Uint, sampleCount);
  debugMarker.name(attachments.offscreenImage->image(), "offscreenImage");
  debugMarker.name(attachments.position->image(), "position attchment");
  debugMarker.name(attachments.normal->image(), "normal attchment");
  debugMarker.name(attachments.albedo->image(), "albedo attchment");
  debugMarker.name(attachments.pbr->image(), "pbr attchment");
  debugMarker.name(attachments.emissive->image(), "emissive attchment");
  debugMarker.name(attachments.depth->image(), "depth attchment");

  std::vector<vk::ImageView> _attachments = {{},
                                             attachments.offscreenImage->imageView(),
                                             attachments.position->imageView(),
                                             attachments.normal->imageView(),
                                             attachments.albedo->imageView(),
                                             attachments.pbr->imageView(),
                                             attachments.emissive->imageView(),
                                             attachments.depth->imageView()};

  vk::FramebufferCreateInfo info{{},
                                 *renderPass,
                                 uint32_t(_attachments.size()),
                                 _attachments.data(),
                                 extent.width,
                                 extent.height,
                                 1};
  framebuffers.resize(swapchain->getImageCount());
  for(auto i = 0u; i < swapchain->getImageCount(); i++) {
    _attachments[0] = *swapchain->getImageViews()[i];
    framebuffers[i] = vkDevice.createFramebufferUnique(info);
  }

  mm->resize(extent);
}

void BasicRenderer::resize() {
  VulkanBase::resize();
  recreateResources();
}

void BasicRenderer::updateFrame(
  std::function<void(uint32_t, float)> &updater, uint32_t imageIndex,
  float elapsedDuration) {
  auto &transfeCB = transferCmdBuffers[imageIndex];
  auto &compCB = computeCmdBuffers[imageIndex];
  auto &cb = graphicsCmdBuffers[imageIndex];
  updater(imageIndex, elapsedDuration);

  mm->updateScene(transfeCB, imageIndex);

  std::array<vk::ClearValue, 8> clearValues{
    vk::ClearColorValue{std::array{0.0f, 0.0f, 0.0f, 0.0f}},
    vk::ClearColorValue{std::array{0.0f, 0.0f, 0.0f, 0.0f}},
    vk::ClearColorValue{std::array{0.0f, 0.0f, 0.0f, 0.0f}},
    vk::ClearColorValue{std::array{0.0f, 0.0f, 0.0f, 0.0f}},
    vk::ClearColorValue{std::array{0.0f, 0.0f, 0.0f, 0.0f}},
    vk::ClearColorValue{std::array{0.0f, 0.0f, 0.0f, 0.0f}},
    vk::ClearColorValue{std::array{0.0f, 0.0f, 0.0f, 0.0f}},
    vk::ClearDepthStencilValue{1.0f, 0},
  };
  vk::RenderPassBeginInfo renderPassBeginInfo{
    *renderPass, *framebuffers[imageIndex],
    vk::Rect2D{{0, 0}, {extent.width, extent.height}}, uint32_t(clearValues.size()),
    clearValues.data()};
  cb.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
  vk::Viewport viewport{0, 0, float(extent.width), float(extent.height), 0.0f, 1.0f};
  cb.setViewport(0, viewport);
  vk::Rect2D scissor{{0, 0}, {extent.width, extent.height}};
  cb.setScissor(0, scissor);

  debugMarker.begin(cb, "subpass direct shading");

  mm->drawScene(cb, imageIndex);

  debugMarker.end(cb);
  cb.endRenderPass();

  ImageBase::setLayout(
    cb, swapchain->getImage(imageIndex), layout::eUndefined, layout::ePresentSrcKHR, {},
    {});
}
}

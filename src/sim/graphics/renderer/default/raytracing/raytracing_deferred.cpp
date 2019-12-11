#include "raytracing.h"

#include <memory>
#include "raytracing_pipeline.h"
#include "sim/graphics/base/pipeline/render_pass.h"

#include "sim/graphics/compiledShaders/raytracing/raster/raster_gbuffer_vert.h"
#include "sim/graphics/compiledShaders/raytracing/raster/raster_gbuffer_frag.h"
#include "sim/graphics/compiledShaders/raytracing/raster/raster_deferred_vert.h"
#include "sim/graphics/compiledShaders/raytracing/raster/raster_deferred_frag.h"

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

void RayTracing::createDeferredRenderPass() {
  RenderPassMaker maker;
  auto color = maker.attachment(offscreenImage->format())
                 .samples(vk::SampleCountFlagBits::e1)
                 .loadOp(vk::AttachmentLoadOp::eDontCare)
                 .storeOp(vk::AttachmentStoreOp::eStore)
                 .stencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                 .stencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                 .initialLayout(layout::eUndefined)
                 .finalLayout(layout::eGeneral)
                 .index();
  auto position = maker.attachment(vk::Format::eR32G32B32A32Sfloat)
                    .loadOp(vk::AttachmentLoadOp::eClear)
                    .storeOp(vk::AttachmentStoreOp::eDontCare)
                    .finalLayout(layout::eColorAttachmentOptimal)
                    .index();
  auto normal = maker.attachment(vk::Format::eR32G32B32A32Sfloat)
                  .initialLayout(layout::eUndefined)
                  .index();
  auto albedo = maker.attachment(vk::Format::eR8G8B8A8Unorm).index();
  auto pbr = maker.attachment(vk::Format::eR32G32Sfloat).index();
  auto depth = maker.attachment(vk::Format::eD32Sfloat)
                 .finalLayout(layout::eDepthStencilAttachmentOptimal)
                 .index();

  // First subpass: Fill G-Buffer components
  debug.gBuffer.subpass = maker.subpass(bindpoint::eGraphics)
                            .color(position)
                            .color(normal)
                            .color(albedo)
                            .color(pbr)
                            .depthStencil(depth)
                            .index();
  // Second subpass: Final composition (using G-Buffer components)
  debug.deferred.subpass = maker.subpass(bindpoint::eGraphics)
                             .color(color)
                             .input(position)
                             .input(normal)
                             .input(albedo)
                             .input(pbr)
                             .depthStencil(depth, layout::eDepthStencilReadOnlyOptimal)
                             .index();

  // A dependency to reset the layout of both attachments.
  maker.dependency(VK_SUBPASS_EXTERNAL, debug.gBuffer.subpass)
    .srcStageMask(stage::eBottomOfPipe)
    .dstStageMask(stage::eColorAttachmentOutput | stage::eFragmentShader)
    .srcAccessMask(access::eMemoryRead)
    .dstAccessMask(access::eColorAttachmentWrite | access::eColorAttachmentRead)
    .dependencyFlags(vk::DependencyFlagBits::eByRegion);

  maker.dependency(debug.gBuffer.subpass, debug.deferred.subpass)
    .srcStageMask(stage::eColorAttachmentOutput)
    .dstStageMask(stage::eFragmentShader)
    .srcAccessMask(access::eColorAttachmentWrite | access::eColorAttachmentRead)
    .dstAccessMask(access::eInputAttachmentRead)
    .dependencyFlags(vk::DependencyFlagBits::eByRegion);

  maker.dependency(debug.deferred.subpass, VK_SUBPASS_EXTERNAL)
    .srcStageMask(stage::eColorAttachmentOutput)
    .dstStageMask(stage::eBottomOfPipe)
    .srcAccessMask(access::eColorAttachmentWrite | access::eColorAttachmentRead)
    .dstAccessMask(access::eMemoryRead)
    .dependencyFlags(vk::DependencyFlagBits::eByRegion);

  debug.pass = maker.createUnique(vkDevice);
}

void RayTracing::recreateDeferredFrameBuffer() {
  std::vector<vk::ImageView> _attachments = {
    offscreenImage->imageView(), debug.position->imageView(), debug.normal->imageView(),
    debug.albedo->imageView(),   debug.pbr->imageView(),      debug.depth->imageView()};
  vk::FramebufferCreateInfo info{{},
                                 *debug.pass,
                                 uint32_t(_attachments.size()),
                                 _attachments.data(),
                                 extent.width,
                                 extent.height,
                                 1};

  debug.frameBuffers.resize(swapchain->getImageCount());
  for(auto &framebuffer: debug.frameBuffers)
    framebuffer = vkDevice.createFramebufferUnique(info);
}

void RayTracing::createDeferredSets() {
  debug.inputsDef.init(vkDevice);
  debug.inputsSet = debug.inputsDef.createSet(*descriptorPool);
  debugMarker.name(debug.inputsSet, "deferred descriptor set");
}

void RayTracing::createDeferredPipeline() {
  createDeferredRenderPass();

  createDeferredSets();

  debug.gBuffer.pipelineLayout =
    PipelineLayoutMaker()
      .descriptorSetLayout(*commonSetDef.descriptorSetLayout)
      .descriptorSetLayout(*debug.inputsDef.descriptorSetLayout)
      .createUnique(vkDevice);

  debug.deferred.pipelineLayout =
    PipelineLayoutMaker()
      .descriptorSetLayout(*commonSetDef.descriptorSetLayout)
      .descriptorSetLayout(*debug.inputsDef.descriptorSetLayout)
      .createUnique(vkDevice);

  debug.pipeCache = vkDevice.createPipelineCacheUnique({});
  { //gbuffer pipeline
    GraphicsPipelineMaker pipelineMaker{vkDevice, extent.width, extent.height};
    pipelineMaker.subpass(debug.gBuffer.subpass)
      .vertexBinding(0, Vertex::stride(), Vertex::attributes(0, 0))
      .cullMode(vk::CullModeFlagBits::eBack)
      .frontFace(vk::FrontFace::eCounterClockwise)
      .depthTestEnable(VK_TRUE)
      .depthCompareOp(vk::CompareOp::eLessOrEqual)
      .dynamicState(vk::DynamicState::eViewport)
      .dynamicState(vk::DynamicState::eScissor);

    pipelineMaker.blendColorAttachment(false);
    pipelineMaker.blendColorAttachment(false);
    pipelineMaker.blendColorAttachment(false);
    pipelineMaker.blendColorAttachment(false);

    pipelineMaker
      .shader(shader::eVertex, raster_gbuffer_vert, __ArraySize__(raster_gbuffer_vert))
      .shader(shader::eFragment, raster_gbuffer_frag, __ArraySize__(raster_gbuffer_frag));

    debug.gBuffer.pipeline = pipelineMaker.createUnique(
      vkDevice, *debug.pipeCache, *debug.gBuffer.pipelineLayout, *debug.pass);
    debugMarker.name(*debug.gBuffer.pipeline, "gbuffer triangle pipeline");

    pipelineMaker.topology(vk::PrimitiveTopology::eLineList)
      .cullMode(vk::CullModeFlagBits::eNone)
      .lineWidth(mm->config.lineWidth);
    debug.gBufferLine.pipeline = pipelineMaker.createUnique(
      vkDevice, *debug.pipeCache, *debug.gBuffer.pipelineLayout, *debug.pass);
    debugMarker.name(*debug.gBuffer.pipeline, "gbuffer line pipeline");

    if(config.displayWireFrame) {
      pipelineMaker.topology(vk::PrimitiveTopology::eTriangleList)
        .cullMode(vk::CullModeFlagBits::eBack)
        .polygonMode(vk::PolygonMode::eLine)
        .lineWidth(mm->config.lineWidth);
      debug.wireFrame.pipeline = pipelineMaker.createUnique(
        vkDevice, *debug.pipeCache, *debug.gBuffer.pipelineLayout, *debug.pass);
      debugMarker.name(*debug.wireFrame.pipeline, "wireframe pipeline");
    }
  }
  {
    // deferred pipeline
    GraphicsPipelineMaker pipelineMaker{vkDevice, extent.width, extent.height};
    pipelineMaker.subpass(debug.deferred.subpass)
      .cullMode(vk::CullModeFlagBits::eNone)
      .frontFace(vk::FrontFace::eClockwise)
      .depthTestEnable(VK_FALSE)
      .dynamicState(vk::DynamicState::eViewport)
      .dynamicState(vk::DynamicState::eScissor)
      .blendColorAttachment(VK_FALSE);

    pipelineMaker
      .shader(shader::eVertex, raster_deferred_vert, __ArraySize__(raster_deferred_vert))
      .shader(
        shader::eFragment, raster_deferred_frag, __ArraySize__(raster_deferred_frag));

    debug.deferred.pipeline = pipelineMaker.createUnique(
      vkDevice, *debug.pipeCache, *debug.deferred.pipelineLayout, *debug.pass);
    debugMarker.name(*debug.deferred.pipeline, "deferred pipeline");
  }
}

void RayTracing::resizeDeferred() {
  debug.position = u<ColorInputAttachmentImage>(
    *device, extent.width, extent.height, vk::Format::eR32G32B32A32Sfloat);
  debug.normal = u<ColorInputAttachmentImage>(
    *device, extent.width, extent.height, vk::Format::eR32G32B32A32Sfloat);
  debug.albedo = u<ColorInputAttachmentImage>(
    *device, extent.width, extent.height, vk::Format::eR8G8B8A8Unorm);
  debug.pbr = u<ColorInputAttachmentImage>(
    *device, extent.width, extent.height, vk::Format::eR32G32Sfloat);
  debug.depth =
    u<DepthStencilImage>(*device, extent.width, extent.height, vk::Format::eD32Sfloat);

  debugMarker.name(debug.position->image(), "position attchment");
  debugMarker.name(debug.normal->image(), "normal attchment");
  debugMarker.name(debug.albedo->image(), "albedo attchment");
  debugMarker.name(debug.pbr->image(), "pbr attchment");
  debugMarker.name(debug.depth->image(), "depth attchment");

  debug.inputsDef.position(debug.position->imageView());
  debug.inputsDef.normal(debug.normal->imageView());
  debug.inputsDef.albedo(debug.albedo->imageView());
  debug.inputsDef.pbr(debug.pbr->imageView());
  debug.inputsDef.depth(*rayTracing.depth);
  debug.inputsDef.update(debug.inputsSet);

  recreateDeferredFrameBuffer();
}

void RayTracing::drawDeferred(uint32_t imageIndex) {
  if(mm->sizeDebugTri + mm->sizeDebugLine == 0) return;
  auto &cb = graphicsCmdBuffers[imageIndex];

  std::array<vk::ClearValue, 6> clearValues{
    vk::ClearColorValue{std::array{0.0f, 0.0f, 0.0f, 1.0f}},
    vk::ClearColorValue{std::array{0.0f, 0.0f, 0.0f, 1.0f}},
    vk::ClearColorValue{std::array{0.0f, 0.0f, 0.0f, 1.0f}},
    vk::ClearColorValue{std::array{0.0f, 0.0f, 0.0f, 1.0f}},
    vk::ClearColorValue{std::array{0.0f, 0.0f, 0.0f, 1.0f}},
    vk::ClearDepthStencilValue{1.0f, 0}};
  vk::RenderPassBeginInfo renderPassBeginInfo{
    *debug.pass, *debug.frameBuffers[imageIndex],
    vk::Rect2D{{0, 0}, {extent.width, extent.height}}, uint32_t(clearValues.size()),
    clearValues.data()};
  cb.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
  vk::Viewport viewport{0, 0, float(extent.width), float(extent.height), 0.0f, 1.0f};
  cb.setViewport(0, viewport);
  vk::Rect2D scissor{{0, 0}, {extent.width, extent.height}};
  cb.setScissor(0, scissor);
  const vk::DeviceSize zeroOffset{0};
  {
    debugMarker.begin(cb, "Subpass 0: G-Buffer creation");
    cb.bindVertexBuffers(0, mm->Buffer.vertices->buffer(), zeroOffset);
    cb.bindIndexBuffer(mm->Buffer.indices->buffer(), zeroOffset, vk::IndexType::eUint32);
    cb.bindDescriptorSets(
      bindpoint::eGraphics, *debug.gBuffer.pipelineLayout, 0,
      std::array{commonSet, debug.inputsSet}, nullptr);
    if(mm->sizeDebugTri > 0) {
      cb.bindPipeline(bindpoint::eGraphics, *debug.gBuffer.pipeline);
      cb.drawIndexedIndirect(
        mm->drawCMDs_buffer->buffer(), zeroOffset, mm->sizeDebugTri,
        sizeof(vk::DrawIndexedIndirectCommand));
    }
    if(mm->sizeDebugLine > 0) {
      cb.bindPipeline(bindpoint::eGraphics, *debug.gBufferLine.pipeline);
      auto offset = mm->sizeDebugTri * sizeof(vk::DrawIndexedIndirectCommand);
      cb.drawIndexedIndirect(
        mm->drawCMDs_buffer->buffer(), offset, mm->sizeDebugLine,
        sizeof(vk::DrawIndexedIndirectCommand));
    }
    debugMarker.end(cb);
  }
  {
    debugMarker.begin(cb, "Subpass 1: Deferred composition");
    cb.nextSubpass(vk::SubpassContents::eInline);
    cb.bindPipeline(bindpoint::eGraphics, *debug.deferred.pipeline);
    cb.bindDescriptorSets(
      bindpoint::eGraphics, *debug.deferred.pipelineLayout, 0,
      std::array{commonSet, debug.inputsSet}, nullptr);
    cb.draw(3, 1, 0, 0);
    debugMarker.end(cb);
  }
  cb.endRenderPass();
}
}
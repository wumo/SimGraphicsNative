#include "basic_renderer.h"
#include "sim/graphics/base/pipeline/render_pass.h"
#include "sim/graphics/base/pipeline/pipeline.h"

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
  createRenderPass();
  recreateResources();

  createQueryPool();
}

BasicModelManager &BasicRenderer::modelManager() const { return *mm; }

void BasicRenderer::createModelManager() { mm = u<BasicModelManager>(*this); }

void BasicRenderer::createQueryPool() {
  using flag = vk::QueryPipelineStatisticFlagBits;

  vk::QueryPoolCreateInfo info{};
  info.pipelineStatistics =
    flag::eInputAssemblyPrimitives | flag::eInputAssemblyVertices |
    flag::eVertexShaderInvocations | flag::eFragmentShaderInvocations |
    flag::eClippingInvocations | flag::eClippingPrimitives |
    flag::eTessellationControlShaderPatches |
    flag::eTessellationEvaluationShaderInvocations | flag::eComputeShaderInvocations;
  info.queryType = vk::QueryType ::ePipelineStatistics;
  info.queryCount = 9;
  queryPool = vkDevice.createQueryPoolUnique(info);

  pipelineStatNames = {
    "Input assembly vertex count",     "Input assembly primitives count",
    "Vertex shader invocations",       "Clipping stage primitives processed",
    "Clipping stage primtives output", "Fragment shader invocations",
    "Tess. control shader patches",    "Tess. eval. shader invocations",
    "Compute shader invocations"};
  pipelineStats.resize(pipelineStatNames.size());
}

void BasicRenderer::createRenderPass() {
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
  auto color = maker.attachment(swapchain->getImageFormat())
                 .samples(sampleCount)
                 .storeOp(vk::AttachmentStoreOp::eDontCare)
                 .index();
  auto position = maker.attachment(vk::Format::eR32G32B32A32Sfloat).index();
  auto normal = maker.attachment(vk::Format::eR32G32B32A32Sfloat).index();
  auto albedo = maker.attachment(vk::Format::eR8G8B8A8Unorm).index();
  auto pbr = maker.attachment(vk::Format::eR8G8B8A8Unorm).index();
  auto emissive = maker.attachment(vk::Format::eR8G8B8A8Unorm).index();
  auto depth = maker.attachment(vk::Format::eD24UnormS8Uint)
                 .storeOp(vk::AttachmentStoreOp::eDontCare)
                 .finalLayout(layout::eDepthStencilAttachmentOptimal)
                 .index();
  Subpasses.gBuffer = maker.subpass(bindpoint::eGraphics)
                        .color(position)
                        .color(normal)
                        .color(albedo)
                        .color(pbr)
                        .color(emissive)
                        .depthStencil(depth)
                        .index();
  Subpasses.deferred = maker.subpass(bindpoint::eGraphics)
                         .color(color)
                         .input(position)
                         .input(normal)
                         .input(albedo)
                         .input(pbr)
                         .input(emissive)
                         .preserve(depth)
                         .index();
  auto &spMaker = maker.subpass(bindpoint::eGraphics).color(color).depthStencil(depth);
  if(config.sampleCount > 1)
    spMaker.resolve(presentImage); //resolve using tiled on chip memory.
  Subpasses.translucent = spMaker.index();

  maker.dependency(VK_SUBPASS_EXTERNAL, Subpasses.gBuffer)
    .srcStageMask(stage::eBottomOfPipe)
    .dstStageMask(stage::eColorAttachmentOutput)
    .srcAccessMask(access::eMemoryRead)
    .dstAccessMask(access::eColorAttachmentRead | access::eColorAttachmentWrite)
    .dependencyFlags(vk::DependencyFlagBits::eByRegion);
  maker.dependency(Subpasses.gBuffer, Subpasses.deferred)
    .srcStageMask(stage::eColorAttachmentOutput)
    .dstStageMask(stage::eFragmentShader)
    .srcAccessMask(access::eColorAttachmentWrite)
    .dstAccessMask(access::eInputAttachmentRead)
    .dependencyFlags(vk::DependencyFlagBits::eByRegion);
  maker.dependency(Subpasses.deferred, Subpasses.translucent)
    .srcStageMask(stage::eColorAttachmentOutput)
    .dstStageMask(stage::eEarlyFragmentTests | stage::eLateFragmentTests)
    .srcAccessMask(access::eColorAttachmentWrite)
    .dstAccessMask(access::eDepthStencilAttachmentRead)
    .dependencyFlags(vk::DependencyFlagBits::eByRegion);
  maker.dependency(Subpasses.translucent, VK_SUBPASS_EXTERNAL)
    .srcStageMask(stage::eColorAttachmentOutput)
    .dstStageMask(stage::eBottomOfPipe)
    .srcAccessMask(access::eColorAttachmentRead | access::eColorAttachmentWrite)
    .dstAccessMask(access::eMemoryRead)
    .dependencyFlags(vk::DependencyFlagBits::eByRegion);
  renderPass = maker.createUnique(vkDevice);

  createPipelines();
}

void BasicRenderer::createPipelines() {
  auto pipelineLayout = *mm->basicLayout.pipelineLayout;
  pipelineCache = vkDevice.createPipelineCacheUnique({});

  createOpaquePipeline(pipelineLayout);
  createDeferredPipeline(pipelineLayout);
  createTranslucentPipeline(pipelineLayout);
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

  cb.resetQueryPool(*queryPool, 0, pipelineStats.size());

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

  cb.beginQuery(*queryPool, 0, {});

  debugMarker.begin(cb, "subpass direct shading");

  mm->drawScene(cb, imageIndex);

  debugMarker.end(cb);
  cb.endRenderPass();

  cb.endQuery(*queryPool, 0);

  ImageBase::setLayout(
    cb, swapchain->getImage(imageIndex), layout::eUndefined, layout::ePresentSrcKHR, {},
    {});

  vkDevice.getQueryPoolResults(
    *queryPool, 0, 1, pipelineStats.size() * sizeof(uint64_t), pipelineStats.data(),
    sizeof(uint64_t), vk::QueryResultFlagBits::e64);

  //  for(int i = 0; i < pipelineStats.size(); ++i) {
  //    println(pipelineStatNames[i], pipelineStats[i]);
  //  }
}
}

#include "gbuffer_pass.h"
#include "sim/graphics/base/pipeline/pipeline.h"
#include "sim/graphics/compiledShaders/basic_vert.h"
#include "sim/graphics/compiledShaders/deferred/gbuffer_frag.h"

namespace sim::graphics::renderer::basic {
using layout = vk::ImageLayout;
using loadOp = vk::AttachmentLoadOp;
using storeOp = vk::AttachmentStoreOp;
using stage = vk::PipelineStageFlagBits;
using access = vk::AccessFlagBits;

GBufferPass::GBufferPass(FrameGraphBuilder &builder): RenderPass(builder) {
  auto &swapchain = builder.swapchain();
  TextureDesc desc{swapchain.getImageExtent(), vk::Format::eR32G32B32A32Sfloat,
                   builder.sampleCount(), loadOp::eClear};

  position = builder.createTexture(desc);
  normal = builder.createTexture(desc);

  desc.format = vk::Format::eR8G8B8A8Unorm;
  albedo = builder.createTexture(desc);
  pbr = builder.createTexture(desc);
  emissive = builder.createTexture(desc);

  desc.format = vk::Format::eD24UnormS8Uint;
  depth = builder.createTexture(desc);

  builder.writeColor(position);
  builder.writeColor(normal);
  builder.writeColor(albedo);
  builder.writeColor(pbr);
  builder.writeColor(emissive);
  builder.writeDepth(depth);

  builder.dependency(
    SubpassExternal, stage::eBottomOfPipe, stage::eColorAttachmentOutput,
    access::eMemoryRead, access::eColorAttachmentWrite);

  set_ = builder.createDescriptorSet(setDef);
}

void GBufferPass::compile() {
  GraphicsPipelineMaker pipelineMaker{vkDevice, extent.width, extent.height};
  pipelineMaker.subpass(Subpasses.gBuffer)
    .vertexBinding(0, sizeof(Vertex::Position))
    .vertexAttribute(0, 0, f::eR32G32B32Sfloat, 0)
    .vertexBinding(1, sizeof(Vertex::Normal))
    .vertexAttribute(1, 1, f::eR32G32B32Sfloat, 0)
    .vertexBinding(2, sizeof(Vertex::UV))
    .vertexAttribute(2, 2, f::eR32G32Sfloat, 0)
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
  Pipelines.opaqueTri =
    pipelineMaker.createUnique(*pipelineCache, pipelineLayout, *renderPass);
  debugMarker.name(*Pipelines.opaqueTri, "opaque triangle pipeline");
}
void GBufferPass::execute() {}
}
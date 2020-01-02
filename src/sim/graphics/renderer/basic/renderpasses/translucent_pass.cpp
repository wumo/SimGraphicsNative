#include "../basic_renderer.h"
#include "sim/graphics/compiledShaders/basic_vert.h"
#include "sim/graphics/compiledShaders/translucent_frag.h"

namespace sim::graphics::renderer::basic {
using shader = vk::ShaderStageFlagBits;
using f = vk::Format;

void BasicRenderer::createTranslucentPipeline(
  const vk::PipelineLayout &pipelineLayout) { // translucent pipeline
  GraphicsPipelineMaker pipelineMaker{vkDevice, extent.width, extent.height};
  pipelineMaker.subpass(Subpasses.translucent)
    .vertexBinding(0, sizeof(Vertex::Position))
    .vertexAttribute(0, 0, f::eR32G32B32Sfloat, 0)
    .vertexBinding(1, sizeof(Vertex::Normal))
    .vertexAttribute(1, 1, f::eR32G32B32Sfloat, 0)
    .vertexBinding(2, sizeof(Vertex::UV))
    .vertexAttribute(2, 2, f::eR32G32Sfloat, 0)
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

  Pipelines.transTri =
    pipelineMaker.createUnique(*pipelineCache, pipelineLayout, *renderPass);
  debugMarker.name(*Pipelines.transTri, "translucent tri pipeline");

  pipelineMaker.topology(vk::PrimitiveTopology::eLineList)
    .cullMode(vk::CullModeFlagBits::eNone)
    .lineWidth(1.f);
  Pipelines.transLine =
    pipelineMaker.createUnique(*pipelineCache, pipelineLayout, *renderPass);
  debugMarker.name(*Pipelines.transLine, "translucent line pipeline");
}
}
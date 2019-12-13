#include "../basic_renderer.h"
#include "sim/graphics/compiledShaders/basic/basic_vert.h"
#include "sim/graphics/compiledShaders/basic/gbuffer_frag.h"

namespace sim::graphics::renderer::basic {
using shader = vk::ShaderStageFlagBits;

void BasicRenderer::createOpaquePipeline(
  const vk::PipelineLayout &pipelineLayout) { // Pipeline
  GraphicsPipelineMaker pipelineMaker{vkDevice, extent.width, extent.height};
  pipelineMaker.subpass(Subpasses.gBuffer)
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
  Pipelines.opaqueTri =
    pipelineMaker.createUnique(vkDevice, *pipelineCache, pipelineLayout, *renderPass);
  debugMarker.name(*Pipelines.opaqueTri, "opaque triangle pipeline");

  pipelineMaker.polygonMode(vk::PolygonMode::eLine);

  Pipelines.opaqueTriWireframe =
    pipelineMaker.createUnique(vkDevice, *pipelineCache, pipelineLayout, *renderPass);
  debugMarker.name(*Pipelines.opaqueTriWireframe, "opaque wireframe pipeline");

  pipelineMaker.topology(vk::PrimitiveTopology::eLineList)
    .polygonMode(vk::PolygonMode::eFill)
    .cullMode(vk::CullModeFlagBits::eNone)
    .lineWidth(1.f);
  Pipelines.opaqueLine =
    pipelineMaker.createUnique(vkDevice, *pipelineCache, pipelineLayout, *renderPass);
  debugMarker.name(*Pipelines.opaqueLine, "opaque line pipeline");
}

}
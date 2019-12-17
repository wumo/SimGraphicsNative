#include "../basic_renderer.h"
#include "sim/graphics/compiledShaders/basic/terrain/terrain_vert.h"
#include "sim/graphics/compiledShaders/basic/gbuffer_frag.h"

namespace sim::graphics::renderer::basic {
using shader = vk::ShaderStageFlagBits;
using f = vk::Format;

void BasicRenderer::createTerrainPipeline(
  const vk::PipelineLayout &pipelineLayout) { // Pipeline
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

  SpecializationMaker sp;
  auto spInfo = sp.entry(modelConfig.maxNumTexture).create();
  pipelineMaker
    .shader(shader::eVertex, terrain_vert, __ArraySize__(terrain_vert), &spInfo)
    .shader(shader::eFragment, gbuffer_frag, __ArraySize__(gbuffer_frag), &spInfo);
  Pipelines.terrain =
    pipelineMaker.createUnique(vkDevice, *pipelineCache, pipelineLayout, *renderPass);
  debugMarker.name(*Pipelines.terrain, "terrain pipeline");

  pipelineMaker.polygonMode(vk::PolygonMode::eLine);

  Pipelines.terrainWireframe =
    pipelineMaker.createUnique(vkDevice, *pipelineCache, pipelineLayout, *renderPass);
  debugMarker.name(*Pipelines.terrainWireframe, "opaque wireframe pipeline");
}

}
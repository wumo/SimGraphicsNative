#include "../basic_renderer.h"
#include "sim/graphics/compiledShaders/deferred/gbuffer_frag.h"
#include "sim/graphics/compiledShaders/terrain/terrain_tess_vert.h"
#include "sim/graphics/compiledShaders/terrain/terrain_tesc.h"
#include "sim/graphics/compiledShaders/terrain/terrain_tese.h"

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
    .topology(vk::PrimitiveTopology::ePatchList)
    .tesselationState(4)
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
    .shader(shader::eVertex, terrain_tess_vert, __ArraySize__(terrain_tess_vert))
    .shader(
      shader::eTessellationControl, terrain_tesc, __ArraySize__(terrain_tesc), &spInfo)
    .shader(
      shader::eTessellationEvaluation, terrain_tese, __ArraySize__(terrain_tese), &spInfo)
    .shader(shader::eFragment, gbuffer_frag, __ArraySize__(gbuffer_frag), &spInfo);

  Pipelines.terrainTess =
    pipelineMaker.createUnique(*pipelineCache, pipelineLayout, *renderPass);
  debugMarker.name(*Pipelines.terrainTess, "terrain tessellation pipeline");

  pipelineMaker.polygonMode(vk::PolygonMode::eLine);

  Pipelines.terrainTessWireframe =
    pipelineMaker.createUnique(*pipelineCache, pipelineLayout, *renderPass);
  debugMarker.name(
    *Pipelines.terrainTessWireframe, "terrain tessellation wireframe pipeline");
}

}
#include "../basic_renderer.h"
#include "sim/graphics/compiledShaders/quad_vert.h"
#include "sim/graphics/compiledShaders/deferred/deferred_frag.h"
#include "sim/graphics/compiledShaders/deferred/deferred_ms_frag.h"
#include "sim/graphics/compiledShaders/deferred/deferred_ibl_frag.h"
#include "sim/graphics/compiledShaders/deferred/deferred_ibl_ms_frag.h"
#include "sim/graphics/compiledShaders/deferred/deferred_sky_frag.h"
#include "sim/graphics/compiledShaders/deferred/deferred_sky_ms_frag.h"

namespace sim::graphics::renderer::basic {
using shader = vk::ShaderStageFlagBits;

void BasicRenderer::createDeferredPipeline(
  const vk::PipelineLayout &pipelineLayout) { // shading pipeline
  GraphicsPipelineMaker pipelineMaker{vkDevice, extent.width, extent.height};
  pipelineMaker.subpass(Subpasses.deferred)
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
    pipelineMaker.shader(shader::eFragment, deferred_frag, __ArraySize__(deferred_frag));
  }
  Pipelines.deferred =
    pipelineMaker.createUnique(*pipelineCache, pipelineLayout, *renderPass);
  debugMarker.name(*Pipelines.deferred, "deferred pipeline");

  pipelineMaker.clearShaders();

  pipelineMaker.shader(shader::eVertex, quad_vert, __ArraySize__(quad_vert));
  if(config.sampleCount > 1)
    pipelineMaker.shader(
      shader::eFragment, deferred_ibl_ms_frag, __ArraySize__(deferred_ibl_ms_frag));
  else
    pipelineMaker.shader(
      shader::eFragment, deferred_ibl_frag, __ArraySize__(deferred_ibl_frag));
  Pipelines.deferredIBL =
    pipelineMaker.createUnique(*pipelineCache, pipelineLayout, *renderPass);
  debugMarker.name(*Pipelines.deferredIBL, "deferred IBL pipeline");

  pipelineMaker.clearShaders();

  pipelineMaker.shader(shader::eVertex, quad_vert, __ArraySize__(quad_vert));
  if(config.sampleCount > 1)
    pipelineMaker.shader(
      shader::eFragment, deferred_sky_ms_frag, __ArraySize__(deferred_sky_ms_frag));
  else
    pipelineMaker.shader(
      shader::eFragment, deferred_sky_frag, __ArraySize__(deferred_sky_frag));
  Pipelines.deferredSky =
    pipelineMaker.createUnique(*pipelineCache, pipelineLayout, *renderPass);
  debugMarker.name(*Pipelines.deferredSky, "deferred Sky pipeline");
}
}
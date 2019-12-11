#include "raytracing.h"

#include <memory>

#include "sim/graphics/compiledShaders/raytracing/primary_ray_chit/primary_brdf_direct_rchit.h"
#include "sim/graphics/compiledShaders/raytracing/primary_ray_chit/primary_brdf_trace_rchit.h"
#include "sim/graphics/compiledShaders/raytracing/primary_ray_chit/primary_bsdf_trace_rchit.h"
#include "sim/graphics/compiledShaders/raytracing/ray_rgen.h"
#include "sim/graphics/compiledShaders/raytracing/ray_rmiss.h"
#include "sim/graphics/compiledShaders/raytracing/shadow_ray_rmiss.h"
#include "sim/graphics/compiledShaders/raytracing/procedural_ray_chit/procedural_brdf_direct_rchit.h"
#include "sim/graphics/compiledShaders/raytracing/procedural_ray_chit/procedural_brdf_trace_rchit.h"
#include "sim/graphics/compiledShaders/raytracing/procedural_ray_chit/procedural_bsdf_trace_rchit.h"

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

void RayTracing::updateModels() {
  mm->updateDescriptorSet(commonSetDef.texture);
  commonSetDef.update(commonSet);

  mm->createAS();
  rayTracing.def.as(*mm->tlas.as);
  rayTracing.def.update(rayTracing.set);
  mm->updateInstances();
  mm->updateTransformsAndMaterials();

  SpecializationMaker sp;
  auto spInfo = sp.entry(modelLimit.numLights).create();
  SpecializationMaker spRayGen;
  auto spRayGenInfo = spRayGen.entry(config.maxRecursion).create();
  ShaderModule rayBRDFDirectChit{vkDevice, primary_brdf_direct_rchit,
                                 __ArraySize__(primary_brdf_direct_rchit), &spInfo};
  ShaderModule rayBRDFTraceChit{vkDevice, primary_brdf_trace_rchit,
                                __ArraySize__(primary_brdf_trace_rchit), &spInfo};
  ShaderModule rayBSDFTraceChit{vkDevice, primary_bsdf_trace_rchit,
                                __ArraySize__(primary_bsdf_trace_rchit), &spInfo};

  RayTracingPipelineMaker maker{{*device}};
  maker.maxRecursionDepth(config.maxRecursion)
    .rayGenGroup(ray_rgen, __ArraySize__(ray_rgen), &spRayGenInfo)
    .missGroup(ray_rmiss, __ArraySize__(ray_rmiss))
    .missGroup(shadow_ray_rmiss, __ArraySize__(shadow_ray_rmiss))
    .hitGroup({&rayBRDFDirectChit})
    .hitGroup()
    .hitGroup({&rayBRDFTraceChit})
    .hitGroup()
    .hitGroup({&rayBSDFTraceChit})
    .hitGroup();

  if(!mm->procShaders.empty()) {
    ShaderModule procBRDFDirectChit{vkDevice, procedural_brdf_direct_rchit,
                                    __ArraySize__(procedural_brdf_direct_rchit), &spInfo};
    ShaderModule procBRDFTraceChit{vkDevice, procedural_brdf_trace_rchit,
                                   __ArraySize__(procedural_brdf_trace_rchit), &spInfo};
    ShaderModule procBSDFTraceChit{vkDevice, procedural_bsdf_trace_rchit,
                                   __ArraySize__(procedural_bsdf_trace_rchit), &spInfo};
    for(auto &shader: mm->procShaders)
      maker.hitGroup({&procBRDFDirectChit, &shader})
        .hitGroup()
        .hitGroup({&procBRDFTraceChit, &shader})
        .hitGroup()
        .hitGroup({&procBSDFTraceChit, &shader})
        .hitGroup();
    std::tie(rayTracing.pipe.pipeline, rayTracing.pipe.sbt) = maker.createUnique(
      rayTracing.properties, *rayTracing.pipe.pipelineLayout, *rayTracing.pipeCache);
  } else
    std::tie(rayTracing.pipe.pipeline, rayTracing.pipe.sbt) = maker.createUnique(
      rayTracing.properties, *rayTracing.pipe.pipelineLayout, *rayTracing.pipeCache);
}

void RayTracing::render(
  std::function<void(uint32_t, float)> &updater, uint32_t imageIndex,
  float elapsedDuration) {

  auto &cb = graphicsCmdBuffers[imageIndex];
  updater(imageIndex, elapsedDuration);
  mm->updateTransformsAndMaterials();
  mm->uploadPixels(cb);

  debugMarker.begin(cb, "update AS");
  mm->updateAS(cb);
  debugMarker.end(cb);

  offscreenImage->setLayout(
    cb, layout::eUndefined, layout::eGeneral, {}, access::eShaderWrite);

  traceRays(cb);
  if(config.debug) drawDeferred(imageIndex);

  offscreenImage->setLayout(
    cb, layout::eGeneral, layout::eTransferSrcOptimal, access::eShaderWrite,
    access::eTransferRead);

  auto &swapchainImage = swapchain->getImage(imageIndex);
  ImageBase::setLayout(
    cb, swapchainImage, layout::eUndefined, layout::eTransferDstOptimal, {},
    access::eTransferWrite);
  ImageBase::copy(cb, *offscreenImage, swapchainImage);

//  if(config.gui) gui->drawGui(imageIndex, cb);
}

void RayTracing::traceRays(const vk::CommandBuffer &cb) {
  debugMarker.begin(cb, "subpass: ray tracing");
  cb.bindPipeline(bindpoint::eRayTracingNV, *rayTracing.pipe.pipeline);
  cb.bindDescriptorSets(
    bindpoint::eRayTracingNV, *rayTracing.pipe.pipelineLayout, 0,
    std::array{rayTracing.set, commonSet}, nullptr);
  auto &sbt = rayTracing.pipe.sbt.shaderBindingTable->buffer();
  cb.traceRaysNV(
    sbt, rayTracing.pipe.sbt.rayGenOffset, sbt, rayTracing.pipe.sbt.missGroupOffset,
    rayTracing.pipe.sbt.missGroupStride, sbt, rayTracing.pipe.sbt.hitGroupOffset,
    rayTracing.pipe.sbt.hitGroupStride, nullptr, 0, 0, extent.width, extent.height, 2);
  debugMarker.end(cb);
}
}
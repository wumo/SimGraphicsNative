#include "sky_model.h"
#include "sim/graphics/base/pipeline/render_pass.h"
#include "sim/graphics/base/pipeline/pipeline.h"
#include "sim/graphics/compiledShaders/basic/sky/scattering_density_comp.h"

namespace sim::graphics::renderer::basic {
using address = vk::SamplerAddressMode;
using loadOp = vk::AttachmentLoadOp;
using storeOp = vk::AttachmentStoreOp;
using layout = vk::ImageLayout;
using bindpoint = vk::PipelineBindPoint;
using stage = vk::PipelineStageFlagBits;
using access = vk::AccessFlagBits;
using flag = vk::DescriptorBindingFlagBitsEXT;
using shader = vk::ShaderStageFlagBits;
using aspect = vk::ImageAspectFlagBits;
using namespace glm;

void SkyModel::createScatteringDensitySets() {
  scatteringDensitySet = scatteringDensitySetDef.createSet(*descriptorPool);
  scatteringDensitySetDef.atmosphere(_atmosphereUBO->buffer());
  scatteringDensitySetDef.transmittance(*transmittance_texture_);
  scatteringDensitySetDef.delta_rayleigh(*delta_rayleigh_scattering_texture);
  scatteringDensitySetDef.delta_mie(*delta_mie_scattering_texture);
  scatteringDensitySetDef.multpli_scattering(*delta_rayleigh_scattering_texture);
  scatteringDensitySetDef.irradiance(*delta_irradiance_texture);
  scatteringDensitySetDef.scattering_density(*delta_scattering_density_texture);
  scatteringDensitySetDef.update(scatteringDensitySet);

  SpecializationMaker sp{};
  auto spInfo = sp.entry<uint32_t>(8).entry<uint32_t>(8).entry<uint32_t>(1).create();

  ComputePipelineMaker pipelineMaker{device.getDevice()};
  pipelineMaker.shader(
    scattering_density_comp, __ArraySize__(scattering_density_comp), &spInfo);
  scatteringDensityPipeline =
    pipelineMaker.createUnique(nullptr, *scatteringDensityLayoutDef.pipelineLayout);
}

void SkyModel::recordScatteringDensityCMD(vk::CommandBuffer cb, int32_t scatteringOrder) {

  auto dim = delta_scattering_density_texture->extent();
  delta_scattering_density_texture->transitToLayout(
    cb, layout::eGeneral, access::eShaderWrite, stage::eComputeShader);

  cb.bindPipeline(bindpoint::eCompute, *scatteringDensityPipeline);
  cb.bindDescriptorSets(
    bindpoint::eCompute, *scatteringDensityLayoutDef.pipelineLayout,
    scatteringDensityLayoutDef.set.set(), scatteringDensitySet, nullptr);
  cb.pushConstants<int32_t>(
    *scatteringDensityLayoutDef.pipelineLayout, shader::eCompute, 0, scatteringOrder);
  cb.dispatch(dim.width / 8, dim.height / 8, dim.depth / 1);

  cb.pipelineBarrier(
    stage::eComputeShader, stage::eComputeShader, {}, nullptr, nullptr,
    delta_scattering_density_texture->barrier(
      layout::eShaderReadOnlyOptimal, access::eShaderRead, stage::eComputeShader));
}
}

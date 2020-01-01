#include "sky_model.h"
#include "sim/graphics/compiledShaders/basic/sky/single_scattering_comp.h"
#include "sim/graphics/base/pipeline/descriptor_pool_maker.h"

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

void SkyModel::createSingleScatteringSets() {
  singleScatteringSet = singleScatteringSetDef.createSet(*descriptorPool);
  singleScatteringSetDef.atmosphere(_atmosphereUBO->buffer());
  singleScatteringSetDef.transmittance(*transmittance_texture_);
  singleScatteringSetDef.delta_rayleigh(*delta_rayleigh_scattering_texture);
  singleScatteringSetDef.delta_mie(*delta_mie_scattering_texture);
  singleScatteringSetDef.scattering(*scattering_texture_);
  singleScatteringSetDef.update(singleScatteringSet);

  SpecializationMaker sp{};
  auto spInfo = sp.entry<uint32_t>(8).entry<uint32_t>(8).entry<uint32_t>(1).create();

  ComputePipelineMaker pipelineMaker{device.getDevice()};
  pipelineMaker.shader(
    single_scattering_comp, __ArraySize__(single_scattering_comp), &spInfo);
  singleScatteringPipeline =
    pipelineMaker.createUnique(nullptr, *singleScatteringLayoutDef.pipelineLayout);
}

void SkyModel::recordSingleScatteringCMD(
  vk::CommandBuffer cb, const glm::mat4 &luminance_from_radiance, vk::Bool32 cumulate) {

  auto dim = scattering_texture_->extent();

  delta_rayleigh_scattering_texture->transitToLayout(
    cb, layout::eGeneral, access::eShaderWrite, stage::eComputeShader);
  delta_mie_scattering_texture->transitToLayout(
    cb, layout::eGeneral, access::eShaderWrite, stage::eComputeShader);
  scattering_texture_->transitToLayout(
    cb, layout::eGeneral, access::eShaderWrite, stage::eComputeShader);

  cb.bindPipeline(bindpoint::eCompute, *singleScatteringPipeline);
  cb.bindDescriptorSets(
    bindpoint::eCompute, *singleScatteringLayoutDef.pipelineLayout,
    singleScatteringLayoutDef.set.set(), singleScatteringSet, nullptr);
  CumulateLFUConstant constant{luminance_from_radiance, cumulate};
  cb.pushConstants<CumulateLFUConstant>(
    *singleScatteringLayoutDef.pipelineLayout, shader::eCompute, 0, constant);
  cb.dispatch(dim.width / 8, dim.height / 8, dim.depth / 1);

  cb.pipelineBarrier(
    stage::eComputeShader, stage::eComputeShader, {}, nullptr, nullptr,
    {delta_rayleigh_scattering_texture->barrier(
       layout::eShaderReadOnlyOptimal, access::eShaderRead, stage::eComputeShader),
     delta_mie_scattering_texture->barrier(
       layout::eShaderReadOnlyOptimal, access::eShaderRead, stage::eComputeShader),
     scattering_texture_->barrier(
       layout::eShaderReadOnlyOptimal, access::eShaderRead, stage::eComputeShader)});
}
}

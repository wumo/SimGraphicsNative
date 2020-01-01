#include "sky_model.h"
#include "sim/graphics/compiledShaders/sky/direct_irradiance_comp.h"

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

void SkyModel::createDirectIrradianceSets() {
  directIrradianceSet = directIrradianceSetDef.createSet(*descriptorPool);
  directIrradianceSetDef.atmosphere(_atmosphereUBO->buffer());
  directIrradianceSetDef.transmittance(*transmittance_texture_);
  directIrradianceSetDef.delta_irradiance(*delta_irradiance_texture);
  directIrradianceSetDef.irradiance(*irradiance_texture_);
  directIrradianceSetDef.update(directIrradianceSet);

  SpecializationMaker sp{};
  auto spInfo = sp.entry<uint32_t>(8).entry<uint32_t>(8).entry<uint32_t>(1).create();
  ComputePipelineMaker pipelineMaker{device.getDevice()};
  pipelineMaker.shader(
    direct_irradiance_comp, __ArraySize__(direct_irradiance_comp), &spInfo);

  directIrradiancePipeline =
    pipelineMaker.createUnique(nullptr, *directIrradianceLayoutDef.pipelineLayout);
}

void SkyModel::recordDirectIrradianceCMD(vk::CommandBuffer cb, vk::Bool32 cumulate) {

  auto dim = irradiance_texture_->extent();

  delta_irradiance_texture->transitToLayout(
    cb, layout::eGeneral, access::eShaderWrite, stage::eComputeShader);
  irradiance_texture_->transitToLayout(
    cb, layout::eGeneral, access::eShaderWrite, stage::eComputeShader);

  cb.bindPipeline(bindpoint::eCompute, *directIrradiancePipeline);
  cb.bindDescriptorSets(
    bindpoint::eCompute, *directIrradianceLayoutDef.pipelineLayout,
    directIrradianceLayoutDef.set.set(), directIrradianceSet, nullptr);
  cb.pushConstants<vk::Bool32>(
    *directIrradianceLayoutDef.pipelineLayout, shader::eCompute, 0, cumulate);
  cb.dispatch(dim.width / 8, dim.height / 8, 1);

  cb.pipelineBarrier(
    stage::eComputeShader, stage::eComputeShader, {}, nullptr, nullptr,
    {delta_irradiance_texture->barrier(
       layout::eShaderReadOnlyOptimal, access::eShaderRead, stage::eComputeShader),
     irradiance_texture_->barrier(
       layout::eShaderReadOnlyOptimal, access::eShaderRead, stage::eComputeShader)});
}
}

#include "sky_model.h"
#include "sim/graphics/base/pipeline/render_pass.h"
#include "sim/graphics/base/pipeline/pipeline.h"
#include "sim/graphics/compiledShaders/basic/sky/indirect_irradiance_comp.h"

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

void SkyModel::createIndirectIrradianceSets() {
  indirectIrradianceSet = indirectIrradianceSetDef.createSet(*descriptorPool);
  indirectIrradianceSetDef.atmosphere(_atmosphereUBO->buffer());
  indirectIrradianceSetDef.delta_rayleigh(*delta_rayleigh_scattering_texture);
  indirectIrradianceSetDef.delta_mie(*delta_mie_scattering_texture);
  indirectIrradianceSetDef.multpli_scattering(*delta_rayleigh_scattering_texture);
  indirectIrradianceSetDef.delta_irradiance(*delta_irradiance_texture);
  indirectIrradianceSetDef.irradiance(*irradiance_texture_);
  indirectIrradianceSetDef.update(indirectIrradianceSet);

  SpecializationMaker sp{};
  auto spInfo = sp.entry<uint32_t>(8).entry<uint32_t>(8).entry<uint32_t>(1).create();

  ComputePipelineMaker pipelineMaker{device.getDevice()};
  pipelineMaker.shader(
    indirect_irradiance_comp, __ArraySize__(indirect_irradiance_comp), &spInfo);
  indirectIrradiancePipeline =
    pipelineMaker.createUnique(nullptr, *indirectIrradianceLayoutDef.pipelineLayout);
}

void SkyModel::recordIndirectIrradianceCMD(
  vk::CommandBuffer cb, const glm::mat4 &luminance_from_radiance,
  int32_t scatteringOrder) {

  auto dim = irradiance_texture_->extent();

  delta_irradiance_texture->transitToLayout(
    cb, layout::eGeneral, access::eShaderWrite, stage::eComputeShader);

  irradiance_texture_->transitToLayout(
    cb, layout::eGeneral, access::eShaderWrite, stage::eComputeShader);

  cb.bindPipeline(bindpoint::eCompute, *indirectIrradiancePipeline);
  cb.bindDescriptorSets(
    bindpoint::eCompute, *indirectIrradianceLayoutDef.pipelineLayout,
    indirectIrradianceLayoutDef.set.set(), indirectIrradianceSet, nullptr);
  ScatteringOrderLFUConstant constant{luminance_from_radiance, scatteringOrder};
  cb.pushConstants<ScatteringOrderLFUConstant>(
    *indirectIrradianceLayoutDef.pipelineLayout, shader::eCompute, 0, constant);
  cb.pushConstants<int32_t>(
    *indirectIrradianceLayoutDef.pipelineLayout, shader::eCompute, sizeof(glm::mat4),
    scatteringOrder);
  cb.dispatch(dim.width / 8, dim.height / 8, 1);

  cb.pipelineBarrier(
    stage::eComputeShader, stage::eComputeShader, {}, nullptr, nullptr,
    {delta_irradiance_texture->barrier(
       layout::eShaderReadOnlyOptimal, access::eShaderRead, stage::eComputeShader),
     irradiance_texture_->barrier(
       layout::eShaderReadOnlyOptimal, access::eShaderRead, stage::eComputeShader)});
}
}

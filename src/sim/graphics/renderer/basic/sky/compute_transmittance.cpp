#include "sky_model.h"
#include "sim/graphics/compiledShaders/sky/transmittance_comp.h"

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

void SkyModel::createTransmittanceSets() {
  ComputePipelineMaker pipelineMaker{device.getDevice()};

  SpecializationMaker sp{};
  auto spInfo = sp.entry<uint32_t>(8).entry<uint32_t>(8).entry<uint32_t>(1).create();
  pipelineMaker.shader(transmittance_comp, __ArraySize__(transmittance_comp), &spInfo);

  transmittancePipeline =
    pipelineMaker.createUnique(nullptr, *transmittanceLayoutDef.pipelineLayout);

  transmittanceSet = transmittanceSetDef.createSet(*descriptorPool);
  transmittanceSetDef.atmosphere(_atmosphereUBO->buffer());
  transmittanceSetDef.transmittance(*transmittance_texture_);
  transmittanceSetDef.update(transmittanceSet);
}

void SkyModel::recordTransmittanceCMD(vk::CommandBuffer cb) {

  auto dim = transmittance_texture_->extent();

  transmittance_texture_->transitToLayout(
    cb, layout::eGeneral, access::eShaderWrite, stage::eComputeShader);

  cb.bindPipeline(bindpoint::eCompute, *transmittancePipeline);
  cb.bindDescriptorSets(
    bindpoint::eCompute, *transmittanceLayoutDef.pipelineLayout,
    transmittanceLayoutDef.set.set(), transmittanceSet, nullptr);
  cb.dispatch(dim.width / 8, dim.height / 8, 1);

  cb.pipelineBarrier(
    stage::eComputeShader, stage::eComputeShader, {}, nullptr, nullptr,
    transmittance_texture_->barrier(
      layout::eShaderReadOnlyOptimal, access::eShaderRead, stage::eComputeShader));
}
}

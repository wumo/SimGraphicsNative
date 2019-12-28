#include "sky_model.h"
#include "sim/graphics/base/pipeline/render_pass.h"
#include "sim/graphics/compiledShaders/basic/sky/computeTransmittance_comp.h"
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

auto SkyModel::createTransmittance(Texture &transmittanceTexture)
  -> SkyModel::ComputeCMD {
  ComputePipelineMaker pipelineMaker{device.getDevice()};

  SpecializationMaker sp{};
  auto spInfo = sp.entry<uint32_t>(8).entry<uint32_t>(8).entry<uint32_t>(1).create();
  pipelineMaker.shader(
    computeTransmittance_comp, __ArraySize__(computeTransmittance_comp), &spInfo);

  transmittancePipeline =
    pipelineMaker.createUnique(nullptr, *transmittanceLayoutDef.pipelineLayout);

  transmittanceSet = transmittanceSetDef.createSet(*descriptorPool);
  transmittanceSetDef.atmosphere(_atmosphereUBO->buffer());
  transmittanceSetDef.transmittance(transmittanceTexture);
  transmittanceSetDef.update(transmittanceSet);

  return [&](vk::CommandBuffer cb) {
    auto dim = transmittanceTexture.extent();

    transmittanceTexture.transitToLayout(
      cb, layout::eGeneral, access::eShaderWrite, stage::eComputeShader);

    cb.bindPipeline(bindpoint::eCompute, *transmittancePipeline);
    cb.bindDescriptorSets(
      bindpoint::eCompute, *transmittanceLayoutDef.pipelineLayout,
      transmittanceLayoutDef.set.set(), transmittanceSet, nullptr);
    cb.dispatch(dim.width / 8, dim.height / 8, 1);

    cb.pipelineBarrier(
      stage::eComputeShader, stage::eComputeShader, {}, nullptr, nullptr,
      transmittanceTexture.barrier(
        layout::eShaderReadOnlyOptimal, access::eShaderRead, stage::eComputeShader));
  };
}
}

#include "sky_model.h"
#include "sim/graphics/base/pipeline/render_pass.h"
#include "sim/graphics/base/pipeline/pipeline.h"
#include "sim/graphics/base/pipeline/descriptors.h"
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
namespace {
struct ComputeTransmittanceDescriptorDef: DescriptorSetDef {
  __uniform__(atmosphere, shader::eCompute);
  __storageImage__(transmittance, shader::eCompute);
};
struct ComputeTransmittanceLayoutDef: PipelineLayoutDef {
  __set__(set, ComputeTransmittanceDescriptorDef);
};
}

void SkyModel::computeTransmittance(Texture &transmittanceTexture) {
  ComputePipelineMaker pipelineMaker{device.getDevice()};

  pipelineMaker.shader(
    computeTransmittance_comp, __ArraySize__(computeTransmittance_comp));

  auto dim = transmittanceTexture.extent();

  ComputeTransmittanceDescriptorDef setDef{};
  setDef.init(device.getDevice());
  ComputeTransmittanceLayoutDef layoutDef{};
  layoutDef.set(setDef);
  layoutDef.init(device.getDevice());

  auto descriptorPool =
    DescriptorPoolMaker().pipelineLayout(layoutDef).createUnique(device.getDevice());

  auto set = setDef.createSet(*descriptorPool);
  setDef.atmosphere(_atmosphereUBO->buffer());
  setDef.transmittance(transmittanceTexture);
  setDef.update(set);

  auto pipeline = pipelineMaker.createUnique(nullptr, *layoutDef.pipelineLayout);

  device.computeImmediately([&](vk::CommandBuffer cb) {
    transmittanceTexture.transitToLayout(
      cb, layout::eGeneral, access::eShaderWrite, stage::eComputeShader);

    cb.bindPipeline(bindpoint::eCompute, *pipeline);
    cb.bindDescriptorSets(
      bindpoint::eCompute, *layoutDef.pipelineLayout, layoutDef.set.set(), set, nullptr);
    cb.dispatch(dim.width, dim.height, 1);

    cb.pipelineBarrier(
      stage::eComputeShader, stage::eComputeShader, {}, nullptr, nullptr,
      transmittanceTexture.barrier(
        layout::eShaderReadOnlyOptimal, access::eShaderRead, stage::eComputeShader));
  });
}
}

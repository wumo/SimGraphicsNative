#include "sky_model.h"
#include "constants.h"
#include "sim/graphics/base/pipeline/render_pass.h"
#include "sim/graphics/base/pipeline/pipeline.h"
#include "sim/graphics/base/pipeline/descriptors.h"
#include "sim/graphics/compiledShaders/basic/sky/computeTransmittance_comp.h"

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
}

void SkyModel::computeTransmittance(Texture &transmittanceTexture) {
  SpecializationMaker sp;
  auto spInfo = sp.entry<uint32_t>(1u).entry<uint32_t>(1u).create();

  ComputePipelineMaker pipelineMaker{device.getDevice()};

  pipelineMaker.shader(
    computeTransmittance_comp, __ArraySize__(computeTransmittance_comp), &spInfo);

  auto dim = transmittanceTexture.extent();

  // Descriptor Pool
  std::vector<vk::DescriptorPoolSize> poolSizes{
    {vk::DescriptorType::eUniformBuffer, 1},
    {vk::DescriptorType::eStorageImage, 1},
  };
  vk::DescriptorPoolCreateInfo descriptorPoolInfo{
    {}, 1, (uint32_t)poolSizes.size(), poolSizes.data()};
  auto descriptorPool = device.getDevice().createDescriptorPoolUnique(descriptorPoolInfo);
  // Descriptor sets
  ComputeTransmittanceDescriptorDef setDef;
  setDef.init(device.getDevice());
  auto set = setDef.createSet(*descriptorPool);
  setDef.atmosphere(uboBuffer->buffer());
  setDef.transmittance(transmittanceTexture);
  setDef.update(set);

  auto pipelineLayout = PipelineLayoutMaker()
                          .descriptorSetLayout(*setDef.descriptorSetLayout)
                          .createUnique(device.getDevice());

  auto pipeline = pipelineMaker.createUnique(nullptr, *pipelineLayout);

  device.computeImmediately([&](vk::CommandBuffer cb) {
    transmittance_texture_->setLayout(
      cb, layout::eGeneral, access::eShaderRead, access::eShaderWrite,
      stage::eComputeShader, stage::eComputeShader);

    cb.bindPipeline(bindpoint::eCompute, *pipeline);
    cb.bindDescriptorSets(bindpoint::eCompute, *pipelineLayout, 0, set, nullptr);
    cb.dispatch(dim.width, dim.height, 1);
  });
}
}

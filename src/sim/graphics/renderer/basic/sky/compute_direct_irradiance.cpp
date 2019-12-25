#include "sky_model.h"
#include "constants.h"
#include "sim/graphics/base/pipeline/render_pass.h"
#include "sim/graphics/base/pipeline/pipeline.h"
#include "sim/graphics/base/pipeline/descriptors.h"
#include "sim/graphics/compiledShaders/basic/sky/computeDirectIrradiance_comp.h"

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
struct ComputeDirectIrradianceDescriptorDef: DescriptorSetDef {
  __uniform__(atmosphere, shader::eCompute);
  __sampler__(transmittance, shader::eCompute);
  __storageImage__(delta_irradiance, shader::eCompute);
  __storageImage__(irradiance, shader::eCompute);
};
}

void SkyModel::computeDirectIrradiance(
  bool blend, Texture &deltaIrradianceTexture, Texture &irradianceTexture,
  Texture &transmittanceTexture) {
  SpecializationMaker sp;
  auto spInfo = sp.entry<uint32_t>(1u).entry<uint32_t>(1u).create();

  ComputePipelineMaker pipelineMaker{device.getDevice()};

  pipelineMaker.shader(
    computeDirectIrradiance_comp, __ArraySize__(computeDirectIrradiance_comp), &spInfo);

  auto dim = irradianceTexture.extent();

  // Descriptor Pool
  std::vector<vk::DescriptorPoolSize> poolSizes{
    {vk::DescriptorType::eUniformBuffer, 1},
    {vk::DescriptorType::eCombinedImageSampler, 1},
    {vk::DescriptorType::eStorageImage, 2},
  };
  vk::DescriptorPoolCreateInfo descriptorPoolInfo{
    {}, 1, (uint32_t)poolSizes.size(), poolSizes.data()};
  auto descriptorPool = device.getDevice().createDescriptorPoolUnique(descriptorPoolInfo);
  // Descriptor sets
  ComputeDirectIrradianceDescriptorDef setDef;
  setDef.init(device.getDevice());
  auto set = setDef.createSet(*descriptorPool);
  setDef.atmosphere(uboBuffer->buffer());
  setDef.transmittance(transmittanceTexture);
  setDef.delta_irradiance(deltaIrradianceTexture);
  setDef.irradiance(irradianceTexture);
  setDef.update(set);

  auto pipelineLayout = PipelineLayoutMaker()
                          .descriptorSetLayout(*setDef.descriptorSetLayout)
                          .createUnique(device.getDevice());

  auto pipeline = pipelineMaker.createUnique(nullptr, *pipelineLayout);

  device.computeImmediately([&](vk::CommandBuffer cb) {
    deltaIrradianceTexture.setLayout(
      cb, layout::eGeneral, access::eShaderRead, access::eShaderWrite,
      stage::eComputeShader, stage::eComputeShader);
    irradiance_texture_->setLayout(
      cb, layout::eGeneral, access::eShaderRead, access::eShaderWrite,
      stage::eComputeShader, stage::eComputeShader);
    transmittance_texture_->setLayout(
      cb, layout::eShaderReadOnlyOptimal, access::eShaderWrite, access::eShaderRead,
      stage::eComputeShader, stage::eComputeShader);

    cb.bindPipeline(bindpoint::eCompute, *pipeline);
    cb.bindDescriptorSets(bindpoint::eCompute, *pipelineLayout, 0, set, nullptr);
    cb.dispatch(dim.width, dim.height, 1);
  });
}
}

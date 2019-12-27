#include "sky_model.h"
#include "constants.h"
#include "sim/graphics/base/pipeline/render_pass.h"
#include "sim/graphics/base/pipeline/pipeline.h"
#include "sim/graphics/base/pipeline/descriptors.h"
#include "sim/graphics/compiledShaders/basic/sky/computeIndirectIrradiance_comp.h"

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
struct ComputeIndirectIrradianceDescriptorDef: DescriptorSetDef {
  __uniform__(atmosphere, shader::eCompute);
  __uniform__(luminance_from_radiance, shader::eCompute);
  __uniform__(scattering_order, shader::eCompute);
  __sampler__(delta_rayleigh, shader::eCompute);
  __sampler__(delta_mie, shader::eCompute);
  __sampler__(multpli_scattering, shader::eCompute);
  __storageImage__(delta_irradiance, shader::eCompute);
  __storageImage__(irradiance, shader::eCompute);
};
}

void SkyModel::computeIndirectIrradiance(
  Texture &deltaIrradianceTexture, Texture &irradianceTexture,
  Texture &deltaRayleighScatteringTexture, Texture &deltaMieScatteringTexture,
  Texture &deltaMultipleScatteringTexture) {
  auto dim = irradianceTexture.extent();

  // Descriptor Pool
  std::vector<vk::DescriptorPoolSize> poolSizes{
    {vk::DescriptorType::eUniformBuffer, 3},
    {vk::DescriptorType::eCombinedImageSampler, 3},
    {vk::DescriptorType::eStorageImage, 2},
  };
  vk::DescriptorPoolCreateInfo descriptorPoolInfo{
    {}, 1, (uint32_t)poolSizes.size(), poolSizes.data()};
  auto descriptorPool = device.getDevice().createDescriptorPoolUnique(descriptorPoolInfo);
  // Descriptor sets
  ComputeIndirectIrradianceDescriptorDef setDef;
  setDef.init(device.getDevice());
  auto set = setDef.createSet(*descriptorPool);
  setDef.atmosphere(uboBuffer->buffer());
  setDef.luminance_from_radiance(LFRUniformBuffer->buffer());
  setDef.scattering_order(ScatterOrderBuffer->buffer());
  setDef.delta_rayleigh(deltaRayleighScatteringTexture);
  setDef.delta_mie(deltaMieScatteringTexture);
  setDef.multpli_scattering(deltaMultipleScatteringTexture);
  setDef.delta_irradiance(deltaIrradianceTexture);
  setDef.irradiance(irradianceTexture);
  setDef.update(set);

  auto pipelineLayout = PipelineLayoutMaker()
                          .descriptorSetLayout(*setDef.descriptorSetLayout)
                          .createUnique(device.getDevice());

  ComputePipelineMaker pipelineMaker{device.getDevice()};
  pipelineMaker.shader(
    computeIndirectIrradiance_comp, __ArraySize__(computeIndirectIrradiance_comp));
  auto pipeline = pipelineMaker.createUnique(nullptr, *pipelineLayout);

  device.computeImmediately([&](vk::CommandBuffer cb) {
    deltaIrradianceTexture.transitToLayout(
      cb, layout::eGeneral, access::eShaderWrite, stage::eComputeShader);

    irradianceTexture.transitToLayout(
      cb, layout::eGeneral, access::eShaderWrite, stage::eComputeShader);

    cb.bindPipeline(bindpoint::eCompute, *pipeline);
    cb.bindDescriptorSets(bindpoint::eCompute, *pipelineLayout, 0, set, nullptr);
    cb.dispatch(dim.width, dim.height, dim.depth);

    {
      vk::ImageMemoryBarrier deltaIrradianceBarrier{
        access::eShaderWrite,
        access::eShaderRead,
        layout::eGeneral,
        layout::eShaderReadOnlyOptimal,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        deltaIrradianceTexture.image(),
        deltaIrradianceTexture.subresourceRange(vk::ImageAspectFlagBits::eColor)};
      vk::ImageMemoryBarrier irradianceBarrier{
        access::eShaderWrite,
        access::eShaderRead,
        layout::eGeneral,
        layout::eShaderReadOnlyOptimal,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        irradianceTexture.image(),
        irradianceTexture.subresourceRange(vk::ImageAspectFlagBits::eColor)};
      cb.pipelineBarrier(
        stage::eComputeShader, stage::eComputeShader, {}, nullptr, nullptr,
        {deltaIrradianceBarrier, irradianceBarrier});

      deltaIrradianceTexture.setCurrentState(
        layout::eShaderReadOnlyOptimal, access::eShaderRead, stage::eComputeShader);
      irradianceTexture.setCurrentState(
        layout::eShaderReadOnlyOptimal, access::eShaderRead, stage::eComputeShader);
    }
  });
}
}

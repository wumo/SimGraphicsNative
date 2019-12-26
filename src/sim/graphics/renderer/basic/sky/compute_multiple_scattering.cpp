#include "sky_model.h"
#include "constants.h"
#include "sim/graphics/base/pipeline/render_pass.h"
#include "sim/graphics/base/pipeline/pipeline.h"
#include "sim/graphics/base/pipeline/descriptors.h"
#include "sim/graphics/compiledShaders/basic/sky/computeMultipleScattering_comp.h"

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
struct ComputeMultipleScatteringDescriptorDef: DescriptorSetDef {
  __uniform__(atmosphere, shader::eCompute);
  __uniform__(luminance_from_radiance, shader::eCompute);
  __sampler__(transmittance, shader::eCompute);
  __sampler__(scattering_density, shader::eCompute);
  __storageImage__(delta_multiple_scattering, shader::eCompute);
  __storageImage__(scattering, shader::eCompute);
};
}

void SkyModel::computeMultipleScattering(
  Texture &deltaMultipleScatteringTexture, Texture &scatteringTexture,
  Texture &transmittanceTexture, Texture &deltaScatteringDensityTexture) {
  auto dim = scatteringTexture.extent();

  // Descriptor Pool
  std::vector<vk::DescriptorPoolSize> poolSizes{
    {vk::DescriptorType::eUniformBuffer, 2},
    {vk::DescriptorType::eCombinedImageSampler, 2},
    {vk::DescriptorType::eStorageImage, 2},
  };
  vk::DescriptorPoolCreateInfo descriptorPoolInfo{
    {}, 1, (uint32_t)poolSizes.size(), poolSizes.data()};
  auto descriptorPool = device.getDevice().createDescriptorPoolUnique(descriptorPoolInfo);
  // Descriptor sets
  ComputeMultipleScatteringDescriptorDef setDef;
  setDef.init(device.getDevice());
  auto set = setDef.createSet(*descriptorPool);
  setDef.atmosphere(uboBuffer->buffer());
  setDef.luminance_from_radiance(LFRUniformBuffer->buffer());
  setDef.transmittance(transmittanceTexture);
  setDef.scattering_density(deltaScatteringDensityTexture);
  setDef.delta_multiple_scattering(deltaMultipleScatteringTexture);
  setDef.scattering(scatteringTexture);
  setDef.update(set);

  auto pipelineLayout = PipelineLayoutMaker()
                          .descriptorSetLayout(*setDef.descriptorSetLayout)
                          .createUnique(device.getDevice());

  ComputePipelineMaker pipelineMaker{device.getDevice()};
  pipelineMaker.shader(
    computeMultipleScattering_comp, __ArraySize__(computeMultipleScattering_comp));
  auto pipeline = pipelineMaker.createUnique(nullptr, *pipelineLayout);

  device.computeImmediately([&](vk::CommandBuffer cb) {
    deltaMultipleScatteringTexture.setLayout(
      cb, layout::eShaderReadOnlyOptimal, layout ::eGeneral, access::eShaderRead,
      access::eShaderWrite, stage::eComputeShader, stage::eComputeShader);
    scatteringTexture.setLayout(
      cb, layout::eShaderReadOnlyOptimal, layout ::eGeneral, access::eShaderRead,
      access::eShaderWrite, stage::eComputeShader, stage::eComputeShader);

    cb.bindPipeline(bindpoint::eCompute, *pipeline);
    cb.bindDescriptorSets(bindpoint::eCompute, *pipelineLayout, 0, set, nullptr);
    cb.dispatch(dim.width, dim.height, dim.depth);

    {
      vk::ImageMemoryBarrier deltaMultipleScatterBarrier{
        access::eShaderWrite,
        access::eShaderRead,
        layout::eGeneral,
        layout::eShaderReadOnlyOptimal,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        deltaMultipleScatteringTexture.image(),
        deltaMultipleScatteringTexture.subresourceRange(vk::ImageAspectFlagBits::eColor)};
      vk::ImageMemoryBarrier deltaScatterBarrier{
        access::eShaderWrite,
        access::eShaderRead,
        layout::eGeneral,
        layout::eShaderReadOnlyOptimal,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        scatteringTexture.image(),
        scatteringTexture.subresourceRange(vk::ImageAspectFlagBits::eColor)};
      cb.pipelineBarrier(
        stage::eComputeShader, stage::eComputeShader, {}, nullptr, nullptr,
        {deltaMultipleScatterBarrier, deltaScatterBarrier});
      deltaScatteringDensityTexture.setCurrentLayout(layout::eShaderReadOnlyOptimal);
      scatteringTexture.setCurrentLayout(layout::eShaderReadOnlyOptimal);
    }
  });
}
}

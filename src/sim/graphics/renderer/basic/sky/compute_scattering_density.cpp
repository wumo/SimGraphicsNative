#include "sky_model.h"
#include "sim/graphics/base/pipeline/render_pass.h"
#include "sim/graphics/base/pipeline/pipeline.h"
#include "sim/graphics/base/pipeline/descriptors.h"
#include "sim/graphics/compiledShaders/basic/sky/computeScatteringDensity_comp.h"

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
struct ComputeScatteringDensityDescriptorDef: DescriptorSetDef {
  __uniform__(atmosphere, shader::eCompute);
  __uniform__(scattering_order, shader::eCompute);
  __sampler__(transmittance, shader::eCompute);
  __sampler__(delta_rayleigh, shader::eCompute);
  __sampler__(delta_mie, shader::eCompute);
  __sampler__(multpli_scattering, shader::eCompute);
  __sampler__(irradiance, shader::eCompute);
  __storageImage__(scattering_density, shader::eCompute);
};
}

void SkyModel::computeScatteringDensity(
  Texture &deltaScatteringDensityTexture, Texture &transmittanceTexture,
  Texture &deltaRayleighScatteringTexture, Texture &deltaMieScatteringTexture,
  Texture &deltaMultipleScatteringTexture, Texture &deltaIrradianceTexture) {
  auto dim = deltaScatteringDensityTexture.extent();

  // Descriptor Pool
  std::vector<vk::DescriptorPoolSize> poolSizes{
    {vk::DescriptorType::eUniformBuffer, 2},
    {vk::DescriptorType::eCombinedImageSampler, 5},
    {vk::DescriptorType::eStorageImage, 1},
  };
  vk::DescriptorPoolCreateInfo descriptorPoolInfo{
    {}, 1, (uint32_t)poolSizes.size(), poolSizes.data()};
  auto descriptorPool = device.getDevice().createDescriptorPoolUnique(descriptorPoolInfo);
  // Descriptor sets
  ComputeScatteringDensityDescriptorDef setDef;
  setDef.init(device.getDevice());
  auto set = setDef.createSet(*descriptorPool);
  setDef.atmosphere(uboBuffer->buffer());
  setDef.scattering_order(ScatterOrderBuffer->buffer());
  setDef.transmittance(transmittanceTexture);
  setDef.delta_rayleigh(deltaRayleighScatteringTexture);
  setDef.delta_mie(deltaMieScatteringTexture);
  setDef.multpli_scattering(deltaMultipleScatteringTexture);
  setDef.irradiance(deltaIrradianceTexture);
  setDef.scattering_density(deltaScatteringDensityTexture);
  setDef.update(set);

  auto pipelineLayout = PipelineLayoutMaker()
                          .descriptorSetLayout(*setDef.descriptorSetLayout)
                          .createUnique(device.getDevice());

  ComputePipelineMaker pipelineMaker{device.getDevice()};
  pipelineMaker.shader(
    computeScatteringDensity_comp, __ArraySize__(computeScatteringDensity_comp));
  auto pipeline = pipelineMaker.createUnique(nullptr, *pipelineLayout);

  device.computeImmediately([&](vk::CommandBuffer cb) {
    deltaScatteringDensityTexture.setLayout(
      cb, layout::eUndefined, layout::eGeneral, access::eShaderRead, access::eShaderWrite,
      stage::eComputeShader, stage::eComputeShader);

    cb.bindPipeline(bindpoint::eCompute, *pipeline);
    cb.bindDescriptorSets(bindpoint::eCompute, *pipelineLayout, 0, set, nullptr);
    cb.dispatch(dim.width, dim.height, dim.depth);

    {
      vk::ImageMemoryBarrier barrier{
        access::eShaderWrite,
        access::eShaderRead,
        layout::eGeneral,
        layout::eShaderReadOnlyOptimal,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        deltaScatteringDensityTexture.image(),
        deltaScatteringDensityTexture.subresourceRange(vk::ImageAspectFlagBits::eColor)};
      cb.pipelineBarrier(
        stage::eComputeShader, stage::eComputeShader, {}, nullptr, nullptr, barrier);
      deltaScatteringDensityTexture.setCurrentLayout(layout::eShaderReadOnlyOptimal);
    }
  });
}
}

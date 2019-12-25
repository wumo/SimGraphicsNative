#include "sky_model.h"
#include "sim/graphics/base/pipeline/render_pass.h"
#include "sim/graphics/base/pipeline/pipeline.h"
#include "sim/graphics/base/pipeline/descriptors.h"
#include "sim/graphics/compiledShaders/basic/sky/computeSingleScattering_comp.h"
#include "sim/graphics/compiledShaders/basic/sky/computeDebug_comp.h"
#include "sim/graphics/compiledShaders/basic/sky/computeDebug3D_comp.h"

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
struct ComputeSingleScatteringDescriptorDef: DescriptorSetDef {
  __uniform__(atmosphere, shader::eCompute);
  __uniform__(luminance_from_radiance, shader::eCompute);
  __sampler__(transmittance, shader::eCompute);
  __storageImage__(delta_rayleigh, shader::eCompute);
  __storageImage__(delta_mie, shader::eCompute);
  __storageImage__(scattering, shader::eCompute);
};
}

void SkyModel::computeSingleScattering(
  bool blend, Texture &deltaRayleighScatteringTexture, Texture &deltaMieScatteringTexture,
  Texture &scatteringTexture, Texture &transmittanceTexture) {
  auto dim = scatteringTexture.extent();

  // Descriptor Pool
  std::vector<vk::DescriptorPoolSize> poolSizes{
    {vk::DescriptorType::eUniformBuffer, 2},
    {vk::DescriptorType::eCombinedImageSampler, 1},
    {vk::DescriptorType::eStorageImage, 3},
  };
  vk::DescriptorPoolCreateInfo descriptorPoolInfo{
    {}, 1, (uint32_t)poolSizes.size(), poolSizes.data()};
  auto descriptorPool = device.getDevice().createDescriptorPoolUnique(descriptorPoolInfo);
  // Descriptor sets
  ComputeSingleScatteringDescriptorDef setDef;
  setDef.init(device.getDevice());
  auto set = setDef.createSet(*descriptorPool);
  setDef.atmosphere(uboBuffer->buffer());
  setDef.luminance_from_radiance(LFRUniformBuffer->buffer());
  setDef.transmittance(transmittanceTexture);
  setDef.delta_rayleigh(deltaRayleighScatteringTexture);
  setDef.delta_mie(deltaMieScatteringTexture);
  setDef.scattering(scatteringTexture);
  setDef.update(set);

  auto pipelineLayout = PipelineLayoutMaker()
                          .descriptorSetLayout(*setDef.descriptorSetLayout)
                          .createUnique(device.getDevice());

  ComputePipelineMaker pipelineMaker{device.getDevice()};
  pipelineMaker.shader(
    computeSingleScattering_comp, __ArraySize__(computeSingleScattering_comp));
  auto pipeline = pipelineMaker.createUnique(nullptr, *pipelineLayout);

  device.computeImmediately([&](vk::CommandBuffer cb) {
    deltaRayleighScatteringTexture.setLayout(
      cb, layout::eUndefined, layout::eGeneral, access::eShaderRead, access::eShaderWrite,
      stage::eComputeShader, stage::eComputeShader);
    deltaMieScatteringTexture.setLayout(
      cb, layout::eUndefined, layout::eGeneral, access::eShaderRead, access::eShaderWrite,
      stage::eComputeShader, stage::eComputeShader);
    scatteringTexture.setLayout(
      cb, layout::eUndefined, layout::eGeneral, access::eShaderRead, access::eShaderWrite,
      stage::eComputeShader, stage::eComputeShader);

    cb.bindPipeline(bindpoint::eCompute, *pipeline);
    cb.bindDescriptorSets(bindpoint::eCompute, *pipelineLayout, 0, set, nullptr);
    cb.dispatch(dim.width, dim.height, dim.depth);

    {
      vk::ImageMemoryBarrier deltaRayBarrier{
        access::eShaderWrite,
        access::eShaderRead,
        layout::eGeneral,
        layout::eShaderReadOnlyOptimal,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        deltaRayleighScatteringTexture.image(),
        deltaRayleighScatteringTexture.subresourceRange(vk::ImageAspectFlagBits::eColor)};
      vk::ImageMemoryBarrier deltaMieBarrier{
        access::eShaderWrite,
        access::eShaderRead,
        layout::eGeneral,
        layout::eShaderReadOnlyOptimal,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        deltaMieScatteringTexture.image(),
        deltaMieScatteringTexture.subresourceRange(vk::ImageAspectFlagBits::eColor)};
      vk::ImageMemoryBarrier scatteringBarrier{
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
        {deltaRayBarrier, deltaMieBarrier, scatteringBarrier});
    }
  });
}
}

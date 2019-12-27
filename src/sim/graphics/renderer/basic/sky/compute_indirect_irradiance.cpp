#include "sky_model.h"
#include "sim/graphics/base/pipeline/render_pass.h"
#include "sim/graphics/base/pipeline/pipeline.h"
#include "sim/graphics/base/pipeline/descriptors.h"
#include "sim/graphics/compiledShaders/basic/sky/computeIndirectIrradiance_comp.h"
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
struct ComputeIndirectIrradianceLayoutDef: PipelineLayoutDef {
  __set__(set, ComputeIndirectIrradianceDescriptorDef);
};
}

void SkyModel::computeIndirectIrradiance(
  Texture &deltaIrradianceTexture, Texture &irradianceTexture,
  Texture &deltaRayleighScatteringTexture, Texture &deltaMieScatteringTexture,
  Texture &deltaMultipleScatteringTexture) {
  auto dim = irradianceTexture.extent();

  ComputeIndirectIrradianceDescriptorDef setDef{};
  setDef.init(device.getDevice());
  ComputeIndirectIrradianceLayoutDef layoutDef{};
  layoutDef.set(setDef);
  layoutDef.init(device.getDevice());

  auto descriptorPool =
    DescriptorPoolMaker().pipelineLayout(layoutDef).createUnique(device.getDevice());

  auto set = setDef.createSet(*descriptorPool);
  setDef.atmosphere(_atmosphereUBO->buffer());
  setDef.luminance_from_radiance(LFRUBO->buffer());
  setDef.scattering_order(ScatterOrderUBO->buffer());
  setDef.delta_rayleigh(deltaRayleighScatteringTexture);
  setDef.delta_mie(deltaMieScatteringTexture);
  setDef.multpli_scattering(deltaMultipleScatteringTexture);
  setDef.delta_irradiance(deltaIrradianceTexture);
  setDef.irradiance(irradianceTexture);
  setDef.update(set);

  ComputePipelineMaker pipelineMaker{device.getDevice()};
  pipelineMaker.shader(
    computeIndirectIrradiance_comp, __ArraySize__(computeIndirectIrradiance_comp));
  auto pipeline = pipelineMaker.createUnique(nullptr, *layoutDef.pipelineLayout);

  device.computeImmediately([&](vk::CommandBuffer cb) {
    deltaIrradianceTexture.transitToLayout(
      cb, layout::eGeneral, access::eShaderWrite, stage::eComputeShader);

    irradianceTexture.transitToLayout(
      cb, layout::eGeneral, access::eShaderWrite, stage::eComputeShader);

    cb.bindPipeline(bindpoint::eCompute, *pipeline);
    cb.bindDescriptorSets(
      bindpoint::eCompute, *layoutDef.pipelineLayout, layoutDef.set.set(), set, nullptr);
    cb.dispatch(dim.width, dim.height, dim.depth);

    cb.pipelineBarrier(
      stage::eComputeShader, stage::eComputeShader, {}, nullptr, nullptr,
      {deltaIrradianceTexture.barrier(
         layout::eShaderReadOnlyOptimal, access::eShaderRead, stage::eComputeShader),
       irradianceTexture.barrier(
         layout::eShaderReadOnlyOptimal, access::eShaderRead, stage::eComputeShader)});
  });
}
}

#include "sky_model.h"
#include "sim/graphics/base/pipeline/render_pass.h"
#include "sim/graphics/base/pipeline/pipeline.h"
#include "sim/graphics/base/pipeline/descriptors.h"
#include "sim/graphics/compiledShaders/basic/sky/computeScatteringDensity_comp.h"
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
struct ComputeScatteringDensityLayoutDef: PipelineLayoutDef {
  __set__(set, ComputeScatteringDensityDescriptorDef);
};
}

void SkyModel::computeScatteringDensity(
  Texture &deltaScatteringDensityTexture, Texture &transmittanceTexture,
  Texture &deltaRayleighScatteringTexture, Texture &deltaMieScatteringTexture,
  Texture &deltaMultipleScatteringTexture, Texture &deltaIrradianceTexture) {
  auto dim = deltaScatteringDensityTexture.extent();

  ComputeScatteringDensityDescriptorDef setDef{};
  setDef.init(device.getDevice());
  ComputeScatteringDensityLayoutDef layoutDef{};
  layoutDef.set(setDef);
  layoutDef.init(device.getDevice());

  auto descriptorPool =
    DescriptorPoolMaker().pipelineLayout(layoutDef).createUnique(device.getDevice());

  auto set = setDef.createSet(*descriptorPool);
  setDef.atmosphere(_atmosphereUBO->buffer());
  setDef.scattering_order(ScatterOrderUBO->buffer());
  setDef.transmittance(transmittanceTexture);
  setDef.delta_rayleigh(deltaRayleighScatteringTexture);
  setDef.delta_mie(deltaMieScatteringTexture);
  setDef.multpli_scattering(deltaMultipleScatteringTexture);
  setDef.irradiance(deltaIrradianceTexture);
  setDef.scattering_density(deltaScatteringDensityTexture);
  setDef.update(set);

  ComputePipelineMaker pipelineMaker{device.getDevice()};
  pipelineMaker.shader(
    computeScatteringDensity_comp, __ArraySize__(computeScatteringDensity_comp));
  auto pipeline = pipelineMaker.createUnique(nullptr, *layoutDef.pipelineLayout);

  device.computeImmediately([&](vk::CommandBuffer cb) {
    deltaScatteringDensityTexture.transitToLayout(
      cb, layout::eGeneral, access::eShaderWrite, stage::eComputeShader);

    cb.bindPipeline(bindpoint::eCompute, *pipeline);
    cb.bindDescriptorSets(
      bindpoint::eCompute, *layoutDef.pipelineLayout, layoutDef.set.set(), set, nullptr);
    cb.dispatch(dim.width, dim.height, dim.depth);

    cb.pipelineBarrier(
      stage::eComputeShader, stage::eComputeShader, {}, nullptr, nullptr,
      deltaScatteringDensityTexture.barrier(
        layout::eShaderReadOnlyOptimal, access::eShaderRead, stage::eComputeShader));
  });
}
}

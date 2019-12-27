#include "sky_model.h"
#include "sim/graphics/base/pipeline/render_pass.h"
#include "sim/graphics/base/pipeline/pipeline.h"
#include "sim/graphics/base/pipeline/descriptors.h"
#include "sim/graphics/compiledShaders/basic/sky/computeSingleScattering_comp.h"
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
struct ComputeSingleScatteringDescriptorDef: DescriptorSetDef {
  __uniform__(atmosphere, shader::eCompute);
  __uniform__(cumulate, shader::eCompute);
  __uniform__(luminance_from_radiance, shader::eCompute);
  __sampler__(transmittance, shader::eCompute);
  __storageImage__(delta_rayleigh, shader::eCompute);
  __storageImage__(delta_mie, shader::eCompute);
  __storageImage__(scattering, shader::eCompute);
};
struct ComputeSingleScatteringLayoutDef: PipelineLayoutDef {
  __set__(set, ComputeSingleScatteringDescriptorDef);
};
}

void SkyModel::computeSingleScattering(
  Texture &deltaRayleighScatteringTexture, Texture &deltaMieScatteringTexture,
  Texture &scatteringTexture, Texture &transmittanceTexture) {
  auto dim = scatteringTexture.extent();

  ComputeSingleScatteringDescriptorDef setDef{};
  setDef.init(device.getDevice());
  ComputeSingleScatteringLayoutDef layoutDef{};
  layoutDef.set(setDef);
  layoutDef.init(device.getDevice());

  auto descriptorPool =
    DescriptorPoolMaker().pipelineLayout(layoutDef).createUnique(device.getDevice());

  auto set = setDef.createSet(*descriptorPool);
  setDef.atmosphere(_atmosphereUBO->buffer());
  setDef.cumulate(cumulateUBO->buffer());
  setDef.luminance_from_radiance(LFRUBO->buffer());
  setDef.transmittance(transmittanceTexture);
  setDef.delta_rayleigh(deltaRayleighScatteringTexture);
  setDef.delta_mie(deltaMieScatteringTexture);
  setDef.scattering(scatteringTexture);
  setDef.update(set);

  ComputePipelineMaker pipelineMaker{device.getDevice()};
  pipelineMaker.shader(
    computeSingleScattering_comp, __ArraySize__(computeSingleScattering_comp));
  auto pipeline = pipelineMaker.createUnique(nullptr, *layoutDef.pipelineLayout);

  device.computeImmediately([&](vk::CommandBuffer cb) {
    deltaRayleighScatteringTexture.transitToLayout(
      cb, layout::eGeneral, access::eShaderWrite, stage::eComputeShader);
    deltaMieScatteringTexture.transitToLayout(
      cb, layout::eGeneral, access::eShaderWrite, stage::eComputeShader);
    scatteringTexture.transitToLayout(
      cb, layout::eGeneral, access::eShaderWrite, stage::eComputeShader);

    cb.bindPipeline(bindpoint::eCompute, *pipeline);
    cb.bindDescriptorSets(
      bindpoint::eCompute, *layoutDef.pipelineLayout, layoutDef.set.set(), set, nullptr);
    cb.dispatch(dim.width, dim.height, dim.depth);

    cb.pipelineBarrier(
      stage::eComputeShader, stage::eComputeShader, {}, nullptr, nullptr,
      {deltaRayleighScatteringTexture.barrier(
         layout::eShaderReadOnlyOptimal, access::eShaderRead, stage::eComputeShader),
       deltaMieScatteringTexture.barrier(
         layout::eShaderReadOnlyOptimal, access::eShaderRead, stage::eComputeShader),
       scatteringTexture.barrier(
         layout::eShaderReadOnlyOptimal, access::eShaderRead, stage::eComputeShader)});
  });
}
}

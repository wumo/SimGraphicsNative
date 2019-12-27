#include "sky_model.h"
#include "sim/graphics/base/pipeline/render_pass.h"
#include "sim/graphics/base/pipeline/pipeline.h"
#include "sim/graphics/base/pipeline/descriptors.h"
#include "sim/graphics/compiledShaders/basic/sky/computeMultipleScattering_comp.h"
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
struct ComputeMultipleScatteringDescriptorDef: DescriptorSetDef {
  __uniform__(atmosphere, shader::eCompute);
  __uniform__(luminance_from_radiance, shader::eCompute);
  __sampler__(transmittance, shader::eCompute);
  __sampler__(scattering_density, shader::eCompute);
  __storageImage__(delta_multiple_scattering, shader::eCompute);
  __storageImage__(scattering, shader::eCompute);
};
struct ComputeMultipleScatteringLayoutDef: PipelineLayoutDef {
  __set__(set, ComputeMultipleScatteringDescriptorDef);
};
}

void SkyModel::computeMultipleScattering(
  Texture &deltaMultipleScatteringTexture, Texture &scatteringTexture,
  Texture &transmittanceTexture, Texture &deltaScatteringDensityTexture) {
  auto dim = scatteringTexture.extent();

  ComputeMultipleScatteringDescriptorDef setDef{};
  setDef.init(device.getDevice());
  ComputeMultipleScatteringLayoutDef layoutDef{};
  layoutDef.set(setDef);
  layoutDef.init(device.getDevice());

  auto descriptorPool =
    DescriptorPoolMaker().pipelineLayout(layoutDef).createUnique(device.getDevice());

  auto set = setDef.createSet(*descriptorPool);
  setDef.atmosphere(_atmosphereUBO->buffer());
  setDef.luminance_from_radiance(LFRUBO->buffer());
  setDef.transmittance(transmittanceTexture);
  setDef.scattering_density(deltaScatteringDensityTexture);
  setDef.delta_multiple_scattering(deltaMultipleScatteringTexture);
  setDef.scattering(scatteringTexture);
  setDef.update(set);

  ComputePipelineMaker pipelineMaker{device.getDevice()};
  pipelineMaker.shader(
    computeMultipleScattering_comp, __ArraySize__(computeMultipleScattering_comp));
  auto pipeline = pipelineMaker.createUnique(nullptr, *layoutDef.pipelineLayout);

  device.computeImmediately([&](vk::CommandBuffer cb) {
    deltaMultipleScatteringTexture.transitToLayout(
      cb, layout ::eGeneral, access::eShaderWrite, stage::eComputeShader);
    scatteringTexture.transitToLayout(
      cb, layout ::eGeneral, access::eShaderWrite, stage::eComputeShader);

    cb.bindPipeline(bindpoint::eCompute, *pipeline);
    cb.bindDescriptorSets(
      bindpoint::eCompute, *layoutDef.pipelineLayout, layoutDef.set.set(), set, nullptr);
    cb.dispatch(dim.width, dim.height, dim.depth);

    cb.pipelineBarrier(
      stage::eComputeShader, stage::eComputeShader, {}, nullptr, nullptr,
      {deltaMultipleScatteringTexture.barrier(
         layout::eShaderReadOnlyOptimal, access::eShaderRead, stage::eComputeShader),
       scatteringTexture.barrier(
         layout::eShaderReadOnlyOptimal, access::eShaderRead, stage::eComputeShader)});
  });
}
}

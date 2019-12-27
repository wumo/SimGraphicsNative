#include "sky_model.h"
#include "sim/graphics/base/pipeline/render_pass.h"
#include "sim/graphics/base/pipeline/pipeline.h"
#include "sim/graphics/base/pipeline/descriptors.h"
#include "sim/graphics/compiledShaders/basic/sky/computeDirectIrradiance_comp.h"
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
struct ComputeDirectIrradianceDescriptorDef: DescriptorSetDef {
  __uniform__(atmosphere, shader::eCompute);
  __uniform__(cumulate, shader::eCompute);
  __sampler__(transmittance, shader::eCompute);
  __storageImage__(delta_irradiance, shader::eCompute);
  __storageImage__(irradiance, shader::eCompute);
};
struct ComputeDirectIrradianceLayoutDef: PipelineLayoutDef {
  __set__(set, ComputeDirectIrradianceDescriptorDef);
};
}

void SkyModel::computeDirectIrradiance(
  Texture &deltaIrradianceTexture, Texture &irradianceTexture,
  Texture &transmittanceTexture) {

  ComputePipelineMaker pipelineMaker{device.getDevice()};

  pipelineMaker.shader(
    computeDirectIrradiance_comp, __ArraySize__(computeDirectIrradiance_comp));

  auto dim = irradianceTexture.extent();

  ComputeDirectIrradianceDescriptorDef setDef{};
  setDef.init(device.getDevice());
  ComputeDirectIrradianceLayoutDef layoutDef{};
  layoutDef.set(setDef);
  layoutDef.init(device.getDevice());

  auto descriptorPool =
    DescriptorPoolMaker().pipelineLayout(layoutDef).createUnique(device.getDevice());

  auto set = setDef.createSet(*descriptorPool);
  setDef.atmosphere(_atmosphereUBO->buffer());
  setDef.cumulate(cumulateUBO->buffer());
  setDef.transmittance(transmittanceTexture);
  setDef.delta_irradiance(deltaIrradianceTexture);
  setDef.irradiance(irradianceTexture);
  setDef.update(set);

  auto pipeline = pipelineMaker.createUnique(nullptr, *layoutDef.pipelineLayout);

  device.computeImmediately([&](vk::CommandBuffer cb) {
    deltaIrradianceTexture.transitToLayout(
      cb, layout::eGeneral, access::eShaderWrite, stage::eComputeShader);
    irradianceTexture.transitToLayout(
      cb, layout::eGeneral, access::eShaderWrite, stage::eComputeShader);

    cb.bindPipeline(bindpoint::eCompute, *pipeline);
    cb.bindDescriptorSets(
      bindpoint::eCompute, *layoutDef.pipelineLayout, layoutDef.set.set(), set, nullptr);
    cb.dispatch(dim.width, dim.height, 1);

    cb.pipelineBarrier(
      stage::eComputeShader, stage::eComputeShader, {}, nullptr, nullptr,
      {deltaIrradianceTexture.barrier(
         layout::eShaderReadOnlyOptimal, access::eShaderRead, stage::eComputeShader),
       irradianceTexture.barrier(
         layout::eShaderReadOnlyOptimal, access::eShaderRead, stage::eComputeShader)});
  });
}
}

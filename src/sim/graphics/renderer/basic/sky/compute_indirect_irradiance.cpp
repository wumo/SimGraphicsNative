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

auto SkyModel::createIndirectIrradiance(
  Texture &deltaIrradianceTexture, Texture &irradianceTexture,
  Texture &deltaRayleighScatteringTexture, Texture &deltaMieScatteringTexture,
  Texture &deltaMultipleScatteringTexture) -> SkyModel::ComputeCMD {

  indirectIrradianceSet = indirectIrradianceSetDef.createSet(*descriptorPool);
  indirectIrradianceSetDef.atmosphere(_atmosphereUBO->buffer());
  indirectIrradianceSetDef.luminance_from_radiance(LFRUBO->buffer());
  indirectIrradianceSetDef.scattering_order(ScatterOrderUBO->buffer());
  indirectIrradianceSetDef.delta_rayleigh(deltaRayleighScatteringTexture);
  indirectIrradianceSetDef.delta_mie(deltaMieScatteringTexture);
  indirectIrradianceSetDef.multpli_scattering(deltaMultipleScatteringTexture);
  indirectIrradianceSetDef.delta_irradiance(deltaIrradianceTexture);
  indirectIrradianceSetDef.irradiance(irradianceTexture);
  indirectIrradianceSetDef.update(indirectIrradianceSet);

  SpecializationMaker sp{};
  auto spInfo = sp.entry<uint32_t>(8).entry<uint32_t>(8).entry<uint32_t>(1).create();

  ComputePipelineMaker pipelineMaker{device.getDevice()};
  pipelineMaker.shader(
    computeIndirectIrradiance_comp, __ArraySize__(computeIndirectIrradiance_comp),
    &spInfo);
  indirectIrradiancePipeline =
    pipelineMaker.createUnique(nullptr, *indirectIrradianceLayoutDef.pipelineLayout);

  return [&](vk::CommandBuffer cb) {
    auto dim = irradianceTexture.extent();

    deltaIrradianceTexture.transitToLayout(
      cb, layout::eGeneral, access::eShaderWrite, stage::eComputeShader);

    irradianceTexture.transitToLayout(
      cb, layout::eGeneral, access::eShaderWrite, stage::eComputeShader);

    cb.bindPipeline(bindpoint::eCompute, *indirectIrradiancePipeline);
    cb.bindDescriptorSets(
      bindpoint::eCompute, *indirectIrradianceLayoutDef.pipelineLayout,
      indirectIrradianceLayoutDef.set.set(), indirectIrradianceSet, nullptr);
    cb.dispatch(dim.width / 8, dim.height / 8, 1);

    cb.pipelineBarrier(
      stage::eComputeShader, stage::eComputeShader, {}, nullptr, nullptr,
      {deltaIrradianceTexture.barrier(
         layout::eShaderReadOnlyOptimal, access::eShaderRead, stage::eComputeShader),
       irradianceTexture.barrier(
         layout::eShaderReadOnlyOptimal, access::eShaderRead, stage::eComputeShader)});
  };
}
}

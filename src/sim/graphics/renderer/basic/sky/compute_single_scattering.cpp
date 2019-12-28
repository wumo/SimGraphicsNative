#include "sky_model.h"
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

auto SkyModel::createSingleScattering(
  Texture &deltaRayleighScatteringTexture, Texture &deltaMieScatteringTexture,
  Texture &scatteringTexture, Texture &transmittanceTexture) -> SkyModel::ComputeCMD {

  singleScatteringSet = singleScatteringSetDef.createSet(*descriptorPool);
  singleScatteringSetDef.atmosphere(_atmosphereUBO->buffer());
  singleScatteringSetDef.cumulate(cumulateUBO->buffer());
  singleScatteringSetDef.luminance_from_radiance(LFRUBO->buffer());
  singleScatteringSetDef.transmittance(transmittanceTexture);
  singleScatteringSetDef.delta_rayleigh(deltaRayleighScatteringTexture);
  singleScatteringSetDef.delta_mie(deltaMieScatteringTexture);
  singleScatteringSetDef.scattering(scatteringTexture);
  singleScatteringSetDef.update(singleScatteringSet);

  SpecializationMaker sp{};
  auto spInfo = sp.entry<uint32_t>(8).entry<uint32_t>(8).entry<uint32_t>(1).create();

  ComputePipelineMaker pipelineMaker{device.getDevice()};
  pipelineMaker.shader(
    computeSingleScattering_comp, __ArraySize__(computeSingleScattering_comp), &spInfo);
  singleScatteringPipeline =
    pipelineMaker.createUnique(nullptr, *singleScatteringLayoutDef.pipelineLayout);

  return [&](vk::CommandBuffer cb) {
    auto dim = scatteringTexture.extent();

    deltaRayleighScatteringTexture.transitToLayout(
      cb, layout::eGeneral, access::eShaderWrite, stage::eComputeShader);
    deltaMieScatteringTexture.transitToLayout(
      cb, layout::eGeneral, access::eShaderWrite, stage::eComputeShader);
    scatteringTexture.transitToLayout(
      cb, layout::eGeneral, access::eShaderWrite, stage::eComputeShader);

    cb.bindPipeline(bindpoint::eCompute, *singleScatteringPipeline);
    cb.bindDescriptorSets(
      bindpoint::eCompute, *singleScatteringLayoutDef.pipelineLayout,
      singleScatteringLayoutDef.set.set(), singleScatteringSet, nullptr);
    cb.dispatch(dim.width / 8, dim.height / 8, dim.depth / 1);

    cb.pipelineBarrier(
      stage::eComputeShader, stage::eComputeShader, {}, nullptr, nullptr,
      {deltaRayleighScatteringTexture.barrier(
         layout::eShaderReadOnlyOptimal, access::eShaderRead, stage::eComputeShader),
       deltaMieScatteringTexture.barrier(
         layout::eShaderReadOnlyOptimal, access::eShaderRead, stage::eComputeShader),
       scatteringTexture.barrier(
         layout::eShaderReadOnlyOptimal, access::eShaderRead, stage::eComputeShader)});
  };
}
}

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

auto SkyModel::createScatteringDensity(
  Texture &deltaScatteringDensityTexture, Texture &transmittanceTexture,
  Texture &deltaRayleighScatteringTexture, Texture &deltaMieScatteringTexture,
  Texture &deltaMultipleScatteringTexture, Texture &deltaIrradianceTexture)
  -> SkyModel::ComputeCMD {

  scatteringDensitySet = scatteringDensitySetDef.createSet(*descriptorPool);
  scatteringDensitySetDef.atmosphere(_atmosphereUBO->buffer());
  scatteringDensitySetDef.scattering_order(ScatterOrderUBO->buffer());
  scatteringDensitySetDef.transmittance(transmittanceTexture);
  scatteringDensitySetDef.delta_rayleigh(deltaRayleighScatteringTexture);
  scatteringDensitySetDef.delta_mie(deltaMieScatteringTexture);
  scatteringDensitySetDef.multpli_scattering(deltaMultipleScatteringTexture);
  scatteringDensitySetDef.irradiance(deltaIrradianceTexture);
  scatteringDensitySetDef.scattering_density(deltaScatteringDensityTexture);
  scatteringDensitySetDef.update(scatteringDensitySet);

  SpecializationMaker sp{};
  auto spInfo = sp.entry<uint32_t>(8).entry<uint32_t>(8).entry<uint32_t>(1).create();

  ComputePipelineMaker pipelineMaker{device.getDevice()};
  pipelineMaker.shader(
    computeScatteringDensity_comp, __ArraySize__(computeScatteringDensity_comp), &spInfo);
  scatteringDensityPipeline =
    pipelineMaker.createUnique(nullptr, *scatteringDensityLayoutDef.pipelineLayout);

  return [&](vk::CommandBuffer cb) {
    auto dim = deltaScatteringDensityTexture.extent();
    deltaScatteringDensityTexture.transitToLayout(
      cb, layout::eGeneral, access::eShaderWrite, stage::eComputeShader);

    cb.bindPipeline(bindpoint::eCompute, *scatteringDensityPipeline);
    cb.bindDescriptorSets(
      bindpoint::eCompute, *scatteringDensityLayoutDef.pipelineLayout,
      scatteringDensityLayoutDef.set.set(), scatteringDensitySet, nullptr);
    cb.dispatch(dim.width / 8, dim.height / 8, dim.depth / 1);

    cb.pipelineBarrier(
      stage::eComputeShader, stage::eComputeShader, {}, nullptr, nullptr,
      deltaScatteringDensityTexture.barrier(
        layout::eShaderReadOnlyOptimal, access::eShaderRead, stage::eComputeShader));
  };
}
}

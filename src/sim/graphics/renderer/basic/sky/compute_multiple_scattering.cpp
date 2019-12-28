#include "sky_model.h"
#include "sim/graphics/base/pipeline/render_pass.h"
#include "sim/graphics/base/pipeline/pipeline.h"
#include "sim/graphics/base/pipeline/descriptors.h"
#include "sim/graphics/compiledShaders/basic/sky/computeMultipleScattering_comp.h"

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

auto SkyModel::createMultipleScattering(
  Texture &deltaMultipleScatteringTexture, Texture &scatteringTexture,
  Texture &transmittanceTexture, Texture &deltaScatteringDensityTexture)
  -> SkyModel::ComputeCMD {

  multipleScatteringSet = multipleScatteringSetDef.createSet(*descriptorPool);
  multipleScatteringSetDef.atmosphere(_atmosphereUBO->buffer());
  multipleScatteringSetDef.luminance_from_radiance(LFRUBO->buffer());
  multipleScatteringSetDef.transmittance(transmittanceTexture);
  multipleScatteringSetDef.scattering_density(deltaScatteringDensityTexture);
  multipleScatteringSetDef.delta_multiple_scattering(deltaMultipleScatteringTexture);
  multipleScatteringSetDef.scattering(scatteringTexture);
  multipleScatteringSetDef.update(multipleScatteringSet);

  SpecializationMaker sp{};
  auto spInfo = sp.entry<uint32_t>(8).entry<uint32_t>(8).entry<uint32_t>(1).create();
  ComputePipelineMaker pipelineMaker{device.getDevice()};
  pipelineMaker.shader(
    computeMultipleScattering_comp, __ArraySize__(computeMultipleScattering_comp),
    &spInfo);
  multipleScatteringPipeline =
    pipelineMaker.createUnique(nullptr, *multipleScatteringLayoutDef.pipelineLayout);

  return [&](vk::CommandBuffer cb) {
    auto dim = scatteringTexture.extent();

    deltaMultipleScatteringTexture.transitToLayout(
      cb, layout ::eGeneral, access::eShaderWrite, stage::eComputeShader);
    scatteringTexture.transitToLayout(
      cb, layout ::eGeneral, access::eShaderWrite, stage::eComputeShader);

    cb.bindPipeline(bindpoint::eCompute, *multipleScatteringPipeline);
    cb.bindDescriptorSets(
      bindpoint::eCompute, *multipleScatteringLayoutDef.pipelineLayout,
      multipleScatteringLayoutDef.set.set(), multipleScatteringSet, nullptr);
    cb.dispatch(dim.width / 8, dim.height / 8, dim.depth / 1);

    cb.pipelineBarrier(
      stage::eComputeShader, stage::eComputeShader, {}, nullptr, nullptr,
      {deltaMultipleScatteringTexture.barrier(
         layout::eShaderReadOnlyOptimal, access::eShaderRead, stage::eComputeShader),
       scatteringTexture.barrier(
         layout::eShaderReadOnlyOptimal, access::eShaderRead, stage::eComputeShader)});
  };
}
}

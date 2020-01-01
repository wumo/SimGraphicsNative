#include "sky_model.h"
#include "sim/graphics/base/pipeline/render_pass.h"
#include "sim/graphics/base/pipeline/pipeline.h"
#include "sim/graphics/compiledShaders/basic/sky/multiple_scattering_comp.h"

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

void SkyModel::createMultipleScatteringSets() {
  multipleScatteringSet = multipleScatteringSetDef.createSet(*descriptorPool);
  multipleScatteringSetDef.atmosphere(_atmosphereUBO->buffer());
  multipleScatteringSetDef.transmittance(*transmittance_texture_);
  multipleScatteringSetDef.scattering_density(*delta_scattering_density_texture);
  multipleScatteringSetDef.delta_multiple_scattering(*delta_rayleigh_scattering_texture);
  multipleScatteringSetDef.scattering(*scattering_texture_);
  multipleScatteringSetDef.update(multipleScatteringSet);

  SpecializationMaker sp{};
  auto spInfo = sp.entry<uint32_t>(8).entry<uint32_t>(8).entry<uint32_t>(1).create();
  ComputePipelineMaker pipelineMaker{device.getDevice()};
  pipelineMaker.shader(
    multiple_scattering_comp, __ArraySize__(multiple_scattering_comp), &spInfo);
  multipleScatteringPipeline =
    pipelineMaker.createUnique(nullptr, *multipleScatteringLayoutDef.pipelineLayout);
}

void SkyModel::recordMultipleScatteringCMD(
  vk::CommandBuffer cb, const glm::mat4 &luminance_from_radiance) {

  auto dim = scattering_texture_->extent();

  delta_rayleigh_scattering_texture->transitToLayout(
    cb, layout ::eGeneral, access::eShaderWrite, stage::eComputeShader);
  scattering_texture_->transitToLayout(
    cb, layout ::eGeneral, access::eShaderWrite, stage::eComputeShader);

  cb.bindPipeline(bindpoint::eCompute, *multipleScatteringPipeline);
  cb.bindDescriptorSets(
    bindpoint::eCompute, *multipleScatteringLayoutDef.pipelineLayout,
    multipleScatteringLayoutDef.set.set(), multipleScatteringSet, nullptr);
  cb.pushConstants<glm::mat4>(
    *multipleScatteringLayoutDef.pipelineLayout, shader::eCompute, 0,
    luminance_from_radiance);
  cb.dispatch(dim.width / 8, dim.height / 8, dim.depth / 1);

  cb.pipelineBarrier(
    stage::eComputeShader, stage::eComputeShader, {}, nullptr, nullptr,
    {delta_rayleigh_scattering_texture->barrier(
       layout::eShaderReadOnlyOptimal, access::eShaderRead, stage::eComputeShader),
     scattering_texture_->barrier(
       layout::eShaderReadOnlyOptimal, access::eShaderRead, stage::eComputeShader)});
}
}

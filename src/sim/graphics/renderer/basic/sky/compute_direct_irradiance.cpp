#include "sky_model.h"
#include "constants.h"
#include "sim/graphics/base/pipeline/render_pass.h"
#include "sim/graphics/base/pipeline/pipeline.h"
#include "sim/graphics/base/pipeline/descriptors.h"
#include "sim/graphics/compiledShaders/basic/sky/quad_vert.h"
#include "sim/graphics/compiledShaders/basic/sky/computeDirectIrradiance_frag.h"

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
  __uniform__(atmosphere, shader::eFragment);
  __sampler__(transmittance, shader::eFragment);
};
}

void SkyModel::computeDirectIrradiance(
  bool blend, Texture &deltaIrradianceTexture, Texture &irradianceTexture,
  Texture &transmittanceTexture) {
  auto dim = irradianceTexture.extent();

  device.executeImmediately([&](vk::CommandBuffer cb) {
    ImageBase::setLayout(
      cb, deltaIrradianceTexture.image(), layout::eUndefined,
      layout::eColorAttachmentOptimal, {}, {});
    ImageBase::setLayout(
      cb, irradianceTexture.image(), layout::eUndefined, layout::eColorAttachmentOptimal,
      {}, {});
  });

  // Renderpass
  RenderPassMaker maker;
  auto deltaIrradiance = maker.attachment(deltaIrradianceTexture.format())
                           .samples(vk::SampleCountFlagBits::e1)
                           .loadOp(loadOp::eDontCare)
                           .storeOp(storeOp::eStore)
                           .stencilLoadOp(loadOp::eDontCare)
                           .stencilStoreOp(storeOp::eDontCare)
                           .initialLayout(layout::eColorAttachmentOptimal)
                           .finalLayout(layout::eShaderReadOnlyOptimal)
                           .index();
  auto irradiance =
    maker.attachment(irradianceTexture.format()).loadOp(loadOp::eLoad).index();

  auto subpass =
    maker.subpass(bindpoint::eGraphics).color(deltaIrradiance).color(irradiance).index();

  maker.dependency(VK_SUBPASS_EXTERNAL, subpass)
    .srcStageMask(stage::eBottomOfPipe)
    .dstStageMask(stage::eColorAttachmentOutput)
    .srcAccessMask(access::eMemoryRead)
    .dstAccessMask(access::eColorAttachmentRead | access::eColorAttachmentWrite)
    .dependencyFlags(vk::DependencyFlagBits::eByRegion);
  maker.dependency(subpass, VK_SUBPASS_EXTERNAL)
    .srcStageMask(stage::eColorAttachmentOutput)
    .dstStageMask(stage::eBottomOfPipe)
    .srcAccessMask(access::eColorAttachmentRead | access::eColorAttachmentWrite)
    .dstAccessMask(access::eMemoryRead)
    .dependencyFlags(vk::DependencyFlagBits::eByRegion);
  vk::UniqueRenderPass renderPass = maker.createUnique(device.getDevice());

  std::array<vk::ImageView, 2> attachments{deltaIrradianceTexture.imageView(),
                                           irradianceTexture.imageView()};
  vk::FramebufferCreateInfo info{
    {}, *renderPass, attachments.size(), attachments.data(), dim.width, dim.height, 1};
  auto framebuffer = device.getDevice().createFramebufferUnique(info);

  // Descriptor Pool
  std::vector<vk::DescriptorPoolSize> poolSizes{
    {vk::DescriptorType::eUniformBuffer, 1},
    {vk::DescriptorType::eCombinedImageSampler, 1},
  };
  vk::DescriptorPoolCreateInfo descriptorPoolInfo{
    {}, 1, (uint32_t)poolSizes.size(), poolSizes.data()};
  auto descriptorPool = device.getDevice().createDescriptorPoolUnique(descriptorPoolInfo);
  // Descriptor sets
  ComputeDirectIrradianceDescriptorDef setDef;
  setDef.init(device.getDevice());
  auto set = setDef.createSet(*descriptorPool);
  setDef.atmosphere(uboBuffer->buffer());
  setDef.transmittance(transmittanceTexture);
  setDef.update(set);

  auto pipelineLayout = PipelineLayoutMaker()
                          .descriptorSetLayout(*setDef.descriptorSetLayout)
                          .createUnique(device.getDevice());

  vk::UniquePipeline pipeline;
  { // Pipeline
    GraphicsPipelineMaker pipelineMaker{device.getDevice(), dim.width, dim.height};
    pipelineMaker.subpass(subpass)
      .topology(vk::PrimitiveTopology::eTriangleList)
      .polygonMode(vk::PolygonMode::eFill)
      .cullMode(vk::CullModeFlagBits::eNone)
      .frontFace(vk::FrontFace::eCounterClockwise)
      .depthTestEnable(false)
      .depthWriteEnable(false)
      .depthCompareOp(vk::CompareOp::eLessOrEqual)
      .dynamicState(vk::DynamicState::eViewport)
      .dynamicState(vk::DynamicState::eScissor)
      .rasterizationSamples(vk::SampleCountFlagBits::e1);

    pipelineMaker.blendColorAttachment(false);

    if(blend) {
      using colorComp = vk::ColorComponentFlagBits;
      pipelineMaker.blendColorAttachment(true)
        .srcColorBlendFactor(vk::BlendFactor::eOne)
        .dstColorBlendFactor(vk::BlendFactor::eOne)
        .colorBlendOp(vk::BlendOp::eAdd)
        .srcAlphaBlendFactor(vk::BlendFactor::eOne)
        .dstAlphaBlendFactor(vk::BlendFactor::eOne)
        .alphaBlendOp(vk::BlendOp::eAdd)
        .colorWriteMask(colorComp::eR | colorComp::eG | colorComp::eB | colorComp::eA);
    } else {
      pipelineMaker.blendColorAttachment(false);
    }

    pipelineMaker.shader(shader::eVertex, quad_vert, __ArraySize__(quad_vert))
      .shader(
        shader::eFragment, computeDirectIrradiance_frag,
        __ArraySize__(computeDirectIrradiance_frag));
    pipeline = pipelineMaker.createUnique(nullptr, *pipelineLayout, *renderPass);
  }

  // Render
  std::array<vk::ClearValue, 2> clearValues{
    vk::ClearColorValue{std::array{0.0f, 0.0f, 0.0f, 1.0f}},
    vk::ClearColorValue{std::array{0.0f, 0.0f, 0.0f, 1.0f}}};
  vk::RenderPassBeginInfo renderPassBeginInfo{
    *renderPass, *framebuffer, vk::Rect2D{{0, 0}, {dim.width, dim.height}},
    uint32_t(clearValues.size()), clearValues.data()};
  vk::Viewport viewport{0, 0, float(dim.width), float(dim.height), 0.0f, 1.0f};
  vk::Rect2D scissor{{0, 0}, {dim.width, dim.height}};
  device.executeImmediately([&](vk::CommandBuffer cb) {
    cb.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
    cb.setViewport(0, viewport);
    cb.setScissor(0, scissor);
    cb.bindPipeline(bindpoint::eGraphics, *pipeline);
    cb.bindDescriptorSets(bindpoint::eGraphics, *pipelineLayout, 0, set, nullptr);
    cb.draw(3, 1, 0, 0);
    cb.endRenderPass();
  });
}
}

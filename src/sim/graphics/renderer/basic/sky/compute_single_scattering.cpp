#include "sky_model.h"
#include "constants.h"
#include "sim/graphics/base/pipeline/render_pass.h"
#include "sim/graphics/base/pipeline/pipeline.h"
#include "sim/graphics/base/pipeline/descriptors.h"
#include "sim/graphics/compiledShaders/basic/sky/quad_vert.h"
#include "sim/graphics/compiledShaders/basic/sky/computeSingleScattering_frag.h"
#include "sim/graphics/compiledShaders/basic/sky/sky_geom.h"

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
  __uniform__(atmosphere, shader::eFragment);
  __uniform__(layer, shader::eGeometry | shader ::eFragment);
  __uniform__(luminance_from_radiance, shader::eFragment);
  __sampler__(transmittance, shader::eFragment);
};
}

void SkyModel::computeSingleScattering(
  bool blend, Texture &deltaRayleighScatteringTexture, Texture &deltaMieScatteringTexture,
  Texture &scatteringTexture, Texture &transmittanceTexture) {
  auto dim = scatteringTexture.extent();

  device.executeImmediately([&](vk::CommandBuffer cb) {
    ImageBase::setLayout(
      cb, deltaRayleighScatteringTexture.image(), layout::eUndefined,
      layout::eColorAttachmentOptimal, {}, {});
    ImageBase::setLayout(
      cb, deltaMieScatteringTexture.image(), layout::eUndefined,
      layout::eColorAttachmentOptimal, {}, {});
    ImageBase::setLayout(
      cb, scatteringTexture.image(), layout::eUndefined, layout::eColorAttachmentOptimal,
      {}, {});
  });

  // Renderpass
  RenderPassMaker maker;
  auto deltaRayleigh = maker.attachment(deltaRayleighScatteringTexture.format())
                         .samples(vk::SampleCountFlagBits::e1)
                         .loadOp(loadOp::eLoad)
                         .storeOp(storeOp::eStore)
                         .stencilLoadOp(loadOp::eDontCare)
                         .stencilStoreOp(storeOp::eDontCare)
                         .initialLayout(layout::eColorAttachmentOptimal)
                         .finalLayout(layout::eColorAttachmentOptimal)
                         .index();
  auto deltaMie = maker.attachment(deltaMieScatteringTexture.format()).index();
  auto scattering = maker.attachment(scatteringTexture.format()).index();

  auto subpass = maker.subpass(bindpoint::eGraphics)
                   .color(deltaRayleigh)
                   .color(deltaMie)
                   .color(scattering)
                   .index();
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

  // Descriptor Pool
  std::vector<vk::DescriptorPoolSize> poolSizes{
    {vk::DescriptorType::eUniformBuffer, 3},
    {vk::DescriptorType::eCombinedImageSampler, 1},
  };
  vk::DescriptorPoolCreateInfo descriptorPoolInfo{
    {}, 1, (uint32_t)poolSizes.size(), poolSizes.data()};
  auto descriptorPool = device.getDevice().createDescriptorPoolUnique(descriptorPoolInfo);
  // Descriptor sets
  ComputeSingleScatteringDescriptorDef setDef;
  setDef.init(device.getDevice());
  auto set = setDef.createSet(*descriptorPool);
  setDef.atmosphere(uboBuffer->buffer());
  setDef.layer(layerBuffer->buffer());
  setDef.luminance_from_radiance(LFRUniformBuffer->buffer());
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

    pipelineMaker
      .shader(shader::eVertex, quad_vert, __ArraySize__(quad_vert))
      //      .shader(shader::eGeometry, sky_geom, __ArraySize__(sky_geom))
      .shader(
        shader::eFragment, computeSingleScattering_frag,
        __ArraySize__(computeSingleScattering_frag));
    pipeline = pipelineMaker.createUnique(nullptr, *pipelineLayout, *renderPass);
  }

  std::array<vk::ImageView, 3> attachments{deltaRayleighScatteringTexture.imageView(),
                                           deltaMieScatteringTexture.imageView(),
                                           scatteringTexture.imageView()};
  vk::FramebufferCreateInfo info{
    {}, *renderPass, attachments.size(), attachments.data(), dim.width, dim.height, 1};
  auto framebuffer = device.getDevice().createFramebufferUnique(info);

  // Render
  std::array<vk::ClearValue, 3> clearValues{
    vk::ClearColorValue{std::array{0.0f, 0.0f, 0.0f, 1.0f}}};
  vk::RenderPassBeginInfo renderPassBeginInfo{
    *renderPass, *framebuffer, vk::Rect2D{{0, 0}, {dim.width, dim.height}},
    uint32_t(clearValues.size()), clearValues.data()};
  vk::Viewport viewport{0, 0, float(dim.width), float(dim.height), 0.0f, 1.0f};
  vk::Rect2D scissor{{0, 0}, {dim.width, dim.height}};

  layerBuffer->updateSingle(1);
  device.executeImmediately([&](vk::CommandBuffer cb) {
    cb.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
    cb.setViewport(0, viewport);
    cb.setScissor(0, scissor);
    cb.bindPipeline(bindpoint::eGraphics, *pipeline);
    cb.bindDescriptorSets(bindpoint::eGraphics, *pipelineLayout, 0, set, nullptr);
    cb.draw(3, 1, 0, 0);
    cb.endRenderPass();
  });

  //  for(uint32_t layer = 0; layer < dim.depth; ++layer) {
  //    layerBuffer->updateSingle(layer);
  //    this->device.executeImmediately([&](vk::CommandBuffer cb) {
  //      cb.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
  //      cb.setViewport(0, viewport);
  //      cb.setScissor(0, scissor);
  //      cb.bindPipeline(bindpoint::eGraphics, *pipeline);
  //      cb.bindDescriptorSets(bindpoint::eGraphics, *pipelineLayout, 0, set, nullptr);
  //      cb.draw(3, 1, 0, 0);
  //      cb.endRenderPass();
  //    });
  //  }
}
}

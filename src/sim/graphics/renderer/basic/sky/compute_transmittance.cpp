#include "sky_model.h"
#include "constants.h"
#include "sim/graphics/base/pipeline/render_pass.h"
#include "sim/graphics/base/pipeline/pipeline.h"
#include "sim/graphics/base/pipeline/descriptors.h"
#include "sim/graphics/compiledShaders/basic/quad_vert.h"
#include "sim/graphics/compiledShaders/basic/sky/computeTransmittance_frag.h"

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
struct ComputeTransmittanceDescriptorDef: DescriptorSetDef {};
}

void SkyModel::computeTransmittance(Texture2D &transmittanceTexture) {
  auto dim = transmittanceTexture.extent();

  // Renderpass
  RenderPassMaker maker;
  auto color = maker.attachment(transmittanceTexture.format())
                 .samples(vk::SampleCountFlagBits::e1)
                 .loadOp(loadOp::eClear)
                 .storeOp(storeOp::eStore)
                 .stencilLoadOp(loadOp::eDontCare)
                 .stencilStoreOp(storeOp::eDontCare)
                 .initialLayout(layout::eUndefined)
                 .finalLayout(layout::eShaderReadOnlyOptimal)
                 .index();
  auto subpass = maker.subpass(bindpoint::eGraphics).color(color).index();
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
  vk::UniqueRenderPass renderPass = maker.createUnique(this->device.getDevice());

  vk::FramebufferCreateInfo info{
    {}, *renderPass, 1, &transmittanceTexture.imageView(), dim.width, dim.height, 1};
  auto framebuffer = this->device.getDevice().createFramebufferUnique(info);

  // Descriptor sets
  ComputeTransmittanceDescriptorDef setDef;
  setDef.init(this->device.getDevice());

  auto pipelineLayout = PipelineLayoutMaker()
                          .descriptorSetLayout(*setDef.descriptorSetLayout)
                          .createUnique(this->device.getDevice());

  vk::UniquePipeline pipeline;
  { // Pipeline
    GraphicsPipelineMaker pipelineMaker{this->device.getDevice(), dim.width, dim.height};
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
      .rasterizationSamples(vk::SampleCountFlagBits::e1)
      .blendColorAttachment(false);

    pipelineMaker.shader(shader::eVertex, quad_vert, __ArraySize__(quad_vert))
      .shader(
        shader::eFragment, computeTransmittance_frag,
        __ArraySize__(computeTransmittance_frag));
    pipeline = pipelineMaker.createUnique(nullptr, *pipelineLayout, *renderPass);
  }

  // Render
  std::array<vk::ClearValue, 1> clearValues{
    vk::ClearColorValue{std::array{0.0f, 0.0f, 0.0f, 1.0f}}};
  vk::RenderPassBeginInfo renderPassBeginInfo{
    *renderPass, *framebuffer, vk::Rect2D{{0, 0}, {dim.width, dim.height}},
    uint32_t(clearValues.size()), clearValues.data()};
  vk::Viewport viewport{0, 0, float(dim.width), float(dim.height), 0.0f, 1.0f};
  vk::Rect2D scissor{{0, 0}, {dim.width, dim.height}};
  this->device.executeImmediately([&](vk::CommandBuffer cb) {
    cb.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
    cb.setViewport(0, viewport);
    cb.setScissor(0, scissor);
    cb.bindPipeline(bindpoint::eGraphics, *pipeline);
    cb.draw(3, 1, 0, 0);
    cb.endRenderPass();
  });
}
}

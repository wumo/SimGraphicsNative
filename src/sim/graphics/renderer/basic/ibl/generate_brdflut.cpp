#include "envmap_generator.h"
#include "../basic_model_manager.h"
#include "sim/graphics/base/pipeline/render_pass.h"
#include "sim/graphics/base/pipeline/pipeline.h"
#include "sim/graphics/compiledShaders/envmap/genbrdflut_vert.h"
#include "sim/graphics/compiledShaders/envmap/genbrdflut_frag.h"

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

struct BRDFLUTDescriptorDef: DescriptorSetDef {};

uPtr<TextureImage2D> EnvMapGenerator::generateBRDFLUT() {
  auto tStart = std::chrono::high_resolution_clock::now();

  auto brdfLUT =
    u<TextureImage2D>(device, 512, 512, false, vk::Format::eR16G16Sfloat, true);
  {
    SamplerMaker maker{};
    maker.addressModeU(vk::SamplerAddressMode::eClampToEdge)
      .addressModeV(vk::SamplerAddressMode::eClampToEdge)
      .addressModeW(vk::SamplerAddressMode::eClampToEdge)
      .maxLod(1.f)
      .maxAnisotropy(1.f)
      .borderColor(vk::BorderColor::eFloatOpaqueWhite);
    brdfLUT->setSampler(maker.createUnique(device.getDevice()));
  }

  vk::Format format = brdfLUT->getInfo().format;
  uint32_t dim = brdfLUT->getInfo().extent.width;

  // Renderpass
  RenderPassMaker maker;
  auto color = maker.attachment(format)
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
  vk::UniqueRenderPass renderPass = maker.createUnique(device.getDevice());

  vk::FramebufferCreateInfo info{{}, *renderPass, 1, &brdfLUT->imageView(), dim, dim, 1};
  auto framebuffer = device.getDevice().createFramebufferUnique(info);

  // Descriptor sets
  BRDFLUTDescriptorDef brdfLUTDef;
  brdfLUTDef.init(device.getDevice());

  auto pipelineLayout = PipelineLayoutMaker()
                          .descriptorSetLayout(*brdfLUTDef.descriptorSetLayout)
                          .createUnique(device.getDevice());
  vk::UniquePipeline pipeline;
  { // Pipeline
    GraphicsPipelineMaker pipelineMaker{device.getDevice(), dim, dim};
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

    pipelineMaker.shader(shader::eVertex, genbrdflut_vert, __ArraySize__(genbrdflut_vert))
      .shader(shader::eFragment, genbrdflut_frag, __ArraySize__(genbrdflut_frag));
    pipeline = pipelineMaker.createUnique(
      device.getDevice(), nullptr, *pipelineLayout, *renderPass);
  }

  // Render
  std::array<vk::ClearValue, 1> clearValues{
    vk::ClearColorValue{std::array{0.0f, 0.0f, 0.0f, 1.0f}}};
  vk::RenderPassBeginInfo renderPassBeginInfo{
    *renderPass, *framebuffer, vk::Rect2D{{0, 0}, {dim, dim}},
    uint32_t(clearValues.size()), clearValues.data()};
  vk::Viewport viewport{0, 0, float(dim), float(dim), 0.0f, 1.0f};
  vk::Rect2D scissor{{0, 0}, {dim, dim}};
  device.executeImmediately([&](vk::CommandBuffer cb) {
    cb.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
    cb.setViewport(0, viewport);
    cb.setScissor(0, scissor);
    cb.bindPipeline(bindpoint::eGraphics, *pipeline);
    cb.draw(3, 1, 0, 0);
    cb.endRenderPass();
  });

  auto tEnd = std::chrono::high_resolution_clock::now();
  auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
  debugLog("Generating BRDF LUT took ", tDiff, " ms");
  return std::move(brdfLUT);
}
}

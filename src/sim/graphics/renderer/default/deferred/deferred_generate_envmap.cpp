#include "deferred_model.h"
#include "sim/graphics/base/pipeline/render_pass.h"
#include "sim/graphics/base/pipeline/pipeline.h"
#include "sim/graphics/compiledShaders/envmap/filtercube_vert.h"
#include "sim/graphics/compiledShaders/envmap/irradiancecube_frag.h"
#include "sim/graphics/compiledShaders/envmap/prefilterenvmap_frag.h"

namespace sim::graphics::renderer::defaultRenderer {
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

struct EnvMapDescriptorSet: DescriptorSetDef {
  __sampler__(texture, shader::eFragment);
};

void DeferredModel::generateEnvMap() {
  if(IBL.genEnvMap) {
    generateEnvMap(EnvMap::Irradiance);
    generateEnvMap(EnvMap::PreFiltered);
    IBL.genEnvMap = false;
  }
}

void DeferredModel::generateEnvMap(DeferredModel::EnvMap envMap) {
  std::string envMapName;
  switch(envMap) {
    case EnvMap::Irradiance: envMapName = "Irradiance EnvMap"; break;
    case EnvMap::PreFiltered: envMapName = "PreFiltered EnvMap"; break;
  }

  auto tStart = std::chrono::high_resolution_clock::now();

  uint32_t tex;
  switch(envMap) {
    case EnvMap::Irradiance: tex = IBL.irradiance; break;
    case EnvMap::PreFiltered: tex = IBL.prefiltered; break;
  }

  TextureImageCube &cubeMap = Image.cubeTextures[tex];
  vk::Format format = cubeMap.getInfo().format;
  uint32_t dim = cubeMap.getInfo().extent.width;

  auto mipLevels = cubeMap.getInfo().mipLevels;
  // Renderpass
  RenderPassMaker maker;

  auto color = maker.attachment(format)
                 .samples(vk::SampleCountFlagBits::e1)
                 .loadOp(loadOp::eClear)
                 .storeOp(storeOp::eStore)
                 .stencilLoadOp(loadOp::eDontCare)
                 .stencilStoreOp(storeOp::eDontCare)
                 .initialLayout(layout::eUndefined)
                 .finalLayout(layout::eColorAttachmentOptimal)
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

  TextureImage2D offscreen{device, dim, dim, false, format, true};

  vk::FramebufferCreateInfo info{{}, *renderPass, 1, &offscreen.imageView(), dim, dim, 1};
  auto framebuffer = device.getDevice().createFramebufferUnique(info);

  // Descriptor Pool
  std::vector<vk::DescriptorPoolSize> poolSizes{
    {vk::DescriptorType::eCombinedImageSampler, 1},
  };
  vk::DescriptorPoolCreateInfo descriptorPoolInfo{
    {}, 2, (uint32_t)poolSizes.size(), poolSizes.data()};
  auto descriptorPool = device.getDevice().createDescriptorPoolUnique(descriptorPoolInfo);
  // Descriptor sets
  EnvMapDescriptorSet envMapDef;
  envMapDef.init(device.getDevice());
  auto envMapSet = envMapDef.createSet(*descriptorPool);
  auto &envCube = Image.cubeTextures[IBL.environment];
  envMapDef.texture(envCube);
  envMapDef.update(envMapSet);

  // Pipeline layout
  struct PushBlockIrradiance {
    glm::mat4 mvp{};
    float deltaPhi = (2.0f * pi<float>()) / 180.0f;
    float deltaTheta = (0.5f * pi<float>()) / 64.0f;
  } pushBlockIrradiance;

  struct PushBlockPrefilterEnv {
    glm::mat4 mvp{};
    float roughness{};
    uint32_t numSamples = 32u;
  } pushBlockPrefilterEnv;

  vk::PushConstantRange pushConstantRange{shader::eVertex | shader::eFragment};

  switch(envMap) {
    case EnvMap::Irradiance: pushConstantRange.size = sizeof(PushBlockIrradiance); break;
    case EnvMap::PreFiltered:
      pushConstantRange.size = sizeof(PushBlockPrefilterEnv);
      break;
  };
  auto pipelineLayout = PipelineLayoutMaker()
                          .descriptorSetLayout(*envMapDef.descriptorSetLayout)
                          .pushConstantRange(pushConstantRange)
                          .createUnique(device.getDevice());
  vk::UniquePipeline pipeline;
  { // Pipeline
    GraphicsPipelineMaker pipelineMaker{device.getDevice(), dim, dim};
    pipelineMaker.subpass(subpass)
      .vertexBinding(0, Vertex::stride())
      .vertexAttribute(Vertex::attributes(0, 0)[0])
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

    pipelineMaker.shader(
      shader::eVertex, filtercube_vert, __ArraySize__(filtercube_vert));
    switch(envMap) {
      case EnvMap::Irradiance:
        pipelineMaker.shader(
          shader::eFragment, irradiancecube_frag, __ArraySize__(irradiancecube_frag));
        break;
      case EnvMap::PreFiltered:
        pipelineMaker.shader(
          shader::eFragment, prefilterenvmap_frag, __ArraySize__(prefilterenvmap_frag));
        break;
    }
    pipeline = pipelineMaker.createUnique(
      device.getDevice(), nullptr, *pipelineLayout, *renderPass);
  }

  // Render cubemap
  std::array<vk::ClearValue, 1> clearValues{
    vk::ClearColorValue{std::array{0.0f, 0.0f, 0.0f, 0.0f}}};
  vk::RenderPassBeginInfo renderPassBeginInfo{
    *renderPass, *framebuffer, vk::Rect2D{{0, 0}, {dim, dim}},
    uint32_t(clearValues.size()), clearValues.data()};

  std::vector<glm::mat4> matrices = {
    glm::rotate(
      glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
      glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
    glm::rotate(
      glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
      glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
    glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
    glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
    glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
    glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
  };

  vk::Viewport viewport{0, 0, float(dim), float(dim), 0.0f, 1.0f};
  vk::Rect2D scissor{{0, 0}, {dim, dim}};

  device.executeImmediately([&](vk::CommandBuffer cb) {
    offscreen.setLayout(cb, layout::eColorAttachmentOptimal);
    cubeMap.setLayout(cb, layout::eTransferDstOptimal);
  });
  for(auto m = 0u; m < mipLevels; m++)
    for(auto f = 0u; f < 6; f++) {
      device.executeImmediately([&](vk::CommandBuffer cb) {
        // Render scene from cube face's point of view
        cb.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
        viewport.width = static_cast<float>(dim * std::pow(0.5f, m));
        viewport.height = static_cast<float>(dim * std::pow(0.5f, m));
        cb.setViewport(0, viewport);
        cb.setScissor(0, scissor);
        // Pass parameters for current pass using a push constant block
        switch(envMap) {
          case EnvMap::Irradiance:
            pushBlockIrradiance.mvp =
              glm::perspective(pi<float>() / 2, 1.0f, 0.1f, 512.0f) * matrices[f];
            cb.pushConstants<PushBlockIrradiance>(
              *pipelineLayout, shader::eVertex | shader::eFragment, 0,
              pushBlockIrradiance);
            break;
          case EnvMap::PreFiltered:
            pushBlockPrefilterEnv.mvp =
              glm::perspective(pi<float>() / 2, 1.0f, 0.1f, 512.0f) * matrices[f];
            pushBlockPrefilterEnv.roughness = (float)m / (float)(mipLevels - 1);
            cb.pushConstants<PushBlockPrefilterEnv>(
              *pipelineLayout, shader::eVertex | shader::eFragment, 0,
              pushBlockPrefilterEnv);
            break;
        }
        cb.bindPipeline(bindpoint::eGraphics, *pipeline);
        cb.bindDescriptorSets(
          bindpoint::eGraphics, *pipelineLayout, 0, envMapSet, nullptr);
        const vk::DeviceSize zeroOffset{0};
        cb.bindVertexBuffers(0, Buffer.vertices->buffer(), zeroOffset);
        cb.bindIndexBuffer(Buffer.indices->buffer(), zeroOffset, vk::IndexType::eUint32);
        cb.drawIndexedIndirect(
          drawSkyboxCMD->buffer(), zeroOffset, 1, sizeof(vk::DrawIndexedIndirectCommand));
        cb.endRenderPass();

        offscreen.setLayout(
          cb, layout::eColorAttachmentOptimal, layout::eTransferSrcOptimal);

        vk::ImageCopy region;
        region.srcSubresource = {aspect::eColor, 0, 0, 1};
        region.dstSubresource = {aspect::eColor, m, f, 1};
        region.extent.width = static_cast<uint32_t>(viewport.width);
        region.extent.height = static_cast<uint32_t>(viewport.height);
        region.extent.depth = 1;
        cb.copyImage(
          offscreen.image(), layout::eTransferSrcOptimal, cubeMap.image(),
          layout::eTransferDstOptimal, region);

        offscreen.setLayout(
          cb, layout::eTransferSrcOptimal, layout::eColorAttachmentOptimal);
      });
    }
  device.executeImmediately([&](vk::CommandBuffer cb) {
    cubeMap.setLayout(cb, layout::eTransferDstOptimal, layout::eShaderReadOnlyOptimal);
  });

  auto tEnd = std::chrono::high_resolution_clock::now();
  auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
  debugLog(
    "Generating ", envMapName, "cube map with ", mipLevels, " mip levels took ", tDiff,
    " ms");
}
}
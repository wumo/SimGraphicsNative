#include "shadow_manager.h"
#include "sim/graphics/renderer/basic/basic_scene_manager.h"
#include "sim/graphics/base/pipeline/render_pass.h"
#include "sim/graphics/compiledShaders/quad_vert.h"

namespace sim::graphics::renderer::basic {
using loadOp = vk::AttachmentLoadOp;
using storeOp = vk::AttachmentStoreOp;
using layout = vk::ImageLayout;
using bindpoint = vk::PipelineBindPoint;
using stage = vk::PipelineStageFlagBits;
using access = vk::AccessFlagBits;
using flag = vk::DescriptorBindingFlagBitsEXT;
using shader = vk::ShaderStageFlagBits;
using aspect = vk::ImageAspectFlagBits;
using imageUsage = vk::ImageUsageFlagBits;

ShadowManager::ShadowManager(BasicSceneManager &mm)
  : mm(mm), device{mm.device()}, debugMarker(mm.debugMarker()) {
  shadowSetDef.init(device.getDevice());
}

void ShadowManager::init() {
  lightAttribs.shadowAttribs.iNumCascades = 4;
  lightAttribs.shadowAttribs.fixedDepthBias = 0.0025f;
  lightAttribs.shadowAttribs.fixedFilterSize = 5;
  lightAttribs.shadowAttribs.filterWorldSize = 0.1f;
  lightAttribs.direction = {-0.522699475f, -0.481321275f, -0.703671455f, 1};
  lightAttribs.intensity = {1, 0.8f, 0.5f, 1};
  lightAttribs.ambientLight = {0.125f, 0.125f, 0.125f, 1};

  lightAttribsUBO_ = u<HostUniformBuffer>(device.allocator(), lightAttribs);
  createShadowMap();
}

void ShadowManager::createShadowMap() {
  auto &shadowAttribs = lightAttribs.shadowAttribs;

  if(ShadowSettings.resolution >= 2048) shadowAttribs.fixedDepthBias = 0.0025f;
  else if(ShadowSettings.resolution >= 1024)
    shadowAttribs.fixedDepthBias = 0.005f;
  else
    shadowAttribs.fixedDepthBias = 0.0075f;

  {
    shadowMap = u<Texture>(
      device.allocator(),
      vk::ImageCreateInfo{
        {},
        vk::ImageType::e2D,
        ShadowSettings.format,
        {uint32_t(ShadowSettings.resolution), uint32_t(ShadowSettings.resolution), 1U},
        1,
        uint32_t(shadowAttribs.iNumCascades),
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        imageUsage::eSampled | imageUsage::eTransferSrc | imageUsage::eTransferDst |
          imageUsage::eDepthStencilAttachment,
      },
      VMA_MEMORY_USAGE_GPU_ONLY, vk::MemoryPropertyFlags{}, "shadowMap");
    shadowMap->setImageView(
      device.getDevice(), vk::ImageViewType::e2DArray, vk::ImageAspectFlagBits::eDepth);
    SamplerMaker maker{};
    maker.minFilter(vk::Filter::eLinear)
      .magFilter(vk::Filter::eLinear)
      .mipmapMode(vk::SamplerMipmapMode::eLinear)
      .compareEnable(true)
      .compareOp(vk::CompareOp::eLess);

    shadowMap->setSampler(maker.createUnique(device.getDevice()));

    shadowMapDSVs.clear();
    shadowMapDSVs.reserve(shadowAttribs.iNumCascades);
    for(uint32_t arraySlice = 0; arraySlice < shadowAttribs.iNumCascades; ++arraySlice)
      shadowMapDSVs.emplace_back(shadowMap->createImageView(
        device.getDevice(), vk::ImageViewType::e2D, vk::ImageAspectFlagBits::eDepth,
        arraySlice, 1));
  }

  if(
    ShadowSettings.shadowMode == ShadowMode::VSM ||
    ShadowSettings.shadowMode == ShadowMode::EVSM2 ||
    ShadowSettings.shadowMode == ShadowMode::EVSM4) {
    vk::Format format;
    if(ShadowSettings.shadowMode == ShadowMode::VSM)
      format = ShadowSettings.Is32BitFilterableFmt ? vk::Format::eR32G32Sfloat :
                                                     vk::Format::eR16G16Unorm;
    else if(ShadowSettings.shadowMode == ShadowMode::EVSM2)
      format = ShadowSettings.Is32BitFilterableFmt ? vk::Format::eR32G32Sfloat :
                                                     vk::Format::eR16G16Sfloat;
    else
      format = ShadowSettings.Is32BitFilterableFmt ? vk::Format::eR32G32B32A32Sfloat :
                                                     vk::Format::eR16G16B16A16Sfloat;

    {
      filterableShadowMap = u<Texture>(
        device.allocator(),
        vk::ImageCreateInfo{
          {},
          vk::ImageType::e2D,
          format,
          {uint32_t(ShadowSettings.resolution), uint32_t(ShadowSettings.resolution), 1U},
          1,
          uint32_t(shadowAttribs.iNumCascades),
          vk::SampleCountFlagBits::e1,
          vk::ImageTiling::eOptimal,
          imageUsage::eSampled | imageUsage::eTransferSrc | imageUsage::eTransferDst |
            imageUsage::eColorAttachment,
        },
        VMA_MEMORY_USAGE_GPU_ONLY, vk::MemoryPropertyFlags{}, "filterableShadowMap");

      filterableShadowMap->setImageView(
        device.getDevice(), vk::ImageViewType::e2DArray, vk::ImageAspectFlagBits::eColor);

      SamplerMaker maker{};
      maker.anisotropyEnable(true).maxAnisotropy(shadowAttribs.maxAnisotropy);
      filterableShadowMap->setSampler(maker.createUnique(device.getDevice()));

      filterableShadowMapRTVs.clear();
      filterableShadowMapRTVs.reserve(shadowAttribs.iNumCascades);
      for(uint32_t arraySlice = 0; arraySlice < shadowAttribs.iNumCascades; ++arraySlice)
        filterableShadowMapRTVs.emplace_back(filterableShadowMap->createImageView(
          device.getDevice(), vk::ImageViewType::e2D, vk::ImageAspectFlagBits::eColor,
          arraySlice, 1));
    }
    {
      intermediateMap = u<Texture>(
        device.allocator(),
        vk::ImageCreateInfo{
          {},
          vk::ImageType::e2D,
          format,
          {uint32_t(ShadowSettings.resolution), uint32_t(ShadowSettings.resolution), 1U},
          1,
          1,
          vk::SampleCountFlagBits::e1,
          vk::ImageTiling::eOptimal,
          imageUsage::eSampled | imageUsage::eTransferSrc | imageUsage::eTransferDst |
            imageUsage::eColorAttachment,
        },
        VMA_MEMORY_USAGE_GPU_ONLY, vk::MemoryPropertyFlags{}, "filterableShadowMap");

      intermediateMap->setImageView(
        device.getDevice(), vk::ImageViewType::e2D, vk::ImageAspectFlagBits::eColor);

      SamplerMaker maker{};
      filterableShadowMap->setSampler(maker.createUnique(device.getDevice()));
    }
    createConversionTechs(format);
  }
}

void ShadowManager::createConversionTechs(vk::Format format) {
  vk::DeviceSize size = 64;
  conversionAttribsUBO = u<HostUniformBuffer>(device.allocator(), size);

  shadowLayoutDef.init(device.getDevice());
  for(auto mode = value(ShadowMode::VSM); mode <= value(ShadowMode::EVSM4); ++mode) {
    auto &tech = conversionTech[mode];

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

    { // Pipeline
      auto dim = uint32_t(ShadowSettings.resolution);
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

      pipelineMaker.shader(shader::eVertex, quad_vert, __ArraySize__(quad_vert));

      //      tech.pipeline = pipelineMaker.createUnique(nullptr, *pipelineLayout, *renderPass);
    }
  }
}
}
#include "shadow_manager.h"
#include "../basic_scene_manager.h"

namespace sim::graphics::renderer::basic {
using imageUsage = vk::ImageUsageFlagBits;

ShadowManager::ShadowManager(BasicSceneManager &mm)
  : mm(mm), device{mm.device()}, debugMarker(mm.debugMarker()) {}

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
  if(ShadowSettings.Resolution >= 2048)
    lightAttribs.shadowAttribs.fixedDepthBias = 0.0025f;
  else if(ShadowSettings.Resolution >= 1024)
    lightAttribs.shadowAttribs.fixedDepthBias = 0.005f;
  else
    lightAttribs.shadowAttribs.fixedDepthBias = 0.0075f;

  auto texture = u<Texture>(
    device.allocator(),
    vk::ImageCreateInfo{
      {},
      vk::ImageType::e2D,
      ShadowSettings.Format,
      {uint32_t(ShadowSettings.Resolution), uint32_t(ShadowSettings.Resolution), 1U},
      1,
      uint32_t(lightAttribs.shadowAttribs.iNumCascades),
      vk::SampleCountFlagBits::e1,
      vk::ImageTiling::eOptimal,
      imageUsage::eSampled | imageUsage::eTransferSrc |
        imageUsage::eDepthStencilAttachment,
    },
    VMA_MEMORY_USAGE_GPU_ONLY, vk::MemoryPropertyFlags{}, "shadowMap");
  texture->setImageView(
    device.getDevice(), vk::ImageViewType::e2DArray, vk::ImageAspectFlagBits::eDepth);
  SamplerMaker maker{};
  maker.minFilter(vk::Filter::eLinear)
    .magFilter(vk::Filter::eLinear)
    .mipmapMode(vk::SamplerMipmapMode::eLinear)
    .compareEnable(true)
    .compareOp(vk::CompareOp::eLess);

  texture->setSampler(maker.createUnique(device.getDevice()));

  shadowMapDSVs.clear();
  shadowMapDSVs.reserve(lightAttribs.shadowAttribs.iNumCascades);
  for(uint32_t arraySlice = 0; arraySlice < lightAttribs.shadowAttribs.iNumCascades;
      ++arraySlice)
    shadowMapDSVs.emplace_back(texture->createImageView(
      device.getDevice(), vk::ImageViewType::e2D, vk::ImageAspectFlagBits::eDepth,
      arraySlice, 1));
}
}
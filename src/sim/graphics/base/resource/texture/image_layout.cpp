#include "../images.h"
#include "../buffers.h"
#include "sim/util/syntactic_sugar.h"

namespace sim::graphics {

using layout = vk::ImageLayout;
using access = vk::AccessFlagBits;
using aspect = vk::ImageAspectFlagBits;
using stage = vk::PipelineStageFlagBits;
using imageUsage = vk::ImageUsageFlagBits;
using buffer = vk::BufferUsageFlagBits;

void Texture::setLayout(
  const vk::CommandBuffer &cb, vk::Image image, vk::ImageLayout oldLayout,
  vk::ImageLayout newLayout, vk::AccessFlags srcAccessMask, vk::AccessFlags dstAccessMask,
  vk::PipelineStageFlagBits srcStageMask, vk::PipelineStageFlagBits dstStageMask) {
  vk::ImageAspectFlags aspectMask;
  if(newLayout == layout::eDepthStencilAttachmentOptimal) {
    aspectMask = aspect::eDepth | aspect::eStencil;
  } else
    aspectMask = aspect::eColor;

  vk::ImageMemoryBarrier imageMemoryBarrier{
    srcAccessMask, dstAccessMask,           oldLayout,
    newLayout,     VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
    image,         {aspectMask, 0, 1, 0, 1}};
  cb.pipelineBarrier(
    srcStageMask, dstStageMask, {}, nullptr, nullptr, imageMemoryBarrier);
}

void Texture::setLayout(
  const vk::CommandBuffer &cb, vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
  vk::AccessFlags srcAccessMask, vk::AccessFlags dstAccessMask, uint32_t baseMipLevel,
  uint32_t levelCount, stage srcStageMask, stage dstStageMask, bool record) {
  if(oldLayout == newLayout) return;
  if(record) currentLayout = newLayout;

  vk::ImageAspectFlags aspectMask;
  if(newLayout == layout::eDepthStencilAttachmentOptimal) {
    aspectMask = aspect::eDepth | aspect::eStencil;
  } else
    aspectMask = aspect::eColor;

  vk::ImageMemoryBarrier imageMemoryBarrier{
    srcAccessMask,
    dstAccessMask,
    oldLayout,
    newLayout,
    VK_QUEUE_FAMILY_IGNORED,
    VK_QUEUE_FAMILY_IGNORED,
    vmaImage->image,
    {aspectMask, baseMipLevel, levelCount, 0, _info.arrayLayers}};
  cb.pipelineBarrier(
    srcStageMask, dstStageMask, {}, nullptr, nullptr, imageMemoryBarrier);
}

void Texture::setLayout(
  const vk::CommandBuffer &cb, vk::ImageLayout newLayout, vk::AccessFlags srcAccessMask,
  vk::AccessFlags dstAccessMask, stage srcStageMask, stage dstStageMask) {
  setLayout(
    cb, currentLayout, newLayout, srcAccessMask, dstAccessMask, 0, _info.mipLevels,
    srcStageMask, dstStageMask, true);
}

void Texture::setLayout(
  const vk::CommandBuffer &cb, vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
  vk::AccessFlags srcAccessMask, vk::AccessFlags dstAccessMask,
  vk::PipelineStageFlagBits srcStageMask, vk::PipelineStageFlagBits dstStageMask) {
  setLayout(
    cb, currentLayout, newLayout, srcAccessMask, dstAccessMask, 0, _info.mipLevels,
    srcStageMask, dstStageMask, false);
}

void Texture::setLayout(
  const vk::CommandBuffer &cb, vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
  vk::PipelineStageFlagBits srcStageMask, vk::PipelineStageFlagBits dstStageMask) {
  setLayout(
    cb, oldLayout, newLayout, guessSrcAccess(oldLayout), guessDstAccess(newLayout), 0,
    _info.mipLevels, srcStageMask, dstStageMask, true);
}

void Texture::setLayoutByGuess(
  const vk::CommandBuffer &cb, vk::ImageLayout newLayout, stage srcStageMask,
  stage dstStageMask) {
  setLayout(cb, currentLayout, newLayout, srcStageMask, dstStageMask);
}

void Texture::setLevelLayout(
  const vk::CommandBuffer &cb, uint32_t baseMipLevel, vk::ImageLayout oldLayout,
  vk::ImageLayout newLayout, vk::PipelineStageFlagBits srcStageMask,
  vk::PipelineStageFlagBits dstStageMask) {
  setLayout(
    cb, oldLayout, newLayout, guessSrcAccess(oldLayout), guessDstAccess(newLayout),
    baseMipLevel, 1, srcStageMask, dstStageMask, true);
}

vk::AccessFlags Texture::guessSrcAccess(const vk::ImageLayout &oldLayout) {
  vk::AccessFlags srcAccessMask;
  switch(oldLayout) {
    case layout::eUndefined: break;
    case layout::eGeneral: srcAccessMask = access::eTransferWrite; break;
    case layout::eColorAttachmentOptimal:
      srcAccessMask = access::eColorAttachmentWrite;
      break;
    case layout::eDepthStencilAttachmentOptimal:
      srcAccessMask = access::eDepthStencilAttachmentWrite;
      break;
    case layout::eDepthStencilReadOnlyOptimal:
      srcAccessMask = access::eDepthStencilAttachmentRead;
      break;
    case layout::eShaderReadOnlyOptimal: srcAccessMask = access::eShaderRead; break;
    case layout::eTransferSrcOptimal: srcAccessMask = access::eTransferRead; break;
    case layout::eTransferDstOptimal: srcAccessMask = access::eTransferWrite; break;
    case layout::ePreinitialized:
      srcAccessMask = access::eTransferWrite | access::eHostWrite;
      break;
    case layout::ePresentSrcKHR: srcAccessMask = access::eMemoryRead; break;
    case layout::eDepthReadOnlyStencilAttachmentOptimal: [[fallthrough]];
    case layout::eDepthAttachmentStencilReadOnlyOptimal: [[fallthrough]];
    case layout::eSharedPresentKHR: [[fallthrough]];
    case layout::eShadingRateOptimalNV: [[fallthrough]];
    case vk::ImageLayout::eFragmentDensityMapOptimalEXT: [[fallthrough]];
    case vk::ImageLayout::eDepthAttachmentOptimalKHR: [[fallthrough]];
    case vk::ImageLayout::eDepthReadOnlyOptimalKHR: [[fallthrough]];
    case vk::ImageLayout::eStencilAttachmentOptimalKHR: [[fallthrough]];
    case vk::ImageLayout::eStencilReadOnlyOptimalKHR:
      error("not supported old layout! ");
      break;
  }
  return srcAccessMask;
}

vk::AccessFlags Texture::guessDstAccess(const vk::ImageLayout &newLayout) {
  vk::AccessFlags dstAccessMask;

  switch(newLayout) {
    case layout::eUndefined: break;
    case layout::eGeneral: dstAccessMask = access::eTransferWrite; break;
    case layout::eColorAttachmentOptimal:
      dstAccessMask = access::eColorAttachmentWrite;
      break;
    case layout::eDepthStencilAttachmentOptimal:
      dstAccessMask = access::eDepthStencilAttachmentWrite;
      break;
    case layout::eDepthStencilReadOnlyOptimal:
      dstAccessMask = access::eDepthStencilAttachmentRead;
      break;
    case layout::eShaderReadOnlyOptimal: dstAccessMask = access::eShaderRead; break;
    case layout::eTransferSrcOptimal: dstAccessMask = access::eTransferRead; break;
    case layout::eTransferDstOptimal: dstAccessMask = access::eTransferWrite; break;
    case layout::ePreinitialized: dstAccessMask = access::eTransferWrite; break;
    case layout::ePresentSrcKHR: dstAccessMask = access::eMemoryRead; break;
    case layout::eDepthReadOnlyStencilAttachmentOptimal: [[fallthrough]];
    case layout::eDepthAttachmentStencilReadOnlyOptimal: [[fallthrough]];
    case layout::eSharedPresentKHR: [[fallthrough]];
    case layout::eShadingRateOptimalNV: [[fallthrough]];
    case vk::ImageLayout::eFragmentDensityMapOptimalEXT:
    case vk::ImageLayout::eDepthAttachmentOptimalKHR: [[fallthrough]];
    case vk::ImageLayout::eDepthReadOnlyOptimalKHR: [[fallthrough]];
    case vk::ImageLayout::eStencilAttachmentOptimalKHR: [[fallthrough]];
    case vk::ImageLayout::eStencilReadOnlyOptimalKHR:
      error("not supported new layout!");
      break;
  }
  return dstAccessMask;
}

void Texture::transitToLayout(
  const vk::CommandBuffer &cb, vk::ImageLayout newLayout,
  const vk::AccessFlags &dstAccess, vk::PipelineStageFlagBits dstStage) {
  setLayout(cb, currentLayout, newLayout, srcAccess, dstAccess, srcStage, dstStage);
  currentLayout = newLayout;
  srcAccess = dstAccess;
  srcStage = dstStage;
}

vk::ImageMemoryBarrier Texture::barrier(
  vk::ImageLayout newLayout, const vk::AccessFlags &dstAccessMask,
  vk::PipelineStageFlagBits dstStageMask, uint32_t srcQueueFamilyIndex,
  uint32_t dstQueueFamilyIndex, vk::ImageAspectFlagBits aspect) {

  vk::ImageMemoryBarrier _barrier{srcAccess, dstAccessMask,           currentLayout,
                                  newLayout, srcQueueFamilyIndex,     dstQueueFamilyIndex,
                                  image(),   subresourceRange(aspect)};
  currentLayout = newLayout;
  srcAccess = dstAccessMask;
  srcStage = dstStageMask;
  return _barrier;
}

void Texture::setCurrentLayout(vk::ImageLayout oldLayout) { currentLayout = oldLayout; }
void Texture::setSrcAccess(const vk::AccessFlags &srcAccess) {
  Texture::srcAccess = srcAccess;
}
void Texture::setSrcStage(vk::PipelineStageFlagBits srcStage) {
  Texture::srcStage = srcStage;
}
void Texture::setCurrentState(
  vk::ImageLayout newLayout, vk::AccessFlags dstAccess,
  vk::PipelineStageFlagBits dstStage) {
  currentLayout = newLayout;
  srcAccess = dstAccess;
  srcStage = dstStage;
}
}
#include <utility>

#include "images.h"
#include "buffers.h"
#include "syntactic_sugar.h"

#include <stb_image.h>
#include <gli/gli.hpp>

namespace sim::graphics {

uint32_t calcMipLevels(uint32_t dim) {
  return static_cast<uint32_t>(std::floor(std::log2(dim))) + 1;
}

using layout = vk::ImageLayout;
using access = vk::AccessFlagBits;
using aspect = vk::ImageAspectFlagBits;
using stage = vk::PipelineStageFlagBits;
using image = vk::ImageUsageFlagBits;
using buffer = vk::BufferUsageFlagBits;

void ImageBase::setLayout(
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

void ImageBase::copy(
  const vk::CommandBuffer &cb, ImageBase &srcImage, vk::Image &dstImage) {
  //  srcImage.setLayout(cb, layout::eTransferSrcOptimal);
  //  ImageBase::setLayout(cb, dstImage, layout::eUndefined,
  //                       layout::eTransferDstOptimal, {}, access::eTransferWrite);
  vk::ImageCopy region;
  region.srcSubresource = {aspect::eColor, 0, 0, 1};
  region.dstSubresource = {aspect::eColor, 0, 0, 1};
  region.extent = srcImage.extent();
  cb.copyImage(
    srcImage.image(), layout::eTransferSrcOptimal, dstImage, layout::eTransferDstOptimal,
    region);
}

void ImageBase::resolve(
  const vk::CommandBuffer &cb, ImageBase &srcImage, vk::Image &dstImage) {
  vk::ImageResolve region;
  region.srcSubresource = {aspect::eColor, 0, 0, 1};
  region.dstSubresource = {aspect::eColor, 0, 0, 1};
  region.extent = srcImage.extent();
  cb.resolveImage(
    srcImage.image(), layout::eTransferSrcOptimal, dstImage, layout::eTransferDstOptimal,
    region);
}

ImageBase::ImageBase(
  const VmaAllocator &allocator, vk::ImageCreateInfo info, VmaMemoryUsage memoryUsage,
  const vk::MemoryPropertyFlags &flags)
  : ImageBase{
      allocator, std::move(info), {{}, memoryUsage, VkMemoryPropertyFlags(flags)}} {}

ImageBase::ImageBase(
  const VmaAllocator &allocator, vk::ImageCreateInfo info,
  VmaAllocationCreateInfo allocInfo) {
  currentLayout = info.initialLayout;
  _info = info;
  vmaImage = UniquePtr(new VmaImage{allocator}, [](VmaImage *ptr) {
    debugLog("deallocate image:", ptr->image);
    vmaDestroyImage(ptr->allocator, ptr->image, ptr->allocation);
    delete ptr;
  });
  auto result = vmaCreateImage(
    allocator, reinterpret_cast<VkImageCreateInfo *>(&info), &allocInfo,
    reinterpret_cast<VkImage *>(&(vmaImage->image)), &vmaImage->allocation,
    &allocationInfo);
  errorIf(result != VK_SUCCESS, "failed to allocate buffer!");
  debugLog(
    "allocate image:", vmaImage->image, "[", allocationInfo.deviceMemory, "+",
    allocationInfo.offset, "]");
  VkMemoryPropertyFlags memFlags;
  vmaGetMemoryTypeProperties(vmaImage->allocator, allocationInfo.memoryType, &memFlags);
  if(
    (memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
    (memFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
    mappable = true;
  }
}

void ImageBase::createImageView(
  const vk::Device &device, vk::ImageViewType viewType, vk::ImageAspectFlags aspectMask) {
  vk::ImageViewCreateInfo viewCreateInfo{
    {},
    vmaImage->image,
    viewType,
    _info.format,
    {vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB,
     vk::ComponentSwizzle::eA},
    {aspectMask, 0, _info.mipLevels, 0, _info.arrayLayers}};
  _imageView = device.createImageViewUnique(viewCreateInfo);
}

void ImageBase::setSampler(vk::UniqueSampler &&sampler) { _sampler = std::move(sampler); }

void ImageBase::clear(const vk::CommandBuffer &cb, const std::array<float, 4> &color) {
  setLayout(cb, layout::eTransferDstOptimal);
  vk::ClearColorValue clearColorValue{color};
  vk::ImageSubresourceRange range{aspect::eColor, 0, 1, 0, 1};
  cb.clearColorImage(
    vmaImage->image, layout::eTransferDstOptimal, clearColorValue, range);
}

void ImageBase::copy(const vk::CommandBuffer &cb, ImageBase &srcImage) {
  srcImage.setLayout(cb, layout::eTransferSrcOptimal);
  setLayout(cb, layout::eTransferDstOptimal);
  for(uint32_t mipLevel = 0; mipLevel < info().mipLevels; ++mipLevel) {
    vk::ImageCopy region;
    region.srcSubresource = {aspect::eColor, mipLevel, 0, 1};
    region.dstSubresource = {aspect::eColor, mipLevel, 0, 1};
    region.extent = _info.extent;
    cb.copyImage(
      srcImage.image(), layout::eTransferSrcOptimal, vmaImage->image,
      layout::eTransferDstOptimal, region);
  }
}

void ImageBase::copy(
  const vk::CommandBuffer &cb, vk::Buffer buffer, uint32_t mipLevel, uint32_t arrayLayer,
  uint32_t width, uint32_t height, uint32_t depth, uint32_t offset) {
  setLayout(cb, layout::eTransferDstOptimal, stage::eTopOfPipe, stage::eTransfer);
  vk::BufferImageCopy region;
  region.bufferOffset = offset;
  region.imageSubresource = {aspect::eColor, mipLevel, arrayLayer, 1};
  region.imageExtent = vk::Extent3D{width, height, depth};
  cb.copyBufferToImage(buffer, vmaImage->image, layout::eTransferDstOptimal, region);
}

void ImageBase::upload(
  Device &device, std::vector<unsigned char> &bytes, bool transitToShaderRead) {
  upload(device, bytes.data(), bytes.size(), transitToShaderRead);
}

void ImageBase::upload(
  Device &device, const unsigned char *bytes, size_t sizeInBytes, bool transitToShaderRead) {
  HostBuffer stagingBuffer{vmaImage->allocator, buffer::eTransferSrc,
                           static_cast<vk::DeviceSize>(sizeInBytes)};
  stagingBuffer.updateRaw(reinterpret_cast<const void *>(bytes), sizeInBytes);
  device.executeImmediately([&](vk::CommandBuffer cb) {
    auto buf = stagingBuffer.buffer();
    copy(cb, buf, 0, 0, _info.extent.width, _info.extent.height, _info.extent.depth, 0);
    if(transitToShaderRead) setLayout(cb, layout::eShaderReadOnlyOptimal);
  });
}

void ImageBase::setLayout(
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

void ImageBase::setLayout(
  const vk::CommandBuffer &cb, vk::ImageLayout newLayout, vk::AccessFlags srcAccessMask,
  vk::AccessFlags dstAccessMask, stage srcStageMask, stage dstStageMask) {
  setLayout(
    cb, currentLayout, newLayout, srcAccessMask, dstAccessMask, 0, _info.mipLevels,
    srcStageMask, dstStageMask, true);
}

void ImageBase::setLayout(
  const vk::CommandBuffer &cb, vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
  vk::PipelineStageFlagBits srcStageMask, vk::PipelineStageFlagBits dstStageMask) {
  setLayout(
    cb, oldLayout, newLayout, guessSrcAccess(oldLayout), guessDstAccess(newLayout), 0,
    _info.mipLevels, srcStageMask, dstStageMask, true);
}

void ImageBase::setLayout(
  const vk::CommandBuffer &cb, vk::ImageLayout newLayout, stage srcStageMask,
  stage dstStageMask) {
  setLayout(cb, currentLayout, newLayout, srcStageMask, dstStageMask);
}

void ImageBase::setLevelLayout(
  const vk::CommandBuffer &cb, uint32_t baseMipLevel, vk::ImageLayout oldLayout,
  vk::ImageLayout newLayout, vk::PipelineStageFlagBits srcStageMask,
  vk::PipelineStageFlagBits dstStageMask) {
  setLayout(
    cb, oldLayout, newLayout, guessSrcAccess(oldLayout), guessDstAccess(newLayout),
    baseMipLevel, 1, srcStageMask, dstStageMask, true);
}

vk::AccessFlags ImageBase::guessSrcAccess(const vk::ImageLayout &oldLayout) {
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

vk::AccessFlags ImageBase::guessDstAccess(const vk::ImageLayout &newLayout) {
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

void ImageBase::setCurrentLayout(vk::ImageLayout oldLayout) { currentLayout = oldLayout; }

const vk::Format &ImageBase::format() const { return _info.format; }
const vk::Extent3D &ImageBase::extent() const { return _info.extent; }
const vk::ImageCreateInfo &ImageBase::info() const { return _info; }
const vk::Image &ImageBase::image() const { return vmaImage->image; }
const vk::ImageView &ImageBase::imageView() const { return *_imageView; }
const vk::Sampler &ImageBase::sampler() const { return *_sampler; }
const vk::ImageCreateInfo &ImageBase::getInfo() const { return _info; }

DepthStencilImage::DepthStencilImage(
  Device &device, uint32_t width, uint32_t height, vk::Format format,
  vk::SampleCountFlagBits sampleCount)
  : ImageBase{device.allocator(), info(width, height, format, sampleCount),
              VMA_MEMORY_USAGE_GPU_ONLY} {
  createImageView(device.getDevice(), vk::ImageViewType::e2D, aspect::eDepth);
}

vk::ImageCreateInfo DepthStencilImage::info(
  uint32_t width, uint32_t height, vk::Format format,
  vk::SampleCountFlagBits sampleCount) {
  return {{},
          vk::ImageType::e2D,
          format,
          {width, height, 1U},
          1,
          1,
          sampleCount,
          vk::ImageTiling::eOptimal,
          image::eDepthStencilAttachment | image::eTransferSrc | image::eSampled};
}

TransientDepthStencilImage::TransientDepthStencilImage(
  Device &device, uint32_t width, uint32_t height, vk::Format format,
  vk::SampleCountFlagBits sampleCount)
  : ImageBase{device.allocator(), info(width, height, format, sampleCount),
              VMA_MEMORY_USAGE_GPU_ONLY} {
  createImageView(device.getDevice(), vk::ImageViewType::e2D, aspect::eDepth);
}

vk::ImageCreateInfo TransientDepthStencilImage::info(
  uint32_t width, uint32_t height, vk::Format format,
  vk::SampleCountFlagBits sampleCount) {
  return {{},
          vk::ImageType::e2D,
          format,
          {width, height, 1U},
          1,
          1,
          sampleCount,
          vk::ImageTiling::eOptimal,
          image::eDepthStencilAttachment | image::eTransientAttachment};
}

ColorAttachmentImage::ColorAttachmentImage(
  Device &device, uint32_t width, uint32_t height, vk::Format format,
  vk::SampleCountFlagBits sampleCount)
  : ImageBase{device.allocator(), info(width, height, format, sampleCount),
              VMA_MEMORY_USAGE_GPU_ONLY} {
  createImageView(device.getDevice(), vk::ImageViewType::e2D, aspect::eColor);
}

vk::ImageCreateInfo ColorAttachmentImage::info(
  uint32_t width, uint32_t height, vk::Format format,
  vk::SampleCountFlagBits sampleCount) {
  return {{},
          vk::ImageType::e2D,
          format,
          {width, height, 1U},
          1,
          1,
          sampleCount,
          vk::ImageTiling::eOptimal,
          image::eColorAttachment | image::eTransferSrc | image::eSampled};
}

ColorInputAttachmentImage::ColorInputAttachmentImage(
  Device &device, uint32_t width, uint32_t height, vk::Format format,
  vk::SampleCountFlagBits sampleCount)
  : ImageBase{device.allocator(), info(width, height, format, sampleCount),
              VMA_MEMORY_USAGE_GPU_ONLY} {
  createImageView(device.getDevice(), vk::ImageViewType::e2D, aspect::eColor);
}

vk::ImageCreateInfo ColorInputAttachmentImage::info(
  uint32_t width, uint32_t height, vk::Format format,
  vk::SampleCountFlagBits sampleCount) {
  return {{},
          vk::ImageType::e2D,
          format,
          {width, height, 1U},
          1,
          1,
          sampleCount,
          vk::ImageTiling::eOptimal,
          image::eColorAttachment | image::eInputAttachment | image::eTransferSrc |
            image::eSampled};
}

TransientColorInputAttachmentImage::TransientColorInputAttachmentImage(
  Device &device, uint32_t width, uint32_t height, vk::Format format,
  vk::SampleCountFlagBits sampleCount)
  : ImageBase{device.allocator(), info(width, height, format, sampleCount),
              VMA_MEMORY_USAGE_GPU_ONLY} {
  createImageView(device.getDevice(), vk::ImageViewType::e2D, aspect::eColor);
}

vk::ImageCreateInfo TransientColorInputAttachmentImage::info(
  uint32_t width, uint32_t height, vk::Format format,
  vk::SampleCountFlagBits sampleCount) {
  return {
    {},
    vk::ImageType::e2D,
    format,
    {width, height, 1U},
    1,
    1,
    sampleCount,
    vk::ImageTiling::eOptimal,
    image::eColorAttachment | image::eInputAttachment | image::eTransientAttachment};
}

TextureImage2D TextureImage2D::loadFromFile(
  Device &device, const std::string &file, bool generateMipmap) {
  if(endWith(file, ".dds") || endWith(file, ".kmg") || endWith(file, ".ktx")) {
    const auto &t = gli::load(file.c_str());
    errorIf(t.empty(), "failed to load texture image!");
    gli::texture2d tex{t};
    auto extent = tex.extent();
    uint32_t texWidth = extent.x, texHeight = extent.y;
    auto texture = TextureImage2D{device, texWidth, texHeight, generateMipmap};
    auto dataPtr = static_cast<const unsigned char *>(tex[0].data());
    texture.upload(device, dataPtr, tex[0].size(), !generateMipmap);
    if(generateMipmap) texture._generateMipmap(device);
    return texture;
  } else {
    uint32_t texWidth, texHeight, texChannels;
    auto pixels = UniqueBytes(
      stbi_load(
        file.c_str(), reinterpret_cast<int *>(&texWidth),
        reinterpret_cast<int *>(&texHeight), reinterpret_cast<int *>(&texChannels),
        STBI_rgb_alpha),
      [](stbi_uc *ptr) { stbi_image_free(ptr); });

    errorIf(pixels == nullptr, "failed to load texture image!");
    //    errorIf(texChannels != 4, "Currently only support 4 channel image!");
    auto texture = TextureImage2D{device, texWidth, texHeight, generateMipmap};
    auto imageSize = texWidth * texHeight * 4;
    texture.upload(device, pixels.get(), imageSize, !generateMipmap);
    if(generateMipmap) texture._generateMipmap(device);
    return texture;
  }
}

TextureImage2D TextureImage2D::loadFromBytes(
  Device &device, const unsigned char *bytes, size_t size, uint32_t texWidth,
  uint32_t texHeight, bool generateMipmap) {
  auto texture = TextureImage2D{device, texWidth, texHeight, generateMipmap};
  texture.upload(device, bytes, size, !generateMipmap);
  if(generateMipmap) texture._generateMipmap(device);
  return texture;
}

TextureImage2D::TextureImage2D(
  Device &device, uint32_t width, uint32_t height, bool useMipmap, vk::Format format,
  bool attachment)
  : ImageBase{device.allocator(), info(width, height, useMipmap, format, attachment),
              VMA_MEMORY_USAGE_GPU_ONLY} {
  createImageView(device.getDevice(), vk::ImageViewType::e2D, aspect::eColor);
}

vk::ImageCreateInfo TextureImage2D::info(
  uint32_t width, uint32_t height, bool useMipmap, vk::Format format, bool attachment) {
  auto flag = image::eSampled | image::eTransferSrc | image::eTransferDst;
  if(attachment) flag |= image::eColorAttachment;
  return {{},
          vk::ImageType::e2D,
          format,
          {width, height, 1U},
          useMipmap ? calcMipLevels(std::max(width, height)) : 1,
          1,
          vk::SampleCountFlagBits::e1,
          vk::ImageTiling::eOptimal,
          flag};
}

void TextureImage2D::_generateMipmap(Device &device) {
  auto formatProp = device.getPhysicalDevice().getFormatProperties(_info.format);
  errorIf(
    !(formatProp.optimalTilingFeatures & vk::FormatFeatureFlagBits ::eBlitSrc) ||
      !(formatProp.optimalTilingFeatures & vk::FormatFeatureFlagBits ::eBlitDst),
    "blit feature src/dst not satisfied!!");
  errorIf(
    !(formatProp.optimalTilingFeatures &
      vk::FormatFeatureFlagBits ::eSampledImageFilterLinear),
    "texture image format does not support linear blitting!");
  device.executeImmediately([&](vk::CommandBuffer cb) {
    int32_t mipWidth = _info.extent.width;
    int32_t mipHeight = _info.extent.height;
    for(uint32_t level = 1; level < _info.mipLevels; level++) {
      setLevelLayout(
        cb, level - 1, layout::eTransferDstOptimal, layout::eTransferSrcOptimal);
      vk::ImageBlit blit{
        {aspect::eColor, level - 1, 0, 1},
        {vk::Offset3D{}, {mipWidth, mipHeight, 1}},
        {aspect::eColor, level, 0, 1},
        {vk::Offset3D{},
         {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1}}};
      cb.blitImage(
        vmaImage->image, layout::eTransferSrcOptimal, vmaImage->image,
        layout::eTransferDstOptimal, blit, vk::Filter::eLinear);
      if(mipWidth > 1) mipWidth /= 2;
      if(mipHeight > 1) mipHeight /= 2;
    }
    setLevelLayout(
      cb, _info.mipLevels - 1, layout::eTransferDstOptimal, layout::eTransferSrcOptimal);
    setLayout(cb, layout::eTransferSrcOptimal, layout::eShaderReadOnlyOptimal);
  });
}

TextureImageCube TextureImageCube::loadFromFile(
  Device &device, const std::string &file, bool generateMipmap) {
  errorIf(
    !(endWith(file, ".dds") || endWith(file, ".kmg") || endWith(file, ".ktx")),
    "only support dds/kmg/kts!");
  const auto &t = gli::load(file.c_str());
  errorIf(t.empty(), "failed to load texture image!");
  gli::texture_cube tex{t};
  auto extent = tex.extent();
  uint32_t texWidth = extent.x, texHeight = extent.y, miplevels = tex.levels();
  auto texture = TextureImageCube{device, texWidth, texHeight, miplevels};

  HostBuffer stagingBuffer{device.allocator(), buffer::eTransferSrc,
                           static_cast<vk::DeviceSize>(tex.size())};
  stagingBuffer.updateRaw(tex.data(), tex.size());

  device.executeImmediately([&](vk::CommandBuffer cb) {
    auto buf = stagingBuffer.buffer();

    texture.setLayout(cb, layout::eTransferDstOptimal);

    size_t offset = 0;
    for(uint32_t face = 0; face < 6; face++)
      for(uint32_t mipLevel = 0; mipLevel < miplevels; ++mipLevel) {
        auto extent = tex[face][mipLevel].extent();
        texture.copy(
          cb, buf, mipLevel, face, uint32_t(extent.x), uint32_t(extent.y), 1, offset);
        offset += tex[face][mipLevel].size();
      }
    texture.setLayout(cb, layout::eShaderReadOnlyOptimal);
  });

  return texture;
}

TextureImageCube::TextureImageCube(
  Device &device, uint32_t width, uint32_t height, uint32_t mipLevels, vk::Format format)
  : ImageBase{device.allocator(), info(width, height, mipLevels, format),
              VMA_MEMORY_USAGE_GPU_ONLY} {
  createImageView(device.getDevice(), vk::ImageViewType::eCube, aspect::eColor);
}

vk::ImageCreateInfo TextureImageCube::info(
  uint32_t width, uint32_t height, uint32_t mipLevels, vk::Format format) {
  return {vk::ImageCreateFlagBits::eCubeCompatible,
          vk::ImageType::e2D,
          format,
          {width, height, 1U},
          mipLevels,
          6,
          vk::SampleCountFlagBits::e1,
          vk::ImageTiling::eOptimal,
          image::eSampled | image::eTransferSrc | image::eTransferDst,
          vk::SharingMode::eExclusive,
          0,
          nullptr,
          vk::ImageLayout::ePreinitialized};
}

StorageImage::StorageImage(
  Device &device, uint32_t width, uint32_t height, vk::Format format,
  vk::SampleCountFlagBits sampleCount)
  : ImageBase{device.allocator(), info(width, height, format, sampleCount),
              VMA_MEMORY_USAGE_GPU_ONLY} {
  createImageView(device.getDevice(), vk::ImageViewType::e2D, aspect::eColor);
}

vk::ImageCreateInfo StorageImage::info(
  uint32_t width, uint32_t height, vk::Format format,
  vk::SampleCountFlagBits sampleCount) {
  vk::ImageCreateInfo info{{},
                           vk::ImageType::e2D,
                           format,
                           {width, height, 1},
                           1,
                           1,
                           sampleCount,
                           vk::ImageTiling::eOptimal,
                           image::eStorage | image::eTransferSrc};
  return info;
}

StorageAttachmentImage::StorageAttachmentImage(
  Device &device, uint32_t width, uint32_t height, vk::Format format,
  vk::SampleCountFlagBits sampleCount)
  : ImageBase{device.allocator(), info(width, height, format, sampleCount),
              VMA_MEMORY_USAGE_GPU_ONLY} {
  createImageView(device.getDevice(), vk::ImageViewType::e2D, aspect::eColor);
}

vk::ImageCreateInfo StorageAttachmentImage::info(
  uint32_t width, uint32_t height, vk::Format format,
  vk::SampleCountFlagBits sampleCount) {
  vk::ImageCreateInfo info{
    {},
    vk::ImageType::e2D,
    format,
    {width, height, 1},
    1,
    1,
    sampleCount,
    vk::ImageTiling::eOptimal,
    image::eColorAttachment | image::eSampled | image::eStorage | image::eTransferSrc};
  return info;
}

SamplerMaker::SamplerMaker() {
  state.info.magFilter = vk::Filter::eLinear;
  state.info.minFilter = vk::Filter::eLinear;

  state.info.addressModeU = vk::SamplerAddressMode::eRepeat;
  state.info.addressModeV = vk::SamplerAddressMode::eRepeat;
  state.info.addressModeW = vk::SamplerAddressMode::eRepeat;

  state.info.mipmapMode = vk::SamplerMipmapMode::eLinear;
  state.info.mipLodBias = 0.0f;
  state.info.minLod = 0;
  state.info.maxLod = 0;

  state.info.anisotropyEnable = VK_FALSE;
  state.info.maxAnisotropy = 0.0f;

  state.info.compareEnable = VK_FALSE;
  state.info.compareOp = vk::CompareOp::eNever;

  state.info.borderColor = vk::BorderColor{};
  state.info.unnormalizedCoordinates = VK_FALSE;
}

vk::Filter toFilter(Filter f) {
  vk::Filter _f;
  switch(f) {
    case Filter::FiltereNearest: _f = vk::Filter::eNearest; break;
    case Filter::FiltereLinear: _f = vk::Filter::eLinear; break;
  }
  return _f;
}

vk::SamplerMipmapMode toMipmapMode(SamplerMipmapMode m) {
  vk::SamplerMipmapMode _m;
  switch(m) {
    case SamplerMipmapMode::SamplerMipmapModeeNearest:
      _m = vk::SamplerMipmapMode::eNearest;
      break;
    case SamplerMipmapMode::SamplerMipmapModeeLinear:
      _m = vk::SamplerMipmapMode::eLinear;
      break;
  }
  return _m;
}

vk::SamplerAddressMode toSamplerAddressMode(SamplerAddressMode a) {
  vk::SamplerAddressMode _a;
  switch(a) {
    case SamplerAddressMode::SamplerAddressModeeRepeat:
      _a = vk::SamplerAddressMode::eRepeat;
      break;
    case SamplerAddressMode::SamplerAddressModeeMirroredRepeat:
      _a = vk::SamplerAddressMode::eMirroredRepeat;
      break;
    case SamplerAddressMode::SamplerAddressModeeClampToEdge:
      _a = vk::SamplerAddressMode::eClampToEdge;
      break;
    case SamplerAddressMode::SamplerAddressModeeClampToBorder:
      _a = vk::SamplerAddressMode::eClampToBorder;
      break;
    case SamplerAddressMode::SamplerAddressModeeMirrorClampToEdge:
      _a = vk::SamplerAddressMode::eMirrorClampToEdge;
      break;
  }
  return _a;
}

SamplerMaker::SamplerMaker(SamplerDef samplerDef) {
  state.info.magFilter = toFilter(samplerDef.magFilter);
  state.info.minFilter = toFilter(samplerDef.minFilter);
  state.info.mipmapMode = toMipmapMode(samplerDef.mipmapMode);
  state.info.addressModeU = toSamplerAddressMode(samplerDef.addressModeU);
  state.info.addressModeV = toSamplerAddressMode(samplerDef.addressModeV);
  state.info.addressModeW = toSamplerAddressMode(samplerDef.addressModeW);
  state.info.mipLodBias = samplerDef.mipLodBias;

  state.info.mipLodBias = samplerDef.mipLodBias;
  state.info.minLod = samplerDef.minLod;
  state.info.maxLod = samplerDef.maxLod;

  state.info.anisotropyEnable = samplerDef.anisotropyEnable ? VK_TRUE : VK_FALSE;
  state.info.maxAnisotropy = samplerDef.maxAnisotropy;

  state.info.compareEnable = VK_FALSE;
  state.info.compareOp = vk::CompareOp::eNever;

  state.info.borderColor = vk::BorderColor::eIntOpaqueBlack;
  state.info.unnormalizedCoordinates = samplerDef.unnormalizedCoordinates ? VK_TRUE :
                                                                            VK_FALSE;
}

SamplerMaker &SamplerMaker::flags(vk::SamplerCreateFlags value) {
  state.info.flags = value;
  return *this;
}

SamplerMaker &SamplerMaker::magFilter(vk::Filter value) {
  state.info.magFilter = value;
  return *this;
}

SamplerMaker &SamplerMaker::minFilter(vk::Filter value) {
  state.info.minFilter = value;
  return *this;
}

SamplerMaker &SamplerMaker::mipmapMode(vk::SamplerMipmapMode value) {
  state.info.mipmapMode = value;
  return *this;
}

SamplerMaker &SamplerMaker::addressModeU(vk::SamplerAddressMode value) {
  state.info.addressModeU = value;
  return *this;
}

SamplerMaker &SamplerMaker::addressModeV(vk::SamplerAddressMode value) {
  state.info.addressModeV = value;
  return *this;
}

SamplerMaker &SamplerMaker::addressModeW(vk::SamplerAddressMode value) {
  state.info.addressModeW = value;
  return *this;
}

SamplerMaker &SamplerMaker::mipLodBias(float value) {
  state.info.mipLodBias = value;
  return *this;
}

SamplerMaker &SamplerMaker::anisotropyEnable(vk::Bool32 value) {
  state.info.anisotropyEnable = value;
  return *this;
}

SamplerMaker &SamplerMaker::maxAnisotropy(float value) {
  state.info.maxAnisotropy = value;
  return *this;
}

SamplerMaker &SamplerMaker::compareEnable(vk::Bool32 value) {
  state.info.compareEnable = value;
  return *this;
}

SamplerMaker &SamplerMaker::compareOp(vk::CompareOp value) {
  state.info.compareOp = value;
  return *this;
}

SamplerMaker &SamplerMaker::minLod(float value) {
  state.info.minLod = value;
  return *this;
}

SamplerMaker &SamplerMaker::maxLod(float value) {
  state.info.maxLod = value;
  return *this;
}

SamplerMaker &SamplerMaker::borderColor(vk::BorderColor value) {
  state.info.borderColor = value;
  return *this;
}

SamplerMaker &SamplerMaker::unnormalizedCoordinates(vk::Bool32 value) {
  state.info.unnormalizedCoordinates = value;
  return *this;
}

vk::UniqueSampler SamplerMaker::createUnique(const vk::Device &device) const {
  return device.createSamplerUnique(state.info);
}

vk::Sampler SamplerMaker::create(const vk::Device &device) const {
  return device.createSampler(state.info);
}
}

#include <utility>

#include "images.h"
#include "buffers.h"
#include "sim/util/syntactic_sugar.h"

#include <stb_image.h>

namespace sim::graphics {

uint32_t calcMipLevels(uint32_t dim) {
  return static_cast<uint32_t>(std::floor(std::log2(dim))) + 1;
}

using layout = vk::ImageLayout;
using access = vk::AccessFlagBits;
using aspect = vk::ImageAspectFlagBits;
using stage = vk::PipelineStageFlagBits;
using imageUsage = vk::ImageUsageFlagBits;
using buffer = vk::BufferUsageFlagBits;

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
  const vk::MemoryPropertyFlags &flags, std::string name)
  : ImageBase{allocator,
              std::move(info),
              {{}, memoryUsage, VkMemoryPropertyFlags(flags)},
              name} {}

ImageBase::ImageBase(
  const VmaAllocator &allocator, vk::ImageCreateInfo info,
  VmaAllocationCreateInfo allocInfo, std::string name) {
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
    "allocate image: ", name, " ", vmaImage->image, "[", allocationInfo.deviceMemory, "+",
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

void ImageBase::createImageView(const vk::Device &device, vk::ImageViewCreateInfo info) {
  _imageView = device.createImageViewUnique(info);
}

void ImageBase::setSampler(vk::UniqueSampler &&sampler) { _sampler = std::move(sampler); }

void ImageBase::clear(const vk::CommandBuffer &cb, const std::array<float, 4> &color) {
  setLayout(cb, layout::eTransferDstOptimal);
  vk::ClearColorValue clearColorValue{color};
  vk::ImageSubresourceRange range{aspect::eColor, 0, 1, 0, 1};
  cb.clearColorImage(
    vmaImage->image, layout::eTransferDstOptimal, clearColorValue, range);
}

const vk::Format &ImageBase::format() const { return _info.format; }
const vk::Extent3D &ImageBase::extent() const { return _info.extent; }
const vk::ImageCreateInfo &ImageBase::info() const { return _info; }
const vk::Image &ImageBase::image() const { return vmaImage->image; }
const vk::ImageView &ImageBase::imageView() const { return *_imageView; }
vk::ImageSubresourceRange ImageBase::subresourceRange(
  const vk::ImageAspectFlags &aspectMask) const {
  return {aspectMask, 0, _info.mipLevels, 0, _info.arrayLayers};
}
const vk::Sampler &ImageBase::sampler() const { return *_sampler; }
const vk::ImageCreateInfo &ImageBase::getInfo() const { return _info; }

Texture::Texture(Device &device, const vk::ImageCreateInfo &info, std::string name)
  : ImageBase{device.allocator(), info, VMA_MEMORY_USAGE_GPU_ONLY, {}, name} {}
}

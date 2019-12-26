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
}
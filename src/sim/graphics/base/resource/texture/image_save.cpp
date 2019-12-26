#include "../images.h"
#include "../buffers.h"
#include "sim/util/syntactic_sugar.h"

#include <stb_image.h>

namespace sim::graphics {
using layout = vk::ImageLayout;
using stage = vk::PipelineStageFlagBits;
using aspect = vk::ImageAspectFlagBits;
using buffer = vk::BufferUsageFlagBits;

void ImageBase::saveToFile(const vk::CommandBuffer &cb, std::string path) {
  vk::DeviceSize sizeInBytes = allocationInfo.size;
  ReadBackBuffer readBackBuffer{vmaImage->allocator,
                                buffer::eTransferSrc | buffer::eTransferDst, sizeInBytes};
  setLayout(cb, layout::eTransferDstOptimal, stage::eAllCommands, stage::eTransfer);

  vk::BufferImageCopy region;
  for(uint32_t mipLevel = 0; mipLevel < info().mipLevels; ++mipLevel) {
    region.bufferOffset = 0;
    region.imageSubresource = {aspect::eColor, mipLevel, 0, 1};
    region.imageExtent = _info.extent;
    cb.copyImageToBuffer(
      image(), layout::eTransferSrcOptimal, readBackBuffer.buffer(), region);
  }
}
}
#include "../images.h"
#include "../buffers.h"
#include "sim/util/syntactic_sugar.h"

#include <stb_image.h>

namespace sim::graphics {
using layout = vk::ImageLayout;
using buffer = vk::BufferUsageFlagBits;

void Texture::upload(
  Device &device, std::vector<unsigned char> &bytes, bool transitToShaderRead) {
  upload(device, bytes.data(), bytes.size(), transitToShaderRead);
}

void Texture::upload(
  Device &device, const unsigned char *bytes, size_t sizeInBytes,
  bool transitToShaderRead) {
  HostBuffer stagingBuffer{vmaImage->allocator, buffer::eTransferSrc,
                           static_cast<vk::DeviceSize>(sizeInBytes)};
  stagingBuffer.updateRaw(reinterpret_cast<const void *>(bytes), sizeInBytes);
  device.graphicsImmediately([&](vk::CommandBuffer cb) {
    auto buf = stagingBuffer.buffer();
    copy(cb, buf, 0, 0, _info.extent.width, _info.extent.height, _info.extent.depth, 0);
    if(transitToShaderRead) setLayoutByGuess(cb, layout::eShaderReadOnlyOptimal);
  });
}
}
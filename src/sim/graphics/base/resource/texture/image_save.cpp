#include "../images.h"
#include "../buffers.h"
#include "sim/util/syntactic_sugar.h"
#include "sim/graphics/base/device.h"
#include <stb_image.h>
#include <fstream>

namespace sim::graphics {
using layout = vk::ImageLayout;
using stage = vk::PipelineStageFlagBits;
using aspect = vk::ImageAspectFlagBits;
using buffer = vk::BufferUsageFlagBits;

void Texture::saveToFile(
  Device &device, const vk::CommandPool cmdPool, const vk::Queue queue,
  std::string path) {

  auto width = _info.extent.width, height = _info.extent.height;

  auto saveImg = image::linearHostUnique(device, width, height, _info.format);

  Device::executeImmediately(
    device.getDevice(), cmdPool, queue, [&](vk::CommandBuffer cb) {
      setLayout(cb, layout::eTransferSrcOptimal);
      saveImg->setLayout(cb, layout::eTransferDstOptimal);

      vk::ImageCopy region{
        {aspect::eColor, 0, 0, 1}, {}, {aspect::eColor, 0, 0, 1}, {}, {width, height, 1}};

      cb.copyImage(
        image(), layout::eTransferSrcOptimal, saveImg->image(),
        layout::eTransferDstOptimal, region);
    });

  auto subResourceLayout = saveImg->subresourceLayout(
    device.getDevice(), {vk::ImageAspectFlagBits ::eColor, 0, 0});

  auto data = saveImg->ptr<char>();
  data += subResourceLayout.offset;

  std::ofstream file(path + "0.ppm", std::ios::out | std::ios::binary);

  // ppm header
  file << "P6\n" << width << "\n" << height << "\n" << 255 << "\n";

  // ppm binary pixel data
  for(uint32_t y = 0; y < height; y++) {
    unsigned int *row = (unsigned int *)data;
    for(uint32_t x = 0; x < width; x++) {
      file.write((char *)row, 3);
      row++;
    }
    data += subResourceLayout.rowPitch;
  }
  file.close();
}

vk::SubresourceLayout Texture::subresourceLayout(
  const vk::Device &device, const vk::ImageSubresource &subresource) {
  vk::SubresourceLayout layout{};
  device.getImageSubresourceLayout(image(), &subresource, &layout);
  return layout;
}
}
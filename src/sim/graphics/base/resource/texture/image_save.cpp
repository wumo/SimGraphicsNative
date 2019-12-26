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
  Device &device, const vk::CommandPool cmdPool, const vk::Queue queue, std::string path,
  float scale) {

  auto width = _info.extent.width, height = _info.extent.height,
       depth = _info.extent.depth;

  auto depthW = 1u, depthH = depth;

  for(int l = 1; l <= std::sqrt(depth); ++l) {
    if(depth % l == 0 && l > depthW) {
      depthW = l;
      depthH = depth / depthW;
    }
  }

  std::ofstream file(path + ".ppm", std::ios::out | std::ios::binary);

  // ppm header
  file << "P6\n"
       << width * depthW + depthW + 1 << "\n"
       << height * depthH + depthH + 1 << "\n"
       << 255 << "\n";

  std::vector<std::vector<glm::vec3>> colors;
  float min{std::numeric_limits<float>::infinity()},
    max{-std::numeric_limits<float>::infinity()};

  colors.reserve(height * depthH + depthH + 1);
  float nan = std::numeric_limits<float>::quiet_NaN();
  for(int m = 0; m < height * depthH + depthH + 1; ++m) {
    std::vector<glm::vec3> row(width * depthW + depthW + 1, {nan, nan, nan});
    colors.push_back(std::move(row));
  }

  for(int32_t i = 0; i < _info.extent.depth; ++i) {
    auto w = i % depthW, h = i / depthW;

    auto saveImg = image::linearHostUnique(device, width, height, _info.format);

    vk::ImageCopy region{{aspect::eColor, 0, 0, 1},
                         {0, 0, i},
                         {aspect::eColor, 0, 0, 1},
                         {},
                         {width, height, 1}};

    Device::executeImmediately(
      device.getDevice(), cmdPool, queue, [&](vk::CommandBuffer cb) {
        auto oldLayout = currentLayout;
        setLayout(cb, layout::eTransferSrcOptimal);
        saveImg->setLayout(cb, layout::eTransferDstOptimal);

        cb.copyImage(
          image(), layout::eTransferSrcOptimal, saveImg->image(),
          layout::eTransferDstOptimal, region);
        setLayout(cb, oldLayout);

        saveImg->setLayout(cb, layout::eGeneral);
      });

    auto subResourceLayout = saveImg->subresourceLayout(
      device.getDevice(), {vk::ImageAspectFlagBits ::eColor, 0, 0});

    auto data = saveImg->ptr<char>();
    data += subResourceLayout.offset;

    switch(format()) {
      case vk::Format::eR32G32B32A32Sfloat: {

        for(uint32_t y = 0; y < height; y++) {
          auto row = (float *)data;
          for(uint32_t x = 0; x < width; x++) {
            for(int j = 0; j < 3; ++j) {
              min = std::min(min, row[j]);
              max = std::max(max, row[j]);
            }
            auto gRowIdx = h * height + y + h + 1;
            auto gColumnIdx = w * width + x + w + 1;
            colors[gRowIdx][gColumnIdx] = {row[0], row[1], row[2]};
            row += 4;
          }
          data += subResourceLayout.rowPitch;
        }

      } break;
      default: error("Not supported!"); break;
    }
  }

  if(max - min < 1e-6) scale = 0;
  else
    scale = 255 / (max - min);

  // ppm binary pixel data
  std::vector<char> colorRow;
  for(auto &row: colors) {
    for(auto &color: row) {
      colorRow.clear();
      colorRow.push_back(
        std::isnan(color[0]) ? char(255) :
                               glm::clamp((color[0] - min) * scale, 0.f, 255.f));
      colorRow.push_back(
        std::isnan(color[1]) ? char(255) :
                               glm::clamp((color[1] - min) * scale, 0.f, 255.f));
      colorRow.push_back(
        std::isnan(color[2]) ? char(255) :
                               glm::clamp((color[2] - min) * scale, 0.f, 255.f));
      file.write(colorRow.data(), 3);
    }
  }
  file.close();

  println("save ", path, ".ppm", "min:", min, ",max:", max, ",scale:", scale / 255);
}

vk::SubresourceLayout Texture::subresourceLayout(
  const vk::Device &device, const vk::ImageSubresource &subresource) {
  vk::SubresourceLayout layout{};
  device.getImageSubresourceLayout(image(), &subresource, &layout);
  return layout;
}
}
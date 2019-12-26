#include "../images.h"
#include "../buffers.h"
#include "sim/util/syntactic_sugar.h"

#include <stb_image.h>
#include <gli/gli.hpp>

namespace sim::graphics {
using layout = vk::ImageLayout;
using access = vk::AccessFlagBits;
using aspect = vk::ImageAspectFlagBits;
using stage = vk::PipelineStageFlagBits;
using imageUsage = vk::ImageUsageFlagBits;
using buffer = vk::BufferUsageFlagBits;

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

  device.graphicsImmediately([&](vk::CommandBuffer cb) {
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
  : Texture{device.allocator(), info(width, height, mipLevels, format)} {
  createImageView(
    device.getDevice(), vk::ImageViewType::eCube, vk::ImageAspectFlagBits::eColor);
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
          imageUsage::eSampled | imageUsage::eTransferSrc | imageUsage::eTransferDst,
          vk::SharingMode::eExclusive,
          0,
          nullptr,
          vk::ImageLayout::ePreinitialized};
}
}
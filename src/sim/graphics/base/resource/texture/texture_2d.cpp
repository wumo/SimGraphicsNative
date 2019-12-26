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

Texture2D Texture2D::loadFromFile(
  Device &device, const std::string &file, vk::Format format, bool generateMipmap) {
  if(endWith(file, ".dds") || endWith(file, ".kmg") || endWith(file, ".ktx")) {
    const auto &t = gli::load(file.c_str());
    errorIf(t.empty(), "failed to load texture image!");
    gli::texture2d tex{t};
    auto extent = tex.extent();
    uint32_t texWidth = extent.x, texHeight = extent.y;
    auto texture = Texture2D{device, texWidth, texHeight, format, generateMipmap};
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
    auto texture = Texture2D{device, texWidth, texHeight, format, generateMipmap};
    auto imageSize = texWidth * texHeight * STBI_rgb_alpha * sizeof(stbi_uc);
    texture.upload(device, pixels.get(), imageSize, !generateMipmap);
    if(generateMipmap) texture._generateMipmap(device);
    return texture;
  }
}

Texture2D Texture2D::loadFromGrayScaleFile(
  Device &device, const std::string &file, vk::Format format, bool generateMipmap) {
  uint32_t texWidth, texHeight, texChannels;
  auto pixels = UniqueBytes(
    (unsigned char *)(stbi_load_16(
      file.c_str(), reinterpret_cast<int *>(&texWidth),
      reinterpret_cast<int *>(&texHeight), reinterpret_cast<int *>(&texChannels),
      STBI_grey)),
    [](stbi_uc *ptr) { stbi_image_free(ptr); });

  errorIf(pixels == nullptr, "failed to load texture image!");
  auto texture = Texture2D{device, texWidth, texHeight, format, generateMipmap};
  auto imageSize = texWidth * texHeight * sizeof(stbi_us);
  texture.upload(device, pixels.get(), imageSize, !generateMipmap);
  if(generateMipmap) texture._generateMipmap(device);
  return texture;
}

Texture2D Texture2D::loadFromBytes(
  Device &device, const unsigned char *bytes, size_t size, uint32_t texWidth,
  uint32_t texHeight, bool generateMipmap) {
  auto texture =
    Texture2D{device, texWidth, texHeight, vk::Format::eR8G8B8A8Srgb, generateMipmap};
  texture.upload(device, bytes, size, !generateMipmap);
  if(generateMipmap) texture._generateMipmap(device);
  return texture;
}

Texture2D::Texture2D(
  Device &device, uint32_t width, uint32_t height, vk::Format format, bool useMipmap,
  bool attachment)
  : Texture{device.allocator(), info(width, height, useMipmap, format, attachment)} {
  createImageView(
    device.getDevice(), vk::ImageViewType::e2D, vk::ImageAspectFlagBits::eColor);
}

vk::ImageCreateInfo Texture2D::info(
  uint32_t width, uint32_t height, bool useMipmap, vk::Format format, bool attachment) {
  auto flag = imageUsage::eSampled | imageUsage::eTransferSrc | imageUsage::eTransferDst;
  if(attachment) flag |= imageUsage::eColorAttachment;
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

void Texture2D::_generateMipmap(Device &device) {
  auto formatProp = device.getPhysicalDevice().getFormatProperties(_info.format);
  errorIf(
    !(formatProp.optimalTilingFeatures & vk::FormatFeatureFlagBits ::eBlitSrc) ||
      !(formatProp.optimalTilingFeatures & vk::FormatFeatureFlagBits ::eBlitDst),
    "blit feature src/dst not satisfied!!");
  errorIf(
    !(formatProp.optimalTilingFeatures &
      vk::FormatFeatureFlagBits ::eSampledImageFilterLinear),
    "texture image format does not support linear blitting!");
  device.graphicsImmediately([&](vk::CommandBuffer cb) {
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
}
#include "../images.h"
#include "../buffers.h"
#include "sim/util/syntactic_sugar.h"

namespace sim::graphics::image {
using aspect = vk::ImageAspectFlagBits;
using imageUsage = vk::ImageUsageFlagBits;

uPtr<Texture> depthStencilUnique(
  Device &device, uint32_t width, uint32_t height, vk::Format format,
  vk::SampleCountFlagBits sampleCount) {
  auto texture = u<Texture>(
    device, vk::ImageCreateInfo{{},
                                vk::ImageType::e2D,
                                format,
                                {width, height, 1U},
                                1,
                                1,
                                sampleCount,
                                vk::ImageTiling::eOptimal,
                                imageUsage::eDepthStencilAttachment});
  texture->createImageView(device.getDevice(), vk::ImageViewType::e2D, aspect::eDepth);
  return texture;
}
uPtr<Texture> colorInputAttachmentUnique(
  Device &device, uint32_t width, uint32_t height, vk::Format format,
  vk::SampleCountFlagBits sampleCount) {
  auto texture = u<Texture>(
    device,
    vk::ImageCreateInfo{{},
                        vk::ImageType::e2D,
                        format,
                        {width, height, 1U},
                        1,
                        1,
                        sampleCount,
                        vk::ImageTiling::eOptimal,
                        imageUsage::eColorAttachment | imageUsage::eInputAttachment});
  texture->createImageView(device.getDevice(), vk::ImageViewType::e2D, aspect::eColor);
  return texture;
}
uPtr<Texture> storageAttachmentUnique(
  Device &device, uint32_t width, uint32_t height, vk::Format format,
  vk::SampleCountFlagBits sampleCount) {
  auto texture = u<Texture>(
    device, vk::ImageCreateInfo{{},
                                vk::ImageType::e2D,
                                format,
                                {width, height, 1},
                                1,
                                1,
                                sampleCount,
                                vk::ImageTiling::eOptimal,
                                imageUsage::eColorAttachment | imageUsage::eSampled |
                                  imageUsage::eStorage | imageUsage::eTransferSrc});
  texture->createImageView(device.getDevice(), vk::ImageViewType::e2D, aspect::eColor);
  return texture;
}
}
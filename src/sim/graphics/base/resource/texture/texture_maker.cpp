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
    device.allocator(), vk::ImageCreateInfo{{},
                                            vk::ImageType::e2D,
                                            format,
                                            {width, height, 1U},
                                            1,
                                            1,
                                            sampleCount,
                                            vk::ImageTiling::eOptimal,
                                            imageUsage::eDepthStencilAttachment});
  texture->setImageView(device.getDevice(), vk::ImageViewType::e2D, aspect::eDepth);
  return texture;
}
uPtr<Texture> colorInputAttachmentUnique(
  Device &device, uint32_t width, uint32_t height, vk::Format format,
  vk::SampleCountFlagBits sampleCount) {
  auto texture = u<Texture>(
    device.allocator(),
    vk::ImageCreateInfo{{},
                        vk::ImageType::e2D,
                        format,
                        {width, height, 1U},
                        1,
                        1,
                        sampleCount,
                        vk::ImageTiling::eOptimal,
                        imageUsage::eColorAttachment | imageUsage::eInputAttachment});
  texture->setImageView(device.getDevice(), vk::ImageViewType::e2D, aspect::eColor);
  return texture;
}
uPtr<Texture> storageAttachmentUnique(
  Device &device, uint32_t width, uint32_t height, vk::Format format,
  vk::SampleCountFlagBits sampleCount) {
  auto texture = u<Texture>(
    device.allocator(),
    vk::ImageCreateInfo{{},
                        vk::ImageType::e2D,
                        format,
                        {width, height, 1},
                        1,
                        1,
                        sampleCount,
                        vk::ImageTiling::eOptimal,
                        imageUsage::eColorAttachment | imageUsage::eSampled |
                          imageUsage::eStorage | imageUsage::eTransferSrc});
  texture->setImageView(device.getDevice(), vk::ImageViewType::e2D, aspect::eColor);
  return texture;
}

uPtr<Texture> linearHostUnique(
  Device &device, uint32_t width, uint32_t height, vk::Format format) {
  return u<Texture>(
    device.allocator(),
    vk::ImageCreateInfo{{},
                        vk::ImageType::e2D,
                        format,
                        {width, height, 1},
                        1,
                        1,
                        vk::SampleCountFlagBits::e1,
                        vk::ImageTiling::eLinear,
                        imageUsage::eTransferDst},
    VmaAllocationCreateInfo{VMA_ALLOCATION_CREATE_MAPPED_BIT, VMA_MEMORY_USAGE_GPU_TO_CPU,
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT});
}

uPtr<Texture> depthStencilInputAttachmentUnique(
  Device &device, uint32_t width, uint32_t height, vk::Format format,
  vk::SampleCountFlagBits sampleCount) {
  auto texture = u<Texture>(
    device.allocator(), vk::ImageCreateInfo{{},
                                            vk::ImageType::e2D,
                                            format,
                                            {width, height, 1U},
                                            1,
                                            1,
                                            sampleCount,
                                            vk::ImageTiling::eOptimal,
                                            imageUsage::eDepthStencilAttachment |
                                              imageUsage::eInputAttachment});
  texture->setImageView(device.getDevice(), vk::ImageViewType::e2D, aspect::eDepth);
  return texture;
}
}
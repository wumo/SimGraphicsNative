#pragma once
#include <array>
#include "sim/graphics/base/vkcommon.h"
#include "sim/graphics/base/pipeline/sampler.h"
#include "sim/graphics/base/device.h"

namespace sim::graphics {
uint32_t calcMipLevels(uint32_t dim);

/**
	 * Generic image with a view and memory object.
	 * Vulkan images need a memory object to hold the data and a view object for
	 * the GPU to access the data.
	 */
class ImageBase {
public:
  static void setLayout(
    const vk::CommandBuffer &cb, vk::Image image, vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout, vk::AccessFlags srcAccessMask,
    vk::AccessFlags dstAccessMask,
    vk::PipelineStageFlagBits srcStageMask = vk::PipelineStageFlagBits::eAllCommands,
    vk::PipelineStageFlagBits dstStageMask = vk::PipelineStageFlagBits::eAllCommands);
  static void copy(const vk::CommandBuffer &cb, ImageBase &srcImage, vk::Image &dstImage);
  static void resolve(
    const vk::CommandBuffer &cb, ImageBase &srcImage, vk::Image &dstImage);

public:
  __only_move__(ImageBase);
  ImageBase(
    const VmaAllocator &allocator, vk::ImageCreateInfo info, VmaMemoryUsage memoryUsage,
    const vk::MemoryPropertyFlags &flags = {});
  ImageBase(
    const VmaAllocator &allocator, vk::ImageCreateInfo info,
    VmaAllocationCreateInfo allocInfo);
  void clear(
    const vk::CommandBuffer &cb, const std::array<float, 4> &color = {1, 1, 1, 1});
  void copy(const vk::CommandBuffer &cb, ImageBase &srcImage);
  void copy(
    const vk::CommandBuffer &cb, vk::Buffer buffer, uint32_t mipLevel,
    uint32_t arrayLayer, uint32_t width, uint32_t height, uint32_t depth,
    uint32_t offset);
  void upload(
    Device &device, const unsigned char *bytes, size_t sizeInBytes,
    bool transitToShaderRead = true);
  void upload(
    Device &device, std::vector<unsigned char> &bytes, bool transitToShaderRead = true);

  void setLayout(
    const vk::CommandBuffer &cb, vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
    vk::AccessFlags srcAccessMask, vk::AccessFlags dstAccessMask,
    uint32_t baseMipLevel = 0, uint32_t levelCount = 1,
    vk::PipelineStageFlagBits srcStageMask = vk::PipelineStageFlagBits::eAllCommands,
    vk::PipelineStageFlagBits dstStageMask = vk::PipelineStageFlagBits::eAllCommands,
    bool record = false);
  void setLayout(
    const vk::CommandBuffer &cb, vk::ImageLayout newLayout, vk::AccessFlags srcAccessMask,
    vk::AccessFlags dstAccessMask,
    vk::PipelineStageFlagBits srcStageMask = vk::PipelineStageFlagBits::eAllCommands,
    vk::PipelineStageFlagBits dstStageMask = vk::PipelineStageFlagBits::eAllCommands);
  void setLayout(
    const vk::CommandBuffer &cb, vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
    vk::PipelineStageFlagBits srcStageMask = vk::PipelineStageFlagBits::eAllCommands,
    vk::PipelineStageFlagBits dstStageMask = vk::PipelineStageFlagBits::eAllCommands);
  void setLevelLayout(
    const vk::CommandBuffer &cb, uint32_t baseMipLevel, vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout,
    vk::PipelineStageFlagBits srcStageMask = vk::PipelineStageFlagBits::eAllCommands,
    vk::PipelineStageFlagBits dstStageMask = vk::PipelineStageFlagBits::eAllCommands);
  void setLayout(
    const vk::CommandBuffer &cb, vk::ImageLayout newLayout,
    vk::PipelineStageFlagBits srcStageMask = vk::PipelineStageFlagBits::eAllCommands,
    vk::PipelineStageFlagBits dstStageMask = vk::PipelineStageFlagBits::eAllCommands);

  void setCurrentLayout(vk::ImageLayout oldLayout);

  void setSampler(vk::UniqueSampler &&sampler);
  void createImageView(
    const vk::Device &device, vk::ImageViewType viewType,
    vk::ImageAspectFlags aspectMask);
  void createImageView(const vk::Device &device, vk::ImageViewCreateInfo info);

  const vk::ImageCreateInfo &getInfo() const;
  const vk::Format &format() const;
  const vk::Extent3D &extent() const;
  const vk::ImageCreateInfo &info() const;
  const vk::Image &image() const;
  const vk::ImageView &imageView() const;
  const vk::ImageSubresourceRange subresourceRange(vk::ImageAspectFlags aspectMask) const;
  const vk::Sampler &sampler() const;

protected:
  struct VmaImage {
    const VmaAllocator &allocator;
    VmaAllocation allocation{nullptr};
    vk::Image image{nullptr};
  };

  using UniquePtr = std::unique_ptr<VmaImage, std::function<void(VmaImage *)>>;
  UniquePtr vmaImage;
  VmaAllocationInfo allocationInfo{};
  bool mappable{false};

  vk::UniqueImageView _imageView;
  vk::UniqueSampler _sampler;
  vk::ImageLayout currentLayout;
  vk::ImageCreateInfo _info;
  static vk::AccessFlags guessSrcAccess(const vk::ImageLayout &oldLayout);
  static vk::AccessFlags guessDstAccess(const vk::ImageLayout &newLayout);
};

class DepthStencilImage: public ImageBase {
public:
  DepthStencilImage(
    Device &device, uint32_t width, uint32_t height,
    vk::Format format = vk::Format::eD24UnormS8Uint,
    vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1);

private:
  static vk::ImageCreateInfo info(
    uint32_t width, uint32_t height, vk::Format format,
    vk::SampleCountFlagBits sampleCount);
};

class TransientDepthStencilImage: public ImageBase {
public:
  TransientDepthStencilImage(
    Device &device, uint32_t width, uint32_t height,
    vk::Format format = vk::Format::eD24UnormS8Uint,
    vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1);

private:
  static vk::ImageCreateInfo info(
    uint32_t width, uint32_t height, vk::Format format,
    vk::SampleCountFlagBits sampleCount);
};

class ColorAttachmentImage: public ImageBase {
public:
  ColorAttachmentImage(
    Device &device, uint32_t width, uint32_t height,
    vk::Format format = vk::Format::eR8G8B8A8Unorm,
    vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1);

private:
  static vk::ImageCreateInfo info(
    uint32_t width, uint32_t height, vk::Format format,
    vk::SampleCountFlagBits sampleCount);
};

class ColorInputAttachmentImage: public ImageBase {
public:
  ColorInputAttachmentImage(
    Device &device, uint32_t width, uint32_t height,
    vk::Format format = vk::Format::eR8G8B8A8Unorm,
    vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1);

private:
  static vk::ImageCreateInfo info(
    uint32_t width, uint32_t height, vk::Format format,
    vk::SampleCountFlagBits sampleCount);
};

class TransientColorInputAttachmentImage: public ImageBase {
public:
  TransientColorInputAttachmentImage(
    Device &device, uint32_t width, uint32_t height,
    vk::Format format = vk::Format::eR8G8B8A8Unorm,
    vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1);

private:
  static vk::ImageCreateInfo info(
    uint32_t width, uint32_t height, vk::Format format,
    vk::SampleCountFlagBits sampleCount);
};

class StorageAttachmentImage: public ImageBase {
public:
  StorageAttachmentImage(
    Device &device, uint32_t width, uint32_t height, vk::Format format,
    vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1);

private:
  static vk::ImageCreateInfo info(
    uint32_t width, uint32_t height, vk::Format format,
    vk::SampleCountFlagBits sampleCount);
};

class StorageImage: public ImageBase {
public:
  StorageImage(
    Device &device, uint32_t width, uint32_t height, vk::Format format,
    vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1);

private:
  static vk::ImageCreateInfo info(
    uint32_t width, uint32_t height, vk::Format format,
    vk::SampleCountFlagBits sampleCount);
};

class Texture: public ImageBase {
public:
  Texture(Device &device, vk::ImageCreateInfo info);
};

class Texture2D: public Texture {
public:
  static Texture2D loadFromFile(
    Device &device, const std::string &file,
    vk::Format format = vk::Format::eR8G8B8A8Srgb, bool generateMipmap = false);
  static Texture2D loadFromGrayScaleFile(
    Device &device, const std::string &file, vk::Format format = vk::Format::eR16Sfloat,
    bool generateMipmap = false);
  static Texture2D loadFromBytes(
    Device &device, const unsigned char *bytes, size_t size, uint32_t texWidth,
    uint32_t texHeight, bool generateMipmap = false);
  Texture2D(
    Device &device, uint32_t width, uint32_t height,
    vk::Format format = vk::Format::eR8G8B8A8Srgb, bool useMipmap = false,
    bool attachment = false);

private:
  void _generateMipmap(Device &device);
  static vk::ImageCreateInfo info(
    uint32_t width, uint32_t height, bool useMipmap, vk::Format format, bool attachment);
};

class Texture3D: public Texture {
public:
  Texture3D(
    Device &device, uint32_t width, uint32_t height, uint32_t depth,
    vk::Format format = vk::Format::eR8G8B8A8Srgb, bool useMipmap = false,
    bool attachment = false);

private:
  static vk::ImageCreateInfo info(
    uint32_t width, uint32_t height, uint32_t depth, bool useMipmap, vk::Format format,
    bool attachment);
};

class TextureImageCube: public Texture {
public:
  static TextureImageCube loadFromFile(
    Device &device, const std::string &file, bool generateMipmap = false);
  TextureImageCube(
    Device &device, uint32_t width, uint32_t height, uint32_t mipLevels = 1,
    vk::Format format = vk::Format::eR16G16B16A16Sfloat);

private:
  static vk::ImageCreateInfo info(
    uint32_t width, uint32_t height, uint32_t mipLevels, vk::Format format);
};

class SamplerMaker {
public:
  SamplerMaker();
  explicit SamplerMaker(SamplerDef samplerDef);
  SamplerMaker &flags(vk::SamplerCreateFlags value);
  SamplerMaker &magFilter(vk::Filter value);
  SamplerMaker &minFilter(vk::Filter value);
  SamplerMaker &mipmapMode(vk::SamplerMipmapMode value);
  SamplerMaker &addressModeU(vk::SamplerAddressMode value);
  SamplerMaker &addressModeV(vk::SamplerAddressMode value);
  SamplerMaker &addressModeW(vk::SamplerAddressMode value);
  SamplerMaker &mipLodBias(float value);
  SamplerMaker &anisotropyEnable(vk::Bool32 value);
  SamplerMaker &maxAnisotropy(float value);
  SamplerMaker &compareEnable(vk::Bool32 value);
  SamplerMaker &compareOp(vk::CompareOp value);
  SamplerMaker &minLod(float value);
  SamplerMaker &maxLod(float value);
  SamplerMaker &borderColor(vk::BorderColor value);
  SamplerMaker &unnormalizedCoordinates(vk::Bool32 value);
  vk::UniqueSampler createUnique(const vk::Device &device) const;
  vk::Sampler create(const vk::Device &device) const;

private:
  struct State {
    vk::SamplerCreateInfo info;
  } state;
};
}

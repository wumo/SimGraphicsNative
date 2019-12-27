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
class Texture {
public:
  static void setLayout(
    const vk::CommandBuffer &cb, vk::Image image, vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout, vk::AccessFlags srcAccessMask,
    vk::AccessFlags dstAccessMask,
    vk::PipelineStageFlagBits srcStageMask = vk::PipelineStageFlagBits::eAllCommands,
    vk::PipelineStageFlagBits dstStageMask = vk::PipelineStageFlagBits::eAllCommands);
  static void copy(const vk::CommandBuffer &cb, Texture &srcImage, vk::Image &dstImage);
  static void resolve(
    const vk::CommandBuffer &cb, Texture &srcImage, vk::Image &dstImage);

public:
  __only_move__(Texture);
  Texture(
    const VmaAllocator &allocator, vk::ImageCreateInfo info,
    VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY,
    const vk::MemoryPropertyFlags &flags = {}, std::string name = "");
  Texture(
    const VmaAllocator &allocator, vk::ImageCreateInfo info,
    VmaAllocationCreateInfo allocInfo, std::string name = "");

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
    vk::AccessFlags srcAccessMask, vk::AccessFlags dstAccessMask,
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
  void setLayoutByGuess(
    const vk::CommandBuffer &cb, vk::ImageLayout newLayout,
    vk::PipelineStageFlagBits srcStageMask = vk::PipelineStageFlagBits::eAllCommands,
    vk::PipelineStageFlagBits dstStageMask = vk::PipelineStageFlagBits::eAllCommands);

  void transitToLayout(
    const vk::CommandBuffer &cb, vk::ImageLayout newLayout,
    const vk::AccessFlags &dstAccess, vk::PipelineStageFlagBits dstStage);

  void setCurrentLayout(vk::ImageLayout oldLayout);
  void setSrcAccess(const vk::AccessFlags &srcAccess);
  void setSrcStage(vk::PipelineStageFlagBits srcStage);
  void setCurrentState(
    vk::ImageLayout newLayout, vk::AccessFlags dstAccess,
    vk::PipelineStageFlagBits dstStage);
  void setSampler(vk::UniqueSampler &&sampler);
  void createImageView(
    const vk::Device &device, vk::ImageViewType viewType,
    vk::ImageAspectFlags aspectMask);
  void createImageView(const vk::Device &device, vk::ImageViewCreateInfo info);

  void clear(
    const vk::CommandBuffer &cb, const std::array<float, 4> &color = {1, 1, 1, 1});
  void copy(const vk::CommandBuffer &cb, Texture &srcImage);
  void copy(
    const vk::CommandBuffer &cb, vk::Buffer buffer, uint32_t mipLevel,
    uint32_t arrayLayer, uint32_t width, uint32_t height, uint32_t depth,
    uint32_t offset);
  void upload(
    Device &device, const unsigned char *bytes, size_t sizeInBytes,
    bool transitToShaderRead = true);
  void upload(
    Device &device, std::vector<unsigned char> &bytes, bool transitToShaderRead = true);

  void saveToFile(
    Device &device, vk::CommandPool cmdPool, vk::Queue queue, std::string path,
    float scale = 1.f);

  vk::SubresourceLayout subresourceLayout(
    const vk::Device &device, const vk::ImageSubresource &subresource);

  const vk::ImageCreateInfo &getInfo() const;
  const vk::Format &format() const;
  const vk::Extent3D &extent() const;
  const vk::ImageCreateInfo &info() const;
  const vk::Image &image() const;
  const vk::ImageView &imageView() const;
  vk::ImageSubresourceRange subresourceRange(
    const vk::ImageAspectFlags &aspectMask) const;
  const vk::Sampler &sampler() const;

  template<class T = void>
  T *ptr() {
    return static_cast<T *>(allocationInfo.pMappedData);
  }

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
  vk::AccessFlags srcAccess;
  vk::PipelineStageFlagBits srcStage;
  vk::ImageCreateInfo _info;
  static vk::AccessFlags guessSrcAccess(const vk::ImageLayout &oldLayout);
  static vk::AccessFlags guessDstAccess(const vk::ImageLayout &newLayout);
};

namespace image {

uPtr<Texture> depthStencilUnique(
  Device &device, uint32_t width, uint32_t height,
  vk::Format format = vk::Format::eD24UnormS8Uint,
  vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1);

uPtr<Texture> colorInputAttachmentUnique(
  Device &device, uint32_t width, uint32_t height,
  vk::Format format = vk::Format::eR8G8B8A8Unorm,
  vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1);

uPtr<Texture> storageAttachmentUnique(
  Device &device, uint32_t width, uint32_t height, vk::Format format,
  vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1);

uPtr<Texture> linearHostUnique(
  Device &device, uint32_t width, uint32_t height, vk::Format format);

}

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

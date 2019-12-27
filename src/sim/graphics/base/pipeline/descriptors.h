#pragma once
#include "sim/graphics/base/vkcommon.h"
#include "sim/graphics/base/resource/images.h"

namespace sim::graphics {
class DescriptorSetLayoutMaker {
public:
  DescriptorSetLayoutMaker &vertexShader();
  DescriptorSetLayoutMaker &fragmentShader();
  DescriptorSetLayoutMaker &computeShader();
  DescriptorSetLayoutMaker &shaderStage(vk::ShaderStageFlags stage);
  DescriptorSetLayoutMaker &uniform(
    uint32_t binding, uint32_t descriptorCount = 1,
    vk::DescriptorBindingFlagsEXT bindingFlags = {});
  DescriptorSetLayoutMaker &uniformDynamic(
    uint32_t binding, uint32_t descriptorCount = 1,
    vk::DescriptorBindingFlagsEXT bindingFlags = {});
  DescriptorSetLayoutMaker &buffer(
    uint32_t binding, uint32_t descriptorCount = 1,
    vk::DescriptorBindingFlagsEXT bindingFlags = {});
  DescriptorSetLayoutMaker &sampler2D(
    uint32_t binding, uint32_t descriptorCount = 1,
    vk::DescriptorBindingFlagsEXT bindingFlags = {});
  DescriptorSetLayoutMaker &storageImage(
    uint32_t binding, uint32_t descriptorCount = 1,
    vk::DescriptorBindingFlagsEXT bindingFlags = {});
  DescriptorSetLayoutMaker &input(
    uint32_t binding, uint32_t descriptorCount = 1,
    vk::DescriptorBindingFlagsEXT bindingFlags = {});
  DescriptorSetLayoutMaker &accelerationStructure(
    uint32_t binding, uint32_t descriptorCount = 1,
    vk::DescriptorBindingFlagsEXT bindingFlags = {});

  DescriptorSetLayoutMaker &descriptor(
    uint32_t binding, vk::DescriptorType descriptorType, vk::ShaderStageFlags stageFlags,
    uint32_t descriptorCount = 1, vk::DescriptorBindingFlagsEXT bindingFlags = {});

  vk::UniqueDescriptorSetLayout createUnique(const vk::Device &device) const;
  int getVariableDescriptorBinding() const;
  uint32_t getVariableDescriptorCount() const;

  auto bindings() const -> const std::vector<vk::DescriptorSetLayoutBinding> &;

private:
  friend struct DescriptorUpdater;

  vk::ShaderStageFlags currentStage{vk::ShaderStageFlagBits::eVertex};
  std::vector<vk::DescriptorBindingFlagsEXT> flags;
  bool useDescriptorIndexing{false};
  bool updateAfterBindPool{false};
  int variableDescriptorBinding{-1};
  std::vector<vk::DescriptorSetLayoutBinding> _bindings;
};

/**
	 * A factory class for descriptor sets (A set of uniform bindings)
	 */
class DescriptorSetMaker {
public:
  DescriptorSetMaker &layout(vk::DescriptorSetLayout layout);
  DescriptorSetMaker &layout(
    vk::DescriptorSetLayout layout, uint32_t variableDescriptorCount);
  std::vector<vk::DescriptorSet> create(
    const vk::Device &device, vk::DescriptorPool descriptorPool) const;
  std::vector<vk::UniqueDescriptorSet> createUnique(
    const vk::Device &device, vk::DescriptorPool descriptorPool) const;

private:
  std::vector<vk::DescriptorSetLayout> layouts;
  std::vector<uint32_t> variableDescriptorCounts;
  bool useVariableCounts{false};
};

/**
	 * Convenience class for updating descriptor sets (uniforms)
	 */
class DescriptorSetUpdater {
public:
  class BufferUpdater {
  public:
    explicit BufferUpdater(DescriptorSetUpdater &updater, uint32_t index);
    /**Call this to add a new buffer.*/
    BufferUpdater &buffer(vk::Buffer buffer, vk::DeviceSize offset, vk::DeviceSize range);

  private:
    DescriptorSetUpdater &updater;
    uint32_t index;
  };

  class ImageUpdater {
  public:
    explicit ImageUpdater(DescriptorSetUpdater &updater, uint32_t index);
    /**Call this to add a combined image sampler.*/
    ImageUpdater &image(
      vk::Sampler sampler, vk::ImageView imageView, vk::ImageLayout imageLayout);

  private:
    DescriptorSetUpdater &updater;
    uint32_t index;
  };

  class AccelerationStructureUpdater {
  public:
    explicit AccelerationStructureUpdater(DescriptorSetUpdater &updater, uint32_t index);
    AccelerationStructureUpdater &accelerationStructure(
      const vk::AccelerationStructureNV &as);

  private:
    DescriptorSetUpdater &updater;
    uint32_t index;
  };

  explicit DescriptorSetUpdater(
    size_t maxBuffers = 10, size_t maxImages = 10, size_t maxBufferViews = 0,
    size_t maxAccelerationStructures = 10);
  /**
		 * Call this to begin a new set of images.
		 */
  ImageUpdater beginImages(
    uint32_t dstBinding, uint32_t dstArrayElement, vk::DescriptorType descriptorType);
  /**Call this to start defining buffers.*/
  BufferUpdater beginBuffers(
    uint32_t dstBinding, uint32_t dstArrayElement, vk::DescriptorType descriptorType);

  AccelerationStructureUpdater beginAccelerationStructures(
    uint32_t dstBinding, uint32_t dstArrayElement);
  DescriptorSetUpdater &buffer(
    uint32_t dstBinding, uint32_t dstArrayElement, uint32_t descriptorCount,
    vk::DescriptorBufferInfo *pBufferInfo);
  DescriptorSetUpdater &buffer(
    uint32_t dstBinding, uint32_t dstArrayElement, vk::DescriptorType descriptorType,
    vk::Buffer buffer, vk::DeviceSize offset, vk::DeviceSize range);
  DescriptorSetUpdater &uniform(uint32_t dstBinding, vk::Buffer buffer);
  DescriptorSetUpdater &uniformDynamic(uint32_t dstBinding, vk::Buffer buffer);
  DescriptorSetUpdater &buffer(uint32_t dstBinding, vk::Buffer buffer);

  DescriptorSetUpdater &images(
    uint32_t dstBinding, uint32_t dstArrayElement, vk::DescriptorType descriptorType,
    uint32_t descriptorCount, vk::DescriptorImageInfo *pImageInfo);
  DescriptorSetUpdater &sampler2D(
    uint32_t dstBinding, uint32_t dstArrayElement, uint32_t descriptorCount,
    vk::DescriptorImageInfo *pImageInfo);
  DescriptorSetUpdater &image(
    uint32_t dstBinding, uint32_t dstArrayElement, vk::DescriptorType descriptorType,
    vk::Sampler sampler, vk::ImageView imageView, vk::ImageLayout imageLayout);
  DescriptorSetUpdater &input(uint32_t dstBinding, vk::ImageView imageView);
  DescriptorSetUpdater &storageImage(uint32_t dstBinding, vk::ImageView imageView);
  DescriptorSetUpdater &accelerationStructure(
    uint32_t dstBinding, const vk::AccelerationStructureNV &as);

  /**
		 * Call this to update the descriptor sets with their pointers (but not data).
		 */
  void update(const vk::Device &device, const vk::DescriptorSet &descriptorSet);

  void reset();

private:
  std::vector<vk::DescriptorBufferInfo> bufferInfo;
  std::vector<vk::DescriptorImageInfo> imageInfo;
  std::vector<vk::WriteDescriptorSetAccelerationStructureNV> accelerationStructureInfo;
  std::vector<vk::WriteDescriptorSet> descriptorWrites;
  std::vector<vk::CopyDescriptorSet> descriptorCopies;
  std::vector<vk::BufferView> bufferViews;
  size_t numBuffers{}, numImages{}, numBufferViews{}, numAccelerationStructures{};
};

class DescriptorSetDef {
protected:
  DescriptorSetLayoutMaker layout{};
  DescriptorSetUpdater updater{};
  uint32_t binding{0};

  vk::Device _device;

public:
  vk::UniqueDescriptorSetLayout descriptorSetLayout{};

  void init(const vk::Device &device) {
    descriptorSetLayout = layout.createUnique(device);
    this->_device = device;
  }

  vk::DescriptorSet createSet(vk::DescriptorPool &pool) {
    errorIf(!descriptorSetLayout, "descriptorSetLayout hasn't been created!");
    return DescriptorSetMaker()
      .layout(*descriptorSetLayout, layout.getVariableDescriptorCount())
      .create(_device, pool)[0];
  }

  void update(const vk::DescriptorSet &descriptorSet) {
    errorIf(!_device, "call init() first");
    updater.update(_device, descriptorSet);
    updater.reset();
  }

  const DescriptorSetLayoutMaker &layoutDef() const { return layout; }
};

struct DescriptorUpdater {
  explicit DescriptorUpdater(
    DescriptorSetLayoutMaker &layout, DescriptorSetUpdater &updater, uint32_t binding,
    uint32_t descriptorCount = 1)
    : layout{layout}, updater{updater}, binding{binding} {}

  const uint32_t binding;

  uint32_t &descriptorCount() const { return layout._bindings[binding].descriptorCount; }

protected:
  DescriptorSetLayoutMaker &layout;
  DescriptorSetUpdater &updater;
};

struct DescriptorUniformUpdater: DescriptorUpdater {
  explicit DescriptorUniformUpdater(
    DescriptorSetLayoutMaker &layout, DescriptorSetUpdater &updater, uint32_t binding,
    uint32_t descriptorCount = 1)
    : DescriptorUpdater(layout, updater, binding, descriptorCount) {}

  DescriptorUniformUpdater &operator()(vk::Buffer buffer) {
    updater.uniform(binding, buffer);
    return *this;
  }
};

struct DescriptorUniformDynamicUpdater: DescriptorUpdater {
  explicit DescriptorUniformDynamicUpdater(
    DescriptorSetLayoutMaker &layout, DescriptorSetUpdater &updater, uint32_t binding,
    uint32_t descriptorCount = 1)
    : DescriptorUpdater(layout, updater, binding, descriptorCount) {}

  DescriptorUniformDynamicUpdater &operator()(vk::Buffer buffer) {
    updater.uniformDynamic(binding, buffer);
    return *this;
  }
};

struct DescriptorBufferUpdater: DescriptorUpdater {
  explicit DescriptorBufferUpdater(
    DescriptorSetLayoutMaker &layout, DescriptorSetUpdater &updater, uint32_t binding,
    uint32_t descriptorCount = 1)
    : DescriptorUpdater(layout, updater, binding, descriptorCount) {}

  DescriptorBufferUpdater &operator()(vk::Buffer buffer) {
    updater.buffer(binding, buffer);
    return *this;
  }

  DescriptorBufferUpdater &operator()(vk::DescriptorBufferInfo *pBufferInfo) {
    updater.buffer(binding, 0, descriptorCount(), pBufferInfo);
    return *this;
  }
};

struct DescriptorStorageImageUpdater: DescriptorUpdater {
  explicit DescriptorStorageImageUpdater(
    DescriptorSetLayoutMaker &layout, DescriptorSetUpdater &updater, uint32_t binding,
    uint32_t descriptorCount = 1)
    : DescriptorUpdater(layout, updater, binding, descriptorCount) {}

  DescriptorStorageImageUpdater &operator()(vk::ImageView imageView) {
    updater.storageImage(binding, imageView);
    return *this;
  }

  DescriptorStorageImageUpdater &operator()(Texture &image) {
    updater.storageImage(binding, image.imageView());
    return *this;
  }
};

struct DescriptorSampler2DUpdater: DescriptorUpdater {
  explicit DescriptorSampler2DUpdater(
    DescriptorSetLayoutMaker &layout, DescriptorSetUpdater &updater, uint32_t binding,
    uint32_t descriptorCount = 1)
    : DescriptorUpdater(layout, updater, binding, descriptorCount) {}

  DescriptorSampler2DUpdater &operator()(
    Texture &image, vk::ImageLayout layout = vk::ImageLayout::eShaderReadOnlyOptimal) {
    updater.image(
      binding, 0, vk::DescriptorType::eCombinedImageSampler, image.sampler(),
      image.imageView(), layout);
    return *this;
  }

  DescriptorSampler2DUpdater &operator()(vk::DescriptorImageInfo *pImageInfo) {
    updater.sampler2D(binding, 0, descriptorCount(), pImageInfo);
    return *this;
  }

  DescriptorSampler2DUpdater &operator()(
    uint32_t descriptorCount, vk::DescriptorImageInfo *pImageInfo) {
    updater.sampler2D(binding, 0, descriptorCount, pImageInfo);
    return *this;
  }

  DescriptorSampler2DUpdater &operator()(
    uint32_t dstArrayElement, uint32_t descriptorCount,
    vk::DescriptorImageInfo *pImageInfo) {
    updater.sampler2D(binding, dstArrayElement, descriptorCount, pImageInfo);
    return *this;
  }
};

struct DescriptorInputUpdater: DescriptorUpdater {
  explicit DescriptorInputUpdater(
    DescriptorSetLayoutMaker &layout, DescriptorSetUpdater &updater, uint32_t binding,
    uint32_t descriptorCount = 1)
    : DescriptorUpdater(layout, updater, binding, descriptorCount) {}

  DescriptorInputUpdater &operator()(vk::ImageView imageView) {
    updater.input(binding, imageView);
    return *this;
  }
};

struct DescriptorASUpdater: DescriptorUpdater {
  explicit DescriptorASUpdater(
    DescriptorSetLayoutMaker &layout, DescriptorSetUpdater &updater, uint32_t binding,
    uint32_t descriptorCount = 1)
    : DescriptorUpdater(layout, updater, binding, descriptorCount) {}

  DescriptorASUpdater &operator()(const vk::AccelerationStructureNV &as) {
    updater.accelerationStructure(binding, as);
    return *this;
  }
};

#define __uniform__(field, stage) \
public:                           \
  DescriptorUniformUpdater field{ \
    layout, updater, (layout.shaderStage(stage), layout.uniform(binding), binding++)};
#define __uniformDynamic__(field, stage) \
public:                                  \
  DescriptorUniformDynamicUpdater field{ \
    layout, updater,                     \
    (layout.shaderStage(stage), layout.uniformDynamic(binding), binding++)};
#define __buffer__(field, stage) \
public:                          \
  DescriptorBufferUpdater field{ \
    layout, updater, (layout.shaderStage(stage), layout.buffer(binding), binding++)};
#define __sampler__(field, stage)   \
public:                             \
  DescriptorSampler2DUpdater field{ \
    layout, updater,                \
    (layout.shaderStage(stage), layout.sampler2D(binding, 1), binding++)};
#define __samplers__(field, flag, stage) \
public:                                  \
  DescriptorSampler2DUpdater field{      \
    layout, updater,                     \
    (layout.shaderStage(stage), layout.sampler2D(binding, 1, flag), binding++)};
#define __storageImage__(field, stage) \
public:                                \
  DescriptorStorageImageUpdater field{ \
    layout, updater,                   \
    (layout.shaderStage(stage), layout.storageImage(binding, 1), binding++)};
#define __input__(field, stage) \
public:                         \
  DescriptorInputUpdater field{ \
    layout, updater, (layout.shaderStage(stage), layout.input(binding), binding++)};
#define __accelerationStructure__(field, stage) \
public:                                         \
  DescriptorASUpdater field{                    \
    layout, updater,                            \
    (layout.shaderStage(stage), layout.accelerationStructure(binding), binding++)};
}

#include "descriptors.h"
#include "sim/util/syntactic_sugar.h"
#include <algorithm>

namespace sim::graphics {
using descriptor = vk::DescriptorType;

auto DescriptorSetLayoutMaker::vertexShader() -> DescriptorSetLayoutMaker & {
  currentStage = vk::ShaderStageFlagBits::eVertex;
  return *this;
}

auto DescriptorSetLayoutMaker::fragmentShader() -> DescriptorSetLayoutMaker & {
  currentStage = vk::ShaderStageFlagBits::eFragment;
  return *this;
}

auto DescriptorSetLayoutMaker::computeShader() -> DescriptorSetLayoutMaker & {
  currentStage = vk::ShaderStageFlagBits::eCompute;
  return *this;
}

auto DescriptorSetLayoutMaker::shaderStage(vk::ShaderStageFlags stage)
  -> DescriptorSetLayoutMaker & {
  currentStage = stage;
  return *this;
}

auto DescriptorSetLayoutMaker::descriptor(
  uint32_t binding, vk::DescriptorType descriptorType, vk::ShaderStageFlags stageFlags,
  uint32_t descriptorCount, vk::DescriptorBindingFlagsEXT bindingFlags)
  -> DescriptorSetLayoutMaker & {
  _bindings.emplace_back(binding, descriptorType, descriptorCount, stageFlags, nullptr);
  flags.push_back(bindingFlags);
  useDescriptorIndexing = bool(bindingFlags);
  if(bindingFlags & vk::DescriptorBindingFlagBitsEXT::eUpdateAfterBind)
    updateAfterBindPool = true;
  if(bindingFlags & vk::DescriptorBindingFlagBitsEXT::eVariableDescriptorCount)
    variableDescriptorBinding = binding;
  return *this;
}

auto DescriptorSetLayoutMaker::uniform(
  uint32_t binding, uint32_t descriptorCount, vk::DescriptorBindingFlagsEXT bindingFlags)
  -> DescriptorSetLayoutMaker & {
  return descriptor(
    binding, descriptor::eUniformBuffer, currentStage, descriptorCount, bindingFlags);
}

auto DescriptorSetLayoutMaker::uniformDynamic(
  uint32_t binding, uint32_t descriptorCount, vk::DescriptorBindingFlagsEXT bindingFlags)
  -> DescriptorSetLayoutMaker & {
  return descriptor(
    binding, descriptor::eUniformBufferDynamic, currentStage, descriptorCount,
    bindingFlags);
}

auto DescriptorSetLayoutMaker::buffer(
  uint32_t binding, uint32_t descriptorCount, vk::DescriptorBindingFlagsEXT bindingFlags)
  -> DescriptorSetLayoutMaker & {
  return descriptor(
    binding, descriptor::eStorageBuffer, currentStage, descriptorCount, bindingFlags);
}

auto DescriptorSetLayoutMaker::sampler2D(
  uint32_t binding, uint32_t descriptorCount, vk::DescriptorBindingFlagsEXT bindingFlags)
  -> DescriptorSetLayoutMaker & {
  return descriptor(
    binding, descriptor::eCombinedImageSampler, currentStage, descriptorCount,
    bindingFlags);
}

auto DescriptorSetLayoutMaker::input(
  uint32_t binding, uint32_t descriptorCount, vk::DescriptorBindingFlagsEXT bindingFlags)
  -> DescriptorSetLayoutMaker & {
  return descriptor(
    binding, descriptor::eInputAttachment, currentStage, descriptorCount, bindingFlags);
}

auto DescriptorSetLayoutMaker::storageImage(
  uint32_t binding, uint32_t descriptorCount, vk::DescriptorBindingFlagsEXT bindingFlags)
  -> DescriptorSetLayoutMaker & {
  return descriptor(
    binding, descriptor::eStorageImage, currentStage, descriptorCount, bindingFlags);
}

auto DescriptorSetLayoutMaker::accelerationStructure(
  uint32_t binding, uint32_t descriptorCount, vk::DescriptorBindingFlagsEXT bindingFlags)
  -> DescriptorSetLayoutMaker & {
  return descriptor(
    binding, descriptor::eAccelerationStructureNV, currentStage, descriptorCount,
    bindingFlags);
}

auto DescriptorSetLayoutMaker::createUnique(const vk::Device &device) const
  -> vk::UniqueDescriptorSetLayout {
  vk::DescriptorSetLayoutCreateInfo layoutInfo;
  layoutInfo.bindingCount = uint32_t(_bindings.size());
  layoutInfo.pBindings = _bindings.data();
  vk::DescriptorSetLayoutBindingFlagsCreateInfoEXT bindingFlags;
  if(useDescriptorIndexing) {
    bindingFlags.bindingCount = uint32_t(flags.size());
    bindingFlags.pBindingFlags = flags.data();
    layoutInfo.pNext = &bindingFlags;
    if(updateAfterBindPool)
      layoutInfo.flags = vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPoolEXT;
  }
  errorIf(
    variableDescriptorBinding != -1 &&
      variableDescriptorBinding !=
        std::max_element(
          _bindings.begin(), _bindings.end(),
          [](const auto &a, const auto &b) { return a.binding < b.binding; })
          ->binding,
    "varialbe descriptor should only be the last binding!");
  return device.createDescriptorSetLayoutUnique(layoutInfo);
}

auto DescriptorSetLayoutMaker::getVariableDescriptorBinding() const -> int {
  return variableDescriptorBinding;
}

auto DescriptorSetLayoutMaker::getVariableDescriptorCount() const -> uint32_t {
  return variableDescriptorBinding == -1 ?
           0 :
           _bindings[variableDescriptorBinding].descriptorCount;
}
auto DescriptorSetLayoutMaker::bindings() const
  -> const std::vector<vk::DescriptorSetLayoutBinding> & {
  return _bindings;
}

auto DescriptorSetMaker::layout(vk::DescriptorSetLayout layout) -> DescriptorSetMaker & {
  layouts.push_back(layout);
  variableDescriptorCounts.push_back(0);
  return *this;
}

auto DescriptorSetMaker::layout(
  vk::DescriptorSetLayout layout, uint32_t variableDescriptorCount)
  -> DescriptorSetMaker & {
  layouts.push_back(layout);
  variableDescriptorCounts.push_back(variableDescriptorCount);
  useVariableCounts = useVariableCounts || variableDescriptorCount > 0;
  return *this;
}

auto DescriptorSetMaker::create(
  const vk::Device &device, vk::DescriptorPool descriptorPool) const
  -> std::vector<vk::DescriptorSet> {
  vk::DescriptorSetAllocateInfo allocateInfo;
  allocateInfo.descriptorPool = descriptorPool;
  allocateInfo.descriptorSetCount = uint32_t(layouts.size());
  allocateInfo.pSetLayouts = layouts.data();
  vk::DescriptorSetVariableDescriptorCountAllocateInfoEXT info;
  if(useVariableCounts) {
    info.descriptorSetCount = uint32_t(variableDescriptorCounts.size());
    info.pDescriptorCounts = variableDescriptorCounts.data();
    allocateInfo.pNext = &info;
  }
  return device.allocateDescriptorSets(allocateInfo);
}

auto DescriptorSetMaker::createUnique(
  const vk::Device &device, vk::DescriptorPool descriptorPool) const
  -> std::vector<vk::UniqueDescriptorSet> {
  vk::DescriptorSetAllocateInfo allocateInfo;
  allocateInfo.descriptorPool = descriptorPool;
  allocateInfo.descriptorSetCount = uint32_t(layouts.size());
  allocateInfo.pSetLayouts = layouts.data();
  return device.allocateDescriptorSetsUnique(allocateInfo);
}

/**
 * \brief 
 * \param updater 
 * \param index 
 */
DescriptorSetUpdater::BufferUpdater::BufferUpdater(
  DescriptorSetUpdater &updater, uint32_t index)
  : updater{updater}, index{index} {}

auto DescriptorSetUpdater::BufferUpdater::buffer(
  vk::Buffer buffer, vk::DeviceSize offset, vk::DeviceSize range) -> BufferUpdater & {
  if(updater.numBuffers < updater.bufferInfo.size()) {
    updater.descriptorWrites[index].descriptorCount++;
    updater.bufferInfo[updater.numBuffers++] =
      vk::DescriptorBufferInfo{buffer, offset, range};
  } else
    error("exceeding max number of buffers!");
  return *this;
}

DescriptorSetUpdater::ImageUpdater::ImageUpdater(
  DescriptorSetUpdater &updater, uint32_t index)
  : updater{updater}, index{index} {}

auto DescriptorSetUpdater::ImageUpdater::image(
  vk::Sampler sampler, vk::ImageView imageView, vk::ImageLayout imageLayout)
  -> ImageUpdater & {
  if(updater.numImages < updater.imageInfo.size()) {
    updater.descriptorWrites[index].descriptorCount++;
    updater.imageInfo[updater.numImages++] =
      vk::DescriptorImageInfo{sampler, imageView, imageLayout};
  } else
    error("exceeding max number of images!");
  return *this;
}

DescriptorSetUpdater::AccelerationStructureUpdater::AccelerationStructureUpdater(
  DescriptorSetUpdater &updater, uint32_t index)
  : updater{updater}, index{index} {}

auto DescriptorSetUpdater::AccelerationStructureUpdater::accelerationStructure(
  const vk::AccelerationStructureNV &as) -> AccelerationStructureUpdater & {
  if(updater.numAccelerationStructures < updater.accelerationStructureInfo.size()) {
    updater.descriptorWrites[index].descriptorCount++;
    updater.descriptorWrites[index].pNext =
      updater.accelerationStructureInfo.data() + updater.numAccelerationStructures;
    updater.accelerationStructureInfo[updater.numAccelerationStructures++] = {1, &as};
  } else
    error("exceeding max number of Acceleration Structures!");
  return *this;
}

DescriptorSetUpdater::DescriptorSetUpdater(
  size_t maxBuffers, size_t maxImages, size_t maxBufferViews,
  size_t maxAccelerationStructures) {
  bufferInfo.resize(maxBuffers);
  imageInfo.resize(maxImages);
  bufferViews.resize(maxBufferViews);
  accelerationStructureInfo.resize(maxAccelerationStructures);
}

auto DescriptorSetUpdater::beginImages(
  uint32_t dstBinding, uint32_t dstArrayElement, vk::DescriptorType descriptorType)
  -> ImageUpdater {
  vk::WriteDescriptorSet writeDescriptorSet{
    {}, dstBinding, dstArrayElement, 0, descriptorType, imageInfo.data() + numImages};
  descriptorWrites.push_back(writeDescriptorSet);
  return ImageUpdater(*this, uint32_t(descriptorWrites.size() - 1));
}

auto DescriptorSetUpdater::images(
  uint32_t dstBinding, uint32_t dstArrayElement, vk::DescriptorType descriptorType,
  uint32_t descriptorCount, vk::DescriptorImageInfo *pImageInfo)
  -> DescriptorSetUpdater & {
  vk::WriteDescriptorSet writeDescriptorSet{
    {}, dstBinding, dstArrayElement, descriptorCount, descriptorType, pImageInfo};
  descriptorWrites.push_back(writeDescriptorSet);
  return *this;
}

auto DescriptorSetUpdater::sampler2D(
  uint32_t dstBinding, uint32_t dstArrayElement, uint32_t descriptorCount,
  vk::DescriptorImageInfo *pImageInfo) -> DescriptorSetUpdater & {
  return images(
    dstBinding, dstArrayElement, descriptor::eCombinedImageSampler, descriptorCount,
    pImageInfo);
}

auto DescriptorSetUpdater::image(
  uint32_t dstBinding, uint32_t dstArrayElement, vk::DescriptorType descriptorType,
  vk::Sampler sampler, vk::ImageView imageView, vk::ImageLayout imageLayout)
  -> DescriptorSetUpdater & {
  beginImages(dstBinding, dstArrayElement, descriptorType)
    .image(sampler, imageView, imageLayout);
  return *this;
}

auto DescriptorSetUpdater::input(uint32_t dstBinding, vk::ImageView imageView)
  -> DescriptorSetUpdater & {
  return image(
    dstBinding, 0, descriptor::eInputAttachment, {}, imageView,
    vk::ImageLayout::eShaderReadOnlyOptimal);
}

auto DescriptorSetUpdater::storageImage(uint32_t dstBinding, vk::ImageView imageView)
  -> DescriptorSetUpdater & {
  return image(
    dstBinding, 0, descriptor::eStorageImage, {}, imageView, vk::ImageLayout::eGeneral);
}

auto DescriptorSetUpdater::beginBuffers(
  uint32_t dstBinding, uint32_t dstArrayElement, vk::DescriptorType descriptorType)
  -> BufferUpdater {
  vk::WriteDescriptorSet writeDescriptorSet{{},
                                            dstBinding,
                                            dstArrayElement,
                                            0,
                                            descriptorType,
                                            nullptr,
                                            bufferInfo.data() + numBuffers};
  descriptorWrites.push_back(writeDescriptorSet);
  return BufferUpdater(*this, uint32_t(descriptorWrites.size() - 1));
}

auto DescriptorSetUpdater::buffer(
  uint32_t dstBinding, uint32_t dstArrayElement, uint32_t descriptorCount,
  vk::DescriptorBufferInfo *pBufferInfo) -> DescriptorSetUpdater & {
  vk::WriteDescriptorSet writeDescriptorSet{
    {},      dstBinding, dstArrayElement, descriptorCount, descriptor::eStorageBuffer,
    nullptr, pBufferInfo};
  descriptorWrites.push_back(writeDescriptorSet);
  return *this;
}

auto DescriptorSetUpdater::buffer(
  uint32_t dstBinding, uint32_t dstArrayElement, vk::DescriptorType descriptorType,
  vk::Buffer buffer, vk::DeviceSize offset, vk::DeviceSize range)
  -> DescriptorSetUpdater & {
  beginBuffers(dstBinding, dstArrayElement, descriptorType).buffer(buffer, offset, range);
  return *this;
}

auto DescriptorSetUpdater::uniform(uint32_t dstBinding, vk::Buffer buffer)
  -> DescriptorSetUpdater & {
  return this->buffer(
    dstBinding, 0, descriptor::eUniformBuffer, buffer, 0, VK_WHOLE_SIZE);
}

auto DescriptorSetUpdater::uniformDynamic(uint32_t dstBinding, vk::Buffer buffer)
  -> DescriptorSetUpdater & {
  return this->buffer(
    dstBinding, 0, descriptor::eUniformBufferDynamic, buffer, 0, VK_WHOLE_SIZE);
}

auto DescriptorSetUpdater::buffer(uint32_t dstBinding, vk::Buffer buffer)
  -> DescriptorSetUpdater & {
  return this->buffer(
    dstBinding, 0, descriptor::eStorageBuffer, buffer, 0, VK_WHOLE_SIZE);
}

auto DescriptorSetUpdater::beginAccelerationStructures(
  uint32_t dstBinding, uint32_t dstArrayElement) -> AccelerationStructureUpdater {
  vk::WriteDescriptorSet writeDescriptorSet{
    {}, dstBinding, dstArrayElement, 0, descriptor::eAccelerationStructureNV};
  descriptorWrites.push_back(writeDescriptorSet);
  return AccelerationStructureUpdater(*this, uint32_t(descriptorWrites.size() - 1));
}

auto DescriptorSetUpdater::accelerationStructure(
  uint32_t dstBinding, const vk::AccelerationStructureNV &as) -> DescriptorSetUpdater & {
  beginAccelerationStructures(dstBinding, 0).accelerationStructure(as);
  return *this;
}

auto DescriptorSetUpdater::update(
  const vk::Device &device, const vk::DescriptorSet &descriptorSet) -> void {
  for(auto &write: descriptorWrites)
    write.dstSet = descriptorSet;
  for(auto &copy: descriptorCopies)
    copy.dstSet = descriptorSet;
  device.updateDescriptorSets(descriptorWrites, descriptorCopies);
}

auto DescriptorSetUpdater::reset() -> void {
  descriptorWrites.clear();
  descriptorCopies.clear();
  numBuffers = 0;
  numImages = 0;
  numBufferViews = 0;
  numAccelerationStructures = 0;
}
}

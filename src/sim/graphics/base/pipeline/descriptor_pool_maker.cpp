#include "descriptor_pool_maker.h"
namespace sim::graphics {

DescriptorPoolMaker &DescriptorPoolMaker::pipelineLayout(PipelineLayoutDef &def) {
  for(auto setDef: def.layoutDef().setDefs())
    setLayout(*setDef);
  _numSets += def.numSets();
  return *this;
}
DescriptorPoolMaker &DescriptorPoolMaker::setLayout(DescriptorSetDef &def) {
  setLayout(def.layoutDef());
  return *this;
}
void DescriptorPoolMaker::setLayout(const DescriptorSetLayoutMaker &setDef) {
  for(auto &binding: setDef.bindings()) {
    switch(binding.descriptorType) {
      case vk::DescriptorType::eSampler: _numSampler += binding.descriptorCount; break;
      case vk::DescriptorType::eCombinedImageSampler:
        _numCombinedImageSampler += binding.descriptorCount;
        break;
      case vk::DescriptorType::eSampledImage:
        _numSampledImage += binding.descriptorCount;
        break;
      case vk::DescriptorType::eStorageImage:
        _numStorageImage += binding.descriptorCount;
        break;
      case vk::DescriptorType::eUniformTexelBuffer:
        _numUniformTexelBuffer += binding.descriptorCount;
        break;
      case vk::DescriptorType::eStorageTexelBuffer:
        _numStorageTexelBuffer += binding.descriptorCount;
        break;
      case vk::DescriptorType::eUniformBuffer:
        _numUniformBuffer += binding.descriptorCount;
        break;
      case vk::DescriptorType::eStorageBuffer:
        _numStorageBuffer += binding.descriptorCount;
        break;
      case vk::DescriptorType::eUniformBufferDynamic:
        _numUniformDynamic += binding.descriptorCount;
        break;
      case vk::DescriptorType::eStorageBufferDynamic:
        _numStorageBufferDynamic += binding.descriptorCount;
        break;
      case vk::DescriptorType::eInputAttachment:
        _numInputAttachment += binding.descriptorCount;
        break;
      case vk::DescriptorType::eInlineUniformBlockEXT:
        _numInlineUniformBlock += binding.descriptorCount;
        break;
      case vk::DescriptorType::eAccelerationStructureNV:
        _numAccelerationStructure += binding.descriptorCount;
        break;
    }
  }
}
DescriptorPoolMaker &DescriptorPoolMaker::uniform(uint32_t num) {
  _numUniformBuffer += num;
  return *this;
}
DescriptorPoolMaker &DescriptorPoolMaker::uniformDynamic(uint32_t num) {
  _numUniformDynamic += num;
  return *this;
}
DescriptorPoolMaker &DescriptorPoolMaker::buffer(uint32_t num) {
  _numStorageBuffer += num;
  return *this;
}
DescriptorPoolMaker &DescriptorPoolMaker::combinedImageSampler(uint32_t num) {
  _numCombinedImageSampler += num;
  return *this;
}
DescriptorPoolMaker &DescriptorPoolMaker::input(uint32_t num) {
  _numInputAttachment += num;
  return *this;
}
DescriptorPoolMaker &DescriptorPoolMaker::storageImage(uint32_t num) {
  _numStorageImage += num;
  return *this;
}
DescriptorPoolMaker &DescriptorPoolMaker::accelerationStructure(uint32_t num) {
  _numAccelerationStructure += num;
  return *this;
}
DescriptorPoolMaker &DescriptorPoolMaker::sampler(uint32_t numSampler) {
  _numSampler = numSampler;
  return *this;
}
DescriptorPoolMaker &DescriptorPoolMaker::sampledImage(uint32_t numSampledImage) {
  _numSampledImage = numSampledImage;
  return *this;
}
DescriptorPoolMaker &DescriptorPoolMaker::uniformTexelBuffer(
  uint32_t numUniformTexelBuffer) {
  _numUniformTexelBuffer = numUniformTexelBuffer;
  return *this;
}
DescriptorPoolMaker &DescriptorPoolMaker::storageTexelBuffer(
  uint32_t numStorageTexelBuffer) {
  _numStorageTexelBuffer = numStorageTexelBuffer;
  return *this;
}
DescriptorPoolMaker &DescriptorPoolMaker::storageBufferDynamic(
  uint32_t numStorageBufferDynamic) {
  _numStorageBufferDynamic = numStorageBufferDynamic;
  return *this;
}
DescriptorPoolMaker &DescriptorPoolMaker::inlineUniformBlock(
  uint32_t numInlineUniformBlock) {
  _numInlineUniformBlock = numInlineUniformBlock;
  return *this;
}
auto DescriptorPoolMaker::set(uint32_t numSets) -> DescriptorPoolMaker & {
  _numSets += numSets;
  return *this;
}

vk::UniqueDescriptorPool DescriptorPoolMaker::createUnique(const vk::Device &device) {

  std::vector<vk::DescriptorPoolSize> poolSizes{};
  if(_numSampler > 0) poolSizes.emplace_back(vk::DescriptorType::eSampler, _numSampler);
  if(_numCombinedImageSampler > 0)
    poolSizes.emplace_back(
      vk::DescriptorType::eCombinedImageSampler, _numCombinedImageSampler);
  if(_numSampledImage > 0)
    poolSizes.emplace_back(vk::DescriptorType::eSampledImage, _numSampledImage);
  if(_numStorageImage > 0)
    poolSizes.emplace_back(vk::DescriptorType::eStorageImage, _numStorageImage);
  if(_numUniformTexelBuffer > 0)
    poolSizes.emplace_back(
      vk::DescriptorType::eUniformTexelBuffer, _numUniformTexelBuffer);
  if(_numStorageTexelBuffer > 0)
    poolSizes.emplace_back(
      vk::DescriptorType::eStorageTexelBuffer, _numStorageTexelBuffer);
  if(_numUniformBuffer > 0)
    poolSizes.emplace_back(vk::DescriptorType::eUniformBuffer, _numUniformBuffer);
  if(_numStorageBuffer > 0)
    poolSizes.emplace_back(vk::DescriptorType::eStorageBuffer, _numStorageBuffer);
  if(_numUniformDynamic > 0)
    poolSizes.emplace_back(vk::DescriptorType::eUniformBufferDynamic, _numUniformDynamic);
  if(_numStorageBufferDynamic > 0)
    poolSizes.emplace_back(
      vk::DescriptorType::eStorageBufferDynamic, _numStorageBufferDynamic);
  if(_numInputAttachment > 0)
    poolSizes.emplace_back(vk::DescriptorType::eInputAttachment, _numInputAttachment);
  if(_numInlineUniformBlock > 0)
    poolSizes.emplace_back(
      vk::DescriptorType::eInlineUniformBlockEXT, _numInlineUniformBlock);
  if(_numAccelerationStructure > 0)
    poolSizes.emplace_back(
      vk::DescriptorType::eAccelerationStructureNV, _numAccelerationStructure);
  vk::DescriptorPoolCreateInfo descriptorPoolInfo{
    {}, _numSets, (uint32_t)poolSizes.size(), poolSizes.data()};
  return device.createDescriptorPoolUnique(descriptorPoolInfo);
}
}
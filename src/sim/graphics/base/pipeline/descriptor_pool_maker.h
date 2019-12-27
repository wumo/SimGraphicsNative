#pragma once
#include "descriptors.h"
#include "pipeline.h"

namespace sim::graphics {
class DescriptorPoolMaker {
public:
  DescriptorPoolMaker &pipelineLayout(PipelineLayoutDef &def);
  DescriptorPoolMaker &setLayout(DescriptorSetDef &def);
  DescriptorPoolMaker &sampler(uint32_t numSampler);
  DescriptorPoolMaker &sampledImage(uint32_t numSampledImage);
  DescriptorPoolMaker &uniformTexelBuffer(uint32_t numUniformTexelBuffer);
  DescriptorPoolMaker &storageTexelBuffer(uint32_t numStorageTexelBuffer);
  DescriptorPoolMaker &storageBufferDynamic(uint32_t numStorageBufferDynamic);
  DescriptorPoolMaker &inlineUniformBlock(uint32_t numInlineUniformBlock);
  DescriptorPoolMaker &uniform(uint32_t num);
  DescriptorPoolMaker &uniformDynamic(uint32_t num);
  DescriptorPoolMaker &buffer(uint32_t num);
  DescriptorPoolMaker &combinedImageSampler(uint32_t num);
  DescriptorPoolMaker &input(uint32_t num);
  DescriptorPoolMaker &storageImage(uint32_t num);
  DescriptorPoolMaker &accelerationStructure(uint32_t num);
  auto set(uint32_t numSets) -> DescriptorPoolMaker &;
  vk::UniqueDescriptorPool createUnique(const vk::Device &device);

private:
  void setLayout(const DescriptorSetLayoutMaker &setDef);

  uint32_t _numSampler{0}, _numCombinedImageSampler{0}, _numSampledImage{0},
    _numStorageImage{0}, _numUniformTexelBuffer{0}, _numStorageTexelBuffer{0},
    _numUniformBuffer{0}, _numStorageBuffer{0}, _numUniformDynamic{0},
    _numStorageBufferDynamic{0}, _numInputAttachment{0}, _numInlineUniformBlock{0},
    _numAccelerationStructure{0};
  uint32_t _numSets{0};
};
}
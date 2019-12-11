#pragma once
#include "sim/graphics/base/resource/buffers.h"
#include "sim/graphics/base/pipeline/shader.h"
#include "sim/graphics/base/pipeline/pipeline.h"
#include "sim/graphics/base/vkcommon.h"

namespace sim::graphics::renderer::defaultRenderer {
using UniqueRayTracingPipeline =
  vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderDynamic>;

struct ShaderBindingTable {
  uPtr<HostRayTracingBuffer> shaderBindingTable;
  vk::DeviceSize rayGenOffset, missGroupOffset, missGroupStride, hitGroupOffset,
    hitGroupStride;
};

struct ShaderGroup {
  ShaderModule *closestHit{nullptr};
  ShaderModule *intersection{nullptr};
  ShaderModule *anyHit{nullptr};
};

class RayTracingPipelineMaker {
public:
  explicit RayTracingPipelineMaker(Device &vkContext);
  RayTracingPipelineMaker &rayGenGroup(
    const std::string &filename,
    const vk::SpecializationInfo *pSpecializationInfo = nullptr);
  RayTracingPipelineMaker &rayGenGroup(
    const uint32_t *opcodes, size_t size,
    const vk::SpecializationInfo *pSpecializationInfo = nullptr);
  RayTracingPipelineMaker &missGroup(
    const std::string &filename,
    const vk::SpecializationInfo *pSpecializationInfo = nullptr);
  RayTracingPipelineMaker &missGroup(
    const uint32_t *opcodes, size_t size,
    const vk::SpecializationInfo *pSpecializationInfo = nullptr);
  RayTracingPipelineMaker &hitGroup(ShaderGroup group = {});

  uint32_t add(
    const vk::ShaderStageFlagBits &stageFlagBits, vk::ShaderModule &shader,
    const vk::SpecializationInfo *pSpecializationInfo = nullptr);

  RayTracingPipelineMaker &maxRecursionDepth(uint32_t maxRecursionDepth);
  std::tuple<UniqueRayTracingPipeline, ShaderBindingTable> createUnique(
    const vk::PhysicalDeviceRayTracingPropertiesNV &rayTracingProperties,
    const vk::PipelineLayout &pipelineLayout,
    const vk::PipelineCache &pipelineCache = nullptr);

private:
  void addShaderAndGroup(
    const vk::ShaderStageFlagBits &stage, ShaderModule &&shader,
    const vk::SpecializationInfo *pSpecializationInfo = nullptr);

  const vk::Device &device;
  const VmaAllocator &allocator;

  std::vector<ShaderModule> modules;
  std::vector<vk::PipelineShaderStageCreateInfo> shaders;
  std::vector<vk::RayTracingShaderGroupCreateInfoNV> groups;
  uint32_t numMissGroups{0};
  uint32_t _maxRecursionDepth{1};
  bool hitGroupBegun{false};
};
}
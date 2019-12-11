#pragma once
#include "vkcommon.h"
#include "config.h"
#include <vector>
#include <set>

namespace sim::graphics {
struct QueueInfo {
  /**queue family index*/
  uint32_t index{VK_QUEUE_FAMILY_IGNORED};
  vk::Queue queue;
};

class VulkanBase;

class Device {
public:
  __only_move__(Device);

  explicit Device(VulkanBase &framework);

  const vk::PhysicalDevice &getPhysicalDevice() const;
  const vk::PhysicalDeviceMemoryProperties &getMemProps() const;
  const vk::PhysicalDeviceLimits &getLimits() const;
  const vk::CommandPool &getGraphicsCMDPool() const;
  const vk::CommandPool &getComputeCmdPool() const;
  const vk::CommandPool &getTransferCmdPool() const;
  const vk::Device &getDevice() const;
  const QueueInfo &getGraphics() const;
  const vk::Queue &graphicsQueue() const;
  const QueueInfo &getPresent() const;
  const vk::Queue &presentQueue() const;
  const QueueInfo &getCompute() const;
  const vk::Queue &computeQueue() const;
  const QueueInfo &getTransfer() const;
  const vk::Queue &transferQueue() const;
  const VmaAllocator &allocator() const;
  const vk::PhysicalDeviceRayTracingPropertiesNV &getRayTracingProperties() const;

  void executeImmediately(
    const std::function<void(vk::CommandBuffer cb)> &func,
    uint64_t timeout = std::numeric_limits<uint64_t>::max());

private:
  vk::PhysicalDevice physicalDevice;
  vk::PhysicalDeviceMemoryProperties memProps;
  vk::PhysicalDeviceLimits limits;
  vk::UniqueDevice device;

  VmaVulkanFunctions vulkanFunctions{};
  using UniqueAllocator =
    std::unique_ptr<VmaAllocator, std::function<void(VmaAllocator *)>>;
  UniqueAllocator _allocator;

  FeatureConfig featureConfig;
  vk::PhysicalDeviceFeatures features;
  vk::PhysicalDeviceFeatures2 features2;

  vk::PhysicalDeviceRayTracingPropertiesNV rayTracingProperties;

  vk::UniqueCommandPool graphicsCmdPool;
  QueueInfo graphics;
  QueueInfo present;
  vk::UniqueCommandPool computeCmdPool;
  QueueInfo compute;
  vk::UniqueCommandPool transferCmdPool;
  QueueInfo transfer;

  void createAllocator();
};
}
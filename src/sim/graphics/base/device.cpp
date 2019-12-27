#include "device.h"
#include "vulkan_base.h"
#include "sim/util/syntactic_sugar.h"

#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#include "vk_mem_alloc.h"

namespace sim::graphics {
auto findQueueFamily(
  const vk::PhysicalDevice &physicalDevice, const vk::SurfaceKHR &surface) {
  auto queueFamilies = physicalDevice.getQueueFamilyProperties();
  using Flag = vk::QueueFlagBits;

  auto search = [&](vk::QueueFlagBits queueFlag) {
    auto best = VK_QUEUE_FAMILY_IGNORED;
    for(uint32_t i = 0; i < queueFamilies.size(); i++) {
      auto flag = queueFamilies[i].queueFlags;
      if(!(flag & queueFlag)) continue;
      if(best == VK_QUEUE_FAMILY_IGNORED) best = i;
      switch(queueFlag) {
        case vk::QueueFlagBits::eGraphics: return i;
        case vk::QueueFlagBits::eCompute:
          // Dedicated queue for compute
          // Try to find a queue family index that supports compute but not graphics
          if(!(flag & Flag::eGraphics)) return i;
          break;
        case vk::QueueFlagBits::eTransfer:
          // Dedicated queue for transfer
          // Try to find a queue family index that supports transfer but not graphics and compute
          if(!(flag & Flag::eGraphics) && !(flag & Flag::eCompute)) return i;
          break;
        case vk::QueueFlagBits::eSparseBinding:
        case vk::QueueFlagBits::eProtected: return i;
      }
    }
    errorIf(best == VK_QUEUE_FAMILY_IGNORED, "failed to find required queue family");
    return best;
  };
  QueueInfo graphics{search(Flag::eGraphics)};
  QueueInfo compute{search(Flag::eCompute)};
  QueueInfo transfer{search(Flag::eTransfer)};
  QueueInfo present{VK_QUEUE_FAMILY_IGNORED};
  for(uint32_t i = 0; i < queueFamilies.size(); i++)
    if(physicalDevice.getSurfaceSupportKHR(i, surface)) present.index = i;
  errorIf(present.index == VK_QUEUE_FAMILY_IGNORED, "failed to find present family!");
  debugLog(
    "Queue Family:", "graphics[", graphics.index, "]", "compute[", compute.index, "]",
    "transfer[", transfer.index, "]", "present[", present.index, "]");
  return std::make_tuple(graphics, compute, transfer, present);
}

void checkDeviceExtensionSupport(
  const vk::PhysicalDevice &device, std::vector<const char *> &requiredExtensions) {
  std::set<std::string> availableExtensions;
  for(auto extension: device.enumerateDeviceExtensionProperties())
    availableExtensions.insert(extension.extensionName);
  for(auto required: requiredExtensions) {
    errorIf(
      !contains(availableExtensions, required), "required device extension:", required,
      "is not supported!");
    debugLog(required);
  }
}

Device::Device(VulkanBase &framework) {
  auto &instance = *framework.vkInstance;
  auto &surface = *framework.surface;
  auto physicalDevices = instance.enumeratePhysicalDevices();
  errorIf(physicalDevices.empty(), "failed to find GPUs with Vulkan support!");
  physicalDevice = physicalDevices[0];

  std::tie(graphics, compute, transfer, present) =
    findQueueFamily(physicalDevice, surface);

  limits = physicalDevice.getProperties().limits;
  memProps = physicalDevice.getMemoryProperties();
  std::set<uint32_t> indices{graphics.index, present.index, compute.index,
                             transfer.index};
  float priorities[] = {0.0f};
  std::vector<vk::DeviceQueueCreateInfo> queueInfos;
  queueInfos.reserve(indices.size());
  for(auto index: indices)
    queueInfos.emplace_back(vk::DeviceQueueCreateFlags{}, index, 1, priorities);

  features = physicalDevice.getFeatures();

  bool useFeature2{false};

  std::vector<const char *> deviceExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  featureConfig = framework.featureConfig;
  if(featureConfig & FeatureConfig::DedicatedAllocation)
    deviceExtensions.push_back(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);

  if(featureConfig & FeatureConfig::DescriptorIndexing) {
    useFeature2 = true;
    auto featuresExt = physicalDevice.getFeatures2<
      vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceDescriptorIndexingFeaturesEXT>();
    auto descriptorFeatures =
      featuresExt.get<vk::PhysicalDeviceDescriptorIndexingFeaturesEXT>();
    errorIf(
      !descriptorFeatures.descriptorBindingSampledImageUpdateAfterBind ||
        !descriptorFeatures.descriptorBindingVariableDescriptorCount ||
        !descriptorFeatures.descriptorBindingPartiallyBound ||
        !descriptorFeatures.descriptorBindingStorageImageUpdateAfterBind,
      "descriptor features not satisfied!");

    features2 = featuresExt.get<vk::PhysicalDeviceFeatures2>();

    deviceExtensions.push_back(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
    deviceExtensions.push_back(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
    deviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
  }
  if(featureConfig & FeatureConfig::RayTracing) {
    deviceExtensions.push_back(VK_NV_RAY_TRACING_EXTENSION_NAME);

    auto result = physicalDevice.getProperties2<
      vk::PhysicalDeviceProperties2, vk::PhysicalDeviceRayTracingPropertiesNV>();
    rayTracingProperties = result.get<vk::PhysicalDeviceRayTracingPropertiesNV>();
  }

  checkDeviceExtensionSupport(physicalDevice, deviceExtensions);

  vk::DeviceCreateInfo deviceInfo;
  deviceInfo.pNext = useFeature2 ? &features2 : nullptr;
  deviceInfo.queueCreateInfoCount = uint32_t(queueInfos.size());
  deviceInfo.pQueueCreateInfos = queueInfos.data();
  deviceInfo.enabledExtensionCount = uint32_t(deviceExtensions.size());
  deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();
  deviceInfo.pEnabledFeatures = useFeature2 ? nullptr : &features;

  device = physicalDevice.createDeviceUnique(deviceInfo);
  graphics.queue = device->getQueue(graphics.index, 0);
  compute.queue = device->getQueue(compute.index, 0);
  transfer.queue = device->getQueue(transfer.index, 0);
  present.queue = device->getQueue(present.index, 0);

  graphicsCmdPool = device->createCommandPoolUnique(vk::CommandPoolCreateInfo{
    {vk::CommandPoolCreateFlagBits::eResetCommandBuffer}, graphics.index});
  computeCmdPool = device->createCommandPoolUnique(vk::CommandPoolCreateInfo{
    {vk::CommandPoolCreateFlagBits::eResetCommandBuffer}, compute.index});
  transferCmdPool = device->createCommandPoolUnique(vk::CommandPoolCreateInfo{
    {vk::CommandPoolCreateFlagBits::eResetCommandBuffer}, transfer.index});
  presentCmdPool = device->createCommandPoolUnique(vk::CommandPoolCreateInfo{
    {vk::CommandPoolCreateFlagBits::eResetCommandBuffer}, present.index});

  VULKAN_HPP_DEFAULT_DISPATCHER.init(*device);

  createAllocator();
}

void Device::createAllocator() {
  VmaAllocatorCreateInfo createInfo{};
  if(featureConfig & FeatureConfig::DedicatedAllocation)
    createInfo.flags = VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
  createInfo.physicalDevice = physicalDevice;
  createInfo.device = *device;
  auto &dispatcher = VULKAN_HPP_DEFAULT_DISPATCHER;
  vulkanFunctions = {
    dispatcher.vkGetPhysicalDeviceProperties,
    dispatcher.vkGetPhysicalDeviceMemoryProperties,
    dispatcher.vkAllocateMemory,
    dispatcher.vkFreeMemory,
    dispatcher.vkMapMemory,
    dispatcher.vkUnmapMemory,
    dispatcher.vkFlushMappedMemoryRanges,
    dispatcher.vkInvalidateMappedMemoryRanges,
    dispatcher.vkBindBufferMemory,
    dispatcher.vkBindImageMemory,
    dispatcher.vkGetBufferMemoryRequirements,
    dispatcher.vkGetImageMemoryRequirements,
    dispatcher.vkCreateBuffer,
    dispatcher.vkDestroyBuffer,
    dispatcher.vkCreateImage,
    dispatcher.vkDestroyImage,
    dispatcher.vkCmdCopyBuffer,
    dispatcher.vkGetBufferMemoryRequirements2KHR,
    dispatcher.vkGetImageMemoryRequirements2KHR,
    dispatcher.vkBindBufferMemory2KHR,
    dispatcher.vkBindImageMemory2KHR,
  };
  createInfo.pVulkanFunctions = &vulkanFunctions;
  _allocator = UniqueAllocator(new VmaAllocator(), [](VmaAllocator *ptr) {
    vmaDestroyAllocator(*ptr);
    delete ptr;
  });
  auto result = vmaCreateAllocator(&createInfo, _allocator.get());
  errorIf(result != VK_SUCCESS, "failed to create Allocator");
}

void Device::graphicsImmediately(
  const std::function<void(vk::CommandBuffer cb)> &func, uint64_t timeout) {
  executeImmediately(*device, *graphicsCmdPool, graphics.queue, func, timeout);
}

void Device::computeImmediately(
  const std::function<void(vk::CommandBuffer cb)> &func, uint64_t timeout) {
  executeImmediately(*device, *computeCmdPool, compute.queue, func, timeout);
}

void Device::executeImmediately(
  const vk::Device &device, const vk::CommandPool cmdPool, const vk::Queue queue,
  const std::function<void(vk::CommandBuffer cb)> &func, uint64_t timeout) {
  vk::CommandBufferAllocateInfo cmdBufferInfo{cmdPool, vk::CommandBufferLevel::ePrimary,
                                              1};
  auto cmdBuffers = device.allocateCommandBuffers(cmdBufferInfo);
  cmdBuffers[0].begin(
    vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
  func(cmdBuffers[0]);
  cmdBuffers[0].end();

  vk::SubmitInfo submit;
  submit.commandBufferCount = uint32_t(cmdBuffers.size());
  submit.pCommandBuffers = cmdBuffers.data();
  auto fence = device.createFenceUnique(vk::FenceCreateInfo{});
  queue.submit(submit, *fence);
  device.waitForFences(*fence, VK_TRUE, timeout);

  device.freeCommandBuffers(cmdPool, cmdBuffers);
}

const VmaAllocator &Device::allocator() const { return *_allocator; }

const vk::PhysicalDevice &Device::getPhysicalDevice() const { return physicalDevice; }

const vk::PhysicalDeviceMemoryProperties &Device::getMemProps() const { return memProps; }

const vk::CommandPool &Device::getGraphicsCMDPool() const { return *graphicsCmdPool; }

const vk::CommandPool &Device::getComputeCmdPool() const { return *computeCmdPool; }

const vk::CommandPool &Device::getTransferCmdPool() const { return *transferCmdPool; }

const vk::CommandPool &Device::getPresentCmdPool() const { return *presentCmdPool; }

const QueueInfo &Device::getGraphics() const { return graphics; }

const vk::Queue &Device::graphicsQueue() const { return graphics.queue; }

const QueueInfo &Device::getPresent() const { return present; }

const vk::Queue &Device::presentQueue() const { return present.queue; }

const QueueInfo &Device::getCompute() const { return compute; }

const vk::Queue &Device::computeQueue() const { return compute.queue; }

const QueueInfo &Device::getTransfer() const { return transfer; }

const vk::Queue &Device::transferQueue() const { return transfer.queue; }

const vk::Device &Device::getDevice() const { return *device; }

const vk::PhysicalDeviceLimits &Device::getLimits() const { return limits; }

const vk::PhysicalDeviceRayTracingPropertiesNV &Device::getRayTracingProperties() const {
  return rayTracingProperties;
}
}

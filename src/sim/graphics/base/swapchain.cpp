#include "swapchain.h"
#include "vulkan_base.h"
#include <algorithm>

namespace sim::graphics {
vk::SurfaceFormatKHR chooseSurfaceFormat(
  const std::vector<vk::SurfaceFormatKHR> &formats) {
  if(formats.size() == 1 && formats[0].format == vk::Format::eUndefined)
    return {
      VkSurfaceFormatKHR{VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}};
  for(const auto &format: formats)
    if(
      format.format == vk::Format::eB8G8R8A8Unorm &&
      format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
      return format;
  return formats[0];
}

vk::PresentModeKHR choosePresentMode(
  const std::vector<vk::PresentModeKHR> &presentModes, bool vsync) {
  std::set<vk::PresentModeKHR> modes(presentModes.begin(), presentModes.end());
  auto preferredMode = vsync ? vk::PresentModeKHR::eFifo : vk::PresentModeKHR::eMailbox;
  return modes.find(preferredMode) != modes.end() ? preferredMode : *modes.begin();
}

vk::Extent2D chooseExtent(
  GLFWwindow *window, const vk::SurfaceCapabilitiesKHR &capabilities) {
  if(capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    return capabilities.currentExtent;
  uint32_t width, height;
  glfwGetFramebufferSize(window, (int *)&width, (int *)&height);
  vk::Extent2D extent;
  extent.width = std::max<uint32_t>(
    capabilities.minImageExtent.width,
    std::min<uint32_t>(capabilities.maxImageExtent.width, width));
  extent.height = std::max<uint32_t>(
    capabilities.minImageExtent.height,
    std::min<uint32_t>(capabilities.maxImageExtent.height, height));
  return extent;
}

Swapchain::Swapchain(const VulkanBase &framework)
  : base{framework},
    physicalDevice{base.device->getPhysicalDevice()},
    vkDevice{base.device->getDevice()} {
  auto presentModes = physicalDevice.getSurfacePresentModesKHR(*base.surface);
  presentMode = choosePresentMode(presentModes, base.config.vsync);

  auto formats = physicalDevice.getSurfaceFormatsKHR(*base.surface);
  surfaceFormat = chooseSurfaceFormat(formats);

  presentQueue = base.device->getPresent().queue;

  auto cap = physicalDevice.getSurfaceCapabilitiesKHR(*base.surface);
  imageCount = std::clamp(base.config.numFrame, cap.minImageCount, cap.maxImageCount);
}

void Swapchain::resize() {
  auto cap = physicalDevice.getSurfaceCapabilitiesKHR(*base.surface);
  imageExtent = chooseExtent(base.window, cap);

  vk::SwapchainCreateInfoKHR info;
  info.surface = *base.surface;
  info.minImageCount = imageCount;
  info.imageFormat = surfaceFormat.format;
  info.imageColorSpace = surfaceFormat.colorSpace;
  info.imageExtent = imageExtent;
  info.imageArrayLayers = 1;
  info.imageUsage = vk::ImageUsageFlagBits::eColorAttachment |
                    vk::ImageUsageFlagBits::eStorage |
                    vk::ImageUsageFlagBits::eTransferDst;

  auto graphicsIndex = base.device->getGraphics().index;
  auto presentIndex = base.device->getPresent().index;

  uint32_t queueFamilyIndices[]{graphicsIndex, presentIndex};
  if(graphicsIndex != presentIndex) {
    info.imageSharingMode = vk::SharingMode::eConcurrent;
    info.queueFamilyIndexCount = 2;
    info.pQueueFamilyIndices = queueFamilyIndices;
  } else
    info.imageSharingMode = vk::SharingMode::eExclusive;

  info.preTransform = cap.currentTransform;
  info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
  info.presentMode = presentMode;
  info.clipped = true;
  info.oldSwapchain = *swapchain;

  swapchain = vkDevice.createSwapchainKHRUnique(info);

  images = vkDevice.getSwapchainImagesKHR(*swapchain);
  imageViews.resize(images.size());
  errorIf(
    images.size() != imageCount,
    "newly created swapchain's imageCount != old imageCount!");
  for(auto i = 0u; i < imageCount; i++) {
    vk::ImageViewCreateInfo imageViewInfo;
    imageViewInfo.image = images[i];
    imageViewInfo.viewType = vk::ImageViewType::e2D;
    imageViewInfo.format = surfaceFormat.format;
    imageViewInfo.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
    imageViews[i] = vkDevice.createImageViewUnique(imageViewInfo);
  }
}

vk::Result Swapchain::acquireNextImage(
  const vk::Semaphore &imageAvailableSemaphore, uint32_t &imageIndex) {
  return vkDevice.acquireNextImageKHR(
    *swapchain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore,
    vk::Fence(), &imageIndex);
}

vk::Result Swapchain::present(
  const uint32_t &imageIndex, const vk::Semaphore &renderFinishedSemaphore) {
  vk::PresentInfoKHR presentInfo;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &renderFinishedSemaphore;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &(*swapchain);
  presentInfo.pImageIndices = &imageIndex;

  return presentQueue.presentKHR(presentInfo);
}

vk::ImageSubresourceRange Swapchain::subresourceRange() const {
  return {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
}
}

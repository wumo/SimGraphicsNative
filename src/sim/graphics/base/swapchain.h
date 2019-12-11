#pragma once
#include "vkcommon.h"
#include "device.h"

namespace sim::graphics {
class VulkanBase;

class Swapchain {
public:
  explicit Swapchain(const VulkanBase &framework);

  void resize();

  vk::Result acquireNextImage(
    const vk::Semaphore &imageAvailableSemaphore, uint32_t &imageIndex);
  vk::Result present(
    const uint32_t &imageIndex, const vk::Semaphore &renderFinishedSemaphore);

  vk::Format &getImageFormat() { return surfaceFormat.format; }
  const vk::Extent2D &getImageExtent() const { return imageExtent; }
  const std::vector<vk::UniqueImageView> &getImageViews() const { return imageViews; }
  vk::Image &getImage(uint32_t index) { return images[index]; }
  const uint32_t &getImageCount() const { return imageCount; }

private:
  const VulkanBase &base;
  const vk::PhysicalDevice &physicalDevice;
  const vk::Device &vkDevice;

  vk::PresentModeKHR presentMode;
  vk::Queue presentQueue;
  vk::SurfaceFormatKHR surfaceFormat{};

  vk::UniqueSwapchainKHR swapchain;

  uint32_t imageCount{};
  vk::Extent2D imageExtent;
  std::vector<vk::Image> images;
  std::vector<vk::UniqueImageView> imageViews;
};
}

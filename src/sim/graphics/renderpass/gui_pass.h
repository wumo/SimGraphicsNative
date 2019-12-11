#pragma once
#include <cstdint>
#include "sim/graphics/base/vkcommon.h"
#include "sim/graphics/base/device.h"
#include "sim/graphics/base/swapchain.h"

namespace sim::graphics {
class GuiPass {

public:
  GuiPass(
    GLFWwindow *window, vk::Instance &vkInstance, Device &device, Swapchain &swapchain,
    vk::DescriptorPool &descriptorPool);
  ~GuiPass();
  void createGuiPipeline();
  void createGuiRenderPass();
  void resizeGui(vk::Extent2D extent);
  void drawGui(uint32_t imageIndex, vk::CommandBuffer &cb);

private:
  GLFWwindow *window;
  vk::Instance &vkInstance;
  Swapchain &swapchain;
  vk::DescriptorPool &descriptorPool;
  Device &device;
  struct {
    vk::UniquePipelineLayout pipelineLayout;
    vk::UniquePipelineCache pipelineCache;
    vk::UniqueRenderPass renderPass;
    uint32_t subpass;
    vk::UniquePipeline pipeline;
    std::vector<vk::UniqueFramebuffer> framebuffers;
    vk::Extent2D extent;
  } gui;
};
}

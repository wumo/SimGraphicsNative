#pragma once
#include "vkcommon.h"
#include <GLFW/glfw3.h>
#include <string>
#include <set>
#include "config.h"
#include "debug_marker.h"
#include "device.h"
#include "input.h"
#include "swapchain.h"

namespace sim ::graphics {
class CallFrameUpdater {
public:
  virtual void update(uint32_t frameIndex, float elapsedDuration) {
    errorIf(true, "not implemented");
  }
};

class VulkanBase {
  friend class Device;

  friend class Swapchain;

public:
  explicit VulkanBase(
    const Config &config = {}, const FeatureConfig &featureConfig = {},
    const DebugConfig &debugConfig = {});
  virtual ~VulkanBase() = default;

  void run(CallFrameUpdater &updater);
  void run(std::function<void(uint32_t, float)> updater = [](uint32_t, float) {});

  void setWindowTitle(const std::string &title);

  void closeWindow();

  Input input;

protected:
  void createWindow();
  bool createVkInstance();
  void createDebug();
  void createSyncObjects();
  void createCommandBuffers();

  virtual void resize();

  void update(std::function<void(uint32_t, float)> &updater, float dt);

  virtual void updateFrame(
    std::function<void(uint32_t, float)> &updater, uint32_t imageIndex,
    float elapsedDuration) = 0;

  void terminate();

  virtual void dispose() {}

  void checkConfig();

protected:
  std::vector<const char *> validationLayers{"VK_LAYER_KHRONOS_validation"};

  Config config;
  FeatureConfig featureConfig;
  DebugConfig debugConfig;
  GLFWwindow *window{nullptr};

  vk::DynamicLoader dl;
  vk::UniqueInstance vkInstance;
  vk::UniqueSurfaceKHR surface;
  uPtr<Device> device;

  vk::UniqueDebugUtilsMessengerEXT callback;
  DebugMarker debugMarker{};
  uPtr<Swapchain> swapchain;
  vk::Extent2D extent;
  std::vector<vk::CommandBuffer> graphicsCmdBuffers, transferCmdBuffers,
    computeCmdBuffers;
  vk::QueryPool queryPool;

  struct Semaphores {
    vk::UniqueSemaphore imageAvailable;
    vk::UniqueSemaphore transferFinished;
    vk::UniqueSemaphore computeFinished;
    vk::UniqueSemaphore renderFinished;

    std::vector<vk::PipelineStageFlags> renderWaitStages;
    std::vector<vk::Semaphore> renderWaits;
  };

  std::vector<Semaphores> semaphores;
  std::vector<vk::UniqueFence> inFlightFrameFences;
  uint32_t frameIndex{0};
};
}

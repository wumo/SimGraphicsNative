#include "vulkan_base.h"
#include "sim/util/syntactic_sugar.h"
#include "imgui.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

namespace sim::graphics {
using namespace sim;
using cbFlag = vk::CommandBufferUsageFlagBits;
using address = vk::SamplerAddressMode;
using layout = vk::ImageLayout;
using stage = vk::PipelineStageFlagBits;
using access = vk::AccessFlagBits;
using shader = vk::ShaderStageFlagBits;
using aspect = vk::ImageAspectFlagBits;

static auto mouse_move_callback(GLFWwindow *window, double xpos, double ypos) -> void {
  auto input = static_cast<Input *>(glfwGetWindowUserPointer(window));

  if(input->gui) {
    auto &io = ImGui::GetIO();
    if(io.WantCaptureKeyboard || io.WantCaptureMouse) return;
  }
  input->mouseXPos = float(xpos);
  input->mouseYPos = float(ypos);
}

static auto scroll_callback(GLFWwindow *window, double xoffset, double yoffset) -> void {
  auto input = static_cast<Input *>(glfwGetWindowUserPointer(window));
  if(input->gui && ImGui::GetIO().WantCaptureMouse) return;
  input->scrollXOffset += float(xoffset);
  input->scrollYOffset += float(yoffset);
}

static auto mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
  -> void {
  auto input = static_cast<Input *>(glfwGetWindowUserPointer(window));
  if(input->gui && ImGui::GetIO().WantCaptureMouse) return;
  input->mouseButtonPressed[button] = (action == GLFW_PRESS);
}

static auto key_callback(GLFWwindow *window, int key, int, int action, int) -> void {
  auto input = static_cast<Input *>(glfwGetWindowUserPointer(window));
  if(key > GLFW_KEY_LAST || key < 0) return;

  if(input->gui && ImGui::GetIO().WantCaptureKeyboard) return;

  if(action == GLFW_PRESS) input->keyPressed[key] = true;
  else if(action == GLFW_RELEASE)
    input->keyPressed[key] = false;
}

static auto resize_callback(GLFWwindow *window, int w, int h) -> void {
  auto input = static_cast<Input *>(glfwGetWindowUserPointer(window));
  input->resizeWanted = true;
  input->width = w;
  input->height = h;
}

VulkanBase::VulkanBase(
  const Config &config, const FeatureConfig &featureConfig,
  const DebugConfig &debugConfig)
  : config{config}, featureConfig{featureConfig}, debugConfig{debugConfig} {
  input.gui = config.gui;
  createWindow();
  createVkInstance();
  createDebug();
  device = u<Device>(*this);
  checkConfig();
  debugMarker = {device->getDevice()};
  swapchain = u<Swapchain>(*this);
  VulkanBase::resize();

  createSyncObjects();
  createCommandBuffers();
}

void VulkanBase::checkConfig() {
  auto sampleCount = static_cast<vk::SampleCountFlagBits>(config.sampleCount);
  auto count = device->getLimits().framebufferColorSampleCounts &
               device->getLimits().framebufferDepthSampleCounts;
  errorIf(!(count & sampleCount), "sample count not supported!");
}

auto VulkanBase::run(CallFrameUpdater &updater) -> void {
  run([&](uint32_t frameIndex, float elapsedDuration) {
    updater.update(frameIndex, elapsedDuration);
  });
}

auto VulkanBase::run(std::function<void(uint32_t, float)> updater) -> void {
  glfwSetTime(0.0);
  auto prevTime = 0.0;
  while(!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    if(input.resizeWanted) {
      device->getDevice().waitIdle();
      resize();
      input.resizeWanted = false;
    }

    auto curTime = glfwGetTime();
    auto deltaTime = curTime - prevTime;
    prevTime = curTime;

    update(updater, static_cast<float>(deltaTime));
  }
  terminate();
}

void VulkanBase::setWindowTitle(const std::string &title) {
  glfwSetWindowTitle(window, title.c_str());
}

void VulkanBase::closeWindow() { glfwSetWindowShouldClose(window, true); }

auto VulkanBase::terminate() -> void {
  device->getDevice().waitIdle();
  dispose();
  glfwDestroyWindow(window);
  glfwTerminate();
}

auto VulkanBase::createWindow() -> void {
  if(!glfwInit()) return;
  if(!glfwVulkanSupported()) return;

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window =
    glfwCreateWindow(config.width, config.height, config.title.c_str(), nullptr, nullptr);
  glfwSetWindowUserPointer(window, &input);

  glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);
  glfwSetCursorPosCallback(window, mouse_move_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetScrollCallback(window, scroll_callback);
  glfwSetKeyCallback(window, key_callback);
  glfwSetFramebufferSizeCallback(window, resize_callback);
}

void checkExtensionSupport(std::vector<const char *> &requiredExtensions) {
  std::set<std::string> availableExtensions;
  for(auto extension: vk::enumerateInstanceExtensionProperties())
    availableExtensions.insert(extension.extensionName);
  for(auto required: requiredExtensions) {
    errorIf(
      !contains(availableExtensions, required), "required instance extension:", required,
      "is not supported!");
    debugLog(required);
  }
}

auto getGLFWRequiredInstanceExtensions() {
  uint32_t glfwExtensionCount = 0;
  auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  std::vector<const char *> extensions(
    glfwExtensions, glfwExtensions + glfwExtensionCount);
  return extensions;
}

bool VulkanBase::createVkInstance() {
  auto vkGetInstanceProcAddr =
    dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
  VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

  vk::ApplicationInfo appInfo;

  std::vector<const char *> extensions{getGLFWRequiredInstanceExtensions()}, layers;
  if(featureConfig & FeatureConfig::DescriptorIndexing)
    extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
  if(debugConfig.enableValidationLayer) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    layers.insert(layers.begin(), validationLayers.begin(), validationLayers.end());
  }
  checkExtensionSupport(extensions);
  std::vector<vk::ValidationFeatureEnableEXT> enabledFeatures;
  if(debugConfig.enableGPUValidation)
    enabledFeatures.push_back(vk::ValidationFeatureEnableEXT::eGpuAssisted);
  vk::StructureChain<vk::InstanceCreateInfo, vk::ValidationFeaturesEXT> instanceInfo{
    vk::InstanceCreateInfo{{},
                           &appInfo,
                           uint32_t(layers.size()),
                           layers.data(),
                           uint32_t(extensions.size()),
                           extensions.data()},
    vk::ValidationFeaturesEXT{uint32_t(enabledFeatures.size()), enabledFeatures.data()}};
  vkInstance = createInstanceUnique(instanceInfo.get<vk::InstanceCreateInfo>());
  VULKAN_HPP_DEFAULT_DISPATCHER.init(*vkInstance);

  VkSurfaceKHR _surface;
  glfwCreateWindowSurface(*vkInstance, window, nullptr, &_surface);
  surface = vk::UniqueSurfaceKHR{
    _surface, vk::ObjectDestroy<vk::Instance, vk::DispatchLoaderDynamic>{
                *vkInstance, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER}};
  return true;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
  VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
  VkDebugUtilsMessageTypeFlagsEXT messageType,
  const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData) {
  std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

  return VK_FALSE;
}

void VulkanBase::createDebug() {
  if(debugConfig.enableValidationLayer) {
    using severity = vk::DebugUtilsMessageSeverityFlagBitsEXT;
    using msgType = vk::DebugUtilsMessageTypeFlagBitsEXT;
    vk::DebugUtilsMessengerCreateInfoEXT createInfo;
    createInfo.messageSeverity = severity::eVerbose | severity::eWarning |
                                 severity::eError;
    createInfo.messageType = msgType::eGeneral | msgType::eValidation |
                             msgType::ePerformance;
    createInfo.pfnUserCallback = debugCallback;

    callback = vkInstance->createDebugUtilsMessengerEXTUnique(createInfo, nullptr);
  }
  // queryPool = device->getDevice().createQueryPool(
  // vk::QueryPoolCreateInfo{{}, vk::QueryType::eTimestamp, 2});
}

void VulkanBase::createSyncObjects() {
  auto &_device = device->getDevice();
  auto numFrames = swapchain->getImageCount();
  semaphores.resize(numFrames);
  inFlightFrameFences.resize(numFrames);
  vk::FenceCreateInfo fenceInfo{vk::FenceCreateFlagBits::eSignaled};
  for(uint32_t i = 0; i < swapchain->getImageCount(); i++) {
    semaphores[i].imageAvailable = _device.createSemaphoreUnique({});
    semaphores[i].transferFinished = _device.createSemaphoreUnique({});
    semaphores[i].computeFinished = _device.createSemaphoreUnique({});
    semaphores[i].renderFinished = _device.createSemaphoreUnique({});

    semaphores[i].renderWaits.push_back(*semaphores[i].imageAvailable);
    semaphores[i].renderWaits.push_back(*semaphores[i].computeFinished);
    semaphores[i].renderWaitStages.emplace_back(stage::eAllCommands);
    semaphores[i].renderWaitStages.emplace_back(stage::eAllCommands);

    inFlightFrameFences[i] = _device.createFenceUnique(fenceInfo);
  }
}

void VulkanBase::createCommandBuffers() {
  auto &_device = device->getDevice();
  vk::CommandBufferAllocateInfo info{device->getGraphicsCMDPool(),
                                     vk::CommandBufferLevel::ePrimary,
                                     uint32_t(swapchain->getImageCount())};
  graphicsCmdBuffers = _device.allocateCommandBuffers(info);
  info.commandPool = device->getTransferCmdPool();
  transferCmdBuffers = _device.allocateCommandBuffers(info);
  info.commandPool = device->getComputeCmdPool();
  computeCmdBuffers = _device.allocateCommandBuffers(info);
}

void VulkanBase::resize() {
  device->getDevice().waitIdle();
  swapchain->resize();
  extent = swapchain->getImageExtent();
}

void VulkanBase::update(std::function<void(uint32_t, float)> &updater, float dt) {
  auto &vkDevice = device->getDevice();

  auto frameFinishedFence = *inFlightFrameFences[frameIndex];
  auto result =
    vkDevice.waitForFences(frameFinishedFence, 1, std::numeric_limits<uint64_t>::max());
  vkDevice.resetFences(frameFinishedFence);

  uint32_t imageIndex = 0;
  try {
    result =
      swapchain->acquireNextImage(*semaphores[frameIndex].imageAvailable, imageIndex);
    if(result == vk::Result::eSuboptimalKHR) {
      input.resizeWanted = true;
      resize();
      input.resizeWanted = false;
    }
  } catch(const vk::OutOfDateKHRError &) {
    input.resizeWanted = true;
    resize();
    input.resizeWanted = false;
  }

  auto &graphicsCB = graphicsCmdBuffers[imageIndex];
  auto &computeCB = computeCmdBuffers[imageIndex];
  auto &transferCB = transferCmdBuffers[imageIndex];

  auto &swapchainImage = swapchain->getImage(imageIndex);

  transferCB.begin({cbFlag::eSimultaneousUse});
  computeCB.begin({cbFlag ::eSimultaneousUse});
  graphicsCB.begin({cbFlag::eSimultaneousUse});

  // dynamicCmdBuffers[imageIndex].resetQueryPool(queryPool, 0, 2);
  // dynamicCmdBuffers[imageIndex].writeTimestamp(stage::eTopOfPipe, queryPool, 0);
  updateFrame(updater, imageIndex, dt);
  // dynamicCmdBuffers[imageIndex].writeTimestamp(stage::eBottomOfPipe, queryPool, 1);

  transferCB.end();
  computeCB.end();
  graphicsCB.end();

  auto &semaphore = semaphores[frameIndex];

  vk::SubmitInfo submit;
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = &transferCB;
  submit.signalSemaphoreCount = 1;
  submit.pSignalSemaphores = &(*semaphore.transferFinished);
  device->transferQueue().submit(submit, vk::Fence{});

  vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eComputeShader;
  submit.pCommandBuffers = &computeCB;
  submit.waitSemaphoreCount = 1;
  submit.pWaitSemaphores = &(*semaphore.transferFinished);
  submit.pWaitDstStageMask = &waitStage;
  submit.signalSemaphoreCount = 1;
  submit.pSignalSemaphores = &(*semaphore.computeFinished);
  device->computeQueue().submit(submit, vk::Fence{});

  submit.pCommandBuffers = &graphicsCB;
  submit.waitSemaphoreCount = uint32_t(semaphore.renderWaits.size());
  submit.pWaitSemaphores = semaphore.renderWaits.data();
  submit.pWaitDstStageMask = semaphore.renderWaitStages.data();
  submit.signalSemaphoreCount = 1;
  submit.pSignalSemaphores = &(*semaphore.renderFinished);
  device->graphicsQueue().submit(submit, frameFinishedFence);

  try {
    result = swapchain->present(imageIndex, *semaphore.renderFinished);
    if(result == vk::Result::eSuboptimalKHR) {
      input.resizeWanted = true;
      resize();
      input.resizeWanted = false;
    }
  } catch(const vk::OutOfDateKHRError &) {
    input.resizeWanted = true;
    resize();
    input.resizeWanted = false;
  }

  frameIndex = (frameIndex + 1) % swapchain->getImageCount();
  //  device->presentQueue().waitIdle();
  //  device->transferQueue().waitIdle();
  //  device->graphicsQueue().waitIdle();
  //  device->computeQueue().waitIdle();
  device->getDevice().waitIdle();
}
}

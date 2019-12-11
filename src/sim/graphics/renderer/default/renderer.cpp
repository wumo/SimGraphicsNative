#include "renderer.h"

#include <utility>
namespace sim::graphics ::renderer::defaultRenderer {

Renderer::Renderer(
  const Config &config, const ModelConfig &modelLimit, const FeatureConfig &featureConfig,
  const DebugConfig &debugConfig)
  : VulkanBase{config, featureConfig, debugConfig},
    modelLimit{modelLimit},
    vkDevice{device->getDevice()} {
  lights.resize(modelLimit.numLights);

  camBuffer = u<HostUniformBuffer>(device->allocator(), cam);
  debugMarker.name(camBuffer->buffer(), "cam Buffer");

  lightsBuffer = u<HostStorageBuffer>(
    device->allocator(), sizeof(uint32_t) * 4 + modelLimit.numLights * sizeof(Light));
  debugMarker.name(lightsBuffer->buffer(), "lightBuffer");

  recreateResources();
}

Camera &Renderer::camera() { return _camera; }
Light &Renderer::light(uint32_t idx) { return lights.at(idx); }

void Renderer::recreateResources() {
  _camera.w = float(extent.width);
  _camera.h = float(extent.height);

  auto sampleCount = static_cast<vk::SampleCountFlagBits>(config.sampleCount);
  offscreenImage = u<StorageAttachmentImage>(
    *device, extent.width, extent.height, swapchain->getImageFormat(), sampleCount);
  debugMarker.name(offscreenImage->image(), "offscreenImage");
}

void Renderer::resize() {
  VulkanBase::resize();
  recreateResources();
}

void Renderer::updateFrame(
  std::function<void(uint32_t, float)> &updater, uint32_t imageIndex,
  float elapsedDuration) {
  render(updater, imageIndex, elapsedDuration);

  using layout = vk::ImageLayout;
  ImageBase::setLayout(
    graphicsCmdBuffers[imageIndex], swapchain->getImage(imageIndex), layout::eUndefined,
    layout::ePresentSrcKHR, {}, {});

  auto proj = _camera.projection();
  cam.view = _camera.view();
  cam.proj = proj;
  cam.viewInv = glm::inverse(cam.view);
  cam.projInv = glm::inverse(proj);
  cam.eye = glm::vec4(_camera.location, 1);
  cam.v = glm::vec4(normalize(_camera.focus - _camera.location), 1);
  cam.r = glm::vec4(normalize(cross(glm::vec3(cam.v), _camera.worldUp)), 1);
  cam.fov = _camera.fov;
  cam.w = _camera.w;
  cam.h = _camera.h;
  cam.zNear = _camera.zNear;
  cam.zFar = _camera.zFar;
  camBuffer->updateSingle(cam);
  lightsBuffer->updateSingle(uint32_t(lights.size()));
  lightsBuffer->updateVector(lights, 4 * sizeof(uint32_t), modelLimit.numLights);
}

}
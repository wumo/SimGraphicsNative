#pragma once

#include "sim/graphics/base/vulkan_base.h"
#include "sim/graphics/base/resource/buffers.h"
#include "sim/graphics/base/resource/images.h"
#include "sim/graphics/renderer/default/model/light.h"
#include "sim/graphics/renderer/default/model/camera.h"
#include "sim/graphics/renderer/default/model/model_limit.h"

namespace sim::graphics::renderer::defaultRenderer {
struct Pipe {
  vk::UniquePipelineLayout pipelineLayout;
  vk::UniquePipeline pipeline;
};

struct Pass: Pipe {
  uint32_t subpass;
};

class Renderer: public VulkanBase {

public:
  explicit Renderer(
    const Config &config = {}, const ModelConfig &modelLimit = {},
    const FeatureConfig &featureConfig = {}, const DebugConfig &debugConfig = {});
  ~Renderer() override = default;

  virtual void updateModels() = 0;

  Camera &camera();
  Light &light(uint32_t idx);

private:
  void recreateResources();
  void updateFrame(
    std::function<void(uint32_t, float)> &updater, uint32_t imageIndex,
    float elapsedDuration) override;

protected:
  void resize() override;

  virtual void render(
    std::function<void(uint32_t, float)> &updater, uint32_t imageIndex,
    float elapsedDuration) = 0;

  const ModelConfig modelLimit;
  const vk::Device &vkDevice;

  Camera _camera{{10, 10, 10}, {0, 0, 0}, {0, 1, 0}};
  std::vector<Light> lights;

  uPtr<StorageAttachmentImage> offscreenImage;

  uPtr<HostUniformBuffer> camBuffer;
  uPtr<HostStorageBuffer> lightsBuffer;

  struct {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewInv;
    glm::mat4 projInv;
    glm::vec4 eye;
    glm::vec4 r, v;
    float w{1080}, h{720}, fov{glm::radians(60.f)}, pad;
    float zNear{0.001f}, zFar{1000000.f};
    Lighting lighting;
  } cam{};
};

}
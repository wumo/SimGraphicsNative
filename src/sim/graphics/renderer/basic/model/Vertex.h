#pragma once
#include "sim/graphics/base/vkcommon.h"

namespace sim::graphics::renderer::basic {
// ref in shaders
struct Vertex {
  glm::vec3 position{0.f};
  glm::vec3 normal{0.f};
  glm::vec2 uv{0.f};

  static uint32_t stride();

  static uint32_t locations();

  static std::vector<vk::VertexInputAttributeDescription> attributes(
    uint32_t binding, uint32_t baseLocation);
};
}
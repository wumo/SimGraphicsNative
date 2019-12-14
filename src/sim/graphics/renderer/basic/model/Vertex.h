#pragma once

#include "sim/graphics/base/vkcommon.h"

namespace sim::graphics::renderer::basic {
// ref in shaders
struct Vertex {
  glm::vec3 position{0.f};
  glm::vec3 normal{0.f};
  glm::vec2 uv{0.f};

  static uint32_t stride() { return sizeof(Vertex); }

  static uint32_t locations() { return 3; }

  static std::vector<vk::VertexInputAttributeDescription> attributes(
    uint32_t binding, uint32_t baseLocation) {
    using f = vk::Format;
    return {{baseLocation + 0, binding, f::eR32G32B32Sfloat, offsetOf(Vertex, position)},
            {baseLocation + 1, binding, f::eR32G32B32Sfloat, offsetOf(Vertex, normal)},
            {baseLocation + 2, binding, f::eR32G32Sfloat, offsetOf(Vertex, uv)}};
  }
};
}
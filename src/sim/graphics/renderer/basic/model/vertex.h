#pragma once
#include "sim/graphics/base/vkcommon.h"

namespace sim::graphics::renderer::basic {
// ref in shaders
struct Vertex {
  using Position = glm::vec3;
  using Normal = glm::vec3;
  using Tangent = glm::vec3;
  using UV = glm::vec2;
  using Color = glm::vec3;
  using Joint = glm::vec4;
  using Weight = glm::vec4;
};
}

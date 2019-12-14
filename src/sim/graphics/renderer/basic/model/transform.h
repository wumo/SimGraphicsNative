#pragma once
#include "sim/graphics/base/glm_common.h"

namespace sim::graphics::renderer::basic {

struct Transform {
  glm::vec3 translation{0.f};
  glm::vec3 scale{1.f};
  glm::quat rotation{1, 0, 0, 0};

  Transform(
    const glm::vec3 &translation = {}, const glm::vec3 &scale = glm::vec3{1.f},
    const glm::quat &rotation = {1, 0, 0, 0});

  Transform(glm::mat4 m);

  glm::mat4 toMatrix();
};
}
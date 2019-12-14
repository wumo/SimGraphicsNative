#pragma once
#include "sim/graphics/base/glm_common.h"

namespace sim::graphics::renderer::basic {

struct Transform {
  glm::vec3 translation{0.f};
  glm::vec3 scale{1.f};
  glm::quat rotation{1, 0, 0, 0};

  Transform(
    const glm::vec3 &translation = {}, const glm::vec3 &scale = glm::vec3{1.f},
    const glm::quat &rotation = {1, 0, 0, 0})
    : translation(translation), scale(scale), rotation(rotation) {}

  Transform(glm::mat4 m) {
    translation = glm::vec3(m[3]);
    scale = {length(m[0]), length(m[1]), length(m[2])};
    m = glm::scale(m, glm::vec3(1 / scale.x, 1 / scale.y, 1 / scale.z));
    m[3] = glm::vec4(0, 0, 0, 1);
    rotation = quat_cast(m);
  }

  glm::mat4 toMatrix() {
    glm::mat4 m{1.0};
    m = glm::scale(m, scale);
    m = glm::mat4_cast(rotation) * m;
    m[3] = glm::vec4(translation, 1);
    return m;
  }
};
}
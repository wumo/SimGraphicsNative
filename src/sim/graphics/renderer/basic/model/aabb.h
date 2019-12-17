#pragma once
#include <ostream>
#include "sim/graphics/base/glm_common.h"

namespace sim::graphics::renderer::basic {
struct AABB {
  glm::vec3 min{std::numeric_limits<float>::infinity()};
  alignas(sizeof(glm::vec4)) glm::vec3 max{-std::numeric_limits<float>::infinity()};

  AABB transform(glm::mat4 m) const;

  template<typename... Args>
  void merge(glm::vec3 p, Args &&... points) {
    merge(p);
    merge(points...);
  }

  void merge(glm::vec3 p);

  void merge(AABB other);

  glm::vec3 center();

  glm::vec3 halfRange();

  friend std::ostream &operator<<(std::ostream &os, const AABB &aabb);
};
}

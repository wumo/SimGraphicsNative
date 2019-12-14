#include "aabb.h"
namespace sim::graphics::renderer::basic {

AABB AABB::transform(glm::mat4 m) const {
  auto _min = glm::vec3(m[3]);
  auto _max = _min;

  auto p = glm::vec3(m[0]);
  auto v0 = p * min.x;
  auto v1 = p * max.x;
  _min += glm::min(v0, v1);
  _max += glm::max(v0, v1);

  p = glm::vec3(m[1]);
  v0 = p * min.y;
  v1 = p * max.y;
  _min += glm::min(v0, v1);
  _max += glm::max(v0, v1);

  p = glm::vec3(m[2]);
  v0 = p * min.z;
  v1 = p * max.z;
  _min += glm::min(v0, v1);
  _max += glm::max(v0, v1);

  return {_min, _max};
}

void AABB::merge(glm::vec3 p) {
  min = glm::min(min, p);
  max = glm::max(max, p);
}

void AABB::merge(AABB other) {
  min = glm::min(min, other.min);
  max = glm::max(max, other.max);
}

glm::vec3 AABB::center() { return (min + max) / 2.f; }

glm::vec3 AABB::halfRange() { return (max - min) / 2.f; }

std::ostream &operator<<(std::ostream &os, const AABB &aabb) {
  os << "min: " << glm::to_string(aabb.min) << " max: " << glm::to_string(aabb.max);
  return os;
}
}
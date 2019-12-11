#pragma once
#include "sim/graphics/base/vkcommon.h"

namespace sim::graphics::renderer::defaultRenderer {
struct Camera {
  glm::vec3 location;
  glm::vec3 focus{};
  glm::vec3 worldUp;
  bool isPerspective{true};
  float fov{glm::radians(60.f)}, w{1080}, h{720};
  /**zNear shouldn't be too small better larger than 0.1f.*/
  float zNear{0.1f}, zFar{1000.f};
  float orthScale{1.0};

  glm::mat4 view() const { return lookAt(location, focus, worldUp); }
  glm::mat4 projection() const {
    return isPerspective ? glm::perspective(fov, w / h, zNear, zFar) :
                           glm::ortho(
                             -w / 2 * orthScale, w / 2 * orthScale, -h / 2 * orthScale,
                             h / 2 * orthScale, zNear, zFar);
  }
};
}

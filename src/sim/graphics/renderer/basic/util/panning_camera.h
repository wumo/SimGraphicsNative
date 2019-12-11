#pragma once
#include "sim/graphics/base/glm_common.h"
#include "sim/graphics/base/input.h"
#include "sim/graphics/renderer/basic/perspective_camera.h"

namespace sim::graphics::renderer::basic {
class PanningCamera {
public:
  explicit PanningCamera(PerspectiveCamera &camera);
  glm::vec3 xzIntersection(float inputX, float inputY) const;
  void updateCamera(Input &input);

private:
  PerspectiveCamera &camera;
  struct MouseAction {
    float lastX, lastY;
    bool start{false};
    float sensitivity{0.1f};
  } rotate;

  MouseAction panning;
};
}
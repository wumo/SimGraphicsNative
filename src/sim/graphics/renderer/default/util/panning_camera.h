#pragma once
#include "sim/graphics/base/glm_common.h"
#include "sim/graphics/renderer/default/model/camera.h"
#include "sim/graphics/base/input.h"
namespace sim::graphics::renderer::defaultRenderer {
class PanningCamera {
public:
  explicit PanningCamera(Camera &camera);
  glm::vec3 xzIntersection(float inputX, float inputY) const;
  void updateCamera(Input &input);

private:
  Camera &camera;
  struct MouseAction {
    float lastX, lastY;
    bool start{false};
    float sensitivity{0.1f};
  } rotate;

  MouseAction panning;
};
}
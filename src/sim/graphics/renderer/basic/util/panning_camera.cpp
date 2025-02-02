#include "panning_camera.h"
namespace sim::graphics::renderer::basic {
using namespace sim;

glm::vec3 PanningCamera::xzIntersection(float inputX, float inputY) const {
  auto view = camera.view();
  auto viewInv = glm::inverse(view);
  auto proj = camera.projection();
  auto c =
    (glm::vec2(inputX, inputY) + 0.5f) / glm::vec2(camera.width(), camera.height());
  c = c * 2.f - 1.f;
  auto projInv = glm::inverse(proj);
  auto target = glm::vec3(projInv * glm::vec4(c.x, -c.y, 0, 1));
  auto dir = glm::vec3(viewInv * glm::vec4(normalize(target), 0));
  if(dir.y == 0) return camera.focus();
  auto t = -camera.location().y / dir.y;
  auto p = camera.location() + t * dir;
  return p;
}

PanningCamera::PanningCamera(PerspectiveCamera &camera): camera{camera} {}

void PanningCamera::updateCamera(Input &input) {
  static auto mouseRightPressed{false};
  auto front = camera.focus() - camera.location();
  auto len = length(front);
  camera.setLocation(
    camera.location() +
    normalize(front) * len * glm::clamp(float(input.scrollYOffset) / 10.f, -1.f, 1.f));
  input.scrollYOffset = 0;

  auto right = normalize(cross(front, camera.worldUp()));

  if(input.mouseButtonPressed[MouseLeft]) {
    if(!rotate.start) {
      rotate.lastX = input.mouseXPos;
      rotate.lastY = input.mouseYPos;
      rotate.start = true;
    }
    auto xoffset = float(input.mouseXPos - rotate.lastX);
    auto yoffset = float(input.mouseYPos - rotate.lastY);
    rotate.lastX = input.mouseXPos;
    rotate.lastY = input.mouseYPos;

    xoffset *= -rotate.sensitivity;
    yoffset *= -rotate.sensitivity;
    auto translation = camera.location() - camera.focus();
    auto pitch = glm::degrees(angle(camera.worldUp(), normalize(translation)));
    pitch += yoffset;
    if(1 >= pitch || pitch >= 179) yoffset = 0;
    camera.setLocation(
      camera.focus() + angleAxis(glm::radians(xoffset), camera.worldUp()) *
                         angleAxis(glm::radians(yoffset), right) * translation);
  } else
    rotate.start = false;

  front = normalize(camera.focus() - camera.location());
  right = normalize(cross(front, camera.worldUp()));

  if(input.mouseButtonPressed[MouseRight]) {
    mouseRightPressed = true;
    if(!panning.start) {
      panning.lastX = input.mouseXPos;
      panning.lastY = input.mouseYPos;
      panning.start = true;
    }
    auto p0 = xzIntersection(panning.lastX, panning.lastY);
    auto p1 = xzIntersection(input.mouseXPos, input.mouseYPos);

    panning.lastX = input.mouseXPos;
    panning.lastY = input.mouseYPos;

    auto translation = p0 - p1;
    camera.focusOn(camera.focus() + translation);
    camera.setLocation(camera.location() + translation);
  } else {
    if(mouseRightPressed) { panning.start = false; }
    mouseRightPressed = false;
  }
}
}
#include "light.h"
#include "sim/graphics/renderer/basic/basic_scene_manager.h"

namespace sim::graphics::renderer::basic {

Light::Light(
  BasicSceneManager &mm, LightType type, glm::vec3 direction, glm::vec3 color,
  glm::vec3 location)
  : _mm{mm},
    _type{type},
    _direction{direction},
    _color{color},
    _location{location},
    ubo{mm.allocateLightUBO()} {
  *ubo.ptr = {_color,    _intensity,           _direction,           _range,
              _location, _spot.innerConeAngle, _spot.outerConeAngle, uint32_t(_type)};
}
LightType Light::type() const { return _type; }
void Light::setType(LightType type) {
  _type = type;
  ubo.ptr->type = static_cast<uint32_t>(type);
}
const glm::vec3 &Light::color() const { return _color; }
void Light::setColor(const glm::vec3 &color) {
  _color = color;
  ubo.ptr->color = color;
}
const glm::vec3 &Light::direction() const { return _direction; }
void Light::setDirection(const glm::vec3 &direction) {
  _direction = direction;
  ubo.ptr->direction = direction;
}
const glm::vec3 &Light::location() const { return _location; }
void Light::setLocation(const glm::vec3 &location) {
  _location = location;
  ubo.ptr->location = location;
}
float Light::intensity() const { return _intensity; }
void Light::setIntensity(float intensity) {
  _intensity = intensity;
  ubo.ptr->intensity = intensity;
}
float Light::range() const { return _range; }
void Light::setRange(float range) {
  _range = range;
  ubo.ptr->range = range;
}
}
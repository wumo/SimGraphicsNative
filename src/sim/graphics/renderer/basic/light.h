#pragma once
#include "sim/graphics/base/glm_common.h"
namespace sim::graphics::renderer::basic {

// ref in shaders
struct Lighting {
public:
  struct UBO {
    uint32_t numLights{1};
    float exposure{4.5f}, gamma{2.2f};
  };

  uint32_t numLights() const { return _numLights; }
  void setNumLights(uint32_t numLights) {
    _numLights = numLights;
    _incoherent = true;
  }
  float exposure() const { return _exposure; }
  void setExposure(float exposure) {
    _exposure = exposure;
    _incoherent = true;
  }
  float gamma() const { return _gamma; }
  void setGamma(float gamma) {
    _gamma = gamma;
    _incoherent = true;
  }
  bool incoherent() const { return _incoherent; }

  UBO flush() {
    _incoherent = false;
    return {_numLights, _exposure, _gamma};
  }

private:
  uint32_t _numLights{1};
  float _exposure{4.5f}, _gamma{2.2f};
  bool _incoherent{true};
};

enum class LightType : uint32_t { Directional = 1u, Point = 2u, Spot = 3u };

struct Light {
  // ref in shaders
  struct alignas(sizeof(glm::vec4)) UBO {
    glm::vec3 color;
    float intensity;
    glm::vec3 direction;
    float range;
    glm::vec3 location;
    float spotInnerConeAngle, spotOuterConeAngle;
    uint32_t type;
  };

public:
  explicit Light(
    LightType type, glm::vec3 direction, glm::vec3 color, glm::vec3 location,
    uint32_t offset)
    : _type{type},
      _direction{direction},
      _color{color},
      _location{location},
      _offset(offset) {}

  LightType type() const { return _type; }
  void setType(LightType type) {
    _type = type;
    _incoherent = true;
  }
  const glm::vec3 &color() const { return _color; }
  void setColor(const glm::vec3 &color) {
    _color = color;
    _incoherent = true;
  }
  const glm::vec3 &direction() const { return _direction; }
  void setDirection(const glm::vec3 &direction) {
    _direction = direction;
    _incoherent = true;
  }
  const glm::vec3 &location() const { return _location; }
  void setLocation(const glm::vec3 &location) {
    _location = location;
    _incoherent = true;
  }
  float intensity() const { return _intensity; }
  void setIntensity(float intensity) {
    _intensity = intensity;
    _incoherent = true;
  }
  float range() const { return _range; }
  void setRange(float range) {
    _range = range;
    _incoherent = true;
  }

  bool incoherent() const { return _incoherent; }
  uint32_t offset() const { return _offset; }
  UBO flush() {
    _incoherent = false;
    return {_color,    _intensity,           _direction,           _range,
            _location, _spot.innerConeAngle, _spot.outerConeAngle, uint32_t(_type)};
  }

private:
  LightType _type{LightType ::Directional};
  glm::vec3 _color{1.f};
  glm::vec3 _direction{0.f, -1.f, 0.f};
  glm::vec3 _location{0.f, 1.f, 0.f};

  float _intensity{1.f};
  float _range{1.f};
  struct {
    float innerConeAngle{}, outerConeAngle{};
  } _spot;

  bool _incoherent{true};
  uint32_t _offset;
};

}
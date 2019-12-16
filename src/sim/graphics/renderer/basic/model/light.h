#pragma once
#include "sim/graphics/base/glm_common.h"
#include "model_buffer.h"

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

class BasicModelManager;

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
    BasicModelManager &mm, LightType type, glm::vec3 direction, glm::vec3 color,
    glm::vec3 location);

  LightType type() const;
  void setType(LightType type);
  const glm::vec3 &color() const;
  void setColor(const glm::vec3 &color);
  const glm::vec3 &direction() const;
  void setDirection(const glm::vec3 &direction);
  const glm::vec3 &location() const;
  void setLocation(const glm::vec3 &location);
  float intensity() const;
  void setIntensity(float intensity);
  float range() const;
  void setRange(float range);

private:
  BasicModelManager &_mm;
  LightType _type{LightType ::Directional};
  glm::vec3 _color{1.f};
  glm::vec3 _direction{0.f, -1.f, 0.f};
  glm::vec3 _location{0.f, 1.f, 0.f};

  float _intensity{1.f};
  float _range{1.f};
  struct {
    float innerConeAngle{}, outerConeAngle{};
  } _spot;

  Allocation<UBO> ubo;
};

}
#pragma once
#include "sim/graphics/base/glm_common.h"

namespace sim::graphics::renderer::basic {
class PerspectiveCamera {
  friend class BasicModelManager;

public:
  // ref in shaders
  struct alignas(sizeof(glm::vec4)) UBO {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 projView;
    //    glm::mat4 viewInv;
    //    glm::mat4 projInv;
    glm::vec4 eye;
    //    glm::vec4 r, v;
    //    float w, h, fov;
    //    float zNear, zFar;
  };

public:
  /**
   *
   * @param location
   * @param focus
   * @param worldUp
   * @param fov in radians
   * @param zNear zNear shouldn't be too small. Better larger than 0.1f.
   * @param zFar
   */
  PerspectiveCamera(
    glm::vec3 location, glm::vec3 focus, glm::vec3 worldUp = glm::vec3{0, 1, 0},
    float fov = glm::radians(45.f), float zNear = 0.1f, float zFar = 1000.f)
    : _location{location},
      _focus{focus},
      _worldUp{worldUp},
      _fov{fov},
      _zNear{zNear},
      _zFar{zFar} {}

  const glm::vec3 &location() const { return _location; }
  const glm::vec3 &focus() const { return _focus; }
  const glm::vec3 &worldUp() const { return _worldUp; }
  float fov() const { return _fov; }
  float zNear() const { return _zNear; }
  float zFar() const { return _zFar; }
  uint32_t width() const { return _width; }
  uint32_t height() const { return _height; }

  glm::mat4 view() const { return glm::lookAt(_location, _focus, _worldUp); }
  glm::mat4 projection() const {
    return glm::perspective(_fov, float(_width) / float(_height), _zNear, _zFar);
  }

  void setLocation(glm::vec3 location) {
    _location = location;
    _view_incoherent = true;
  }
  void focusOn(glm::vec3 focus) {
    _focus = focus;
    _view_incoherent = true;
  }
  void changeWorldUp(glm::vec3 worldUp) {
    _worldUp = worldUp;
    _view_incoherent = true;
  }
  /**
   *
   * @param fov in radians
   */
  void changeFOV(float fov) {
    _fov = fov;
    _proj_incoherent = true;
  }

  void changeZNear(float zNear) {
    _zNear = zNear;
    _proj_incoherent = true;
  }
  void changeZFar(float zFar) {
    _zFar = zFar;
    _proj_incoherent = true;
  }

  void changeDimension(uint32_t width, uint32_t height) {
    _width = width;
    _height = height;
    _proj_incoherent = true;
  }

private:
  glm::vec3 _location;
  glm::vec3 _focus;
  glm::vec3 _worldUp;
  float _fov;
  float _zNear, _zFar;
  uint32_t _width{-1u}, _height{-1u};

  bool _proj_incoherent{true};
  glm::mat4 _proj{};
  bool _view_incoherent{true};
  glm::mat4 _view{};

  UBO flush() {
    auto proj = flushProjection();
    auto view = flushView();
    return {view, proj, proj * view, glm::vec4(_location, 1.0)};
  }

  bool incoherent() const { return _view_incoherent || _proj_incoherent; }

  glm::mat4 flushView() {
    if(_view_incoherent) {
      _view = glm::lookAt(_location, _focus, _worldUp);
      _view_incoherent = false;
    }
    return _view;
  }

  glm::mat4 flushProjection() {
    if(_proj_incoherent) {
      _proj = glm::perspective(_fov, float(_width) / float(_height), _zNear, _zFar);
      _proj_incoherent = false;
    }
    return _proj;
  }
};
}
#include "primitive.h"
#include "sim/graphics/renderer/basic/basic_scene_manager.h"

namespace sim::graphics::renderer::basic {

Primitive::Primitive(
  BasicSceneManager &mm, const Range &index, const Range &position, const Range &normal,
  const Range &uv, const AABB &aabb, const PrimitiveTopology &topology,
  const DynamicType &_type)
  : mm{mm},
    _index(index),
    _position(position),
    _normal{normal},
    _uv{uv},
    _aabb{aabb},
    _topology{topology},
    _type{_type},
    ubo{mm.allocatePrimitiveUBO()} {
  *ubo.ptr = {_index,   _position, _normal,           _uv,       _joint0,
              _weight0, _aabb,     _tesselationLevel, _topology, _type};
}
const Range &Primitive::index() const { return _index; }
const Range &Primitive::position() const { return _position; }
const Range &Primitive::normal() const { return _normal; }
const Range &Primitive::uv() const { return _uv; }
const Range &Primitive::joint0() const { return _joint0; }
const Range &Primitive::weight0() const { return _weight0; }
PrimitiveTopology Primitive::topology() const { return _topology; }
const AABB &Primitive::aabb() const { return _aabb; }
void Primitive::setAabb(const AABB &aabb) {
  _aabb = aabb;
  ubo.ptr->_aabb = _aabb;
}
float Primitive::tesselationLevel() const { return _tesselationLevel; }
void Primitive::setTesselationLevel(float tesselationLevel) {
  _tesselationLevel = tesselationLevel;
  ubo.ptr->_tesselationLevel = _tesselationLevel;
}
DynamicType Primitive::type() const { return _type; }

}
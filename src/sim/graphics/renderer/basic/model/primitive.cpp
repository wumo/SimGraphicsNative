#include "primitive.h"
namespace sim::graphics::renderer::basic {

uint32_t Range::endOffset() const { return offset + size; }

Primitive::Primitive(
  const Range &index, const Range &position, const Range &normal, const Range &uv,
  const AABB &aabb, const PrimitiveTopology &topology, const DynamicType &_type)
  : _index(index),
    _position(position),
    _normal{normal},
    _uv{uv},
    _aabb{aabb},
    _topology{topology},
    _type{_type} {}
const Range &Primitive::index() const { return _index; }
const Range &Primitive::position() const { return _position; }
const Range &Primitive::normal() const { return _normal; }
const Range &Primitive::uv() const { return _uv; }
const Range &Primitive::joint0() const { return _joint0; }
const Range &Primitive::weight0() const { return _weight0; }
PrimitiveTopology Primitive::topology() const { return _topology; }
const AABB &Primitive::aabb() const { return _aabb; }
}
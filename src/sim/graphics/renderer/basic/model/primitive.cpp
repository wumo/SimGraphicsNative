#include "primitive.h"
namespace sim::graphics::renderer::basic {
Primitive::Primitive(
  const Range &index, const Range &vertex, const AABB &aabb,
  const PrimitiveTopology &topology, const DynamicType &_type)
  : _index(index), _vertex(vertex), _aabb{aabb}, _topology{topology}, _type{_type} {}
const Range &Primitive::index() const { return _index; }
const Range &Primitive::vertex() const { return _vertex; }
PrimitiveTopology Primitive::topology() const { return _topology; }
const AABB &Primitive::aabb() const { return _aabb; }
}
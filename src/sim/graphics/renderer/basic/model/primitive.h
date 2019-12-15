#pragma once
#include "aabb.h"

namespace sim::graphics::renderer::basic {
struct Range {
  uint32_t offset{0};
  uint32_t size{0};

  uint32_t endOffset() const;
};

enum class PrimitiveTopology { Triangles, Lines, Procedural };
enum class DynamicType { Static, Dynamic };

class Primitive {
public:
  Primitive() = default;
  /**
   *
   * @param index
   * @param position
   * @param aabb
   * @param topology once set, cannot change.
   */
  Primitive(
    const Range &index, const Range &position, const Range &normal, const Range &uv,
    const AABB &aabb, const PrimitiveTopology &topology,
    const DynamicType &_type = DynamicType::Static);
  const Range &index() const;
  const Range &position() const;
  const Range &normal() const;
  const Range &uv() const;
  const Range &joint0() const;
  const Range &weight0() const;
  const AABB &aabb() const;
  PrimitiveTopology topology() const;

private:
  Range _index, _position, _normal, _uv, _joint0, _weight0;
  AABB _aabb;
  PrimitiveTopology _topology{PrimitiveTopology::Triangles};
  DynamicType _type{DynamicType::Static};
};
}
#pragma once
#include "aabb.h"

namespace sim::graphics::renderer::basic {
struct Range {
  uint32_t offset{0};
  uint32_t size{0};

  uint32_t endOffset() const { return offset + size; }
};

enum class PrimitiveTopology { Triangles, Lines, Procedural };
enum class DynamicType { Static, Dynamic };

class Primitive {
public:
  Primitive() = default;
  /**
   *
   * @param index
   * @param vertex
   * @param aabb
   * @param topology once set, cannot change.
   */
  Primitive(
    const Range &index, const Range &vertex, const AABB &aabb,
    const PrimitiveTopology &topology, const DynamicType &_type = DynamicType::Static);
  const Range &index() const;
  const Range &vertex() const;
  const AABB &aabb() const;
  PrimitiveTopology topology() const;

private:
  Range _index, _vertex;
  AABB _aabb;
  PrimitiveTopology _topology{PrimitiveTopology::Triangles};
  DynamicType _type{DynamicType::Static};
};
}
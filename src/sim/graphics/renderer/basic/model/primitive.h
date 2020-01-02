#pragma once
#include "aabb.h"
#include "sim/util/range.h"
#include "model_buffer.h"

namespace sim::graphics::renderer::basic {
using namespace sim::util;

enum class PrimitiveTopology : uint32_t {
  Triangles = 1u,
  Lines = 2u,
  Procedural = 3u,
  Patches = 4u
};
enum class DynamicType : uint32_t { Static = 1u, Dynamic = 2u };

class BasicSceneManager;

class Primitive {
  friend class MeshInstance;
  friend class BasicSceneManager;

  //ref in shaders
  struct alignas(sizeof(glm::vec4)) UBO {
    Range _index, _position, _normal, _uv, _joint0, _weight0;
    AABB _aabb;
    float _tesselationLevel{64.0f};
    PrimitiveTopology _topology{PrimitiveTopology::Triangles};
    DynamicType _type{DynamicType::Static};
  };

public:
  /**
   *
   * @param index
   * @param position
   * @param aabb
   * @param topology once set, cannot change.
   */
  Primitive(
    BasicSceneManager &mm, const Range &index, const Range &position, const Range &normal,
    const Range &uv, const AABB &aabb, const PrimitiveTopology &topology,
    const DynamicType &_type = DynamicType::Static);
  const Range &index() const;
  const Range &position() const;
  const Range &normal() const;
  const Range &uv() const;
  const Range &joint0() const;
  const Range &weight0() const;
  const AABB &aabb() const;
  void setAabb(const AABB &aabb);
  float tesselationLevel() const;
  void setTesselationLevel(float tesselationLevel);
  PrimitiveTopology topology() const;
  DynamicType type() const;

private:
  BasicSceneManager &mm;

  Range _index, _position, _normal, _uv, _joint0, _weight0;
  AABB _aabb;
  float _tesselationLevel{64.0f};
  PrimitiveTopology _topology{PrimitiveTopology::Triangles};
  DynamicType _type{DynamicType::Static};

  Allocation<UBO> ubo;
};
}
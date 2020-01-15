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
  friend class PrimitiveBuilder;
  friend class BasicSceneManager;

public:
  //ref in shaders
  struct alignas(sizeof(glm::vec4)) UBO {
    Range index_, position_, normal_, uv_, joint0_, weight0_;
    AABB aabb_;
    vk::Bool32 lod_{false};
    float tesselationLevel_{64.0f};
    PrimitiveTopology topology_{PrimitiveTopology::Triangles};
    DynamicType type_{DynamicType::Static};
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
    const DynamicType &type = DynamicType::Static);
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
  bool lod() const;
  void setLod(bool lod);
  PrimitiveTopology topology() const;
  DynamicType type() const;

private:
  BasicSceneManager &mm;

  Range index_, position_, normal_, uv_, joint0_, weight0_;
  AABB aabb_;
  bool lod_{false};
  float tesselationLevel_{64.0f};
  PrimitiveTopology topology_{PrimitiveTopology::Triangles};
  DynamicType type_{DynamicType::Static};

  Allocation<UBO> ubo;
};
}
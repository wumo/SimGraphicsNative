#include "primitive.h"
#include "sim/graphics/renderer/basic/basic_scene_manager.h"

namespace sim::graphics::renderer::basic {

Primitive::Primitive(
  BasicSceneManager &mm, const Range &index, const Range &position, const Range &normal,
  const Range &uv, const AABB &aabb, const PrimitiveTopology &topology,
  const DynamicType &type)
  : mm{mm},
    index_(index),
    position_(position),
    normal_{normal},
    uv_{uv},
    aabb_{aabb},
    topology_{topology},
    type_{type},
    ubo{mm.allocatePrimitiveUBO()} {
  *ubo.ptr = {index_, position_, normal_,           uv_,       joint0_, weight0_,
              aabb_,  lod_,      tesselationLevel_, topology_, type};
}
const Range &Primitive::index() const { return index_; }
const Range &Primitive::position() const { return position_; }
const Range &Primitive::normal() const { return normal_; }
const Range &Primitive::uv() const { return uv_; }
const Range &Primitive::joint0() const { return joint0_; }
const Range &Primitive::weight0() const { return weight0_; }
PrimitiveTopology Primitive::topology() const { return topology_; }
const AABB &Primitive::aabb() const { return aabb_; }
void Primitive::setAabb(const AABB &aabb) {
  aabb_ = aabb;
  ubo.ptr->aabb_ = aabb_;
}
bool Primitive::lod() const { return lod_; }
void Primitive::setLod(bool lod) {
  lod_ = lod;
  ubo.ptr->lod_ = lod_;
}
float Primitive::tesselationLevel() const { return tesselationLevel_; }
void Primitive::setTesselationLevel(float tesselationLevel) {
  tesselationLevel_ = tesselationLevel;
  ubo.ptr->tesselationLevel_ = tesselationLevel_;
}
DynamicType Primitive::type() const { return type_; }

}
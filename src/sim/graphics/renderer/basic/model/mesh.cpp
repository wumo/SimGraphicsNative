#include "mesh.h"

namespace sim::graphics::renderer::basic {
Mesh::Mesh(Ptr<Primitive> primitive, Ptr<Material> material)
  : _primitive{primitive}, _material{material} {}
const Ptr<Primitive> &Mesh::primitive() const { return _primitive; }
const Ptr<Material> &Mesh::material() const { return _material; }
}
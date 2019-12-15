#include "mesh.h"
#include "model_instance.h"

namespace sim::graphics::renderer::basic {
Mesh::Mesh(
  Ptr<Primitive> primitive, Ptr<Material> material, DrawQueueIndex drawIndex,
  uint32_t offset)
  : _primitive{primitive}, _material{material}, _drawIndex{drawIndex}, _offset{offset} {}
const Ptr<Primitive> &Mesh::primitive() const { return _primitive; }
const Ptr<ModelInstance> &Mesh::instance() const { return _instance; }
const Ptr<Node> &Mesh::node() const { return _node; }
const Ptr<Material> &Mesh::material() const { return _material; }
void Mesh::setPrimitive(const Ptr<Primitive> &primitive) {
  _primitive = primitive;
  invalidate();
}
void Mesh::setInstance(const Ptr<ModelInstance> &instance) {
  errorIf(
    !_instance.isNull(), "mesh already owned by model instance ", _instance.index());
  _instance = instance;
  invalidate();
}
void Mesh::setNode(const Ptr<Node> &node) {
  errorIf(!_node.isNull(), "mesh already owned by node ", _node.index());
  _node = node;
  invalidate();
}
void Mesh::setMaterial(const Ptr<Material> &material) {
  _material = material;
  invalidate();
}
bool Mesh::visible() const { return _visible; }
void Mesh::setVisible(bool visible) {
  _visible = visible;
  invalidate();
}
void Mesh::setDrawQueue(DrawQueueIndex drawIndex) { _drawIndex = drawIndex; }

bool Mesh::incoherent() const { return _incoherent; }
void Mesh::invalidate() { _incoherent = true; }
std::pair<uint32_t, DrawQueueIndex> Mesh::offset() const { return {_offset, _drawIndex}; }

auto Mesh::getUBO() const -> Mesh::UBO {
  return {0, _instance ? _instance.get().offset() : -1u,
          _node ? _node.get().offset() : -1u, _material ? _material.get().offset() : -1u};
}
auto Mesh::getDrawCMD() const -> vk::DrawIndexedIndirectCommand {
  Range index, vertex;
  if(_primitive) {
    auto &primitive = _primitive.get();
    vertex = primitive.position();
    index = primitive.index();
  }
  return {
    index.size,   _instance ? (_visible ? 1u : 0u) : 0u,
    index.offset, int32_t(vertex.offset),
    _offset,
  };
}
void Mesh::flush() { _incoherent = false; }

}
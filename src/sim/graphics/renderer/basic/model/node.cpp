#include "node.h"

namespace sim::graphics::renderer::basic {

Node::Node(const Transform &transform, const std::string &name, uint32_t offset)
  : _transform{transform}, _name{name}, _offset{offset} {}
std::string &Node::name() { return _name; }
void Node::setName(const std::string &name) { _name = name; }
const Transform &Node::transform() const { return _transform; }
void Node::setTransform(const Transform &transform) {
  _transform = transform;
  invalidate();
}
glm::mat4 Node::globalMatrix() {
  if(!_globalMatrix_dirty) return _globalMatrix;
  auto m = _transform.toMatrix();
  if(_parent) {
    auto parentMatrix = _parent->globalMatrix();
    m = parentMatrix * m;
  }
  _globalMatrix = m;
  _globalMatrix_dirty = false;
  return _globalMatrix;
}
const std::vector<Ptr<Mesh>> &Node::meshes() const { return _meshes; }
const Ptr<Node> &Node::parent() const { return _parent; }
const std::vector<Ptr<Node>> &Node::children() const { return _children; }
void Node::setInstance(Ptr<ModelInstance> instance) {
  errorIf(
    !_instance.isNull(), "node already owned by model instance ", _instance.index());
  for(auto &mesh: _meshes)
    mesh->setInstance(instance);
  for(auto &child: _children)
    child->setInstance(instance);
}

void Node::addMesh(Ptr<Node> node, Ptr<Mesh> mesh) {
  mesh->setInstance(node->_instance);
  mesh->setNode(node);
  node->_meshes.push_back(mesh);
}

void Node::addChild(Ptr<Node> parent, Ptr<Node> child) {
  parent->_children.push_back(child);
  child->_parent = parent;
  child->invalidate();
}

bool Node::incoherent() const { return _incoherent; }
void Node::invalidate() {
  _globalMatrix_dirty = true;
  _incoherent = true;
  for(auto &child: _children)
    child->invalidate();
}
uint32_t Node::offset() const { return _offset; }
glm::mat4 Node::flush() {
  _incoherent = false;
  return globalMatrix();
}

AABB Node::aabb() {
  _aabb = {};
  auto m = globalMatrix();
  for(auto &mesh: _meshes)
    _aabb.merge(mesh->_primitive->aabb().transform(m));

  for(auto &child: _children)
    _aabb.merge(child->aabb());

  return _aabb;
}
bool Node::visible() const { return _visible; }
void Node::setVisible(bool visible) {
  _visible = visible;
  for(auto &mesh: _meshes)
    mesh->setVisible(visible);
}
}
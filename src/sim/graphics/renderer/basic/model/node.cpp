#include "node.h"
#include "sim/graphics/renderer/basic/basic_scene_manager.h"

namespace sim::graphics::renderer::basic {

Node::Node(BasicSceneManager &mm, const Transform &transform, const std::string &name)
  : mm{mm}, _transform{transform}, _name{name}, ubo{mm.allocateMatrixUBO()} {
  *ubo.ptr = _transform.toMatrix();
}
std::string &Node::name() { return _name; }
void Node::setName(const std::string &name) { _name = name; }
const Transform &Node::transform() const { return _transform; }
void Node::setTransform(const Transform &transform) {
  _transform = transform;
  updateMatrix();
}

void Node::updateMatrix() {
  auto m = _transform.toMatrix();
  if(_parent) {
    auto parentMatrix = *_parent->ubo.ptr;
    m = parentMatrix * m;
  }
  *ubo.ptr = m;

  for(auto &child: _children)
    child->updateMatrix();
}

const std::vector<Ptr<Mesh>> &Node::meshes() const { return _meshes; }
const Ptr<Node> &Node::parent() const { return _parent; }

const std::vector<Ptr<Node>> &Node::children() const { return _children; }

void Node::addMesh(Ptr<Node> node, Ptr<Mesh> mesh) {
  errorIf(node->fixed, "Node is fixed, cannot add mesh");
  node->_meshes.push_back(mesh);
}

void Node::addChild(Ptr<Node> parent, Ptr<Node> child) {
  errorIf(parent->fixed || child->fixed, "Node is fixed, cannot add child or parent");
  parent->_children.push_back(child);
  child->_parent = parent;
  child->updateMatrix();
}

AABB Node::aabb() {
  _aabb = {};
  auto m = *ubo.ptr;
  for(auto &mesh: _meshes)
    _aabb.merge(mesh->_primitive->aabb().transform(m));

  for(auto &child: _children)
    _aabb.merge(child->aabb());

  return _aabb;
}
void Node::fix() { fixed = true; }
}
#include "basic_model.h"

namespace sim::graphics::renderer::basic {

Primitive::Primitive(
  const Range &index, const Range &vertex, const AABB &aabb,
  const PrimitiveTopology &topology, const DynamicType &_type)
  : _index(index), _vertex(vertex), _aabb{aabb}, _topology{topology}, _type{_type} {}
const Range &Primitive::index() const { return _index; }
const Range &Primitive::vertex() const { return _vertex; }
PrimitiveTopology Primitive::topology() const { return _topology; }
const AABB &Primitive::aabb() const { return _aabb; }

Material::Material(MaterialType type, uint32_t offset): _type{type}, _offset{offset} {}

const Ptr<TextureImage2D> &Material::colorTex() const { return _colorTex; }
Material &Material::setColorTex(const Ptr<TextureImage2D> &colorTex) {
  _colorTex = colorTex;
  invalidate();
  return *this;
}
const Ptr<TextureImage2D> &Material::pbrTex() const { return _pbrTex; }
Material &Material::setPbrTex(const Ptr<TextureImage2D> &pbrTex) {
  _pbrTex = pbrTex;
  invalidate();
  return *this;
}
const Ptr<TextureImage2D> &Material::normalTex() const { return _normalTex; }
Material &Material::setNormalTex(const Ptr<TextureImage2D> &normalTex) {
  _normalTex = normalTex;
  invalidate();
  return *this;
}
const Ptr<TextureImage2D> &Material::occlusionTex() const { return _occlusionTex; }
Material &Material::setOcclusionTex(const Ptr<TextureImage2D> &occlusionTex) {
  _occlusionTex = occlusionTex;
  invalidate();
  return *this;
}
const Ptr<TextureImage2D> &Material::emissiveTex() const { return _emissiveTex; }
Material &Material::setEmissiveTex(const Ptr<TextureImage2D> &emissiveTex) {
  _emissiveTex = emissiveTex;
  invalidate();
  return *this;
}
const glm::vec4 &Material::colorFactor() const { return _colorFactor; }
Material &Material::setColorFactor(const glm::vec4 &colorFactor) {
  _colorFactor = colorFactor;
  invalidate();
  return *this;
}
const glm::vec4 &Material::pbrFactor() const { return _pbrFactor; }
Material &Material::setPbrFactor(const glm::vec4 &pbrFactor) {
  _pbrFactor = pbrFactor;
  invalidate();
  return *this;
}
float Material::occlusionStrength() const { return _occlusionStrength; }
Material &Material::setOcclusionStrength(float occlusionStrength) {
  _occlusionStrength = occlusionStrength;
  invalidate();
  return *this;
}
float Material::alphaCutoff() const { return _alphaCutoff; }
Material &Material::setAlphaCutoff(float alphaCutoff) {
  _alphaCutoff = alphaCutoff;
  invalidate();
  return *this;
}
const glm::vec4 &Material::emissiveFactor() const { return _emissiveFactor; }
Material &Material::setEmissiveFactor(const glm::vec4 &emissiveFactor) {
  _emissiveFactor = emissiveFactor;
  invalidate();
  return *this;
}
MaterialType Material::type() const { return _type; }

bool Material::incoherent() const { return _incoherent; }
void Material::invalidate() { _incoherent = true; }
uint32_t Material::offset() const { return _offset; }
Material::UBO Material::flush() {
  _incoherent = false;
  return {_colorFactor,
          _pbrFactor,
          _emissiveFactor,
          _occlusionStrength,
          _alphaCutoff,
          _colorTex ? int32_t(_colorTex.index()) : -1,
          _pbrTex ? int32_t(_pbrTex.index()) : -1,
          _normalTex ? int32_t(_normalTex.index()) : -1,
          _occlusionTex ? int32_t(_occlusionTex.index()) : -1,
          _emissiveTex ? int32_t(_emissiveTex.index()) : -1,
          static_cast<uint32_t>(_type)};
}

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
    vertex = primitive.vertex();
    index = primitive.index();
  }
  return {
    index.size,   _instance ? (_visible ? 1u : 0u) : 0u,
    index.offset, int32_t(vertex.offset),
    _offset,
  };
}
void Mesh::flush() { _incoherent = false; }

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

Model::Model(std::vector<Ptr<Node>> nodes): _nodes{std::move(nodes)} {}
const std::vector<Ptr<Node>> &Model::nodes() const { return _nodes; }

AABB Model::aabb() {
  _aabb = {};
  for(auto &node: _nodes)
    _aabb.merge(node->aabb());
  return _aabb;
}

void ModelInstance::applyModel(Ptr<ModelInstance> instance, Ptr<Model> model) {
  errorIf(!instance->_model.isNull(), "model instance already has model");
  instance->_model = model;
  for(auto &node: model->_nodes)
    node->setInstance(instance);
}
ModelInstance::ModelInstance(const Transform &transform, uint32_t offset)
  : _transform{transform}, _offset{offset} {}
Ptr<Model> ModelInstance::model() { return _model; }
const Transform &ModelInstance::transform() const { return _transform; }
void ModelInstance::setTransform(const Transform &transform) {
  _transform = transform;
  invalidate();
}
bool ModelInstance::visible() const { return _visible; }
void ModelInstance::setVisible(bool visible) {
  _visible = visible;
  for(auto &node: _model->_nodes)
    node->setVisible(visible);
}
glm::mat4 ModelInstance::matrix() {
  if(!_matrix_dirty) return _matrix;
  _matrix = _transform.toMatrix();
  _matrix_dirty = false;
  return _matrix;
}

bool ModelInstance::incoherent() const { return _incoherent; }
void ModelInstance::invalidate() {
  _matrix_dirty = true;
  _incoherent = true;
}
uint32_t ModelInstance::offset() const { return _offset; }
glm::mat4 ModelInstance::flush() {
  _incoherent = false;
  return matrix();
}

}
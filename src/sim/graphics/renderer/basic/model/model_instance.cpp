#include "model_instance.h"
#include "sim/graphics/renderer/basic/basic_scene_manager.h"

namespace sim::graphics::renderer::basic {

MeshInstance::MeshInstance(
  BasicSceneManager &mm, const Ptr<Primitive> &primitive, const Ptr<Material> &material,
  const Ptr<Node> &node, const Ptr<ModelInstance> &instance)
  : _mm{mm},
    _primitive(primitive),
    _material(material),
    _node(node),
    _instance(instance),
    _ubo{mm.allocateMeshInstanceUBO()},
    _drawCMD{mm.allocateDrawCMD(primitive, material)} {
  *_ubo.ptr = {_primitive->ubo.offset, _material ? _material->ubo.offset : -1u,
               _node ? _node->ubo.offset : -1u, _instance ? _instance->_ubo.offset : -1u};
  setVisible(_visible);
  Range index, vertex;
  if(_primitive) {
    auto &p = _primitive.get();
    vertex = p.position();
    index = p.index();
  }
  *_drawCMD.ptr = vk::DrawIndexedIndirectCommand{
    index.size,   _instance ? (_visible ? 1u : 0u) : 0u,
    index.offset, int32_t(vertex.offset),
    _ubo.offset,
  };
}

void MeshInstance::setVisible(bool visible) {
  if(_visible != visible) {
    _visible = visible;
    _drawCMD.ptr->instanceCount = _instance ? (_visible ? 1u : 0u) : 0u;
  }
}

void ModelInstance::applyModel(Ptr<Model> model, Ptr<ModelInstance> instance) {
  errorIf(!instance->_model, "instance already has model");
  for(auto &node: model->_nodes)
    generateMeshInstances(instance, node);
}

void ModelInstance::generateMeshInstances(Ptr<ModelInstance> instance, Ptr<Node> node) {
  for(auto &mesh: node->_meshes)
    instance->_meshInstances.emplace_back(
      instance->_mm, mesh->primitive(), mesh->material(), node, instance);
  for(auto &child: node->_children)
    generateMeshInstances(instance, child);
}

ModelInstance::ModelInstance(
  BasicSceneManager &mm, Ptr<Model> model, const Transform &transform)
  : _mm{mm}, _model{model}, _ubo{mm.allocateMatrixUBO()} {
  setTransform(transform);
}

Ptr<Model> ModelInstance::model() { return _model; }
const Transform &ModelInstance::transform() const { return _transform; }
void ModelInstance::setTransform(const Transform &transform) {
  _transform = transform;
  *_ubo.ptr = _transform.toMatrix();
}
bool ModelInstance::visible() const { return _visible; }
void ModelInstance::setVisible(bool visible) {
  if(_visible != visible) {
    _visible = visible;
    for(auto &meshInstance: _meshInstances)
      meshInstance.setVisible(visible);
  }
}

}
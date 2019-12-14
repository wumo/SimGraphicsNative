#include "model_instance.h"

namespace sim::graphics::renderer::basic {

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
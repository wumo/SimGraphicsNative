//
// Created by WuMo on 12/14/2019.
//

#pragma once
#include "model.h"

namespace sim::graphics::renderer::basic {

class ModelInstance {
  friend class BasicModelManager;
  friend class Mesh;

public:
  static void applyModel(Ptr<ModelInstance> instance, Ptr<Model> model);
  explicit ModelInstance(const Transform &transform, uint32_t offset);

  const Transform &transform() const;
  void setTransform(const Transform &transform);
  glm::mat4 matrix();
  Ptr<Model> model();
  bool visible() const;
  void setVisible(bool visible);

private:
  glm::mat4 flush();

  bool incoherent() const;
  void invalidate();
  uint32_t offset() const;

  Transform _transform;
  Ptr<Model> _model;
  bool _visible{true};

  glm::mat4 _matrix{1.f};
  bool _matrix_dirty{true};
  bool _incoherent{true};
  uint32_t _offset;
};
}
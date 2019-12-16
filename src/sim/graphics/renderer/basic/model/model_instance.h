#pragma once
#include "model.h"

namespace sim::graphics::renderer::basic {
class ModelInstance;
class BasicModelManager;

class MeshInstance {
  friend class BasicModelManager;
  friend class ModelInstance;

  // ref in shaders
  struct UBO {
    uint32_t primitive, material, node, instance;
  };

public:
  MeshInstance(
    BasicModelManager &mm, const Ptr<Primitive> &primitive, const Ptr<Material> &material,
    const Ptr<Node> &node, const Ptr<ModelInstance> &instance);

private:
  void setVisible(bool visible);

private:
  BasicModelManager &_mm;

  Ptr<Primitive> _primitive{};
  Ptr<Material> _material{};
  Ptr<Node> _node{};
  Ptr<ModelInstance> _instance{};

  bool _visible{true};

  Allocation<MeshInstance::UBO> _ubo;
  Allocation<vk::DrawIndexedIndirectCommand> _drawCMD;
};

class ModelInstance {
  friend class BasicModelManager;
  friend class MeshInstance;

public:
  static void applyModel(Ptr<Model> model, Ptr<ModelInstance> instance);
  explicit ModelInstance(
    BasicModelManager &mm, Ptr<Model> model, const Transform &transform);

  const Transform &transform() const;
  void setTransform(const Transform &transform);
  Ptr<Model> model();
  bool visible() const;
  void setVisible(bool visible);

private:
  BasicModelManager &_mm;

  Transform _transform;
  Ptr<Model> _model;
  std::vector<MeshInstance> _meshInstances;

  bool _visible{true};

  Allocation<glm::mat4> _ubo;
};
}
#pragma once
#include "model.h"

namespace sim::graphics::renderer::basic {
class ModelInstance;
class BasicSceneManager;

class MeshInstance {
  friend class BasicSceneManager;
  friend class ModelInstance;

  // ref in shaders
  struct UBO {
    uint32_t primitive, material, node, instance;
  };

public:
  MeshInstance(
    BasicSceneManager &mm, const Ptr<Primitive> &primitive, const Ptr<Material> &material,
    const Ptr<Node> &node, const Ptr<ModelInstance> &instance);

private:
  void setVisible(bool visible);

private:
  BasicSceneManager &_mm;

  Ptr<Primitive> _primitive{};
  Ptr<Material> _material{};
  Ptr<Node> _node{};
  Ptr<ModelInstance> _instance{};

  bool _visible{true};

  Allocation<MeshInstance::UBO> _ubo;
  Allocation<vk::DrawIndexedIndirectCommand> _drawCMD;
};

class ModelInstance {
  friend class BasicSceneManager;
  friend class MeshInstance;

public:
  static void applyModel(Ptr<Model> model, Ptr<ModelInstance> instance);

private:
  static void generateMeshInstances(Ptr<ModelInstance> instance, Ptr<Node> node);

public:
  explicit ModelInstance(
    BasicSceneManager &mm, Ptr<Model> model, const Transform &transform);

  const Transform &transform() const;
  void setTransform(const Transform &transform);
  Ptr<Model> model();
  bool visible() const;
  void setVisible(bool visible);

private:
  BasicSceneManager &_mm;

  Transform _transform;
  Ptr<Model> _model;
  std::vector<MeshInstance> _meshInstances;

  bool _visible{true};

  Allocation<glm::mat4> _ubo;
};
}
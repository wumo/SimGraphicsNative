//
// Created by WuMo on 12/14/2019.
//

#pragma once
#include "aabb.h"
#include "transform.h"
#include "mesh.h"

namespace sim::graphics::renderer::basic {

class Node {
  friend class BasicModelManager;
  friend class Mesh;
  friend class ModelInstance;

public:
  static void addMesh(Ptr<Node> node, Ptr<Mesh> mesh);
  static void addChild(Ptr<Node> parent, Ptr<Node> child);

  explicit Node(const Transform &transform, const std::string &name, uint32_t offset);

  std::string &name();
  void setName(const std::string &name);
  const Transform &transform() const;
  void setTransform(const Transform &transform);
  const std::vector<Ptr<Mesh>> &meshes() const;
  const Ptr<Node> &parent() const;
  const std::vector<Ptr<Node>> &children() const;
  bool visible() const;
  void setVisible(bool visible);

  AABB aabb();
  glm::mat4 globalMatrix();

private:
  void setInstance(Ptr<ModelInstance> instance);
  void invalidate();
  bool incoherent() const;
  uint32_t offset() const;
  glm::mat4 flush();

  std::string _name{};
  Transform _transform{};
  std::vector<Ptr<Mesh>> _meshes{};

  Ptr<Node> _parent{};
  std::vector<Ptr<Node>> _children{};

  Ptr<ModelInstance> _instance{};
  bool _visible{true};

  AABB _aabb;

  glm::mat4 _globalMatrix{1.f};
  bool _globalMatrix_dirty{true};
  bool _incoherent{true};
  uint32_t _offset{-1u};
};

}
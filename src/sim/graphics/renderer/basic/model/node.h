//
// Created by WuMo on 12/14/2019.
//

#pragma once
#include "aabb.h"
#include "transform.h"
#include "mesh.h"

namespace sim::graphics::renderer::basic {

class BasicModelManager;

class Node {
  friend class BasicModelManager;
  friend class Mesh;
  friend class ModelInstance;
  friend class MeshInstance;

public:
  static void addMesh(Ptr<Node> node, Ptr<Mesh> mesh);
  static void addChild(Ptr<Node> parent, Ptr<Node> child);

  explicit Node(
    BasicModelManager &mm, const Transform &transform, const std::string &name);

  std::string &name();
  void setName(const std::string &name);
  const Transform &transform() const;
  void setTransform(const Transform &transform);
  const std::vector<Ptr<Mesh>> &meshes() const;
  const Ptr<Node> &parent() const;
  const std::vector<Ptr<Node>> &children() const;
  
  AABB aabb();
  
  void fix();

private:
  void updateMatrix();

private:
  BasicModelManager &mm;

  std::string _name{};
  Transform _transform{};
  std::vector<Ptr<Mesh>> _meshes{};

  Ptr<Node> _parent{};
  std::vector<Ptr<Node>> _children{};

  AABB _aabb;

  Allocation<glm::mat4> ubo;

  bool fixed{false};
};

}
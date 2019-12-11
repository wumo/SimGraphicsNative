#pragma once
#include "sim/graphics/renderer/default/model/model.h"

namespace sim::graphics::renderer::defaultRenderer {
class ModelManager;

class ModelBuilder {
public:
  explicit ModelBuilder(ModelManager &modelManager);

  Ptr<Node> addNode(
    const Transform &transform = Transform{}, const std::string &name = "");
  void addMeshPart(
    Ptr<Node> node, ConstPtr<Mesh> mesh, Ptr<Material> material,
    const Transform &transform = Transform{});
  void addMeshPart(Ptr<Node> node, MeshPart meshPart);
  void addMeshPart(
    ConstPtr<Mesh> mesh, Ptr<Material> material,
    const Transform &transform = Transform{});
  Ptr<Node> addChildNode(
    Ptr<Node> parent, const Transform &transform = Transform{},
    const std::string &name = "");
  Ptr<Node> addChildNode(Ptr<Node> parent, Ptr<Node> child);

  std::vector<Node> &getNodes();

private:
  ModelManager &mm;
  std::vector<Node> nodes;
};
}

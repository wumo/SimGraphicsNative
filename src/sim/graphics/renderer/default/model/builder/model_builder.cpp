#include "model_builder.h"
#include "sim/graphics/renderer/default/model/model_manager.h"

namespace sim::graphics::renderer::defaultRenderer {
ModelBuilder::ModelBuilder(ModelManager &modelManager): mm{modelManager} {}

Ptr<Node> ModelBuilder::addNode(const Transform &transform, const std::string &name) {
  return Ptr<Node>::add(nodes, {name, mm.newTransform(transform)});
}

void ModelBuilder::addMeshPart(
  Ptr<Node> node, ConstPtr<Mesh> mesh, Ptr<Material> material,
  const Transform &transform) {
  auto trans = mm.newTransform(transform);
  addMeshPart(node, {mesh, material, trans});
}

void ModelBuilder::addMeshPart(Ptr<Node> node, MeshPart meshPart) {
  node->meshParts.push_back(meshPart);
}

void ModelBuilder::addMeshPart(
  ConstPtr<Mesh> mesh, Ptr<Material> material, const Transform &transform) {
  addMeshPart(addNode(), mesh, material, transform);
}

Ptr<Node> ModelBuilder::addChildNode(
  Ptr<Node> parent, const Transform &transform, const std::string &name) {
  return Ptr<Node>::add(parent->children, {name, mm.newTransform(transform)});
}
Ptr<Node> ModelBuilder::addChildNode(Ptr<Node> parent, Ptr<Node> child) {
  return Ptr<Node>::add(parent->children, child);
}

std::vector<Node> &ModelBuilder::getNodes() { return nodes; }
}

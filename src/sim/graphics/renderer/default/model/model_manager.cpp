#include "model_manager.h"
#include "sim/graphics/renderer/default/model/builder/model_glTF.h"

namespace sim::graphics::renderer::defaultRenderer {
ModelInstance::ModelInstance(
  const Model &model, ModelManager &modelManager, Ptr<Transform> transform)
  : mm{&modelManager}, _transform{transform} {
  for(auto &node: model.nodes) {
    auto trans = mm->newTransform(node.transform.get());
    trans->parentID = transform.index();
    nodes.push_back({node.name, trans});
    loadNode(nodes.back(), node);
  }
}

void ModelInstance::loadNode(Node &dst, const Node &src) const {
  for(auto meshPart: src.meshParts) {
    auto trans = mm->newTransform(meshPart.transform.get());
    trans->parentID = dst.transform.index();
    dst.meshParts.push_back({meshPart.mesh, meshPart.material, trans});
  }
  for(auto &child: src.children) {
    auto trans = mm->newTransform(child.transform.get());
    trans->parentID = dst.transform.index();
    dst.children.push_back({child.name, trans});
    loadNode(dst.children.back(), child);
  }
}

Ptr<Transform> ModelInstance::transform() const { return _transform; }
Node &ModelInstance::node(uint32_t index) { return nodes.at(index); }
uint32_t ModelInstance::numNodes() const { return uint32_t(nodes.size()); }

void ModelInstance::visit(std::function<void(Node &)> visitor) {
  for(auto &node: nodes)
    visitNode(node, visitor);
}
void ModelInstance::visitNode(Node &start, std::function<void(Node &)> &visitor) {
  visitor(start);
  for(auto &child: start.children)
    visitNode(child, visitor);
}

using namespace glm;

ModelManager::ModelManager(Device &context, const ModelConfig &config)
  : config{config},
    device{context},
    Pixel{config.texWidth, config.texHeight, config.texWidth * config.texHeight,
          uint32_t(config.texWidth * config.texHeight * sizeof(vec4))} {}

ConstPtr<Mesh> ModelManager::newMesh(
  std::vector<uint32_t> &indices, std::vector<Vertex> &vertices, Primitive primitive) {
  return newMesh(newIndices(indices), newVertices(vertices), primitive);
}

ConstPtr<Mesh> ModelManager::newMesh(
  std::vector<uint32_t> &indices, std::vector<glm::vec3> &vertexPositions,
  Primitive primitive) {
  std::vector<Vertex> vertices;
  vertices.reserve(vertexPositions.size());
  for(auto &pos: vertexPositions)
    vertices.push_back({pos});
  return newMesh(indices, vertices, primitive);
}

ConstPtr<Mesh> ModelManager::newMesh(CallMeshBuilder &build) {
  return newMeshes([&](MeshBuilder &builder) { build.build(builder); })[0];
}

ConstPtr<Mesh> ModelManager::newMesh(Range indices, Range vertices, Primitive primitive) {
  return addMeshes(
    {{indices.offset, indices.size, vertices.offset, vertices.size, primitive}})[0];
}

Tex ModelManager::newTex(vec4 value) {
  errorIf(
    Pixel.count >= Pixel.maxNumPixels,
    "exceeds maximum tex number: ", Pixel.maxNumPixels);
  Pixel.pixels[Pixel.count] = value;
  auto x = Pixel.count % Pixel.texWidth;
  auto y = Pixel.count / Pixel.texWidth;
  auto w = 0.5f;
  auto h = 0.5f;
  Pixel.count++;
  Pixel.isDirty = true;
  return Tex{0, float(x), float(y), w, h};
}

Tex ModelManager::newTex(vec3 color) { return newTex(vec4(color, 1.0)); }

Tex ModelManager::newTex(PBR pbr) { return newTex(pbr.toVec4()); }

Tex ModelManager::newTex(Refractive refractive) { return newTex(refractive.toVec4()); }

void ModelManager::modifyPixel(uint32_t i, vec4 pixel) {
  Pixel.pixels[i] = pixel;
  Pixel.isDirty = true;
}

Ptr<Material> ModelManager::newMaterial(vec3 color, PBR pbr, MaterialFlag flag) {
  return newMaterial(newTex(color), newTex(pbr), flag);
}

Ptr<Material> ModelManager::newMaterial(glm::vec4 color, PBR pbr, MaterialFlag flag) {
  return newMaterial(newTex(color), newTex(pbr), flag);
}

Ptr<Material> ModelManager::newMaterial(Tex color, Tex pbr, MaterialFlag flag) {
  Material m{color, pbr};
  m.flag = flag;
  return newMaterial(m);
}

Ptr<Material> ModelManager::newMaterial(const std::string &colorImage, PBR pbr) {
  return newMaterial(newTex(colorImage, {}, true, 0), newTex(pbr));
}

ConstPtr<Model> ModelManager::newModel(CallModelBuilder &build) {
  return newModel([&](ModelBuilder &builder) { build.build(builder); });
}

ConstPtr<Model> ModelManager::loadGLTF(
  const std::string &filename, const ModelCreateInfo &createInfo) {
  ModelGLTF loader(*this);
  return loader.loadGLTF(filename, createInfo);
}

Ptr<ModelInstance> ModelManager::newModelInstance(
  const Model &model, const Transform &transform) {
  return newModelInstance(model, newTransform(transform));
}

}
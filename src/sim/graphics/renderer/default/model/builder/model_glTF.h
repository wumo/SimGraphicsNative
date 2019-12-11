#pragma once
#include <tinygltf/tiny_gltf.h>
#include "sim/graphics/renderer/default/model/model.h"
#include "sim/graphics/base/resource/images.h"

namespace sim::graphics::renderer::defaultRenderer {
class ModelManager;
class ModelBuilder;

class ModelGLTF {
public:
  explicit ModelGLTF(ModelManager &modelManager);

  ConstPtr<Model> loadGLTF(
    const std::string &filename, const ModelCreateInfo &createInfo);
  void loadTextureSamplers(const tinygltf::Model &model);
  void loadTextures(const tinygltf::Model &model);
  void loadMaterials(const tinygltf::Model &model);
  Ptr<Node> loadNode(
    ModelBuilder &builder, int thisID, const tinygltf::Model &model,
    const ModelCreateInfo &createInfo);
  MeshPart loadPrimitive(
    const tinygltf::Model &model, const tinygltf::Primitive &primitive,
    const ModelCreateInfo &createInfo);
  Range loadIndices(const tinygltf::Model &model, const tinygltf::Primitive &primitive);
  std::pair<Range, AABB> loadVertices(
    const tinygltf::Model &model, const tinygltf::Primitive &primitive,
    const ModelCreateInfo &createInfo);

  std::vector<SamplerDef> samplerDefs;
  std::vector<Tex> textures;
  std::vector<Ptr<Material>> materials;
  std::unordered_map<uint32_t, Range> indicesMap;
  std::unordered_map<uint32_t, std::pair<Range, AABB>> verticesMap;
  ModelManager &mm;

  std::vector<uint32_t> indices;
  std::vector<Vertex> vertices;
  Model _model;
};
}

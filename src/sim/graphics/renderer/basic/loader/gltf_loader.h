#pragma once
#include "../basic_model_manager.h"
#include <tinygltf/tiny_gltf.h>

namespace sim::graphics::renderer::basic {
class GLTFLoader {
public:
  explicit GLTFLoader(BasicModelManager &mm);

  Ptr<Model> load(const std::string &file);

private:
  void loadTextureSamplers(const tinygltf::Model &model);
  void loadTextures(const tinygltf::Model &model);
  void loadMaterials(const tinygltf::Model &model);
  Ptr<Node> loadNode(int thisID, const tinygltf::Model &model);
  Ptr<Mesh> loadPrimitive(
    const tinygltf::Model &model, const tinygltf::Primitive &primitive);
  AABB loadVertices(const tinygltf::Model &model, const tinygltf::Primitive &primitive);
  void loadIndices(const tinygltf::Model &model, const tinygltf::Primitive &primitive);

  void loadAnimations(const tinygltf::Model &model);

  BasicModelManager &mm;

  std::vector<uint32_t> indices;
  std::vector<Vertex::Position> positions;
  std::vector<Vertex::Normal> normals;
  std::vector<Vertex::UV> uvs;
  std::vector<Vertex::Joint> joint0s;
  std::vector<Vertex::Weight> weight0s;
  std::vector<Ptr<Node>> _nodes;
  std::vector<Animation> animations;

  std::vector<SamplerDef> samplerDefs;
  std::vector<Ptr<TextureImage2D>> textures;
  std::vector<Ptr<Material>> materials;
};
}

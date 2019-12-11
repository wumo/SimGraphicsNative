#pragma once

#include "sim/graphics/base/device.h"
#include "model.h"
#include "model_limit.h"
#include "sim/graphics/renderer/default/model/builder/mesh_builder.h"
#include "sim/graphics/renderer/default/model/builder/model_builder.h"
#include "sim/graphics/base/pipeline/sampler.h"
#include "sim/graphics/base/vkcommon.h"

namespace sim::graphics::renderer::defaultRenderer {
class ModelManager;

class ModelInstance {
public:
  ModelInstance(const Model &model, ModelManager &modelManager, Ptr<Transform> transform);
  Ptr<Transform> transform() const;

  Node &node(uint32_t index);
  uint32_t numNodes() const;
  void visit(std::function<void(Node &)> visitor);

  AABB aabb{};

private:
  friend class DefaultModelManager;
  void loadNode(Node &dst, const Node &src) const;

  static void visitNode(Node &start, std::function<void(Node &)> &visitor);

  ModelManager *mm;
  std::vector<Node> nodes;
  Ptr<Transform> _transform;
};

class CallMeshBuilder {
public:
  virtual void build(MeshBuilder &) {}
};

class CallModelBuilder {
public:
  virtual void build(ModelBuilder &) {}
};

class ModelManager {
protected:
  ModelManager(Device &context, const ModelConfig &config);
  virtual ~ModelManager() = default;

public:
  /**
   * load GLTF model from file.
   * @param filename  model file name
   * @param createInfo scale/translation transform to apply to the loaded model.
   * @return pointer to the loaded model.
   */
  ConstPtr<Model> loadGLTF(
    const std::string &filename, const ModelCreateInfo &createInfo = {});

  virtual Range newIndices(std::vector<uint32_t> &indices) = 0;
  virtual Range newVertices(std::vector<Vertex> &vertices) = 0;
  ConstPtr<Mesh> newMesh(
    Range indices, Range vertices, Primitive primitive = Primitive::Triangles);
  ConstPtr<Mesh> newMesh(
    std::vector<uint32_t> &indices, std::vector<Vertex> &vertices,
    Primitive primitive = Primitive::Triangles);
  ConstPtr<Mesh> newMesh(
    std::vector<uint32_t> &indices, std::vector<glm::vec3> &vertexPositions,
    Primitive primitive = Primitive::Triangles);
  /**
   * create new model meshed using MeshBuilder.
   * @return the vector of created meshes.
   */
  ConstPtr<Mesh> newMesh(CallMeshBuilder &build);
  virtual std::vector<ConstPtr<Mesh>> newMeshes(
    const std::function<void(MeshBuilder &)> &build) = 0;
  virtual ConstPtr<Mesh> newProceduralMesh(
    const std::string &filename, std::vector<AABB> const &aabbs) = 0;
  virtual ConstPtr<Mesh> newProceduralMesh(
    const uint32_t *opcodes, size_t size, std::vector<AABB> const &aabbs) = 0;

  Tex newTex(glm::vec4 value);
  Tex newTex(glm::vec3 color);
  Tex newTex(PBR pbr);
  Tex newTex(Refractive refractive);
  virtual Tex newTex(
    const std::string &imagePath, const SamplerDef &samplerDef, bool generateMipmap,
    int32_t lodBias) = 0;

  virtual Tex newCubeTex(
    const std::string &imagePath, const SamplerDef &samplerDef, bool generateMipmap,
    int32_t lodBias) = 0;

  virtual Tex newTex(
    const unsigned char *bytes, size_t size, uint32_t texWidth, uint32_t texHeight,
    uint32_t channel, const SamplerDef &samplerDef, bool generateMipmap,
    int32_t lodBias) = 0;

  virtual Ptr<Material> newMaterial(Material material) = 0;
  /**
   * create new material with color and pbr parameters.
   * @param color rgb color.
   * @param pbr pair of  roughness and metalness
   * @return the pointer to the created material.
   */
  Ptr<Material> newMaterial(
    glm::vec3 color, PBR pbr = {}, MaterialFlag flag = MaterialFlag::eBRDF);
  Ptr<Material> newMaterial(
    glm::vec4 color, PBR pbr = {}, MaterialFlag flag = MaterialFlag::eTranslucent);
  Ptr<Material> newMaterial(Tex color, Tex pbr, MaterialFlag flag = MaterialFlag::eBRDF);
  Ptr<Material> newMaterial(const std::string &colorImage, PBR pbr = {});
  virtual Ptr<Transform> newTransform(const Transform &transform) = 0;
  /**
   * create new model using ModelBuilder.
   * @return the created model.
   */
  ConstPtr<Model> newModel(CallModelBuilder &build);
  virtual ConstPtr<Model> newModel(std::function<void(ModelBuilder &)> build) = 0;
  /**
   * add new model instance of the specified model and apply initial transform.
   * @param model the shape of the instance.
   * @param transform initial transform
   * @return the pointer to the created model instance.
   */
  Ptr<ModelInstance> newModelInstance(
    const Model &model, const Transform &transform = Transform{});
  virtual Ptr<ModelInstance> newModelInstance(
    const Model &model, Ptr<Transform> transform) = 0;

  virtual void updateIndices(
    ConstPtr<Mesh> mesh, uint32_t offset, std::vector<uint32_t> &indices) = 0;
  virtual void updateVertices(
    ConstPtr<Mesh> mesh, uint32_t offset, std::vector<Vertex> &vertices) = 0;

  void modifyPixel(uint32_t i, glm::vec4 pixel);

  virtual void transformChange(uint32_t transformIdx) = 0;
  virtual void materialChange(uint32_t materialID) = 0;

  virtual ConstPtr<Mesh> getMesh(uint32_t meshID) = 0;
  virtual uint32_t numMeshes() const = 0;
  virtual uint32_t numProceduralMeshes() const = 0;
  virtual Ptr<Material> material(uint32_t materialID) = 0;
  virtual uint32_t numMaterials() const = 0;
  virtual Ptr<Transform> transform(uint32_t transformID) = 0;
  virtual uint32_t numTransforms() const = 0;
  virtual ConstPtr<Model> getModel(uint32_t modelID) = 0;
  virtual uint32_t numModels() const = 0;
  virtual Ptr<ModelInstance> modelInstance(uint32_t modelInstanceID) = 0;
  virtual uint32_t numModelInstances() const = 0;
  virtual uint32_t numMeshInstances() const = 0;
  virtual uint32_t numVertices() const = 0;
  virtual uint32_t numIndices() const = 0;

  virtual void ensureVertices(uint32_t toAdd) const = 0;
  virtual void ensureIndices(uint32_t toAdd) const = 0;
  virtual void ensureMeshes(uint32_t toAdd) const = 0;
  virtual void ensureTransforms(uint32_t toAdd) const = 0;
  virtual void ensureMaterials(uint32_t toAdd) const = 0;
  virtual void ensureTextures(uint32_t toAdd) const = 0;

protected:
  virtual std::vector<ConstPtr<Mesh>> addMeshes(const std::vector<Mesh> &meshes) = 0;

  ModelConfig config;
  Device &device;

  struct {
    const uint32_t texWidth, texHeight;
    const uint32_t maxNumPixels, pixelsByteSize;
    /**There are 2048x2048 simple materials*/
    std::vector<glm::vec4> pixels;
    uint32_t count{0};
    bool isDirty{true};
    Tex emptyTex;
    const vk::Format format{vk::Format::eR32G32B32A32Sfloat};
  } Pixel;
};
}
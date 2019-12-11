#pragma once
#include "model_manager.h"
#include "sim/graphics/renderer/default/model/builder/mesh_builder.h"
#include "sim/graphics/renderer/default/model/builder/model_builder.h"
#include "sim/graphics/base/pipeline/descriptors.h"
#include <functional>
#include <unordered_map>
#include "sim/graphics/base/pipeline/shader.h"
#include "sim/graphics/base/pipeline/sampler.h"
#include "sim/graphics/base/resource/buffers.h"
#include "sim/graphics/base/resource/images.h"

namespace sim::graphics::renderer::defaultRenderer {

class DefaultModelManager: public ModelManager {
protected:
  explicit DefaultModelManager(Device &context, const ModelConfig &config = {});
  virtual ~DefaultModelManager() = default;

public:
  using ModelManager::newTex;
  using ModelManager::newMaterial;
  using ModelManager::newModel;
  using ModelManager::newModelInstance;

  Range newIndices(std::vector<uint32_t> &indices) override;
  Range newVertices(std::vector<Vertex> &vertices) override;
  std::vector<ConstPtr<Mesh>> newMeshes(
    const std::function<void(MeshBuilder &)> &function) override;
  ConstPtr<Mesh> newProceduralMesh(
    const std::string &filename, std::vector<AABB> const &aabbs) override;
  ConstPtr<Mesh> newProceduralMesh(
    const uint32_t *opcodes, size_t size, std::vector<AABB> const &aabbs) override;
  Tex newTex(
    const std::string &imagePath, const SamplerDef &samplerDef, bool generateMipmap,
    int32_t lodBias) override;
  Tex newCubeTex(
    const std::string &imagePath, const SamplerDef &samplerDef, bool generateMipmap,
    int32_t lodBias) override;
  Tex newTex(
    const unsigned char *bytes, size_t size, uint32_t texWidth, uint32_t texHeight,
    uint32_t channel, const SamplerDef &samplerDef, bool generateMipmap,
    int32_t lodBias) override;
  Ptr<Material> newMaterial(Material material) override;
  Ptr<Transform> newTransform(const Transform &transform) override;
  ConstPtr<Model> newModel(std::function<void(ModelBuilder &)> function) override;
  Ptr<ModelInstance> newModelInstance(
    const Model &model, Ptr<Transform> transform) override;
  void updateIndices(
    ConstPtr<Mesh> mesh, uint32_t offset, std::vector<uint32_t> &indices) override;
  void updateVertices(
    ConstPtr<Mesh> mesh, uint32_t offset, std::vector<Vertex> &vertices) override;
  void transformChange(uint32_t transformIdx) override;
  void materialChange(uint32_t materialID) override;
  ConstPtr<Mesh> getMesh(uint32_t meshID) override;
  uint32_t numMeshes() const override;
  uint32_t numProceduralMeshes() const override;
  Ptr<Material> material(uint32_t materialID) override;
  uint32_t numMaterials() const override;
  Ptr<Transform> transform(uint32_t transformID) override;
  uint32_t numTransforms() const override;
  ConstPtr<Model> getModel(uint32_t modelID) override;
  uint32_t numModels() const override;
  Ptr<ModelInstance> modelInstance(uint32_t modelInstanceID) override;
  uint32_t numModelInstances() const override;
  uint32_t numMeshInstances() const override;
  uint32_t numVertices() const override;
  uint32_t numIndices() const override;
  void ensureVertices(uint32_t toAdd) const override;
  void ensureIndices(uint32_t toAdd) const override;
  void ensureMeshes(uint32_t toAdd) const override;
  void ensureTransforms(uint32_t toAdd) const override;
  void ensureMaterials(uint32_t toAdd) const override;
  void ensureTextures(uint32_t toAdd) const override;

protected:
  std::vector<ConstPtr<Mesh>> addMeshes(const std::vector<Mesh> &meshes) override;

  virtual void updateInstances();
  void updateTransformsAndMaterials();
  void visitMeshParts(std::function<void(MeshPart &)> visitor);
  void uploadPixels(const vk::CommandBuffer &cb);
  void updateDescriptorSet(DescriptorSampler2DUpdater updater);
  void updateDescriptors();

  std::vector<ShaderModule> procShaders;
  struct {
    uPtr<HostVertexStorageBuffer> vertices;
    uPtr<HostIndexStorageBuffer> indices;
    uPtr<HostStorageBuffer> aabbs;
    uPtr<HostStorageBuffer> meshes;
    uPtr<HostStorageBuffer> transforms;
    uPtr<HostStorageBuffer> materials;
    uPtr<UploadBuffer> pixelStaging;
    uPtr<HostStorageBuffer> instancess;
  } Buffer;

  constexpr static uint32_t meshNoRef = uint32max;
  constexpr static uint32_t meshDupRef = uint32max - 1;
  struct {
    uint32_t indexCount{0}, vertexCount{0};
    uint32_t aabbCount{0};
    uint32_t meshCountTri{0}, meshCountLine{0}, meshCountProc{0};

    uint32_t transformUpdateMin{uint32max}, transformUpdateMax{0};
    uint32_t materialUpdateMin{uint32max}, materialUpdateMax{0};

    std::vector<Mesh> meshes;
    std::vector<Transform> transforms;
    std::vector<Material> materials;
    std::vector<Model> models;
    std::vector<ModelInstance> modelInstances;

    std::vector<uint32_t> meshRefByModel;
  } Geometry;

  struct {
    std::vector<MeshInstance> instances;
    std::vector<uint32_t> meshRefCount;
    uint32_t totalRef_tri{0}, totalRef_line{0}, totalRef_proc{0};
  } Instance;

  struct {
    std::vector<vk::DescriptorImageInfo> sampler2Ds;
    std::vector<TextureImage2D> textures;
    std::vector<TextureImageCube> cubeTextures;
    uint32_t lastImagesCount{0};
    std::unordered_map<std::string, uint32_t> files;
  } Image;
};
}

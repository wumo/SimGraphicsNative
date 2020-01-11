#pragma once
#include <functional>
#include <unordered_map>
#include "sim/graphics/base/pipeline/pipeline.h"

#include "sim/graphics/base/pipeline/shader.h"
#include "sim/graphics/base/pipeline/sampler.h"
#include "sim/graphics/base/pipeline/descriptors.h"
#include "sim/graphics/base/resource/images.h"
#include "sim/graphics/base/resource/buffers.h"
#include "sim/graphics/base/debug_marker.h"
#include "model/model_buffer.h"
#include "model_config.h"
#include "model/basic_model.h"
#include "model/draw_queue.h"
#include "model/light.h"
#include "model/model_instance.h"
#include "builder/primitive_builder.h"
#include "perspective_camera.h"
#include "terrain/terrain_manager.h"
#include "sim/graphics/renderer/basic/sky/sky_manager.h"
#include "ocean/ocean_manager.h"
#include "shadow/shadow_manager.h"
#include "model/dynamic/dynamic_mesh_manager.h"

namespace sim::graphics::renderer::basic {

class BasicRenderer;

class BasicSceneManager {
  using flag = vk::DescriptorBindingFlagBitsEXT;
  using shader = vk::ShaderStageFlagBits;
  friend class BasicRenderer;

public:
  explicit BasicSceneManager(BasicRenderer &renderer);

  Ptr<Primitive> newPrimitive(
    const Vertex::Position *positions, uint32_t numPositions,
    const Vertex::Normal *normals, uint32_t numNormals, const Vertex::UV *uvs,
    uint32_t numUVs, const uint32_t *indices, uint32_t numIndices, const AABB &aabb,
    const PrimitiveTopology &topology = PrimitiveTopology::Triangles,
    const DynamicType &type = DynamicType::Static);

  Ptr<Primitive> newPrimitive(const PrimitiveBuilder &primitiveBuilder);
  std::vector<Ptr<Primitive>> newPrimitives(const PrimitiveBuilder &primitiveBuilder);

  Ptr<Primitive> newDynamicPrimitive(
    uint32_t numVertices, uint32_t numIndices, const AABB &aabb,
    const PrimitiveTopology &topology = PrimitiveTopology::Triangles);

  Ptr<Texture2D> newTexture(
    const std::string &imagePath, const SamplerDef &samplerDef = {},
    bool generateMipmap = true);

  Ptr<Texture2D> newTexture(
    const unsigned char *bytes, size_t size, uint32_t width, uint32_t height,
    const SamplerDef &samplerDef = {}, bool generateMipmap = true);

  Ptr<TextureImageCube> newCubeTexture(
    const std::string &imagePath, const SamplerDef &samplerDef = {},
    bool generateMipmap = true);

  Ptr<Texture2D> newGrayTexture(
    const std::string &imagePath, const SamplerDef &samplerDef = {},
    bool generateMipmap = true);

  Ptr<Material> newMaterial(MaterialType type = MaterialType::eNone);

  Ptr<Mesh> newMesh(Ptr<Primitive> primitive, Ptr<Material> material);

  Ptr<Node> newNode(const Transform &transform = {}, const std::string &name = "");

  Ptr<Model> newModel(
    std::vector<Ptr<Node>> &&nodes, std::vector<Animation> &&animations = {});

  Ptr<Model> loadModel(const std::string &file);

  Ptr<ModelInstance> newModelInstance(Ptr<Model> model, const Transform &transform = {});

  void useEnvironmentMap(Ptr<TextureImageCube> envMap);

  void computeMesh(
    const std::string &shaderPath, Ptr<Primitive> primitive, uint32_t dispatchNumX = 1,
    uint32_t dispatchNumY = 1, uint32_t dispatchNumZ = 1);

  Ptr<Light> addLight(
    LightType type = LightType ::Directional, glm::vec3 direction = {0.f, -1.f, 0.f},
    glm::vec3 color = {1.f, 1.f, 1.f}, glm::vec3 location = {0.f, 1.f, 0.f});

  PerspectiveCamera &camera();

  DynamicMeshManager &dynamicMeshManager();
  SkyManager &skyManager();
  TerrainManager &terrainManager();
  OceanManager &oceanManager();

  ShadowManager &shadowManager();
  
  Ptr<Primitive> primitive(uint32_t index);
  Ptr<Material> material(uint32_t index);
  Ptr<Model> model(uint32_t index);
  Ptr<ModelInstance> modelInstance(uint32_t index);

  void setWireframe(bool wireframe);
  bool wireframe();
  void debugInfo();
  DebugMarker &debugMarker();
  Device &device();
  const Config &config() const;
  const ModelConfig &modelConfig() const;

private:
  friend class Material;
  friend class Node;
  friend class MeshInstance;
  friend class ModelInstance;
  friend class Light;
  friend class Primitive;
  friend class OceanManager;

  Allocation<Material::UBO> allocateMaterialUBO();
  Allocation<Light::UBO> allocateLightUBO();
  Allocation<glm::mat4> allocateMatrixUBO();
  Allocation<Primitive::UBO> allocatePrimitiveUBO();
  Allocation<MeshInstance::UBO> allocateMeshInstanceUBO();
  auto allocateDrawCMD(const Ptr<Primitive> &primitive, const Ptr<Material> &material)
    -> std::vector<Allocation<vk::DrawIndexedIndirectCommand>>;

private:
  void resize(vk::Extent2D extent);
  void updateScene(
    vk::CommandBuffer transferCB, vk::CommandBuffer computeCB, uint32_t imageIndex,
    float elapsedDuration);
  void updateTextures();
  void computeMesh(
    vk::CommandBuffer computeCB, uint32_t imageIndex, float elapsedDuration);

  void drawScene(vk::CommandBuffer cb, uint32_t imageIndex);

  void ensureTextures(uint32_t toAdd) const;

private:
  BasicRenderer &renderer;
  DebugMarker &debugMarker_;
  Device &device_;
  vk::Device vkDevice;
  const Config &config_;
  const ModelConfig &modelConfig_;

  uPtr<DynamicMeshManager> dynamicMeshManager_;
  uPtr<TerrainManager> terrainManager_;
  uPtr<SkyManager> skyManager_;
  uPtr<OceanManager> oceanManager_;
  uPtr<ShadowManager> shadowManager_;

  struct {
    uPtr<DeviceVertexBuffer<Vertex::Position>> position;
    uPtr<DeviceVertexBuffer<Vertex::Normal>> normal;
    uPtr<DeviceVertexBuffer<Vertex::Tangent>> tangent;
    uPtr<DeviceVertexBuffer<Vertex::UV>> uv;
    uPtr<DeviceVertexBuffer<Vertex::Joint>> joint0;
    uPtr<DeviceVertexBuffer<Vertex::Weight>> weight0;
    uPtr<DeviceIndexBuffer> indices;

    uPtr<HostManagedStorageUBOBuffer<Material::UBO>> materials;
    uPtr<HostManagedStorageUBOBuffer<glm::mat4>> transforms;
    uPtr<HostManagedStorageUBOBuffer<Primitive::UBO>> primitives;
    uPtr<HostManagedStorageUBOBuffer<MeshInstance::UBO>> meshInstances;
    uPtr<DrawQueue> drawQueue;

    uPtr<HostUBOBuffer<PerspectiveCamera::UBO>> camera;

    uPtr<HostUBOBuffer<Lighting::UBO>> lighting;
    uPtr<HostManagedStorageUBOBuffer<Light::UBO>> lights;

  } Buffer;

  struct {
    std::vector<Primitive> primitives;
    std::vector<Primitive> dynamicPrimitives;
    std::vector<Material> materials;
    std::vector<Mesh> meshes;
    std::vector<Node> nodes;
    std::vector<Model> models;
    std::vector<ModelInstance> instances;

    PerspectiveCamera camera{{10, 10, 10}, {0, 0, 0}, {0, 1, 0}};
    Lighting lighting{};
    std::vector<Light> lights;
  } Scene;

  struct {
    std::vector<vk::DescriptorImageInfo> sampler2Ds;
    std::vector<Texture2D> textures;
    std::vector<TextureImageCube> cubeTextures;
    uint32_t lastImagesCount{0};

    uPtr<Texture2D> brdfLUT;
    uPtr<TextureImageCube> irradiance, preFiltered;
    bool useEnvironmentMap{false};
  } Image;

  struct {
    vk::DescriptorSet basicSet, deferredSet, iblSet;
    vk::DescriptorSet computeMeshSet;
    vk::UniqueDescriptorPool descriptorPool;
  } Sets;

  struct {
    bool wireframe{false};
  } RenderPass;

  struct BasicSetDef: DescriptorSetDef {
    __uniform__(
      cam, shader::eVertex | shader::eFragment | shader::eTessellationControl |
             shader::eTessellationEvaluation);
    __buffer__(primitives, shader::eVertex | shader::eTessellationControl);
    __buffer__(meshInstances, shader::eVertex | shader::eTessellationControl);
    __buffer__(transforms, shader::eVertex | shader::eTessellationControl);
    __buffer__(
      material, shader::eVertex | shader::eFragment | shader::eTessellationControl);
    __samplers__(
      textures, flag{},
      shader::eVertex | shader::eFragment | shader::eTessellationControl |
        shader::eTessellationEvaluation);
    __uniform__(lighting, shader::eFragment);
    __buffer__(lights, shader::eFragment);
  } basicSetDef;

  struct DeferredSetDef: DescriptorSetDef {
    __input__(position, shader::eFragment);
    __input__(normal, shader::eFragment);
    __input__(albedo, shader::eFragment);
    __input__(pbr, shader::eFragment);
    __input__(emissive, shader::eFragment);
    __input__(depth, shader::eFragment);
  } deferredSetDef;

  struct IBLSetDef: DescriptorSetDef {
    __sampler__(irradiance, shader::eFragment);
    __sampler__(prefiltered, shader::eFragment);
    __sampler__(brdfLUT, shader::eFragment);
  } iblSetDef;

  struct BasicLayoutDef: PipelineLayoutDef {
    __set__(basic, BasicSetDef);
    __set__(deferred, DeferredSetDef);
    __set__(ibl, IBLSetDef);
    __set__(sky, SkyManager::SkySetDef);
  } basicLayout;

  struct ComputeSetDef: DescriptorSetDef {
    __buffer__(positions, shader::eCompute);
    __buffer__(normals, shader::eCompute);
  } computeMeshSetDef;

  struct ComputeMeshConstant {
    uint32_t positionOffset;
    uint32_t normalOffset;
    uint32_t vertexCount;
    float time;
  } computeMeshConstant;

  struct ComputeMeshLayoutDef: PipelineLayoutDef {
    __push_constant__(constant, shader::eCompute, ComputeMeshConstant);
    __set__(set, ComputeSetDef);
  } computeMeshLayoutDef;

  struct ComputeMeshDef {
    uint32_t dispatchNumX, dispatchNumY, dispatchNumZ;
    Ptr<Primitive> primitive;
    vk::UniquePipeline pipeline;
  };

  std::vector<ComputeMeshDef> computeMeshes;
};
}

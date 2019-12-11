#include "default_model_manager.h"

#include <cstring>
#include "sim/graphics/base/pipeline/descriptors.h"
#include <stb_image.h>

namespace sim::graphics::renderer::defaultRenderer {
using namespace glm;

DefaultModelManager::DefaultModelManager(Device &context, const ModelConfig &config)
  : ModelManager{context, config} {
  auto size = config.maxNumVertex * sizeof(Vertex);
  Buffer.vertices = u<HostVertexStorageBuffer>(device.allocator(), size);
  size = config.maxNumIndex * sizeof(uint32_t);
  Buffer.indices = u<HostIndexStorageBuffer>(device.allocator(), size);
  size = config.maxNumMeshes * sizeof(Mesh);
  Buffer.meshes = u<HostStorageBuffer>(device.allocator(), size);
  size = config.maxNumTransform * sizeof(Transform);
  Buffer.transforms = u<HostStorageBuffer>(device.allocator(), size);
  size = config.maxNumMaterial * sizeof(Material);
  Buffer.materials = u<HostStorageBuffer>(device.allocator(), size);

  Buffer.pixelStaging = u<UploadBuffer>(device.allocator(), Pixel.pixelsByteSize);

  size = config.maxNumMeshInstances * sizeof(MeshInstance);
  Buffer.instancess = u<HostStorageBuffer>(device.allocator(), size);

  Geometry.transforms.reserve(config.maxNumTransform);
  Geometry.transforms.emplace_back(); // index=0
  DefaultModelManager::transformChange(0);
  Geometry.materials.reserve(config.maxNumMaterial);
  Geometry.meshes.reserve(config.maxNumMeshes);
  Geometry.meshes.clear();
  Geometry.meshRefByModel.resize(config.maxNumMeshes, meshNoRef);

  Instance.instances.resize(config.maxNumMeshInstances);
  Instance.meshRefCount.resize(config.maxNumMeshes, 0);

  Pixel.pixels.resize(Pixel.maxNumPixels);
  Pixel.emptyTex = ModelManager::newTex(vec3()); //default tex for empty tex.
  Image.textures.emplace_back(
    device, Pixel.texWidth, Pixel.texHeight, false, Pixel.format);
  SamplerMaker samplerMaker;
  samplerMaker.magFilter(vk::Filter::eNearest)
    .minFilter(vk::Filter::eNearest)
    .mipmapMode(vk::SamplerMipmapMode::eNearest)
    .unnormalizedCoordinates(VK_TRUE)
    .addressModeU(vk::SamplerAddressMode::eClampToEdge)
    .addressModeV(vk::SamplerAddressMode::eClampToEdge)
    .addressModeW(vk::SamplerAddressMode::eClampToEdge);
  Image.textures[0].setSampler(samplerMaker.createUnique(context.getDevice()));
}

Range DefaultModelManager::newIndices(std::vector<uint32_t> &indices) {
  ensureIndices(uint32_t(indices.size()));
  auto indexOffset = Geometry.indexCount;
  memcpy(
    Buffer.indices->ptr<uint32_t>() + Geometry.indexCount, indices.data(),
    indices.size() * sizeof(uint32_t));
  Geometry.indexCount += uint32(indices.size());

  return {indexOffset, uint32(indices.size())};
}

Range DefaultModelManager::newVertices(std::vector<Vertex> &vertices) {
  ensureVertices(uint32_t(vertices.size()));
  auto vertexOffset = Geometry.vertexCount;
  memcpy(
    Buffer.vertices->ptr<Vertex>() + Geometry.vertexCount, vertices.data(),
    vertices.size() * sizeof(Vertex));
  Geometry.vertexCount += uint32_t(vertices.size());

  return {vertexOffset, uint32(vertices.size())};
}

std::vector<ConstPtr<Mesh>> DefaultModelManager::newMeshes(
  const std::function<void(MeshBuilder &)> &build) {
  MeshBuilder builder{Geometry.vertexCount, Geometry.indexCount,
                      uint32_t(Geometry.meshes.size())};
  build(builder);
  newVertices(builder.getVertices());
  newIndices(builder.getIndices());
  return addMeshes(builder.getMeshes());
}

std::vector<ConstPtr<Mesh>> DefaultModelManager::addMeshes(
  const std::vector<Mesh> &meshes) {
  ensureMeshes(uint32_t(meshes.size()));

  auto begin = Geometry.meshes.size();
  append(Geometry.meshes, meshes);
  auto end = Geometry.meshes.size();
  memcpy(
    Buffer.meshes->ptr<Mesh>() + begin, Geometry.meshes.data() + begin,
    (end - begin) * sizeof(Mesh));

  std::vector<ConstPtr<Mesh>> outMeshes;
  outMeshes.reserve(end - begin);
  for(auto i = begin; i < end; ++i) {
    switch(Geometry.meshes[i].primitive) {
      case Primitive ::Triangles: Geometry.meshCountTri++; break;
      case Primitive ::Lines: Geometry.meshCountLine++; break;
      default: error("shouldn't happen!"); break;
    }
    outMeshes.emplace_back(&Geometry.meshes, i);
  }
  return outMeshes;
}

ConstPtr<Mesh> DefaultModelManager::newProceduralMesh(
  const std::string &filename, std::vector<AABB> const &aabbs) {
  if(!Buffer.aabbs) {
    auto size = config.maxNumMeshes * sizeof(AABB);
    Buffer.aabbs = u<HostStorageBuffer>(device.allocator(), size);
  }
  auto ptr = Buffer.aabbs->ptr<AABB>() + Geometry.aabbCount;
  for(auto &aabb: aabbs) {
    *ptr = aabb;
    ptr++;
  }
  procShaders.emplace_back(ShaderModule(device.getDevice(), filename));
  Geometry.meshes.push_back(Mesh{
    {{uint32_t(procShaders.size() - 1), uint32_t(aabbs.size()), Geometry.aabbCount, 0}},
    Primitive ::Procedural});
  Geometry.aabbCount += uint32_t(aabbs.size());
  Geometry.meshCountProc++;
  return ConstPtr{&Geometry.meshes, uint32_t(Geometry.meshes.size() - 1)};
}

ConstPtr<Mesh> DefaultModelManager::newProceduralMesh(
  const uint32_t *opcodes, size_t size, std::vector<AABB> const &aabbs) {
  if(!Buffer.aabbs) {
    auto size = config.maxNumMeshes * sizeof(AABB);
    Buffer.aabbs = u<HostStorageBuffer>(device.allocator(), size);
  }
  auto ptr = Buffer.aabbs->ptr<AABB>() + Geometry.aabbCount;
  for(auto &aabb: aabbs) {
    *ptr = aabb;
    ptr++;
  }
  procShaders.emplace_back(ShaderModule(device.getDevice(), opcodes, size));
  Geometry.meshes.push_back(Mesh{
    {{uint32_t(procShaders.size() - 1), uint32_t(aabbs.size()), Geometry.aabbCount, 0}},
    Primitive ::Procedural});
  Geometry.aabbCount += uint32_t(aabbs.size());
  Geometry.meshCountProc++;
  return ConstPtr{&Geometry.meshes, uint32_t(Geometry.meshes.size() - 1)};
}

Tex DefaultModelManager::newTex(
  const std::string &imagePath, const SamplerDef &samplerDef, bool generateMipmap,
  int32_t lodBias) {
  auto imageID = Image.files.find(imagePath);
  if(imageID == Image.files.end()) {
    ensureTextures(1);

    Image.textures.emplace_back(
      TextureImage2D::loadFromFile(device, imagePath, generateMipmap));
    auto &texture = Image.textures.back();
    auto id = uint32_t(Image.textures.size() - 1);
    SamplerMaker maker{samplerDef};
    auto mipLevel = texture.getInfo().mipLevels;
    maker.maxLod(float(texture.getInfo().mipLevels));
    texture.setSampler(maker.createUnique(device.getDevice()));
    Tex tex{id};
    tex.lodBias = lodBias;
    tex.maxLod = texture.getInfo().mipLevels - 1;

    Image.files[imagePath] = tex.texID;
    return tex;
  }
  return Tex{imageID->second};
}

Tex DefaultModelManager::newCubeTex(
  const std::string &imagePath, const SamplerDef &samplerDef, bool generateMipmap,
  int32_t lodBias) {
  auto imageID = Image.files.find(imagePath);
  if(imageID == Image.files.end()) {
    ensureTextures(1);

    Image.cubeTextures.emplace_back(
      TextureImageCube::loadFromFile(device, imagePath, generateMipmap));
    auto &texture = Image.cubeTextures.back();
    auto id = uint32_t(Image.cubeTextures.size() - 1);
    SamplerMaker maker{samplerDef};
    maker.maxLod(float(texture.getInfo().mipLevels));
    texture.setSampler(maker.createUnique(device.getDevice()));
    Tex tex{id};
    tex.lodBias = lodBias;
    tex.maxLod = texture.getInfo().mipLevels - 1;

    Image.files[imagePath] = tex.texID;
    return tex;
  }
  return Tex{imageID->second};
}

Tex DefaultModelManager::newTex(
  const unsigned char *bytes, size_t size, uint32_t texWidth, uint32_t texHeight,
  uint32_t channel, const SamplerDef &samplerDef, bool generateMipmap, int32_t lodBias) {
  errorIf(!(channel == 3 || channel == 4), "Currently only support 4 channel image!");

  ensureTextures(1);

  Image.textures.emplace_back(TextureImage2D::loadFromBytes(
    device, bytes, size, texWidth, texHeight, generateMipmap));
  auto &texture = Image.textures.back();
  auto id = uint32_t(Image.textures.size() - 1);
  SamplerMaker maker{samplerDef};
  auto mipLevel = texture.getInfo().mipLevels;
  maker.maxLod(float(texture.getInfo().mipLevels));
  texture.setSampler(maker.createUnique(device.getDevice()));
  Tex tex{id};
  tex.lodBias = lodBias;
  tex.maxLod = texture.getInfo().mipLevels - 1;
  return tex;
}

Ptr<Material> DefaultModelManager::newMaterial(Material material) {
  ensureMaterials(1);
  Geometry.materials.push_back(material);
  materialChange(uint32_t(Geometry.materials.size() - 1));
  return Ptr<Material>(&Geometry.materials, uint32_t(Geometry.materials.size() - 1));
}

Ptr<Transform> DefaultModelManager::newTransform(const Transform &transform) {
  ensureTransforms(1);
  Geometry.transforms.push_back(transform);
  transformChange(uint32_t(Geometry.transforms.size() - 1));
  return Ptr<Transform>(&Geometry.transforms, uint32_t(Geometry.transforms.size() - 1));
}

ConstPtr<Model> DefaultModelManager::newModel(std::function<void(ModelBuilder &)> build) {
  ModelBuilder builder{*this};
  build(builder);
  auto modelIdx = uint32_t(Geometry.models.size());
  for(auto &node: builder.getNodes())
    for(auto &meshPart: node.meshParts)
      if(auto &ref = Geometry.meshRefByModel[meshPart.mesh.index()]; ref == meshNoRef)
        ref = modelIdx;
      else
        ref = meshDupRef;
  Geometry.models.push_back({std::move(builder.getNodes())});

  return ConstPtr(&Geometry.models, uint32_t(Geometry.models.size() - 1));
}

Ptr<ModelInstance> DefaultModelManager::newModelInstance(
  const Model &model, Ptr<Transform> transform) {
  Geometry.modelInstances.emplace_back(model, *this, transform);
  uint32_t refTriCount{0}, refLineCount{0}, refProcCount{0};
  Geometry.modelInstances.back().visit([&](Node &node) {
    for(auto &meshPart: node.meshParts) {
      auto meshPtr = meshPart.mesh;
      if(meshPtr.isNull()) continue;
      Instance.meshRefCount[meshPtr.index()]++;
      switch(meshPtr->primitive) {
        case Primitive ::Triangles: refTriCount++; break;
        case Primitive ::Lines: refLineCount++; break;
        case Primitive ::Procedural: refProcCount++; break;
        default: error("Invalid mesh flag");
      }
    }
  });
  Instance.totalRef_tri += refTriCount;
  Instance.totalRef_line += refLineCount;
  Instance.totalRef_proc += refProcCount;
  errorIf(
    numMeshInstances() > config.maxNumMeshInstances,
    "exceeding maximum number of mesh instances!");
  return Ptr(&Geometry.modelInstances, uint32_t(Geometry.modelInstances.size() - 1));
}

void DefaultModelManager::updateIndices(
  ConstPtr<Mesh> mesh, uint32_t offset, std::vector<uint32_t> &indices) {
  errorIf(offset + indices.size() > mesh->indexCount, "exceeding mesh indices' range!");
  memcpy(
    Buffer.indices->ptr<uint32_t>() + mesh->indexOffset + offset, indices.data(),
    indices.size() * sizeof(uint32_t));
}

void DefaultModelManager::updateVertices(
  ConstPtr<Mesh> mesh, uint32_t offset, std::vector<Vertex> &vertices) {
  errorIf(
    offset + vertices.size() > mesh->vertexCount, "exceeding mesh vertices' range!");
  memcpy(
    Buffer.vertices->ptr<Vertex>() + mesh->vertexOffset, vertices.data(),
    vertices.size() * sizeof(Vertex));
}

uint32_t DefaultModelManager::numModelInstances() const {
  return uint32_t(Geometry.modelInstances.size());
}
Ptr<ModelInstance> DefaultModelManager::modelInstance(uint32_t modelInstanceID) {
  return Ptr<ModelInstance>(&Geometry.modelInstances, modelInstanceID);
}
uint32_t DefaultModelManager::numModels() const {
  return uint32_t(Geometry.models.size());
}
ConstPtr<Model> DefaultModelManager::getModel(uint32_t modelID) {
  return ConstPtr(&Geometry.models, modelID);
}
uint32_t DefaultModelManager::numTransforms() const {
  return uint32_t(Geometry.transforms.size());
}
Ptr<Transform> DefaultModelManager::transform(uint32_t transformID) {
  return Ptr(&Geometry.transforms, transformID);
}
uint32_t DefaultModelManager::numMaterials() const {
  return uint32_t(Geometry.materials.size());
}
Ptr<Material> DefaultModelManager::material(uint32_t materialID) {
  return Ptr(&Geometry.materials, materialID);
}
uint32_t DefaultModelManager::numProceduralMeshes() const {
  return uint32_t(procShaders.size());
}
uint32_t DefaultModelManager::numMeshes() const {
  return uint32_t(Geometry.meshes.size());
}
ConstPtr<Mesh> DefaultModelManager::getMesh(uint32_t meshID) {
  return ConstPtr{&Geometry.meshes, meshID};
}
uint32_t DefaultModelManager::numMeshInstances() const {
  return Instance.totalRef_tri + Instance.totalRef_line + Instance.totalRef_proc;
}
uint32_t DefaultModelManager::numVertices() const { return Geometry.vertexCount; }
uint32_t DefaultModelManager::numIndices() const { return Geometry.indexCount; }

void DefaultModelManager::ensureVertices(uint32_t toAdd) const {
  errorIf(
    Geometry.vertexCount + toAdd > config.maxNumVertex,
    "exceeding maximum number of vertices!");
}
void DefaultModelManager::ensureIndices(uint32_t toAdd) const {
  errorIf(
    Geometry.indexCount + toAdd > config.maxNumIndex,
    "exceeding maximum number of vertices!");
}
void DefaultModelManager::ensureMeshes(uint32_t toAdd) const {
  errorIf(
    Geometry.meshes.size() + toAdd > config.maxNumMeshes,
    "exceeding maximum number of meshes!");
}
void DefaultModelManager::ensureTransforms(uint32_t toAdd) const {
  errorIf(
    Geometry.transforms.size() + toAdd > config.maxNumTransform,
    "exceeding maximum number of transforms!");
}
void DefaultModelManager::ensureMaterials(uint32_t toAdd) const {
  errorIf(
    Geometry.materials.size() + toAdd > config.maxNumMaterial,
    "exceeding maximum number of vertices!");
}
void DefaultModelManager::ensureTextures(uint32_t toAdd) const {
  errorIf(
    Image.textures.size() + toAdd > config.maxNumTexture,
    "exceeding maximum number of textures!");
}

void DefaultModelManager::uploadPixels(const vk::CommandBuffer &cb) {
  if(Pixel.isDirty) {
    memcpy(Buffer.pixelStaging->ptr(), Pixel.pixels.data(), Pixel.pixelsByteSize);
    Image.textures[0].setCurrentLayout(vk::ImageLayout::eUndefined);
    Image.textures[0].setLayout(cb, vk::ImageLayout::eTransferDstOptimal);
    auto extent = Image.textures[0].extent();
    Image.textures[0].copy(
      cb, Buffer.pixelStaging->buffer(), 0, 0, extent.width, extent.height, extent.depth,
      0);
    Image.textures[0].setLayout(cb, vk::ImageLayout::eShaderReadOnlyOptimal);
    Pixel.isDirty = false;
  }
}

void DefaultModelManager::updateDescriptorSet(DescriptorSampler2DUpdater updater) {
  if(Image.textures.size() > Image.lastImagesCount) {
    updateDescriptors();
    updater(
      Image.lastImagesCount, uint32_t(Image.sampler2Ds.size()) - Image.lastImagesCount,
      Image.sampler2Ds.data() + Image.lastImagesCount);
    Image.lastImagesCount = uint32_t(Image.textures.size());
  }
}

void DefaultModelManager::updateDescriptors() {
  Image.sampler2Ds.resize(Image.textures.size());
  for(size_t i = 0; i < Image.textures.size(); ++i)
    Image.sampler2Ds[i] = {Image.textures[i].sampler(), Image.textures[i].imageView(),
                           vk::ImageLayout::eShaderReadOnlyOptimal};
}

void DefaultModelManager::transformChange(uint32_t transformIdx) {
  Geometry.transformUpdateMin = std::min(Geometry.transformUpdateMin, transformIdx);
  Geometry.transformUpdateMax = std::max(Geometry.transformUpdateMax, transformIdx);
}

void DefaultModelManager::materialChange(uint32_t materialID) {
  Geometry.materialUpdateMin = std::min(Geometry.materialUpdateMin, materialID);
  Geometry.materialUpdateMax = std::max(Geometry.materialUpdateMax, materialID);
}

void DefaultModelManager::updateTransformsAndMaterials() {
  if(Geometry.transformUpdateMin <= Geometry.transformUpdateMax) {
    Buffer.transforms->updateVector(
      Geometry.transforms, Geometry.transformUpdateMin * sizeof(Transform),
      Geometry.transformUpdateMax - Geometry.transformUpdateMin + 1,
      Geometry.transformUpdateMin);
  }
  Geometry.transformUpdateMin = uint32max;
  Geometry.transformUpdateMax = 0;

  if(Geometry.materialUpdateMin <= Geometry.materialUpdateMax) {
    Buffer.materials->updateVector(
      Geometry.materials, Geometry.materialUpdateMin * sizeof(Material),
      Geometry.materialUpdateMax - Geometry.materialUpdateMin + 1,
      Geometry.materialUpdateMin);
  }
  Geometry.materialUpdateMin = uint32max;
  Geometry.materialUpdateMax = 0;
}

void DefaultModelManager::visitMeshParts(std::function<void(MeshPart &)> visitor) {
  for(auto &modelInstance: Geometry.modelInstances)
    modelInstance.visit([&](Node &node) {
      for(auto &meshPart: node.meshParts) {
        if(meshPart.mesh.isNull()) return;
        visitor(meshPart);
      }
    });
}

void DefaultModelManager::updateInstances() {
  uint32_t idx{0};
  visitMeshParts(
    [&](MeshPart &meshPart) { Instance.instances[idx++] = meshPart.toMeshInstance(); });
  if(idx > 0) Buffer.instancess->updateVector(Instance.instances, 0, idx);
}
}

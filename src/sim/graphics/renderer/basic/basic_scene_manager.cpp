#include "basic_scene_manager.h"
#include "basic_renderer.h"
#include "sim/graphics/util/colors.h"
#include "loader/gltf_loader.h"
#include "ibl/envmap_generator.h"
#include "sim/graphics/base/pipeline/descriptor_pool_maker.h"

namespace sim::graphics::renderer::basic {
using namespace glm;
using namespace material;
using bindpoint = vk::PipelineBindPoint;

BasicSceneManager::BasicSceneManager(BasicRenderer &renderer)
  : renderer{renderer},
    debugMarker_{renderer.debugMarker},
    device_{*renderer.device},
    config{renderer.config},
    modelConfig{renderer.modelConfig},
    vkDevice{renderer.vkDevice} {
  {
    auto &allocator = renderer.device->allocator();
    Buffer.position =
      u<DeviceVertexBuffer<Vertex::Position>>(allocator, modelConfig.maxNumVertex);
    Buffer.normal =
      u<DeviceVertexBuffer<Vertex::Normal>>(allocator, modelConfig.maxNumVertex);
    Buffer.uv = u<DeviceVertexBuffer<Vertex::UV>>(allocator, modelConfig.maxNumVertex);
    Buffer.joint0 =
      u<DeviceVertexBuffer<Vertex::Joint>>(allocator, modelConfig.maxNumVertex);
    Buffer.weight0 =
      u<DeviceVertexBuffer<Vertex::Weight>>(allocator, modelConfig.maxNumVertex);
    Buffer.indices = u<DeviceIndexBuffer>(allocator, modelConfig.maxNumVertex);

    Buffer.transforms =
      u<HostManagedStorageUBOBuffer<glm::mat4>>(allocator, modelConfig.maxNumTransform);

    Buffer.materials = u<HostManagedStorageUBOBuffer<Material::UBO>>(
      allocator, modelConfig.maxNumMaterial);

    Buffer.primitives = u<HostManagedStorageUBOBuffer<Primitive::UBO>>(
      allocator, modelConfig.maxNumPrimitives);

    Buffer.camera = u<HostUBOBuffer<PerspectiveCamera::UBO>>(allocator);

    Buffer.lighting = u<HostUBOBuffer<Lighting::UBO>>(allocator);
    Scene.lighting.setNumLights(modelConfig.maxNumLights);
    Buffer.lights =
      u<HostManagedStorageUBOBuffer<Light::UBO>>(allocator, modelConfig.maxNumLights);

    Buffer.drawQueue = u<DrawQueue>(
      allocator, modelConfig.maxNumMeshes, modelConfig.maxNumLineMeshes,
      modelConfig.maxNumTransparentMeshes, modelConfig.maxNumTransparentLineMeshes,
      modelConfig.maxNumTerranMeshes, config.numFrame, modelConfig.maxNumDynamicMeshes,
      modelConfig.maxNumDynamicLineMeshes, modelConfig.maxNumDynamicTransparentMeshes,
      modelConfig.maxNumDynamicTransparentLineMeshes,
      modelConfig.maxNumDynamicTerranMeshes);
    auto totalMeshes = modelConfig.maxNumMeshes + modelConfig.maxNumLineMeshes +
                       modelConfig.maxNumTransparentMeshes +
                       modelConfig.maxNumTransparentLineMeshes +
                       modelConfig.maxNumTerranMeshes;
    Buffer.meshInstances =
      u<HostManagedStorageUBOBuffer<MeshInstance::UBO>>(allocator, totalMeshes);
  }

  dynamicMeshManager_ = u<DynamicMeshManager>(*this);
  terrainManager_ = u<TerrainManager>(*this);
  skyManager_ = u<SkyManager>(*this);
  oceanManager_ = u<OceanManager>(*this);

  {
    basicSetDef.textures.descriptorCount() = uint32_t(modelConfig.maxNumTexture);
    basicSetDef.init(vkDevice);

    deferredSetDef.init(vkDevice);
    iblSetDef.init(vkDevice);

    computeMeshSetDef.init(vkDevice);
    computeMeshLayoutDef.set(computeMeshSetDef);
    computeMeshLayoutDef.init(vkDevice);

    basicLayout.basic(basicSetDef);
    basicLayout.deferred(deferredSetDef);
    basicLayout.ibl(iblSetDef);
    basicLayout.sky(skyManager_->skySetDef);
    basicLayout.init(vkDevice);

    Sets.descriptorPool = DescriptorPoolMaker()
                            .pipelineLayout(basicLayout)
                            .pipelineLayout(computeMeshLayoutDef)
                            .createUnique(vkDevice);

    Sets.basicSet = basicSetDef.createSet(*Sets.descriptorPool);
    Sets.deferredSet = deferredSetDef.createSet(*Sets.descriptorPool);
    Sets.iblSet = iblSetDef.createSet(*Sets.descriptorPool);
    skyManager_->createDescriptorSets(*Sets.descriptorPool);
  }

  {
    basicSetDef.cam(Buffer.camera->buffer());
    basicSetDef.primitives(Buffer.primitives->buffer());
    basicSetDef.meshInstances(Buffer.meshInstances->buffer());
    basicSetDef.transforms(Buffer.transforms->buffer());
    basicSetDef.material(Buffer.materials->buffer());
    basicSetDef.lighting(Buffer.lighting->buffer());
    basicSetDef.lights(Buffer.lights->buffer());

    { // empty texture;
      Image.textures.emplace_back(device_, 1, 1);
      glm::vec4 color{1.f, 1.f, 1.f, 1.f};
      Image.textures.back().upload(
        device_, reinterpret_cast<const unsigned char *>(&color), sizeof(color));
      Image.textures.back().setSampler(SamplerMaker().createUnique(vkDevice));

      // bind unused textures to empty texture;
      Image.sampler2Ds.reserve(modelConfig.maxNumTexture);
      for(auto i = 0u; i < modelConfig.maxNumTexture; ++i)
        Image.sampler2Ds.emplace_back(
          Image.textures.back().sampler(), Image.textures.back().imageView(),
          vk::ImageLayout ::eShaderReadOnlyOptimal);
      Image.lastImagesCount = 1;
      basicSetDef.textures(0, uint32_t(Image.sampler2Ds.size()), Image.sampler2Ds.data());

      newMaterial(); //empty material
    }

    basicSetDef.update(Sets.basicSet);
  }

  {
    Sets.computeMeshSet = computeMeshSetDef.createSet(*Sets.descriptorPool);
    computeMeshSetDef.positions(Buffer.position->buffer());
    computeMeshSetDef.normals(Buffer.normal->buffer());
    computeMeshSetDef.update(Sets.computeMeshSet);
  }

  {
    debugMarker_.name(Buffer.position->buffer(), "position buffer");
    debugMarker_.name(Buffer.normal->buffer(), "normal buffer");
    debugMarker_.name(Buffer.uv->buffer(), "uv buffer");
    debugMarker_.name(Buffer.joint0->buffer(), "joint0 buffer");
    debugMarker_.name(Buffer.weight0->buffer(), "weight0 buffer");
    debugMarker_.name(Buffer.transforms->buffer(), "transforms buffer");
    debugMarker_.name(Buffer.materials->buffer(), "materials buffer");
    debugMarker_.name(Buffer.primitives->buffer(), "primitives buffer");
    debugMarker_.name(Buffer.meshInstances->buffer(), "mesh instances buffer");
    Buffer.drawQueue->mark(debugMarker_);
    debugMarker_.name(Buffer.camera->buffer(), "camera buffer");
    debugMarker_.name(Buffer.lighting->buffer(), "lighting buffer");
    debugMarker_.name(Buffer.lights->buffer(), "lights buffer");
  }
}

Allocation<Material::UBO> BasicSceneManager::allocateMaterialUBO() {
  return Buffer.materials->allocate();
}
Allocation<Light::UBO> BasicSceneManager::allocateLightUBO() {
  return Buffer.lights->allocate();
}
Allocation<glm::mat4> BasicSceneManager::allocateMatrixUBO() {
  return Buffer.transforms->allocate();
}
Allocation<Primitive::UBO> BasicSceneManager::allocatePrimitiveUBO() {
  return Buffer.primitives->allocate();
}
Allocation<MeshInstance::UBO> BasicSceneManager::allocateMeshInstanceUBO() {
  return Buffer.meshInstances->allocate();
}
auto BasicSceneManager::allocateDrawCMD(
  const Ptr<Primitive> &primitive, const Ptr<Material> &material)
  -> std::vector<Allocation<vk::DrawIndexedIndirectCommand>> {
  return Buffer.drawQueue->allocate(primitive, material);
}

void BasicSceneManager::resize(vk::Extent2D extent) {
  Scene.camera.changeDimension(extent.width, extent.height);

  deferredSetDef.position(renderer.attachments.position->imageView());
  deferredSetDef.normal(renderer.attachments.normal->imageView());
  deferredSetDef.albedo(renderer.attachments.albedo->imageView());
  deferredSetDef.pbr(renderer.attachments.pbr->imageView());
  deferredSetDef.emissive(renderer.attachments.emissive->imageView());
  deferredSetDef.depth(renderer.attachments.depth->imageView());
  deferredSetDef.update(Sets.deferredSet);
}

PerspectiveCamera &BasicSceneManager::camera() { return Scene.camera; }
DynamicMeshManager &BasicSceneManager::dynamicMeshManager() {
  return *dynamicMeshManager_;
}
SkyManager &BasicSceneManager::skyManager() { return *skyManager_; }
TerrainManager &BasicSceneManager::terrainManager() { return *terrainManager_; }
OceanManager &BasicSceneManager::oceanManager() { return *oceanManager_; }

Ptr<Primitive> BasicSceneManager::newPrimitive(
  const Vertex::Position *positions, uint32_t numPositions, const Vertex::Normal *normals,
  uint32_t numNormals, const Vertex::UV *uvs, uint32_t numUVs, const uint32_t *indices,
  uint32_t numIndices, const AABB &aabb, const PrimitiveTopology &topology,
  const DynamicType &type) {
  Range positionRange, normalRange, uvRange, indexRange;
  switch(type) {
    case DynamicType::Static:
      positionRange = Buffer.position->add(device_, positions, numPositions);
      normalRange = Buffer.normal->add(device_, normals, numNormals);
      uvRange = Buffer.uv->add(device_, uvs, numUVs);
      indexRange = Buffer.indices->add(device_, indices, numIndices);
      break;
    case DynamicType::Dynamic:
      positionRange =
        Buffer.position->add(device_, positions, numPositions, config.numFrame);
      normalRange = Buffer.normal->add(device_, normals, numNormals, config.numFrame);
      uvRange = Buffer.uv->add(device_, uvs, numUVs, config.numFrame);
      indexRange = Buffer.indices->add(device_, indices, numIndices, config.numFrame);
      break;
  }

  return Ptr<Primitive>::add(
    Scene.primitives,
    {*this, indexRange, positionRange, normalRange, uvRange, aabb, topology, type});
}

Ptr<Primitive> BasicSceneManager::newPrimitive(const PrimitiveBuilder &primitiveBuilder) {
  return newPrimitives(primitiveBuilder)[0];
}

auto BasicSceneManager::newPrimitives(const PrimitiveBuilder &primitiveBuilder)
  -> std::vector<Ptr<Primitive>> {
  std::vector<Ptr<Primitive>> primitives;
  for(auto &primitive: primitiveBuilder.primitives()) {
    primitives.push_back(newPrimitive(
      primitiveBuilder.positions().data() + primitive._position.offset,
      primitive._position.size,
      primitiveBuilder.normals().data() + primitive._normal.offset,
      primitive._normal.size, primitiveBuilder.uvs().data() + primitive._uv.offset,
      primitive._uv.size, primitiveBuilder.indices().data() + primitive._index.offset,
      primitive._index.size, primitive._aabb, primitive._topology, primitive._type));
  }
  return primitives;
}

Ptr<Primitive> BasicSceneManager::newDynamicPrimitive(
  uint32_t numVertices, uint32_t numIndices, const AABB &aabb,
  const PrimitiveTopology &topology) {

  auto positionRange = Buffer.position->add(device_, numVertices * config.numFrame);
  auto normalRange = Buffer.normal->add(device_, numVertices * config.numFrame);
  auto uvRange = Buffer.uv->add(device_, numVertices * config.numFrame);
  auto indexRange = Buffer.indices->add(device_, numIndices * config.numFrame);
  return Ptr<Primitive>::add(
    Scene.primitives, {*this, indexRange, positionRange, normalRange, uvRange, aabb,
                       topology, DynamicType::Dynamic});
}

Ptr<Texture2D> BasicSceneManager::newTexture(
  const std::string &imagePath, const SamplerDef &samplerDef, bool generateMipmap) {
  ensureTextures(1);
  auto t = Texture2D::loadFromFile(
    device_, imagePath, vk::Format::eR8G8B8A8Srgb, generateMipmap);
  t.setSampler(SamplerMaker(samplerDef).createUnique(vkDevice));
  return Ptr<Texture2D>::add(Image.textures, std::move(t));
}

Ptr<Texture2D> BasicSceneManager::newTexture(
  const unsigned char *bytes, size_t size, uint32_t width, uint32_t height,
  const SamplerDef &samplerDef, bool generateMipmap) {
  ensureTextures(1);
  auto t = Texture2D::loadFromBytes(device_, bytes, size, width, height, generateMipmap);
  t.setSampler(SamplerMaker(samplerDef).createUnique(vkDevice));
  return Ptr<Texture2D>::add(Image.textures, std::move(t));
}

Ptr<TextureImageCube> BasicSceneManager::newCubeTexture(
  const std::string &imagePath, const SamplerDef &samplerDef, bool generateMipmap) {
  ensureTextures(1);

  TextureImageCube texture =
    TextureImageCube::loadFromFile(device_, imagePath, generateMipmap);
  SamplerMaker maker{samplerDef};
  maker.maxLod(float(texture.getInfo().mipLevels));
  texture.setSampler(maker.createUnique(device_.getDevice()));

  return Ptr<TextureImageCube>::add(Image.cubeTextures, std::move(texture));
}

Ptr<Texture2D> BasicSceneManager::newGrayTexture(
  const std::string &imagePath, const SamplerDef &samplerDef, bool generateMipmap) {
  ensureTextures(1);
  auto t = Texture2D::loadFromGrayScaleFile(
    device_, imagePath, vk::Format::eR16Unorm, generateMipmap);
  t.setSampler(SamplerMaker(samplerDef).createUnique(vkDevice));
  return Ptr<Texture2D>::add(Image.textures, std::move(t));
}

Ptr<Material> BasicSceneManager::newMaterial(MaterialType type) {
  return Ptr<Material>::add(Scene.materials, Material{*this, type});
}

Ptr<Mesh> BasicSceneManager::newMesh(Ptr<Primitive> primitive, Ptr<Material> material) {
  return Ptr<Mesh>::add(Scene.meshes, Mesh{primitive, material});
}

Ptr<Node> BasicSceneManager::newNode(
  const Transform &transform, const std::string &name) {
  return Ptr<Node>::add(Scene.nodes, Node{*this, transform, name});
}

Ptr<Model> BasicSceneManager::newModel(
  std::vector<Ptr<Node>> &&nodes, std::vector<Animation> &&animations) {
  return Ptr<Model>::add(
    Scene.models, std::move(Model{std::move(nodes), std::move(animations)}));
}

Ptr<Model> BasicSceneManager::loadModel(const std::string &file) {
  GLTFLoader loader{*this};
  return loader.load(file);
}

Ptr<ModelInstance> BasicSceneManager::newModelInstance(
  Ptr<Model> model, const Transform &transform) {
  auto instance =
    Ptr<ModelInstance>::add(Scene.instances, ModelInstance{*this, model, transform});
  ModelInstance::applyModel(model, instance);
  return instance;
}

void BasicSceneManager::useEnvironmentMap(Ptr<TextureImageCube> envMap) {
  EnvMapGenerator envMapGenerator{device_, *this};
  Image.brdfLUT = envMapGenerator.generateBRDFLUT();
  auto cubes = envMapGenerator.generateEnvMap(*envMap);
  Image.irradiance = std::move(cubes.irradiance);
  Image.preFiltered = std::move(cubes.preFiltered);
  Image.useEnvironmentMap = true;

  iblSetDef.irradiance(*Image.irradiance);
  iblSetDef.prefiltered(*Image.preFiltered);
  iblSetDef.brdfLUT(*Image.brdfLUT);
  iblSetDef.update(Sets.iblSet);
}

void BasicSceneManager::computeMesh(
  const std::string &shaderPath, Ptr<Primitive> primitive, uint32_t dispatchNumX,
  uint32_t dispatchNumY, uint32_t dispatchNumZ) {
  errorIf(
    primitive->type() != DynamicType::Dynamic,
    "compute mesh should be dynamic primitive!");
  errorIf(
    computeMeshes.size() + 1 > modelConfig.maxNumDynamicMeshes,
    "exceeding max number of dynamic meshes!");

  ComputePipelineMaker pipelineMaker{vkDevice};
  pipelineMaker.shader(shaderPath);

  vk::UniquePipeline pipeline =
    pipelineMaker.createUnique(nullptr, *computeMeshLayoutDef.pipelineLayout);
  computeMeshes.push_back(
    {dispatchNumX, dispatchNumY, dispatchNumZ, primitive, std::move(pipeline)});
}

Ptr<Light> BasicSceneManager::addLight(
  LightType type, glm::vec3 direction, glm::vec3 color, glm::vec3 location) {
  return Ptr<Light>::add(Scene.lights, Light{*this, type, direction, color, location});
}

void BasicSceneManager::updateScene(
  vk::CommandBuffer transferCB, vk::CommandBuffer computeCB, uint32_t imageIndex,
  float elapsedDuration) {
  if(Scene.camera.incoherent()) Buffer.camera->update(device_, Scene.camera.flush());

  if(Scene.lighting.incoherent())
    Buffer.lighting->update(device_, Scene.lighting.flush());

  updateTextures();

  computeMesh(computeCB, imageIndex, elapsedDuration);
}

void BasicSceneManager::updateTextures() {
  if(Image.textures.size() > Image.lastImagesCount) {
    for(int i = Image.lastImagesCount; i < Image.textures.size(); ++i)
      Image.sampler2Ds[i] = {Image.textures[i].sampler(), Image.textures[i].imageView(),
                             vk::ImageLayout ::eShaderReadOnlyOptimal};
    basicSetDef.textures(
      Image.lastImagesCount, uint32_t(Image.textures.size()) - Image.lastImagesCount,
      Image.sampler2Ds.data() + Image.lastImagesCount);
    Image.lastImagesCount = uint32_t(Image.textures.size());
    basicSetDef.update(Sets.basicSet);
  }
}

void BasicSceneManager::computeMesh(
  vk::CommandBuffer cb, uint32_t imageIndex, float elapsedDuration) {
  static float time = 0;
  time += elapsedDuration;

  cb.bindDescriptorSets(
    bindpoint::eCompute, *computeMeshLayoutDef.pipelineLayout,
    computeMeshLayoutDef.set.set(), Sets.computeMeshSet, nullptr);

  uint32_t total = computeMeshes.size();
  for(int i = 0; i < total; ++i) {
    auto &comp = computeMeshes[i];
    auto &positionRange = comp.primitive->position();
    auto &normalRange = comp.primitive->normal();
    computeMeshConstant = {
      positionRange.offset + imageIndex * positionRange.size / config.numFrame,
      normalRange.offset + imageIndex * normalRange.size / config.numFrame,
      positionRange.size / config.numFrame, time};

    debugMarker_.begin(cb, toString("compute mesh: ", i, " frame:", imageIndex).c_str());
    cb.bindPipeline(bindpoint::eCompute, *comp.pipeline);
    cb.pushConstants<ComputeMeshConstant>(
      *computeMeshLayoutDef.pipelineLayout, shader::eCompute, 0, computeMeshConstant);
    cb.dispatch(comp.dispatchNumX, comp.dispatchNumY, comp.dispatchNumZ);
    debugMarker_.end(cb);
  }
}

void BasicSceneManager::drawScene(vk::CommandBuffer cb, uint32_t imageIndex) {
  vk::DeviceSize zero{0};
  auto stride = sizeof(vk::DrawIndexedIndirectCommand);

  cb.bindDescriptorSets(
    bindpoint::eGraphics, *basicLayout.pipelineLayout, basicLayout.basic.set(),
    Sets.basicSet, nullptr);
  cb.bindDescriptorSets(
    bindpoint::eGraphics, *basicLayout.pipelineLayout, basicLayout.deferred.set(),
    Sets.deferredSet, nullptr);
  if(Image.useEnvironmentMap)
    cb.bindDescriptorSets(
      bindpoint::eGraphics, *basicLayout.pipelineLayout, basicLayout.ibl.set(),
      Sets.iblSet, nullptr);
  if(skyManager_->enabled())
    cb.bindDescriptorSets(
      bindpoint::eGraphics, *basicLayout.pipelineLayout, basicLayout.sky.set(),
      skyManager_->skySet, nullptr);

  cb.bindVertexBuffers(0, Buffer.position->buffer(), zero);
  cb.bindVertexBuffers(1, Buffer.normal->buffer(), zero);
  cb.bindVertexBuffers(2, Buffer.uv->buffer(), zero);
  cb.bindIndexBuffer(Buffer.indices->buffer(), zero, vk::IndexType::eUint32);

  debugMarker_.begin(cb, "Subpass opaque tri");
  if(RenderPass.wireframe)
    cb.bindPipeline(bindpoint::eGraphics, *renderer.Pipelines.opaqueTriWireframe);
  else
    cb.bindPipeline(bindpoint::eGraphics, *renderer.Pipelines.opaqueTri);
  cb.drawIndexedIndirect(
    Buffer.drawQueue->buffer(DrawQueue::DrawType::OpaqueTriangles), 0,
    Buffer.drawQueue->count(DrawQueue::DrawType::OpaqueTriangles), stride);
  debugMarker_.end(cb);

  debugMarker_.begin(cb, "Subpass dynamic opaque tri");
  cb.drawIndexedIndirect(
    Buffer.drawQueue->buffer(DrawQueue::DrawType::OpaqueTriangles, imageIndex), 0,
    Buffer.drawQueue->count(DrawQueue::DrawType::OpaqueTriangles, imageIndex), stride);
  debugMarker_.end(cb);

  debugMarker_.begin(cb, "Subpass terrain");
  if(RenderPass.wireframe)
    cb.bindPipeline(bindpoint::eGraphics, *renderer.Pipelines.terrainTessWireframe);
  else
    cb.bindPipeline(bindpoint::eGraphics, *renderer.Pipelines.terrainTess);

  cb.drawIndexedIndirect(
    Buffer.drawQueue->buffer(DrawQueue::DrawType::Terrain), 0,
    Buffer.drawQueue->count(DrawQueue::DrawType::Terrain), stride);
  debugMarker_.end(cb);

  debugMarker_.begin(cb, "Subpass dynamic terrain");
  cb.drawIndexedIndirect(
    Buffer.drawQueue->buffer(DrawQueue::DrawType::Terrain, imageIndex), 0,
    Buffer.drawQueue->count(DrawQueue::DrawType::Terrain, imageIndex), stride);
  debugMarker_.end(cb);

  debugMarker_.begin(cb, "Subpass opaque line");
  cb.bindPipeline(bindpoint::eGraphics, *renderer.Pipelines.opaqueLine);
  cb.drawIndexedIndirect(
    Buffer.drawQueue->buffer(DrawQueue::DrawType::OpaqueLines), 0,
    Buffer.drawQueue->count(DrawQueue::DrawType::OpaqueLines), stride);
  debugMarker_.end(cb);

  debugMarker_.begin(cb, "Subpass dynamic opaque line");
  cb.drawIndexedIndirect(
    Buffer.drawQueue->buffer(DrawQueue::DrawType::OpaqueLines, imageIndex), 0,
    Buffer.drawQueue->count(DrawQueue::DrawType::OpaqueLines, imageIndex), stride);
  debugMarker_.end(cb);

  debugMarker_.begin(cb, "Subpass deferred shading");
  cb.nextSubpass(vk::SubpassContents::eInline);
  if(Image.useEnvironmentMap)
    cb.bindPipeline(bindpoint::eGraphics, *renderer.Pipelines.deferredIBL);
  else if(skyManager_->enabled())
    cb.bindPipeline(bindpoint::eGraphics, *renderer.Pipelines.deferredSky);
  else
    cb.bindPipeline(bindpoint::eGraphics, *renderer.Pipelines.deferred);
  cb.draw(3, 1, 0, 0);
  debugMarker_.end(cb);

  debugMarker_.begin(cb, "Subpass translucent tri");
  cb.nextSubpass(vk::SubpassContents::eInline);
  cb.bindPipeline(bindpoint::eGraphics, *renderer.Pipelines.transTri);
  cb.drawIndexedIndirect(
    Buffer.drawQueue->buffer(DrawQueue::DrawType::TransparentTriangles), 0,
    Buffer.drawQueue->count(DrawQueue::DrawType::TransparentTriangles), stride);
  debugMarker_.end(cb);

  debugMarker_.begin(cb, "Subpass dynamic translucent tri");
  cb.drawIndexedIndirect(
    Buffer.drawQueue->buffer(DrawQueue::DrawType::TransparentTriangles, imageIndex), 0,
    Buffer.drawQueue->count(DrawQueue::DrawType::TransparentTriangles, imageIndex),
    stride);
  debugMarker_.end(cb);

  debugMarker_.begin(cb, "Subpass translucent line");
  cb.bindPipeline(bindpoint::eGraphics, *renderer.Pipelines.transLine);
  cb.drawIndexedIndirect(
    Buffer.drawQueue->buffer(DrawQueue::DrawType::TransparentLines), 0,
    Buffer.drawQueue->count(DrawQueue::DrawType::TransparentLines), stride);

  debugMarker_.begin(cb, "Subpass dynamic translucent line");
  cb.drawIndexedIndirect(
    Buffer.drawQueue->buffer(DrawQueue::DrawType::TransparentLines, imageIndex), 0,
    Buffer.drawQueue->count(DrawQueue::DrawType::TransparentLines, imageIndex), stride);
  debugMarker_.end(cb);

  debugMarker_.begin(cb, "Subpass resolve");
  cb.nextSubpass(vk::SubpassContents::eInline);
  debugMarker_.end(cb);
  debugMarker_.end(cb);
}

void BasicSceneManager::debugInfo() {
  auto totalMeshes = modelConfig.maxNumMeshes + modelConfig.maxNumLineMeshes +
                     modelConfig.maxNumTransparentMeshes +
                     modelConfig.maxNumTransparentLineMeshes;
  sim::debugLog(
    "vertices: ", Buffer.position->count(), "/", modelConfig.maxNumVertex,
    ", indices: ", Buffer.indices->count(), "/", modelConfig.maxNumIndex,
    ", transforms: ", Buffer.transforms->count(), "/", modelConfig.maxNumTransform,
    ", meshes: ", Scene.meshes.size(), "/", totalMeshes,
    ", materials: ", Buffer.materials->count(), "/", modelConfig.maxNumMaterial,
    ", textures: ", Image.textures.size(), "/", modelConfig.maxNumTexture,
    ", lights: ", Scene.lights.size(), "/", modelConfig.maxNumLights);
}

DebugMarker &BasicSceneManager::debugMarker() { return debugMarker_; }
Device &BasicSceneManager::device() { return device_; }

void BasicSceneManager::ensureTextures(uint32_t toAdd) const {
  errorIf(
    Image.textures.size() + toAdd > modelConfig.maxNumTexture,
    "exceeding maximum number of textures!");
}
Ptr<Primitive> BasicSceneManager::primitive(uint32_t index) {
  assert(index < Scene.primitives.size());
  return Ptr<Primitive>{&Scene.primitives, index};
}
Ptr<Material> BasicSceneManager::material(uint32_t index) {
  assert(index < Scene.materials.size());
  return Ptr<Material>{&Scene.materials, index};
}
Ptr<Model> BasicSceneManager::model(uint32_t index) {
  assert(index < Scene.models.size());
  return Ptr<Model>{&Scene.models, index};
}
Ptr<ModelInstance> BasicSceneManager::modelInstance(uint32_t index) {
  assert(index < Scene.instances.size());
  return Ptr<ModelInstance>{&Scene.instances, index};
}

void BasicSceneManager::setWireframe(bool wireframe) { RenderPass.wireframe = wireframe; }
bool BasicSceneManager::wireframe() { return RenderPass.wireframe; }
}
#include "basic_model_manager.h"
#include "basic_renderer.h"
#include "sim/graphics/util/colors.h"
#include "loader/gltf_loader.h"
#include "ibl/envmap_generator.h"
#include "sim/graphics/base/pipeline/descriptor_pool_maker.h"

namespace sim::graphics::renderer::basic {
using namespace glm;
using namespace material;
using bindpoint = vk::PipelineBindPoint;

BasicModelManager::BasicModelManager(BasicRenderer &renderer)
  : renderer{renderer},
    debugMarker{renderer.debugMarker},
    device{*renderer.device},
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
      modelConfig.maxNumTerranMeshes);
    auto totalMeshes = modelConfig.maxNumMeshes + modelConfig.maxNumLineMeshes +
                       modelConfig.maxNumTransparentMeshes +
                       modelConfig.maxNumTransparentLineMeshes +
                       modelConfig.maxNumTerranMeshes;
    Buffer.meshInstances =
      u<HostManagedStorageUBOBuffer<MeshInstance::UBO>>(allocator, totalMeshes);
  }

  {
    basicSetDef.textures.descriptorCount() = uint32_t(modelConfig.maxNumTexture);
    basicSetDef.init(vkDevice);

    deferredSetDef.init(vkDevice);
    iblSetDef.init(vkDevice);
    skySetDef.init(vkDevice);

    basicLayout.basic(basicSetDef);
    basicLayout.deferred(deferredSetDef);
    basicLayout.ibl(iblSetDef);
    basicLayout.sky(skySetDef);
    basicLayout.init(vkDevice);

    Sets.descriptorPool =
      DescriptorPoolMaker().pipelineLayout(basicLayout).createUnique(vkDevice);

    Sets.basicSet = basicSetDef.createSet(*Sets.descriptorPool);
    Sets.deferredSet = deferredSetDef.createSet(*Sets.descriptorPool);
    Sets.iblSet = iblSetDef.createSet(*Sets.descriptorPool);
    Sets.skySet = skySetDef.createSet(*Sets.descriptorPool);
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
      Image.textures.emplace_back(device, 1, 1);
      glm::vec4 color{1.f, 1.f, 1.f, 1.f};
      Image.textures.back().upload(
        device, reinterpret_cast<const unsigned char *>(&color), sizeof(color));
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

  terrainMgr = u<TerrainManager>(*this);

  skyRenderer = u<SkyRenderer>(device, debugMarker);

  {
    debugMarker.name(Buffer.position->buffer(), "position buffer");
    debugMarker.name(Buffer.normal->buffer(), "normal buffer");
    debugMarker.name(Buffer.uv->buffer(), "uv buffer");
    debugMarker.name(Buffer.joint0->buffer(), "joint0 buffer");
    debugMarker.name(Buffer.weight0->buffer(), "weight0 buffer");
    debugMarker.name(Buffer.transforms->buffer(), "transforms buffer");
    debugMarker.name(Buffer.materials->buffer(), "materials buffer");
    debugMarker.name(Buffer.primitives->buffer(), "primitives buffer");
    debugMarker.name(Buffer.meshInstances->buffer(), "mesh instances buffer");
    Buffer.drawQueue->mark(debugMarker);
    debugMarker.name(Buffer.camera->buffer(), "camera buffer");
    debugMarker.name(Buffer.lighting->buffer(), "lighting buffer");
    debugMarker.name(Buffer.lights->buffer(), "lights buffer");
  }
}

Allocation<Material::UBO> BasicModelManager::allocateMaterialUBO() {
  return Buffer.materials->allocate();
}
Allocation<Light::UBO> BasicModelManager::allocateLightUBO() {
  return Buffer.lights->allocate();
}
Allocation<glm::mat4> BasicModelManager::allocateMatrixUBO() {
  return Buffer.transforms->allocate();
}
Allocation<Primitive::UBO> BasicModelManager::allocatePrimitiveUBO() {
  return Buffer.primitives->allocate();
}
Allocation<MeshInstance::UBO> BasicModelManager::allocateMeshInstanceUBO() {
  return Buffer.meshInstances->allocate();
}
auto BasicModelManager::allocateDrawCMD(
  const Ptr<Primitive> &primitive, const Ptr<Material> &material)
  -> Allocation<vk::DrawIndexedIndirectCommand> {
  return Buffer.drawQueue->allocate(primitive, material);
}

void BasicModelManager::resize(vk::Extent2D extent) {
  Scene.camera.changeDimension(extent.width, extent.height);

  deferredSetDef.position(renderer.attachments.position->imageView());
  deferredSetDef.normal(renderer.attachments.normal->imageView());
  deferredSetDef.albedo(renderer.attachments.albedo->imageView());
  deferredSetDef.pbr(renderer.attachments.pbr->imageView());
  deferredSetDef.emissive(renderer.attachments.emissive->imageView());
  deferredSetDef.depth(renderer.attachments.depth->imageView());
  deferredSetDef.update(Sets.deferredSet);
}

PerspectiveCamera &BasicModelManager::camera() { return Scene.camera; }

TerrainManager &BasicModelManager::terrrainManager() { return *terrainMgr; }

Ptr<Primitive> BasicModelManager::newPrimitive(
  const std::vector<Vertex::Position> &positions,
  const std::vector<Vertex::Normal> &normals, const std::vector<Vertex::UV> &uvs,
  const std::vector<uint32_t> &indices, const AABB &aabb,
  const PrimitiveTopology &topology) {

  auto positionRange = Buffer.position->add(device, positions.data(), positions.size());
  auto normalRange = Buffer.normal->add(device, normals.data(), normals.size());
  auto uvRange = Buffer.uv->add(device, uvs.data(), uvs.size());
  auto indexRange = Buffer.indices->add(device, indices.data(), indices.size());

  return Ptr<Primitive>::add(
    Scene.primitives,
    {*this, indexRange, positionRange, normalRange, uvRange, aabb, topology});
}

Ptr<Primitive> BasicModelManager::newPrimitive(const PrimitiveBuilder &primitiveBuilder) {
  return newPrimitives(primitiveBuilder)[0];
}

auto BasicModelManager::newPrimitives(const PrimitiveBuilder &primitiveBuilder)
  -> std::vector<Ptr<Primitive>> {
  std::vector<Ptr<Primitive>> primitives;
  for(auto &primitive: primitiveBuilder.primitives()) {
    auto positionRange = Buffer.position->add(
      device, primitiveBuilder.positions().data() + primitive.position().offset,
      primitive.position().size);
    auto normalRange = Buffer.normal->add(
      device, primitiveBuilder.normals().data() + primitive.normal().offset,
      primitive.normal().size);
    auto uvRange = Buffer.uv->add(
      device, primitiveBuilder.uvs().data() + primitive.uv().offset, primitive.uv().size);
    auto indexRange = Buffer.indices->add(
      device, primitiveBuilder.indices().data() + primitive.index().offset,
      primitive.index().size);
    primitives.push_back(Ptr<Primitive>::add(
      Scene.primitives, {*this, indexRange, positionRange, normalRange, uvRange,
                         primitive.aabb(), primitive.topology()}));
  }
  return primitives;
}

Ptr<Texture2D> BasicModelManager::newTexture(
  const std::string &imagePath, const SamplerDef &samplerDef, bool generateMipmap) {
  ensureTextures(1);
  auto t =
    Texture2D::loadFromFile(device, imagePath, vk::Format::eR8G8B8A8Srgb, generateMipmap);
  t.setSampler(SamplerMaker(samplerDef).createUnique(vkDevice));
  return Ptr<Texture2D>::add(Image.textures, std::move(t));
}

Ptr<Texture2D> BasicModelManager::newTexture(
  const unsigned char *bytes, size_t size, uint32_t width, uint32_t height,
  const SamplerDef &samplerDef, bool generateMipmap) {
  ensureTextures(1);
  auto t = Texture2D::loadFromBytes(device, bytes, size, width, height, generateMipmap);
  t.setSampler(SamplerMaker(samplerDef).createUnique(vkDevice));
  return Ptr<Texture2D>::add(Image.textures, std::move(t));
}

Ptr<TextureImageCube> BasicModelManager::newCubeTexture(
  const std::string &imagePath, const SamplerDef &samplerDef, bool generateMipmap) {
  ensureTextures(1);

  TextureImageCube texture =
    TextureImageCube::loadFromFile(device, imagePath, generateMipmap);
  SamplerMaker maker{samplerDef};
  maker.maxLod(float(texture.getInfo().mipLevels));
  texture.setSampler(maker.createUnique(device.getDevice()));

  return Ptr<TextureImageCube>::add(Image.cubeTextures, std::move(texture));
}

Ptr<Texture2D> BasicModelManager::newGrayTexture(
  const std::string &imagePath, const SamplerDef &samplerDef, bool generateMipmap) {
  ensureTextures(1);
  auto t = Texture2D::loadFromGrayScaleFile(
    device, imagePath, vk::Format::eR16Unorm, generateMipmap);
  t.setSampler(SamplerMaker(samplerDef).createUnique(vkDevice));
  return Ptr<Texture2D>::add(Image.textures, std::move(t));
}

Ptr<Material> BasicModelManager::newMaterial(MaterialType type) {
  return Ptr<Material>::add(Scene.materials, Material{*this, type});
}

Ptr<Mesh> BasicModelManager::newMesh(Ptr<Primitive> primitive, Ptr<Material> material) {
  return Ptr<Mesh>::add(Scene.meshes, Mesh{primitive, material});
}

Ptr<Node> BasicModelManager::newNode(
  const Transform &transform, const std::string &name) {
  return Ptr<Node>::add(Scene.nodes, Node{*this, transform, name});
}

Ptr<Model> BasicModelManager::newModel(
  std::vector<Ptr<Node>> &&nodes, std::vector<Animation> &&animations) {
  return Ptr<Model>::add(
    Scene.models, std::move(Model{std::move(nodes), std::move(animations)}));
}

Ptr<Model> BasicModelManager::loadModel(const std::string &file) {
  GLTFLoader loader{*this};
  return loader.load(file);
}

Ptr<ModelInstance> BasicModelManager::newModelInstance(
  Ptr<Model> model, const Transform &transform) {
  auto instance =
    Ptr<ModelInstance>::add(Scene.instances, ModelInstance{*this, model, transform});
  ModelInstance::applyModel(model, instance);
  return instance;
}

void BasicModelManager::useEnvironmentMap(Ptr<TextureImageCube> envMap) {
  EnvMapGenerator envMapGenerator{device, *this};
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

void BasicModelManager::useSky() {
  skyRenderer->updateModel();

  skySetDef.atmosphere(skyRenderer->model().atmosphereUBO().buffer());
  skySetDef.sun(skyRenderer->model().sunUBO().buffer());
  skySetDef.transmittance(skyRenderer->model().transmittanceTexture());
  skySetDef.scattering(skyRenderer->model().scatteringTexture());
  skySetDef.irradiance(skyRenderer->model().irradianceTexture());
  skySetDef.update(Sets.skySet);
}

void BasicModelManager::setSunPosition(
  float sun_zenith_angle_radians, float sun_azimuth_angle_radians) {
  skyRenderer->model().updateSunPosition(
    sun_zenith_angle_radians, sun_azimuth_angle_radians);
}

void BasicModelManager::computeMesh(
  const std::string &imagePath, Ptr<Primitive> primitive) {}

Ptr<Light> BasicModelManager::addLight(
  LightType type, glm::vec3 direction, glm::vec3 color, glm::vec3 location) {
  return Ptr<Light>::add(Scene.lights, Light{*this, type, direction, color, location});
}

void BasicModelManager::updateScene(vk::CommandBuffer cb, uint32_t imageIndex) {
  if(Scene.camera.incoherent()) Buffer.camera->update(device, Scene.camera.flush());

  if(Scene.lighting.incoherent()) Buffer.lighting->update(device, Scene.lighting.flush());

  updateTextures();
}

void BasicModelManager::updateTextures() {
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

void BasicModelManager::drawScene(vk::CommandBuffer cb, uint32_t imageIndex) {
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
  if(skyRenderer->enabled())
    cb.bindDescriptorSets(
      bindpoint::eGraphics, *basicLayout.pipelineLayout, basicLayout.sky.set(),
      Sets.skySet, nullptr);

  cb.bindVertexBuffers(0, Buffer.position->buffer(), zero);
  cb.bindVertexBuffers(1, Buffer.normal->buffer(), zero);
  cb.bindVertexBuffers(2, Buffer.uv->buffer(), zero);
  cb.bindIndexBuffer(Buffer.indices->buffer(), zero, vk::IndexType::eUint32);

  debugMarker.begin(cb, "Subpass opaque tri");
  if(RenderPass.wireframe)
    cb.bindPipeline(bindpoint::eGraphics, *renderer.Pipelines.opaqueTriWireframe);
  else
    cb.bindPipeline(bindpoint::eGraphics, *renderer.Pipelines.opaqueTri);
  cb.drawIndexedIndirect(
    Buffer.drawQueue->buffer(DrawQueue::DrawType::OpaqueTriangles), 0,
    Buffer.drawQueue->count(DrawQueue::DrawType::OpaqueTriangles), stride);
  debugMarker.end(cb);

  debugMarker.begin(cb, "Subpass terrain");
  if(RenderPass.wireframe)
    cb.bindPipeline(bindpoint::eGraphics, *renderer.Pipelines.terrainTessWireframe);
  else
    cb.bindPipeline(bindpoint::eGraphics, *renderer.Pipelines.terrainTess);

  cb.drawIndexedIndirect(
    Buffer.drawQueue->buffer(DrawQueue::DrawType::Terrain), 0,
    Buffer.drawQueue->count(DrawQueue::DrawType::Terrain), stride);
  debugMarker.end(cb);

  debugMarker.begin(cb, "Subpass opaque line");
  cb.bindPipeline(bindpoint::eGraphics, *renderer.Pipelines.opaqueLine);
  cb.drawIndexedIndirect(
    Buffer.drawQueue->buffer(DrawQueue::DrawType::OpaqueLines), 0,
    Buffer.drawQueue->count(DrawQueue::DrawType::OpaqueLines), stride);
  debugMarker.end(cb);

  debugMarker.begin(cb, "Subpass deferred shading");
  cb.nextSubpass(vk::SubpassContents::eInline);
  if(Image.useEnvironmentMap)
    cb.bindPipeline(bindpoint::eGraphics, *renderer.Pipelines.deferredIBL);
  else if(skyRenderer->enabled())
    cb.bindPipeline(bindpoint::eGraphics, *renderer.Pipelines.deferredSky);
  else
    cb.bindPipeline(bindpoint::eGraphics, *renderer.Pipelines.deferred);
  cb.draw(3, 1, 0, 0);
  debugMarker.end(cb);

  debugMarker.begin(cb, "Subpass translucent tri");
  cb.nextSubpass(vk::SubpassContents::eInline);
  cb.bindPipeline(bindpoint::eGraphics, *renderer.Pipelines.transTri);
  cb.drawIndexedIndirect(
    Buffer.drawQueue->buffer(DrawQueue::DrawType::TransparentTriangles), 0,
    Buffer.drawQueue->count(DrawQueue::DrawType::TransparentTriangles), stride);
  debugMarker.end(cb);

  debugMarker.begin(cb, "Subpass translucent line");
  cb.bindPipeline(bindpoint::eGraphics, *renderer.Pipelines.transLine);
  cb.drawIndexedIndirect(
    Buffer.drawQueue->buffer(DrawQueue::DrawType::TransparentLines), 0,
    Buffer.drawQueue->count(DrawQueue::DrawType::TransparentLines), stride);

  debugMarker.begin(cb, "Subpass resolve");
  cb.nextSubpass(vk::SubpassContents::eInline);
  debugMarker.end(cb);
  debugMarker.end(cb);
}

void BasicModelManager::debugInfo() {
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
void BasicModelManager::ensureTextures(uint32_t toAdd) const {
  errorIf(
    Image.textures.size() + toAdd > modelConfig.maxNumTexture,
    "exceeding maximum number of textures!");
}
Ptr<Primitive> BasicModelManager::primitive(uint32_t index) {
  assert(index < Scene.primitives.size());
  return Ptr<Primitive>{&Scene.primitives, index};
}
Ptr<Material> BasicModelManager::material(uint32_t index) {
  assert(index < Scene.materials.size());
  return Ptr<Material>{&Scene.materials, index};
}
Ptr<Model> BasicModelManager::model(uint32_t index) {
  assert(index < Scene.models.size());
  return Ptr<Model>{&Scene.models, index};
}
Ptr<ModelInstance> BasicModelManager::modelInstance(uint32_t index) {
  assert(index < Scene.instances.size());
  return Ptr<ModelInstance>{&Scene.instances, index};
}

void BasicModelManager::setWireframe(bool wireframe) { RenderPass.wireframe = wireframe; }
bool BasicModelManager::wireframe() { return RenderPass.wireframe; }
}
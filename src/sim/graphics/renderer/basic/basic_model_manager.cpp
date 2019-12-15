#include "basic_model_manager.h"
#include "basic_renderer.h"
#include "sim/graphics/util/materials.h"
#include "loader/gltf_loader.h"
#include "ibl/envmap_generator.h"

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

    Buffer.dynamicPrimitives = u<DevicePrimitivesBuffer>(
      allocator, modelConfig.maxNumDynamicVertex, modelConfig.maxNumDynamicIndex,
      config.numFrame);

    Buffer.transforms =
      u<HostStorageUBOBuffer<glm::mat4>>(allocator, modelConfig.maxNumTransform);
    Scene.freeTransforms.reserve(modelConfig.maxNumTransform);
    for(int32_t i = modelConfig.maxNumTransform; i > 0; --i)
      Scene.freeTransforms.push_back(i - 1);

    Buffer.materials =
      u<HostStorageUBOBuffer<Material::UBO>>(allocator, modelConfig.maxNumMaterial);
    Scene.freeMaterials.reserve(modelConfig.maxNumMaterial);
    for(int32_t i = modelConfig.maxNumMaterial; i > 0; --i)
      Scene.freeMaterials.push_back(i - 1);

    Buffer.camera = u<HostUBOBuffer<PerspectiveCamera::UBO>>(allocator);

    Buffer.lighting = u<HostUBOBuffer<Lighting::UBO>>(allocator);
    Scene.lighting.setNumLights(modelConfig.maxNumLights);
    Buffer.lights =
      u<HostStorageUBOBuffer<Light::UBO>>(allocator, modelConfig.maxNumLights);
    Scene.freeLights.reserve(modelConfig.maxNumLights);
    for(int32_t i = modelConfig.maxNumLights; i > 0; --i)
      Scene.freeLights.push_back(i - 1);

    Buffer.drawQueue = {u<HostIndirectUBOBuffer<vk::DrawIndexedIndirectCommand>>(
                          allocator, modelConfig.maxNumMeshes),
                        u<HostIndirectUBOBuffer<vk::DrawIndexedIndirectCommand>>(
                          allocator, modelConfig.maxNumLineMeshes),
                        u<HostIndirectUBOBuffer<vk::DrawIndexedIndirectCommand>>(
                          allocator, modelConfig.maxNumTransparentMeshes),
                        u<HostIndirectUBOBuffer<vk::DrawIndexedIndirectCommand>>(
                          allocator, modelConfig.maxNumTransparentLineMeshes)};
    auto totalMeshes = modelConfig.maxNumMeshes + modelConfig.maxNumLineMeshes +
                       modelConfig.maxNumTransparentMeshes +
                       modelConfig.maxNumTransparentLineMeshes;
    Buffer.meshes = u<HostStorageUBOBuffer<Mesh::UBO>>(allocator, totalMeshes);
    Scene.freeMeshes.reserve(totalMeshes);
    for(int32_t i = totalMeshes; i > 0; --i)
      Scene.freeMeshes.push_back(i - 1);
  }

  {
    uint32_t numUniformDescriptor = 1 /*camera*/ + 1 /*Lighting*/ + 1 /*ibl*/;
    uint32_t numStorageBufferDescriptor = 1 /*transforms*/ + 1 /*materials*/ +
                                          1 /*lights*/ + 1 /*drawCMD*/ +
                                          1 /*meshInstances*/;
    auto numTextureDescriptor = modelConfig.maxNumTexture + 3 /*IBL*/;
    uint32_t numInputAttachment = 5 /*deferred*/;
    uint32_t maxSet = 3;
    std::vector<vk::DescriptorPoolSize> poolSizes{
      {vk::DescriptorType::eUniformBuffer, numUniformDescriptor},
      {vk::DescriptorType ::eInputAttachment, numInputAttachment},
      {vk::DescriptorType::eCombinedImageSampler, numTextureDescriptor},
      {vk::DescriptorType::eStorageBuffer, numStorageBufferDescriptor},
    };
    vk::DescriptorPoolCreateInfo descriptorPoolInfo{
      vk::DescriptorPoolCreateFlagBits::eUpdateAfterBindEXT, maxSet,
      (uint32_t)poolSizes.size(), poolSizes.data()};
    Sets.descriptorPool = vkDevice.createDescriptorPoolUnique(descriptorPoolInfo);

    basicSetDef.textures.descriptorCount() = uint32_t(modelConfig.maxNumTexture);
    basicSetDef.init(vkDevice);

    deferredSetDef.init(vkDevice);

    iblSetDef.init(vkDevice);

    basicLayout.basic(basicSetDef);
    basicLayout.deferred(deferredSetDef);
    basicLayout.ibl(iblSetDef);
    basicLayout.init(vkDevice);

    Sets.basicSet = basicSetDef.createSet(*Sets.descriptorPool);
    Sets.deferredSet = deferredSetDef.createSet(*Sets.descriptorPool);
    Sets.iblSet = iblSetDef.createSet(*Sets.descriptorPool);
  }

  {
    basicSetDef.cam(Buffer.camera->buffer());
    basicSetDef.meshes(Buffer.meshes->buffer());
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

  {
    debugMarker.name(Buffer.position->buffer(), "position buffer");
    debugMarker.name(Buffer.normal->buffer(), "normal buffer");
    debugMarker.name(Buffer.uv->buffer(), "uv buffer");
    debugMarker.name(Buffer.joint0->buffer(), "joint0 buffer");
    debugMarker.name(Buffer.weight0->buffer(), "weight0 buffer");
    debugMarker.name(Buffer.transforms->buffer(), "transforms buffer");
    debugMarker.name(Buffer.materials->buffer(), "materials buffer");
    debugMarker.name(Buffer.meshes->buffer(), "meshes buffer");
    debugMarker.name(Buffer.drawQueue[0]->buffer(), "drawOpaqueCMDs buffer");
    debugMarker.name(Buffer.drawQueue[1]->buffer(), "drawLineCMDs buffer");
    debugMarker.name(Buffer.drawQueue[2]->buffer(), "drawTransparentCMDs buffer");
    debugMarker.name(Buffer.drawQueue[3]->buffer(), "drawTransparentLineCMDs buffer");
    debugMarker.name(Buffer.camera->buffer(), "camera buffer");
    debugMarker.name(Buffer.lighting->buffer(), "lighting buffer");
    debugMarker.name(Buffer.lights->buffer(), "lights buffer");
  }
}

void BasicModelManager::resize(vk::Extent2D extent) {
  Scene.camera.changeDimension(extent.width, extent.height);

  deferredSetDef.position(renderer.attachments.position->imageView());
  deferredSetDef.normal(renderer.attachments.normal->imageView());
  deferredSetDef.albedo(renderer.attachments.albedo->imageView());
  deferredSetDef.pbr(renderer.attachments.pbr->imageView());
  deferredSetDef.emissive(renderer.attachments.emissive->imageView());
  deferredSetDef.update(Sets.deferredSet);
}

PerspectiveCamera &BasicModelManager::camera() { return Scene.camera; }

Ptr<Primitive> BasicModelManager::newDynamicPrimitive(
  uint32_t vertexCount, uint32_t indexCount, const AABB &aabb,
  const PrimitiveTopology &topology) {
  auto primitive = Buffer.dynamicPrimitives->add(vertexCount, indexCount, aabb, topology);
  return Ptr<Primitive>::add(Scene.dynamicPrimitives, primitive);
}

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
    Scene.primitives, {indexRange, positionRange, normalRange, uvRange, aabb, topology});
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
      Scene.primitives, {indexRange, positionRange, normalRange, uvRange,
                         primitive.aabb(), primitive.topology()}));
  }
  return primitives;
}

Ptr<TextureImage2D> BasicModelManager::newTexture(
  const std::string &imagePath, const SamplerDef &samplerDef, bool generateMipmap) {
  ensureTextures(1);
  auto t = TextureImage2D::loadFromFile(device, imagePath, generateMipmap);
  t.setSampler(SamplerMaker(samplerDef).createUnique(vkDevice));
  return Ptr<TextureImage2D>::add(Image.textures, std::move(t));
}

Ptr<TextureImage2D> BasicModelManager::newTexture(
  const unsigned char *bytes, size_t size, uint32_t width, uint32_t height,
  const SamplerDef &samplerDef, bool generateMipmap) {
  ensureTextures(1);
  auto t =
    TextureImage2D::loadFromBytes(device, bytes, size, width, height, generateMipmap);
  t.setSampler(SamplerMaker(samplerDef).createUnique(vkDevice));
  return Ptr<TextureImage2D>::add(Image.textures, std::move(t));
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

Ptr<Material> BasicModelManager::newMaterial(MaterialType type) {
  ensureMaterials(1);
  auto offset = Scene.freeMaterials.back();
  Scene.freeMaterials.pop_back();
  return Ptr<Material>::add(Scene.materials, Material{type, offset});
}

uint32_t BasicModelManager::drawQueueIndex(
  Ptr<Primitive> primitive, Ptr<Material> material) {
  auto base = 0u;
  switch(material->type()) {
    case MaterialType::eBRDF:
    case MaterialType::eBRDFSG: break;
    case MaterialType::eReflective:
    case MaterialType::eRefractive:
    case MaterialType::eNone: base = 0u; break;
    case MaterialType::eTranslucent: base = 2u; break;
  }
  switch(primitive->topology()) {
    case PrimitiveTopology::Triangles: base += 0u; break;
    case PrimitiveTopology::Lines: base += 1u; break;
    case PrimitiveTopology::Procedural: error("Not supported"); break;
  }
  return base;
}

Ptr<Mesh> BasicModelManager::newMesh(Ptr<Primitive> primitive, Ptr<Material> material) {
  ensureMeshes(1);
  auto offset = Scene.freeMeshes.back();
  Scene.freeMeshes.pop_back();
  auto mesh = Ptr<Mesh>::add(Scene.meshes, Mesh{primitive, material, {}, offset});
  addToDrawQueue(mesh);
  return mesh;
}

void BasicModelManager::addToDrawQueue(Ptr<Mesh> mesh) {
  auto queueIndex = drawQueueIndex(mesh->primitive(), mesh->material());
  auto &drawQueue = Scene.drawQueues[queueIndex];
  drawQueue.push_back(mesh);
  mesh->setDrawQueue({queueIndex, uint32_t(drawQueue.size() - 1)});
}

DrawQueueIndex BasicModelManager::changeDrawQueue(Ptr<Mesh> mesh) {
  auto oldQueueIndex = mesh->_drawIndex.queueID;
  auto &oldQueue = Scene.drawQueues[oldQueueIndex];

  auto offset = mesh->_drawIndex.offset;
  if(offset != oldQueue.size() - 1) { // not end, swap to end
    auto end = oldQueue.back();
    oldQueue[offset] = end;
    end->setDrawQueue({oldQueueIndex, offset});
    Buffer.drawQueue[oldQueueIndex]->update(device, offset, end->getDrawCMD());
  }
  oldQueue.pop_back();
  addToDrawQueue(mesh);
  return mesh->_drawIndex;
}

Ptr<Node> BasicModelManager::newNode(
  const Transform &transform, const std::string &name) {
  ensureTransforms(1);
  auto offset = Scene.freeTransforms.back();
  Scene.freeTransforms.pop_back();
  return Ptr<Node>::add(Scene.nodes, Node{transform, name, offset});
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
  ensureTransforms(1);
  auto offset = Scene.freeTransforms.back();
  Scene.freeTransforms.pop_back();
  auto instance =
    Ptr<ModelInstance>::add(Scene.instances, ModelInstance{transform, offset});
  ModelInstance::applyModel(instance, model);
  return instance;
}

void BasicModelManager::useEnvironmentMap(Ptr<TextureImageCube> envMap) {
  EnvMapGenerator envMapGenerator{device};
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

void BasicModelManager::computeMesh(
  const std::string &imagePath, Ptr<Primitive> primitive) {}

Ptr<Light> BasicModelManager::addLight(
  LightType type, glm::vec3 direction, glm::vec3 color, glm::vec3 location) {
  ensureLights(1);
  auto offset = Scene.freeLights.back();
  Scene.freeLights.pop_back();
  return Ptr<Light>::add(Scene.lights, Light{type, direction, color, location, offset});
}

void BasicModelManager::updateScene(vk::CommandBuffer cb, uint32_t imageIndex) {
  if(Scene.camera.incoherent()) Buffer.camera->update(device, Scene.camera.flush());

  if(Scene.lighting.incoherent()) Buffer.lighting->update(device, Scene.lighting.flush());

  for(auto &light: Scene.lights)
    if(light.incoherent()) Buffer.lights->update(device, light.offset(), light.flush());

  for(auto &instance: Scene.instances)
    if(instance.incoherent())
      Buffer.transforms->update(device, instance.offset(), instance.flush());

  for(auto &node: Scene.nodes)
    if(node.incoherent()) Buffer.transforms->update(device, node.offset(), node.flush());

  for(auto &material: Scene.materials)
    if(material.incoherent())
      Buffer.materials->update(device, material.offset(), material.flush());

  for(auto i = 0u; i < Scene.meshes.size(); ++i)
    if(Ptr<Mesh> mesh{&Scene.meshes, i}; mesh->incoherent()) {
      auto [offset, drawIndex] = mesh->offset();

      auto _expectedQueueIndex = drawQueueIndex(mesh->primitive(), mesh->material());
      if(drawIndex.queueID != _expectedQueueIndex) // change queue
        drawIndex = changeDrawQueue(mesh);

      Buffer.meshes->update(device, offset, mesh->getUBO());
      Buffer.drawQueue[drawIndex.queueID]->update(
        device, drawIndex.offset, mesh->getDrawCMD());
      mesh->flush();
    }

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
    Buffer.drawQueue[0]->buffer(), 0, Scene.drawQueues[0].size(), stride);
  debugMarker.end(cb);

  debugMarker.begin(cb, "Subpass opaque line");
  cb.bindPipeline(bindpoint::eGraphics, *renderer.Pipelines.opaqueLine);
  cb.drawIndexedIndirect(
    Buffer.drawQueue[1]->buffer(), 0, Scene.drawQueues[1].size(), stride);
  debugMarker.end(cb);

  debugMarker.begin(cb, "Subpass deferred shading");
  cb.nextSubpass(vk::SubpassContents::eInline);
  if(Image.useEnvironmentMap)
    cb.bindPipeline(bindpoint::eGraphics, *renderer.Pipelines.deferredIBL);
  else
    cb.bindPipeline(bindpoint::eGraphics, *renderer.Pipelines.deferred);
  cb.draw(3, 1, 0, 0);
  debugMarker.end(cb);

  debugMarker.begin(cb, "Subpass translucent tri");
  cb.nextSubpass(vk::SubpassContents::eInline);
  cb.bindPipeline(bindpoint::eGraphics, *renderer.Pipelines.transTri);
  cb.drawIndexedIndirect(
    Buffer.drawQueue[2]->buffer(), 0, Scene.drawQueues[2].size(), stride);
  debugMarker.end(cb);

  debugMarker.begin(cb, "Subpass translucent line");
  cb.bindPipeline(bindpoint::eGraphics, *renderer.Pipelines.transLine);
  cb.drawIndexedIndirect(
    Buffer.drawQueue[3]->buffer(), 0, Scene.drawQueues[3].size(), stride);
  debugMarker.end(cb);
}

void BasicModelManager::debugInfo() {
  auto totalMeshes = modelConfig.maxNumMeshes + modelConfig.maxNumLineMeshes +
                     modelConfig.maxNumTransparentMeshes +
                     modelConfig.maxNumTransparentLineMeshes;
  sim::debugLog(
    "vertices: ", Buffer.position->count(), "/", modelConfig.maxNumVertex,
    ", indices: ", Buffer.indices->count(), "/", modelConfig.maxNumIndex,
    ", transforms: ", modelConfig.maxNumTransform - Scene.freeTransforms.size(), "/",
    modelConfig.maxNumTransform, ", meshes: ", Scene.meshes.size(), "/", totalMeshes,
    ", materials: ", modelConfig.maxNumMaterial - Scene.freeMaterials.size(), "/",
    modelConfig.maxNumMaterial, ", textures: ", Image.textures.size(), "/",
    modelConfig.maxNumTexture, ", lights: ", Scene.lights.size(), "/",
    modelConfig.maxNumLights);
}

void BasicModelManager::ensureVertices(uint32_t toAdd) const {
  errorIf(
    Buffer.position->count() + toAdd > modelConfig.maxNumVertex,
    "exceeding maximum number of vertices!");
}
void BasicModelManager::ensureIndices(uint32_t toAdd) const {
  errorIf(
    Buffer.indices->count() + toAdd > modelConfig.maxNumIndex,
    "exceeding maximum number of vertices!");
}
void BasicModelManager::ensureMeshes(uint32_t toAdd) const {
  errorIf(Scene.freeMeshes.size() < toAdd, "exceeding maximum number of meshes!");
}
void BasicModelManager::ensureTransforms(uint32_t toAdd) const {
  errorIf(Scene.freeTransforms.size() < toAdd, "exceeding maximum number of transforms!");
}
void BasicModelManager::ensureMaterials(uint32_t toAdd) const {
  errorIf(Scene.freeMaterials.size() < toAdd, "exceeding maximum number of vertices!");
}
void BasicModelManager::ensureTextures(uint32_t toAdd) const {
  errorIf(
    Image.textures.size() + toAdd > modelConfig.maxNumTexture,
    "exceeding maximum number of textures!");
}
void BasicModelManager::ensureLights(uint32_t toAdd) const {
  errorIf(Scene.freeLights.size() < toAdd, "exceeding maximum number of lights!");
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
#include "model_glTF.h"
#include "sim/graphics/renderer/default/model/model_manager.h"
#include <stb_image.h>

namespace sim::graphics::renderer::defaultRenderer {
using namespace glm;

ModelGLTF::ModelGLTF(ModelManager &modelManager): mm{modelManager} {}

ConstPtr<Model> ModelGLTF::loadGLTF(
  const std::string &filename, const ModelCreateInfo &createInfo) {
  tinygltf::Model model;
  tinygltf::TinyGLTF loader;
  std::string err, warn;

  auto result = endWith(filename, ".gltf") ?
                  loader.LoadASCIIFromFile(&model, &err, &warn, filename) :
                  loader.LoadBinaryFromFile(&model, &err, &warn, filename);
  errorIf(!result, "failed to load glTF", filename);

  loadTextureSamplers(model);
  loadTextures(model);
  loadMaterials(model);

  const auto &scene = model.scenes[std::max(model.defaultScene, 0)];

  return mm.newModel([&](ModelBuilder &builder) {
    for(int i: scene.nodes)
      loadNode(builder, i, model, createInfo);
  });
}

SamplerAddressMode getVkWrapMode(int32_t wrapMode) {
  switch(wrapMode) {
    case 10497: return SamplerAddressMode::SamplerAddressModeeRepeat;
    case 33071: return SamplerAddressMode::SamplerAddressModeeClampToEdge;
    case 33648: return SamplerAddressMode ::SamplerAddressModeeMirroredRepeat;
    default:
      error("not supported SamplerAddressMode", wrapMode);
      return SamplerAddressMode ::SamplerAddressModeeMirroredRepeat;
  }
}

Filter getVkFilterMode(int32_t filterMode) {
  switch(filterMode) {
    case 9728: return Filter::FiltereNearest;
    case 9729: return Filter::FiltereLinear;
    case 9984: return Filter::FiltereNearest;
    case 9985: return Filter::FiltereNearest;
    case 9986: return Filter::FiltereLinear;
    case 9987: return Filter::FiltereLinear;
    default: error("not supported Filter", filterMode); return Filter::FiltereLinear;
  }
}

void ModelGLTF::loadTextureSamplers(const tinygltf::Model &model) {
  for(auto &sampler: model.samplers) {
    SamplerDef samplerDef;
    samplerDef.magFilter = getVkFilterMode(sampler.magFilter);
    samplerDef.minFilter = getVkFilterMode(sampler.minFilter);
    samplerDef.addressModeU = getVkWrapMode(sampler.wrapS);
    samplerDef.addressModeV = getVkWrapMode(sampler.wrapT);
    samplerDef.addressModeW = getVkWrapMode(sampler.wrapT);
    samplerDef.anisotropyEnable = true;
    samplerDef.maxAnisotropy = 16;
    samplerDefs.push_back(samplerDef);
  }
}

void ModelGLTF::loadTextures(const tinygltf::Model &model) {
  mm.ensureTextures(uint32_t(model.textures.size()));
  for(auto &tex: model.textures) {
    auto &image = model.images[tex.source];
    auto size = image.width * image.height * 4;
    auto sampler = tex.sampler == -1 ? SamplerDef{} : samplerDefs[tex.sampler];
    auto dataPtr = UniqueConstBytes(image.image.data(), [](const unsigned char *ptr) {});
    auto channel = image.component;
    if(image.component != 4) {
      int w, h;
      dataPtr = UniqueConstBytes(
        stbi_load_from_memory(
          image.image.data(), image.image.size(), &w, &h, &channel, STBI_rgb_alpha),
        [](const unsigned char *ptr) {
          stbi_image_free(const_cast<unsigned char *>(ptr));
        });
    }
    textures.push_back(mm.newTex(
      dataPtr.get(), size, image.width, image.height, channel, sampler, true, 0));
  }
}

void ModelGLTF::loadMaterials(const tinygltf::Model &model) {
  mm.ensureMaterials(uint32_t(model.materials.size()));
  for(auto &mat: model.materials) {
    Tex color{}, pbr{}, normal{}, occlusion{}, emissive{};
    vec4 baseColor{1.f};
    PBR pbrFactor{1.f, 0.f};
    glm::vec3 emissiveFactor{1.f};
    float occlusionStrength{1.f};
    bool hasTexture = false;
    { //load color
      if(contains(mat.values, "baseColorTexture")) {
        color = textures[mat.values.at("baseColorTexture").TextureIndex()];
        hasTexture = true;
      }
      if(contains(mat.values, "baseColorFactor")) {
        vec4 c = make_vec4(mat.values.at("baseColorFactor").ColorFactor().data());
        if(hasTexture) baseColor = c;
        else
          color = mm.newTex(c);
      }
    }

    { //load metallicRoughness
      hasTexture = false;
      if(contains(mat.values, "metallicRoughnessTexture")) {
        pbr = textures[mat.values.at("metallicRoughnessTexture").TextureIndex()];
        hasTexture = true;
      }
      float roughness = contains(mat.values, "roughnessFactor") ?
                          float(mat.values.at("roughnessFactor").Factor()) :
                          1.f;
      float metallic = contains(mat.values, "metallicFactor") ?
                         float(mat.values.at("metallicFactor").Factor()) :
                         1.f;
      if(hasTexture) pbrFactor = {roughness, metallic};
      else
        pbr = mm.newTex(PBR{roughness, metallic});
    }
    { //load normal
      if(contains(mat.additionalValues, "normalTexture"))
        normal = textures[mat.additionalValues.at("normalTexture").TextureIndex()];
    }
    {
      //load emissive
      hasTexture = false;
      if(contains(mat.additionalValues, "emissiveTexture")) {
        emissive = textures[mat.additionalValues.at("emissiveTexture").TextureIndex()];
        hasTexture = true;
      }

      if(contains(mat.additionalValues, "emissiveFactor")) {
        vec4 c = vec4(
          make_vec3(mat.additionalValues.at("emissiveFactor").ColorFactor().data()),
          1.0f);
        if(hasTexture) emissiveFactor = c;
        else
          emissive = mm.newTex(c);
      }
    }
    {
      if(contains(mat.additionalValues, "occlusionTexture")) {
        auto occlusionTex = mat.additionalValues.at("occlusionTexture");
        occlusion = textures[occlusionTex.TextureIndex()];
        //        occlusionStrength = float(occlusionTex.Factor()); // seems always 0
      }
    }
    Material _mat{color,         pbr,       baseColor,         pbrFactor.toVec4(),
                  normal,        occlusion, occlusionStrength, emissive,
                  emissiveFactor};
    materials.push_back(mm.newMaterial(_mat));
  }
}

Ptr<Node> ModelGLTF::loadNode(
  ModelBuilder &builder, int thisID, const tinygltf::Model &model,
  const ModelCreateInfo &createInfo) {
  auto &node = model.nodes[thisID];
  Transform t;
  if(node.matrix.size() == 16) {
    auto m = make_mat4x4(node.matrix.data());
    vec3 translation = vec3(m[3]);
    vec3 scale = {length(m[0]), length(m[1]), length(m[2])};
    m = glm::scale(m, dvec3(1 / scale.x, 1 / scale.y, 1 / scale.z));
    m[3] = vec4(0, 0, 0, 1);
    quat rotation = quat_cast(m);

    t.translation = translation;
    t.scale = scale;
    t.rotation = rotation;
  } else {
    t.translation = node.translation.size() != 3 ? dvec3{0.} :
                                                   make_vec3(node.translation.data());
    t.scale = node.scale.size() != 3 ? dvec3{1.f} : make_vec3(node.scale.data());
    t.rotation = node.rotation.size() != 4 ? dquat{1, 0, 0, 0} :
                                             make_quat(node.rotation.data());
  }
  auto nodePtr = builder.addNode(t, node.name);
  if(node.mesh != -1) {
    auto &mesh = model.meshes[node.mesh];
    for(auto &primitive: mesh.primitives)
      builder.addMeshPart(nodePtr, loadPrimitive(model, primitive, createInfo));
  }
  if(!node.children.empty())
    for(auto childID: node.children)
      builder.addChildNode(nodePtr, loadNode(builder, childID, model, createInfo));
  return nodePtr;
}

MeshPart ModelGLTF::loadPrimitive(
  const tinygltf::Model &model, const tinygltf::Primitive &primitive,
  const ModelCreateInfo &createInfo) {
  errorIf(primitive.mode != 4, "model primitive mode isn't triangles!");

  auto [vertexRange, aabb] = loadVertices(model, primitive, createInfo);
  auto indexRange = loadIndices(model, primitive);

  if(primitive.material < 0) {
    debugLog("warning: mesh has no material!");
    if(materials.empty()) materials.push_back(mm.newMaterial(vec3(1.f, 1.f, 1.f)));
  }
  auto material = materials[std::max(primitive.material, 0)];
  return {mm.newMesh(indexRange, vertexRange), material, mm.newTransform(Transform{}),
          aabb};
}

std::pair<Range, AABB> ModelGLTF::loadVertices(
  const tinygltf::Model &model, const tinygltf::Primitive &primitive,
  const ModelCreateInfo &createInfo) {
  errorIf(!contains(primitive.attributes, "POSITION"), "missing required POSITION data!");
  auto verticesID = primitive.attributes.at("POSITION");
  if(contains(verticesMap, verticesID)) return verticesMap[verticesID];

  vertices.clear();
  uint32_t vertexCount = 0;
  glm::vec3 posMin{};
  glm::vec3 posMax{};
  bool hasNormals = false;
  const float *bufferPos = nullptr;
  const float *bufferNormals = nullptr;
  const float *bufferTexCoords = nullptr;

  uint32_t posByteStride{-1u};
  uint32_t normByteStride{-1u};
  uint32_t uv0ByteStride{-1u};

  auto posAccessor = model.accessors[verticesID];
  vertexCount = uint32_t(posAccessor.count);
  vertices.reserve(vertexCount);
  this->mm.ensureVertices(vertexCount);
  auto &posView = model.bufferViews[posAccessor.bufferView];
  bufferPos = (const float *)(&(
    model.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]));
  posMin =
    vec3(posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2]);
  posMax =
    vec3(posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2]);
  posByteStride = posAccessor.ByteStride(posView) ?
                    (posAccessor.ByteStride(posView) / uint32_t(sizeof(float))) :
                    tinygltf::GetTypeSizeInBytes(TINYGLTF_TYPE_VEC3);

  if(contains(primitive.attributes, "NORMAL")) {
    auto &normAccessor = model.accessors[primitive.attributes.at("NORMAL")];
    auto &normView = model.bufferViews[normAccessor.bufferView];
    bufferNormals =
      (const float *)(&(model.buffers[normView.buffer]
                          .data[normAccessor.byteOffset + normView.byteOffset]));
    normByteStride = normAccessor.ByteStride(normView) ?
                       (normAccessor.ByteStride(normView) / uint32_t(sizeof(float))) :
                       tinygltf::GetTypeSizeInBytes(TINYGLTF_TYPE_VEC3);
    hasNormals = true;
  }

  if(contains(primitive.attributes, "TEXCOORD_0")) {
    auto &uvAccessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];
    auto &uvView = model.bufferViews[uvAccessor.bufferView];
    bufferTexCoords = (const float *)(&(
      model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
    uv0ByteStride = uvAccessor.ByteStride(uvView) ?
                      (uvAccessor.ByteStride(uvView) / uint32_t(sizeof(float))) :
                      tinygltf::GetTypeSizeInBytes(TINYGLTF_TYPE_VEC2);
  }

  //vertices
  for(size_t v = 0; v < posAccessor.count; v++) {
    Vertex vert{};
    vert.position =
      make_vec3(&bufferPos[v * posByteStride]) * createInfo.scale + createInfo.center;
    if(bufferNormals) vert.normal = make_vec3(&bufferNormals[v * normByteStride]);
    if(bufferTexCoords) vert.uv = make_vec2(&bufferTexCoords[v * uv0ByteStride]);
    vert.uv *= createInfo.uvscale;
    vertices.push_back(vert);
  }

  verticesMap[verticesID] = {mm.newVertices(vertices), {posMin, posMax}};
  return verticesMap[verticesID];
  //  if(!hasNormals) {
  //    std::unordered_map<vec3, vec3> vertexRef(posAccessor.count);
  //    for(auto i = 0; i < indices.size(); i += 3) {
  //      auto i1 = indices[i];
  //      auto i2 = indices[i + 1];
  //      auto i3 = indices[i + 2];
  //      auto p1 = vertices[i1].position;
  //      auto p2 = vertices[i2].position;
  //      auto p3 = vertices[i3].position;
  //      auto N = cross(p2 - p1, p3 - p1);
  //      auto a = angle(normalize(p2 - p1), normalize(p3 - p1));
  //      if(isnan(a)) continue;
  //      N *= abs(a);
  //      auto &n1 = vertexRef[p1];
  //      auto &n2 = vertexRef[p2];
  //      auto &n3 = vertexRef[p3];
  //      n1 += N;
  //      n2 += N;
  //      n3 += N;
  //    }
  //    for(auto &vertice: vertices) {
  //      auto &n = vertexRef[vertice.position];
  //      auto nn = normalize(n);
  //      if(any(isnan(nn))) continue;
  //      vertice.normal = nn;
  //    }
  //  }
}

Range ModelGLTF::loadIndices(
  const tinygltf::Model &model, const tinygltf::Primitive &primitive) {
  errorIf(primitive.indices < 0, "mesh without indices!!");
  auto indicesID = primitive.indices;
  if(contains(indicesMap, indicesID)) return indicesMap[indicesID];

  indices.clear();
  auto &indicesAccessor = model.accessors[indicesID];
  auto &indicesView = model.bufferViews[indicesAccessor.bufferView];
  auto &buffer = model.buffers[indicesView.buffer];
  auto buf = &buffer.data[indicesAccessor.byteOffset + indicesView.byteOffset];

  auto indexCount = uint32_t(indicesAccessor.count);
  indices.reserve(indexCount);
  this->mm.ensureIndices(indexCount);
  switch(indicesAccessor.componentType) {
    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
      auto _buf = (uint32_t *)buf;
      for(size_t index = 0; index < indicesAccessor.count; index++)
        indices.push_back(_buf[index]);
      break;
    }
    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
      auto _buf = (uint16_t *)buf;
      for(size_t index = 0; index < indicesAccessor.count; index++)
        indices.push_back(_buf[index]);
      break;
    }
    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
      auto _buf = (uint8_t *)buf;
      for(size_t index = 0; index < indicesAccessor.count; index++)
        indices.push_back(_buf[index]);
      break;
    }
    default:
      error("Index component type", indicesAccessor.componentType, "not supported!");
  }

  indicesMap[indicesID] = mm.newIndices(indices);
  return indicesMap[indicesID];
}

}

#include "gltf_loader.h"
#include <stb_image.h>
#include "sim/graphics/base/glm_common.h"

namespace sim::graphics::renderer::basic {
using namespace glm;

GLTFLoader::GLTFLoader(BasicModelManager &mm): mm(mm) {}
Ptr<Model> GLTFLoader::load(const std::string &file) {
  tinygltf::Model model;
  tinygltf::TinyGLTF loader;
  std::string err, warn;

  auto result = endWith(file, ".gltf") ?
                  loader.LoadASCIIFromFile(&model, &err, &warn, file) :
                  loader.LoadBinaryFromFile(&model, &err, &warn, file);
  errorIf(!result, "failed to load glTF ", file);

  loadTextureSamplers(model);
  loadTextures(model);
  loadMaterials(model);

  std::vector<Ptr<Node>> nodes;
  const auto &_scene = model.scenes[std::max(model.defaultScene, 0)];
  _nodes.resize(_scene.nodes.size());
  for(int i: _scene.nodes)
    nodes.push_back(loadNode(i, model));

  loadAnimations(model);
  return mm.newModel(std::move(nodes), std::move(animations));
}

SamplerAddressMode _getVkWrapMode(int32_t wrapMode) {
  switch(wrapMode) {
    case 10497: return SamplerAddressMode::SamplerAddressModeeRepeat;
    case 33071: return SamplerAddressMode::SamplerAddressModeeClampToEdge;
    case 33648: return SamplerAddressMode ::SamplerAddressModeeMirroredRepeat;
    default:
      error("not supported SamplerAddressMode", wrapMode);
      return SamplerAddressMode ::SamplerAddressModeeMirroredRepeat;
  }
}
Filter _getVkFilterMode(int32_t filterMode) {
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
void GLTFLoader::loadTextureSamplers(const tinygltf::Model &model) {
  for(auto &sampler: model.samplers) {
    SamplerDef samplerDef;
    samplerDef.magFilter = _getVkFilterMode(sampler.magFilter);
    samplerDef.minFilter = _getVkFilterMode(sampler.minFilter);
    samplerDef.addressModeU = _getVkWrapMode(sampler.wrapS);
    samplerDef.addressModeV = _getVkWrapMode(sampler.wrapT);
    samplerDef.addressModeW = _getVkWrapMode(sampler.wrapT);
    samplerDefs.push_back(samplerDef);
  }
}

void GLTFLoader::loadTextures(const tinygltf::Model &model) {
  for(auto &tex: model.textures) {
    auto &image = model.images[tex.source];
    auto size = image.width * image.height * 4;
    auto samplerDef = tex.sampler == -1 ? SamplerDef{} : samplerDefs[tex.sampler];
    auto dataPtr = UniqueConstBytes(image.image.data(), [](const unsigned char *ptr) {});
    auto channel = image.component;
    if(channel != 4) {
      int w, h;
      dataPtr = UniqueConstBytes(
        stbi_load_from_memory(
          image.image.data(), image.image.size(), &w, &h, &channel, STBI_rgb_alpha),
        [](const unsigned char *ptr) {
          stbi_image_free(const_cast<unsigned char *>(ptr));
        });
    }
    textures.push_back(
      mm.newTexture(dataPtr.get(), size, image.width, image.height, samplerDef, true));
  }
}

void GLTFLoader::loadMaterials(const tinygltf::Model &model) {
  for(auto &mat: model.materials) {
    MaterialType type{MaterialType::eBRDF};
    float alphaCutoff{0.f};
    if(contains(mat.additionalValues, "alphaMode")) {
      auto alphaMode = mat.additionalValues.at("alphaMode").string_value;
      if(alphaMode != "OPAQUE") type = MaterialType ::eTranslucent;
      if(alphaMode == "MASK") {
        alphaCutoff = 0.5f;
        if(contains(mat.additionalValues, "alphaCutoff"))
          alphaCutoff = float(mat.additionalValues.at("alphaCutoff").number_value);
      }
    }
    if(contains(mat.extensions, "KHR_materials_pbrSpecularGlossiness"))
      type = MaterialType ::eBRDFSG;
    auto material = mm.newMaterial(type);

    material->setAlphaCutoff(alphaCutoff);

    if(contains(mat.values, "baseColorFactor"))
      material->setColorFactor(
        make_vec4(mat.values.at("baseColorFactor").ColorFactor().data()));
    if(contains(mat.values, "baseColorTexture"))
      material->setColorTex(
        textures.at(mat.values.at("baseColorTexture").TextureIndex()));

    if(contains(mat.values, "metallicRoughnessTexture"))
      material->setPbrTex(
        textures.at(mat.values.at("metallicRoughnessTexture").TextureIndex()));
    vec4 pbrFactor{1.f, 1.f, 1.f, 1.f};
    if(contains(mat.values, "roughnessFactor"))
      pbrFactor.g = static_cast<float>(mat.values.at("roughnessFactor").Factor());
    if(contains(mat.values, "metallicFactor"))
      pbrFactor.b = static_cast<float>(mat.values.at("metallicFactor").Factor());
    material->setPbrFactor(pbrFactor);

    if(contains(mat.additionalValues, "normalTexture"))
      material->setNormalTex(
        textures.at(mat.additionalValues.at("normalTexture").TextureIndex()));

    if(contains(mat.additionalValues, "emissiveTexture"))
      material->setEmissiveTex(
        textures.at(mat.additionalValues.at("emissiveTexture").TextureIndex()));
    if(mat.additionalValues.find("emissiveFactor") != mat.additionalValues.end()) {
      material->setEmissiveFactor(vec4(
        make_vec3(mat.additionalValues.at("emissiveFactor").ColorFactor().data()), 1.f));
    }

    if(contains(mat.additionalValues, "occlusionTexture"))
      material->setOcclusionTex(
        textures.at(mat.additionalValues.at("occlusionTexture").TextureIndex()));

    if(contains(mat.extensions, "KHR_materials_pbrSpecularGlossiness")) {
      auto ext = mat.extensions.at("KHR_materials_pbrSpecularGlossiness");
      if(ext.Has("specularGlossinessTexture")) {
        auto index = ext.Get("specularGlossinessTexture").Get("index").Get<int>();
        material->setPbrTex(textures.at(index));
      }
      if(ext.Has("diffuseTexture")) {
        auto index = ext.Get("diffuseTexture").Get("index").Get<int>();
        material->setColorTex(textures.at(index));
      }
      if(ext.Has("diffuseFactor")) {
        auto factor = ext.Get("diffuseFactor");
        vec4 diffuse{1.f};
        for(int i = 0; i < factor.ArrayLen(); ++i) {
          auto val = factor.Get(i);
          diffuse[i] = val.IsNumber() ? val.Get<double>() : val.Get<int>();
        }
        material->setColorFactor(diffuse);
      }
      if(ext.Has("specularFactor")) {
        auto factor = ext.Get("specularFactor");
        vec4 specular{1.f};
        for(int i = 0; i < factor.ArrayLen(); ++i) {
          auto val = factor.Get(i);
          specular[i] = val.IsNumber() ? val.Get<double>() : val.Get<int>();
        }
        material->setPbrFactor(specular);
      }
      if(ext.Has("glossinessFactor")) {
        auto glossiness = ext.Get("glossinessFactor").Get<double>();
        auto specular = material->pbrFactor();
        specular.w = glossiness;
        material->setColorFactor(specular);
      }
    }

    materials.push_back(material);
  }
}

Ptr<Node> GLTFLoader::loadNode(int thisID, const tinygltf::Model &model) {
  auto &node = model.nodes[thisID];
  Transform t;
  if(node.matrix.size() == 16) {
    auto m = make_mat4x4(node.matrix.data());
    t = {m};
  } else {
    t.translation = node.translation.size() != 3 ? dvec3{0.} :
                                                   make_vec3(node.translation.data());
    t.scale = node.scale.size() != 3 ? dvec3{1.f} : make_vec3(node.scale.data());
    t.rotation = node.rotation.size() != 4 ? dquat{1, 0, 0, 0} :
                                             make_quat(node.rotation.data());
  }
  auto _node = mm.newNode(t, node.name);
  _nodes[thisID] = _node;

  if(node.mesh > -1) {
    auto &mesh = model.meshes[node.mesh];
    for(auto &primitive: mesh.primitives)
      Node::addMesh(_node, loadPrimitive(model, primitive));
  }
  if(!node.children.empty())
    for(auto childID: node.children)
      Node::addChild(_node, loadNode(childID, model));
  return _node;
}

Ptr<Mesh> GLTFLoader::loadPrimitive(
  const tinygltf::Model &model, const tinygltf::Primitive &primitive) {
  errorIf(primitive.mode != 4, "model primitive mode isn't triangles!");

  auto aabb = loadVertices(model, primitive);
  loadIndices(model, primitive);
  auto _primitive = mm.newPrimitive(vertices, indices, aabb);

  auto material = primitive.material < 0 ? mm.material(0) :
                                           materials.at(primitive.material);
  return mm.newMesh(_primitive, material);
}

AABB GLTFLoader::loadVertices(
  const tinygltf::Model &model, const tinygltf::Primitive &primitive) {
  errorIf(!contains(primitive.attributes, "POSITION"), "missing required POSITION data!");

  vertices.clear();

  auto verticesID = primitive.attributes.at("POSITION");

  glm::vec3 posMin{};
  glm::vec3 posMax{};
  const float *bufferPos = nullptr;
  const float *bufferNormals = nullptr;
  const float *bufferTexCoords = nullptr;

  uint32_t posByteStride{-1u};
  uint32_t normByteStride{-1u};
  uint32_t uv0ByteStride{-1u};

  auto posAccessor = model.accessors[verticesID];
  vertices.reserve(posAccessor.count);
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
    vert.position = make_vec3(&bufferPos[v * posByteStride]);
    if(bufferNormals) vert.normal = make_vec3(&bufferNormals[v * normByteStride]);
    if(bufferTexCoords) vert.uv = make_vec2(&bufferTexCoords[v * uv0ByteStride]);
    vertices.push_back(vert);
  }
  return {posMin, posMax};
}

void GLTFLoader::loadIndices(
  const tinygltf::Model &model, const tinygltf::Primitive &primitive) {
  indices.clear();

  if(primitive.indices < 0) {
    for(int i = 0; i < vertices.size(); ++i)
      indices.push_back(i);
    return;
  }

  auto indicesID = primitive.indices;

  auto &indicesAccessor = model.accessors[indicesID];
  auto &indicesView = model.bufferViews[indicesAccessor.bufferView];
  auto &buffer = model.buffers[indicesView.buffer];
  auto buf = &buffer.data[indicesAccessor.byteOffset + indicesView.byteOffset];

  indices.reserve(indicesAccessor.count);
  switch(indicesAccessor.componentType) {
    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
      auto _buf = (const uint32_t *)buf;
      for(size_t index = 0; index < indicesAccessor.count; index++)
        indices.push_back(_buf[index]);
      break;
    }
    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
      auto _buf = (const uint16_t *)buf;
      for(size_t index = 0; index < indicesAccessor.count; index++)
        indices.push_back(_buf[index]);
      break;
    }
    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
      auto _buf = (const uint8_t *)buf;
      for(size_t index = 0; index < indicesAccessor.count; index++)
        indices.push_back(_buf[index]);
      break;
    }
    default:
      error("Index component type", indicesAccessor.componentType, "not supported!");
  }
}

void GLTFLoader::loadAnimations(const tinygltf::Model &model) {
  using Path = Animation::AnimationChannel::PathType;
  using Interpolation = Animation::AnimationSampler::InterpolationType;

  animations.reserve(model.animations.size());
  for(const auto &animation: model.animations) {
    Animation _animation{};
    _animation.name = animation.name;
    for(auto &sampler: animation.samplers) {
      Animation::AnimationSampler _sampler;
      if(sampler.interpolation == "LINEAR")
        _sampler.interpolation = Interpolation ::Linear;
      else if(sampler.interpolation == "STEP")
        _sampler.interpolation = Interpolation ::Step;
      else if(sampler.interpolation == "CUBICSPLINE")
        _sampler.interpolation = Interpolation ::CubicSpline;
      else
        error("Not supported");
      {
        const auto &accessor = model.accessors[sampler.input];
        const auto &bufferView = model.bufferViews[accessor.bufferView];
        const auto &buffer = model.buffers[bufferView.buffer];

        assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

        auto buf = reinterpret_cast<const float *>(
          &buffer.data[accessor.byteOffset + bufferView.byteOffset]);
        for(size_t index = 0; index < accessor.count; index++)
          _sampler.keyTimings.push_back(buf[index]);
      }
      {
        const auto &accessor = model.accessors[sampler.output];
        const auto &bufferView = model.bufferViews[accessor.bufferView];
        const auto &buffer = model.buffers[bufferView.buffer];

        assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

        auto dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];
        switch(accessor.type) {
          case TINYGLTF_TYPE_VEC3: {
            auto buf = reinterpret_cast<const glm::vec3 *>(dataPtr);
            for(size_t index = 0; index < accessor.count; index++) {
              _sampler.keyFrames.emplace_back(buf[index], 0.0f);
            }
            break;
          }
          case TINYGLTF_TYPE_VEC4: {
            auto buf = reinterpret_cast<const glm::vec4 *>(dataPtr);
            for(size_t index = 0; index < accessor.count; index++) {
              _sampler.keyFrames.push_back(buf[index]);
            }
            break;
          }
          default: {
            error("Not supported");
            break;
          }
        }
      }
      _animation.samplers.push_back(_sampler);
    }

    for(auto &channel: animation.channels) {
      Animation::AnimationChannel _channel;
      if(channel.target_path == "translation") _channel.path = Path::Translation;
      else if(channel.target_path == "rotation")
        _channel.path = Path::Rotation;
      else if(channel.target_path == "scale")
        _channel.path = Path::Scale;
      else
        error("Not supported");
      _channel.samplerIdx = channel.sampler;
      _channel.node = _nodes[channel.target_node];
      _animation.channels.push_back(_channel);
    }

    animations.push_back(_animation);
  }
}

}
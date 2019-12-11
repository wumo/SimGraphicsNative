#pragma once
#include "sim/graphics/base/vkcommon.h"

namespace sim::graphics::renderer::defaultRenderer {
template<class Type>
struct ConstPtr {
  static ConstPtr<Type> add(std::vector<Type> &vec, Type element) {
    vec.push_back(element);
    return ConstPtr(&vec, uint32_t(vec.size() - 1));
  }

  explicit ConstPtr(std::vector<Type> *vec = nullptr, uint32_t element = 0)
    : vec{vec}, _index{element} {}

  const Type &get() { return vec->at(_index); }
  operator const Type &() { return vec->at(_index); }
  const Type &operator*() { return vec->at(_index); }
  const Type *operator->() { return &vec->at(_index); }

  uint32_t index() const { return _index; }
  bool isNull() const { return vec == nullptr; }

private:
  std::vector<Type> *vec;
  uint32_t _index{};
};

template<class Type>
struct Ptr {
  static Ptr<Type> add(std::vector<Type> &vec, Type element) {
    vec.push_back(element);
    return Ptr(&vec, uint32_t(vec.size() - 1));
  }

  explicit Ptr(std::vector<Type> *vec = nullptr, uint32_t element = 0)
    : vec{vec}, _index{element} {}

  Type &get() const { return vec->at(_index); }
  operator Type &() { return vec->at(_index); }
  Type &operator*() { return vec->at(_index); }
  Type *operator->() { return &vec->at(_index); }

  uint32_t index() const { return _index; }
  bool isNull() const { return vec == nullptr; }

private:
  std::vector<Type> *vec;
  uint32_t _index{};
};

// ref in shaders
struct Vertex {
  glm::vec3 position{};
  glm::vec3 normal{};
  glm::vec2 uv{};

  static uint32_t stride() { return sizeof(Vertex); }

  static uint32_t locations() { return 3; }

  static std::vector<vk::VertexInputAttributeDescription> attributes(
    uint32_t binding, uint32_t baseLocation) {
    using f = vk::Format;
    return {{baseLocation + 0, binding, f::eR32G32B32Sfloat, offsetOf(Vertex, position)},
            {baseLocation + 1, binding, f::eR32G32B32Sfloat, offsetOf(Vertex, normal)},
            {baseLocation + 2, binding, f::eR32G32Sfloat, offsetOf(Vertex, uv)}};
  }
};

enum class Primitive { Triangles, Lines, Procedural };

struct Range {
  uint32_t offset{0};
  uint32_t size{0};

  uint32_t endOffset() { return offset + size; }
};

// ref in shaders
struct Mesh {
  union {
    struct {
      uint32_t indexOffset;
      uint32_t indexCount;
      uint32_t vertexOffset;
      uint32_t vertexCount;
    };
    struct {
      uint32_t id;
      uint32_t numAABBs;
      uint32_t AABBoffset;
      uint32_t payloadNumFloat;
    };
  };
  Primitive primitive;
};

const auto infinity = std::numeric_limits<float>::infinity();

// ref in shaders
struct Transform {
  glm::vec3 translation;
  glm::vec3 scale;
  glm::quat rotation;

  explicit Transform(
    const glm::vec3 &translation = {}, const glm::vec3 &scale = glm::vec3{1.0f},
    const glm::quat &rotation = {1, 0, 0, 0})
    : translation(translation), scale(scale), rotation(rotation) {}

  int getParentID() const { return parentID; }

  glm::mat4 toMatrix() {
    glm::mat4 m{1.0};
    m = glm::scale(m, scale);
    m = glm::mat4_cast(rotation) * m;
    m[3] = glm::vec4(translation, 1);
    return m;
  }

private:
  friend class ModelInstance;

  friend class DefaultModelManager;

  int parentID{-1};
  uint32_t precalculated{0};
};

// ref in shaders
struct Tex {
  uint32_t texID{0};
  float x{0.f}, y{0.f}, w{1.f}, h{1.f}; // uvTransform
  int32_t lodBias{0};
  uint32_t maxLod{0};
};
// ref in shaders
struct PBR {
  float roughness{1.f}, metalness{0.f};

  glm::vec4 toVec4() { return {0.f, roughness, metalness, 0.f}; }
};

struct Refractive {
  float n;
  float transparency;
  glm::vec4 toVec4() { return {n, transparency, 0, 0}; }
};

enum class MaterialFlag {
  /**BRDF without reflection trace*/
  eBRDF = 0x1u,
  /**BRDF with reflection trace*/
  eReflective = 0x2u,
  /**BRDF with reflection trace and refraction trace*/
  eRefractive = 0x4u,
  /**diffuse coloring*/
  eNone = 0x8u,
  /**translucent color blending*/
  eTranslucent = 0x10u
};

// ref in shaders
struct Material {
  /*
   * For Metallic-Roughness-Model:
   *  Tex baseColorTexture, metallicRoughnessTexture;
   *  glm::vec4 baseColorFactor;
   *  float pad;float roughness;float metallic;float pad;
   *
   * For Specular-Glossiness-Model:
   *  Tex diffuseTexture, specularGlossinessTexture;
   *  glm::vec4 diffuseFactor;
   *  glm::vec3 specularFactor; float glossinessFactor;
   */
  Tex color, pbr;
  glm::vec4 colorFactor{1.f};
  glm::vec4 pbrFactor{1.f};

  Tex normal;
  Tex occlusion;
  float occlusionStrength{1.f};
  Tex emissive;
  glm::vec3 emissiveFactor{1.f};
  Tex reflective, refractive;
  MaterialFlag flag{MaterialFlag::eBRDF};
  float paded[2];
};

// ref in shaders
struct MeshInstance {
  uint32_t mesh;
  uint32_t material;
  uint32_t transform;
};

struct AABB {
  glm::vec3 min;
  glm::vec3 max;
};

struct MeshPart {
  ConstPtr<Mesh> mesh;
  Ptr<Material> material;
  Ptr<Transform> transform;
  AABB aabb{};
  bool visible{true};

  bool isDummy() { return mesh.isNull() && material.isNull(); }

  MeshInstance toMeshInstance() {
    return {mesh.index(), material.index(), transform.index()};
  }
};

struct Node {
  std::string name;
  Ptr<Transform> transform;
  std::vector<MeshPart> meshParts;
  std::vector<Node> children;
  AABB aabb{};
};

struct Model {
  std::vector<Node> nodes;
  AABB aabb{};
};

struct ModelCreateInfo {
  glm::vec3 scale{1.f};
  glm::vec2 uvscale{1.f};
  glm::vec3 center{0.f};
};
}
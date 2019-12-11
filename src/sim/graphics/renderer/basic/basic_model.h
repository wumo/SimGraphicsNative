#pragma once
#include <ostream>
#include "ptr.h"
#include "sim/graphics/base/vkcommon.h"
#include "sim/graphics/base/pipeline/sampler.h"
#include "sim/graphics/base/resource/buffers.h"
#include "sim/graphics/base/resource/images.h"
#include "sim/graphics/base/pipeline/descriptors.h"

namespace sim::graphics::renderer::basic {
class Primitive;
class Material;
class Mesh;
class Node;
class Model;
class ModelInstance;

// ref in shaders
struct Vertex {
  glm::vec3 position{0.f};
  glm::vec3 normal{0.f};
  glm::vec2 uv{0.f};

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

struct Range {
  uint32_t offset{0};
  uint32_t size{0};

  uint32_t endOffset() const { return offset + size; }
};

struct AABB {
  glm::vec3 min{std::numeric_limits<float>::infinity()},
    max{-std::numeric_limits<float>::infinity()};

  AABB transform(glm::mat4 m) const {
    auto _min = glm::vec3(m[3]);
    auto _max = _min;

    auto p = glm::vec3(m[0]);
    auto v0 = p * min.x;
    auto v1 = p * max.x;
    _min += glm::min(v0, v1);
    _max += glm::max(v0, v1);

    p = glm::vec3(m[1]);
    v0 = p * min.y;
    v1 = p * max.y;
    _min += glm::min(v0, v1);
    _max += glm::max(v0, v1);

    p = glm::vec3(m[2]);
    v0 = p * min.z;
    v1 = p * max.z;
    _min += glm::min(v0, v1);
    _max += glm::max(v0, v1);

    return {_min, _max};
  }

  template<typename... Args>
  void merge(glm::vec3 p, Args &&... points) {
    merge(p);
    merge(points...);
  }

  void merge(glm::vec3 p) {
    min = glm::min(min, p);
    max = glm::max(max, p);
  }

  void merge(AABB other) {
    min = glm::min(min, other.min);
    max = glm::max(max, other.max);
  }

  glm::vec3 center() { return (min + max) / 2.f; }

  glm::vec3 halfRange() { return (max - min) / 2.f; }

  friend std::ostream &operator<<(std::ostream &os, const AABB &aabb) {
    os << "min: " << glm::to_string(aabb.min) << " max: " << glm::to_string(aabb.max);
    return os;
  }
};

enum class PrimitiveTopology { Triangles, Lines, Procedural };
enum class DynamicType { Static, Dynamic };

class Primitive {
public:
  Primitive() = default;
  /**
   *
   * @param index
   * @param vertex
   * @param aabb
   * @param topology once set, cannot change.
   */
  Primitive(
    const Range &index, const Range &vertex, const AABB &aabb,
    const PrimitiveTopology &topology, const DynamicType &_type = DynamicType::Static);
  const Range &index() const;
  const Range &vertex() const;
  const AABB &aabb() const;
  PrimitiveTopology topology() const;

private:
  Range _index, _vertex;
  AABB _aabb;
  PrimitiveTopology _topology{PrimitiveTopology::Triangles};
  DynamicType _type{DynamicType::Static};
};

enum class MaterialType : uint32_t {
  /**BRDF without reflection trace*/
  eBRDF = 0x1u,
  eBRDFSG = 0x2u,
  /**BRDF with reflection trace*/
  eReflective = 0x4u,
  /**BRDF with reflection trace and refraction trace*/
  eRefractive = 0x8u,
  /**diffuse coloring*/
  eNone = 0x10u,
  /**translucent color blending*/
  eTranslucent = 0x20u
};

class Material {
  friend class BasicModelManager;
  friend class Mesh;

  // ref in shaders
  struct alignas(sizeof(glm::vec4)) UBO {
    glm::vec4 baseColorFactor{1.f, 1.f, 1.f, 1.f};
    glm::vec4 pbrFactor{0.f, 1.f, 0.f, 0.f};
    glm::vec4 emissiveFactor{0.f, 0.f, 0.f, 0.f};
    float occlusionStrength{1.f};
    float alphaCutoff{0.f};
    int32_t colorTex{-1}, pbrTex{-1}, normalTex{-1}, occlusionTex{-1}, emissiveTex{-1};
    uint32_t type{0u};
  };

public:
  /**
   *
   * @param type  once set, cannot change.
   * @param offset
   */
  explicit Material(MaterialType type, uint32_t offset);

  const Ptr<TextureImage2D> &colorTex() const;
  Material &setColorTex(const Ptr<TextureImage2D> &colorTex);
  const Ptr<TextureImage2D> &pbrTex() const;
  Material &setPbrTex(const Ptr<TextureImage2D> &pbrTex);
  const Ptr<TextureImage2D> &normalTex() const;
  Material &setNormalTex(const Ptr<TextureImage2D> &normalTex);
  const Ptr<TextureImage2D> &occlusionTex() const;
  Material &setOcclusionTex(const Ptr<TextureImage2D> &occlusionTex);
  const Ptr<TextureImage2D> &emissiveTex() const;
  Material &setEmissiveTex(const Ptr<TextureImage2D> &emissiveTex);
  const glm::vec4 &colorFactor() const;
  Material &setColorFactor(const glm::vec4 &colorFactor);
  const glm::vec4 &pbrFactor() const;
  Material &setPbrFactor(const glm::vec4 &pbrFactor);
  float occlusionStrength() const;
  Material &setOcclusionStrength(float occlusionStrength);
  float alphaCutoff() const;
  Material &setAlphaCutoff(float alphaCutoff);
  const glm::vec4 &emissiveFactor() const;
  Material &setEmissiveFactor(const glm::vec4 &emissiveFactor);
  MaterialType type() const;

private:
  bool incoherent() const;
  void invalidate();
  uint32_t offset() const;
  UBO flush();

  Ptr<TextureImage2D> _colorTex{}, _pbrTex{}, _normalTex{}, _occlusionTex{},
    _emissiveTex{};
  glm::vec4 _colorFactor{1.f};
  glm::vec4 _pbrFactor{0.f, 1.f, 0.f, 0.f};
  float _occlusionStrength{1.f};
  float _alphaCutoff{0.f};
  glm::vec4 _emissiveFactor{1.f};

  MaterialType _type{MaterialType::eNone};

  bool _incoherent{true};
  uint32_t _offset{-1u};
};

struct Transform {
  glm::vec3 translation{0.f};
  glm::vec3 scale{1.f};
  glm::quat rotation{1, 0, 0, 0};

  Transform(
    const glm::vec3 &translation = {}, const glm::vec3 &scale = glm::vec3{1.f},
    const glm::quat &rotation = {1, 0, 0, 0})
    : translation(translation), scale(scale), rotation(rotation) {}

  Transform(glm::mat4 m) {
    translation = glm::vec3(m[3]);
    scale = {length(m[0]), length(m[1]), length(m[2])};
    m = glm::scale(m, glm::vec3(1 / scale.x, 1 / scale.y, 1 / scale.z));
    m[3] = glm::vec4(0, 0, 0, 1);
    rotation = quat_cast(m);
  }

  glm::mat4 toMatrix() {
    glm::mat4 m{1.0};
    m = glm::scale(m, scale);
    m = glm::mat4_cast(rotation) * m;
    m[3] = glm::vec4(translation, 1);
    return m;
  }
};

struct DrawQueueIndex {
  uint32_t queueID{-1u};
  uint32_t offset{-1u};
};

class Mesh {
  friend class BasicModelManager;
  friend class Node;

  // ref in shaders
  struct UBO {
    uint32_t primitive, instance, node, material;
  };

public:
  explicit Mesh(
    Ptr<Primitive> primitive, Ptr<Material> material, DrawQueueIndex drawIndex,
    uint32_t offset);

  auto primitive() const -> const Ptr<Primitive> &;
  auto material() const -> const Ptr<Material> &;
  void setPrimitive(const Ptr<Primitive> &primitive);
  void setMaterial(const Ptr<Material> &material);
  bool visible() const;
  void setVisible(bool visible);

private:
  auto instance() const -> const Ptr<ModelInstance> &;
  void setInstance(const Ptr<ModelInstance> &instance);
  auto node() const -> const Ptr<Node> &;
  void setNode(const Ptr<Node> &node);

  void setDrawQueue(DrawQueueIndex drawIndex);

  bool incoherent() const;
  void invalidate();
  auto offset() const -> std::pair<uint32_t, DrawQueueIndex>;
  auto getUBO() const -> UBO;
  auto getDrawCMD() const -> vk::DrawIndexedIndirectCommand;
  void flush();

  Ptr<Primitive> _primitive{};
  Ptr<ModelInstance> _instance{};
  Ptr<Node> _node{};
  Ptr<Material> _material{};
  bool _visible{true};

  DrawQueueIndex _drawIndex{};

  bool _incoherent{true};
  uint32_t _offset{-1u};
};

class Node {
  friend class BasicModelManager;
  friend class Mesh;
  friend class ModelInstance;

public:
  static void addMesh(Ptr<Node> node, Ptr<Mesh> mesh);
  static void addChild(Ptr<Node> parent, Ptr<Node> child);

  explicit Node(const Transform &transform, const std::string &name, uint32_t offset);

  std::string &name();
  void setName(const std::string &name);
  const Transform &transform() const;
  void setTransform(const Transform &transform);
  const std::vector<Ptr<Mesh>> &meshes() const;
  const Ptr<Node> &parent() const;
  const std::vector<Ptr<Node>> &children() const;
  bool visible() const;
  void setVisible(bool visible);

  AABB aabb();
  glm::mat4 globalMatrix();

private:
  void setInstance(Ptr<ModelInstance> instance);
  void invalidate();
  bool incoherent() const;
  uint32_t offset() const;
  glm::mat4 flush();

  std::string _name{};
  Transform _transform{};
  std::vector<Ptr<Mesh>> _meshes{};

  Ptr<Node> _parent{};
  std::vector<Ptr<Node>> _children{};

  Ptr<ModelInstance> _instance{};
  bool _visible{true};

  AABB _aabb;

  glm::mat4 _globalMatrix{1.f};
  bool _globalMatrix_dirty{true};
  bool _incoherent{true};
  uint32_t _offset{-1u};
};

class Model {
  friend class BasicModelManager;
  friend class ModelInstance;

public:
  explicit Model(std::vector<Ptr<Node>> nodes);

  const std::vector<Ptr<Node>> &nodes() const;

  AABB aabb();

private:
  std::vector<Ptr<Node>> _nodes;

  AABB _aabb;
};

class ModelInstance {
  friend class BasicModelManager;
  friend class Mesh;

public:
  static void applyModel(Ptr<ModelInstance> instance, Ptr<Model> model);
  explicit ModelInstance(const Transform &transform, uint32_t offset);

  const Transform &transform() const;
  void setTransform(const Transform &transform);
  glm::mat4 matrix();
  Ptr<Model> model();
  bool visible() const;
  void setVisible(bool visible);

private:
  glm::mat4 flush();

  bool incoherent() const;
  void invalidate();
  uint32_t offset() const;

  Transform _transform;
  Ptr<Model> _model;
  bool _visible{true};

  glm::mat4 _matrix{1.f};
  bool _matrix_dirty{true};
  bool _incoherent{true};
  uint32_t _offset;
};

}

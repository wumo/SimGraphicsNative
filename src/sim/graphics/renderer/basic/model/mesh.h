
#pragma once
#include "sim/graphics/base/vkcommon.h"
#include "primitive.h"
#include "material.h"

namespace sim::graphics::renderer::basic {

class ModelInstance;
class Node;

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
}
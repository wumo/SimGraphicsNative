
#pragma once
#include "sim/graphics/base/vkcommon.h"
#include "primitive.h"
#include "material.h"

namespace sim::graphics::renderer::basic {

class Mesh {
  friend class BasicSceneManager;
  friend class Node;

public:
  explicit Mesh(Ptr<Primitive> primitive, Ptr<Material> material);

  auto primitive() const -> const Ptr<Primitive> &;
  auto material() const -> const Ptr<Material> &;

private:
  Ptr<Primitive> _primitive{};
  Ptr<Material> _material{};
};
}
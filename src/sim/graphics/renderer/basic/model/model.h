#pragma once
#include "node.h"
#include "animation.h"

namespace sim::graphics::renderer::basic {
class Model {
  friend class BasicModelManager;
  friend class ModelInstance;

public:
  explicit Model(std::vector<Ptr<Node>> nodes);

  const std::vector<Ptr<Node>> &nodes() const;

  AABB aabb();

  void applyAnimation(uint32_t animationIdx, float time);

private:
  std::vector<Ptr<Node>> _nodes;
  std::vector<Animation> _animations;

  AABB _aabb;
};
}
#include "model.h"

namespace sim::graphics::renderer::basic {

Model::Model(std::vector<Ptr<Node>> nodes): _nodes{std::move(nodes)} {}
const std::vector<Ptr<Node>> &Model::nodes() const { return _nodes; }

AABB Model::aabb() {
  _aabb = {};
  for(auto &node: _nodes)
    _aabb.merge(node->aabb());
  return _aabb;
}

void Model::applyAnimation(uint32_t animationIdx, float time) {
  auto &animation = _animations[animationIdx];
  for(auto &channel: animation.channels) {
    auto &sampler = animation.samplers[channel.samplerIdx];
  }
}
}
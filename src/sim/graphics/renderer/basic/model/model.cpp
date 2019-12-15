#include "model.h"

namespace sim::graphics::renderer::basic {
using Path = Animation::AnimationChannel::PathType;
using Interpolation = Animation::AnimationSampler::InterpolationType;

void Animation::animate(uint32_t index, float elapsed) {
  auto &channel = channels[index];
  auto &sampler = samplers[channel.samplerIdx];

  auto transform = channel.node->transform();
  glm::vec4 result{};
  if(sampler.keyTimings.size() == 1) result = sampler.keyFrames[0];
  else {
    auto t = channel.prevTime;
    t += elapsed;
    t = std::max(std::fmod(t, sampler.keyTimings.back()), sampler.keyTimings.front());
    if(channel.prevTime > t) channel.prevKey = 0;
    channel.prevTime = t;
    auto nextKey = 0;
    for(int i = 0; i < sampler.keyTimings.size(); ++i)
      if(t <= sampler.keyTimings[i]) {
        nextKey = std::clamp(i, 1, int(sampler.keyTimings.size() - 1));
        break;
      }
    channel.prevKey = std::clamp(nextKey - 1, 0, nextKey);
    auto keyDelta = sampler.keyTimings[nextKey] - sampler.keyTimings[channel.prevKey];
    auto tn = (t - sampler.keyTimings[channel.prevKey]) / keyDelta;
    if(channel.path == Path::Rotation) {
      if(sampler.interpolation == Interpolation::CubicSpline)
        result = sampler.cubicSpline(channel.prevKey, nextKey, keyDelta, tn);
      else
        result = sampler.interpolateRotation(channel.prevKey, nextKey, tn);
    } else {
      switch(sampler.interpolation) {
        case Interpolation::Linear:
          result = sampler.linear(channel.prevKey, nextKey, tn);
          break;
        case Interpolation::Step: result = sampler.keyFrames[channel.prevKey]; break;
        case Interpolation::CubicSpline:
          result = sampler.cubicSpline(channel.prevKey, nextKey, keyDelta, tn);
          break;
      }
    }
  }
  switch(channel.path) {
    case Path::Translation: transform.translation = result; break;
    case Path::Rotation:
      transform.rotation =
        glm::normalize(glm::quat{result.w, result.x, result.y, result.z});
      break;
    case Path::Scale: transform.scale = result; break;
  }
  channel.node->setTransform(transform);
}

void Animation::animateAll(float elapsed) {
  for(auto i = 0u; i < channels.size(); ++i)
    animate(i, elapsed);
}

glm::vec4 Animation::AnimationSampler::cubicSpline(
  uint32_t key, uint32_t nextKey, float keyDelta, float t) const {
  auto prevIdx = key * 3;
  auto nextIdx = nextKey * 3;

  float tSq = std::pow(t, 2);
  float tCub = std::pow(t, 3);

  auto v0 = keyFrames[prevIdx + 1];
  auto a = keyDelta * keyFrames[nextIdx];
  auto b = keyDelta * keyFrames[prevIdx + 2];
  auto v1 = keyFrames[nextIdx + 1];

  return (2 * tCub - 3 * tSq + 1) * v0 + (tCub - 2 * tSq + t) * b +
         (-2 * tCub + 3 * tSq) * v1 + (tCub - tSq) * a;
}

glm::vec4 Animation::AnimationSampler::interpolateRotation(
  uint32_t key, uint32_t nextKey, float tn) const {
  auto _q0 = keyFrames[key];
  auto _q1 = keyFrames[nextKey];
  glm::quat q0{_q0.w, _q0.x, _q0.y, _q0.z};
  glm::quat q1{_q1.w, _q1.x, _q1.y, _q1.z};
  auto result = glm::slerp(q0, q1, tn);
  return {result.x, result.y, result.z, result.w};
}

glm::vec4 Animation::AnimationSampler::linear(
  uint32_t key, uint32_t nextKey, float tn) const {
  return glm::mix(keyFrames[key], keyFrames[nextKey], tn);
}

Model::Model(std::vector<Ptr<Node>> &&nodes, std::vector<Animation> &&animations)
  : _nodes{std::move(nodes)}, _animations{std::move(animations)} {}

const std::vector<Ptr<Node>> &Model::nodes() const { return _nodes; }
AABB Model::aabb() {
  _aabb = {};
  for(auto &node: _nodes)
    _aabb.merge(node->aabb());
  return _aabb;
}
std::vector<Animation> &Model::animations() { return _animations; }
}
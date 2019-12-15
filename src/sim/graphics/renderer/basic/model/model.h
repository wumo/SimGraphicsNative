#pragma once
#include "node.h"

namespace sim::graphics::renderer::basic {
class Animation {
public:
  class AnimationChannel {
  public:
    enum class PathType { Translation, Rotation, Scale };
    PathType path;
    Ptr<Node> node;
    uint32_t samplerIdx;
    float prevTime{0};
    uint32_t prevKey{0};
  };
  class AnimationSampler {
  public:
    enum class InterpolationType { Linear, Step, CubicSpline };
    InterpolationType interpolation;
    std::vector<float> keyTimings;
    std::vector<glm::vec4> keyFrames;

    glm::vec4 cubicSpline(uint32_t key, uint32_t nextKey, float keyDelta, float t) const;
    glm::vec4 interpolateRotation(uint32_t key, uint32_t nextKey, float t) const;
    glm::vec4 linear(uint32_t key, uint32_t nextKey, float t) const;
  };
  void animate(uint32_t index, float elapsed);
  void animateAll(float elapsed);

  std::string name;
  std::vector<AnimationSampler> samplers;
  std::vector<AnimationChannel> channels;
};

class Model {
  friend class BasicModelManager;
  friend class ModelInstance;

public:
  Model(std::vector<Ptr<Node>> &&nodes, std::vector<Animation> &&animations);

  const std::vector<Ptr<Node>> &nodes() const;

  AABB aabb();

  std::vector<Animation> &animations();

private:
  std::vector<Ptr<Node>> _nodes;
  std::vector<Animation> _animations;

  AABB _aabb;
};
}
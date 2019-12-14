//
// Created by WuMo on 12/14/2019.
//

#pragma once
#include "sim/graphics/base/glm_common.h"

namespace sim::graphics::renderer::basic {

class Animation {
public:
  class AnimationChannel {
  public:
    enum class PathType { Translation, Rotation, Scale };
    PathType path;
    uint32_t nodeIdx;
    uint32_t samplerIdx;
  };
  class AnimationSampler {
  public:
    enum class InterpolationType { Linear, Step, CubicSpline };
    InterpolationType interpolation;
    std::vector<float> inputs;
    std::vector<glm::vec4> outputs;
  };

  std::string name;
  std::vector<AnimationSampler> samplers;
  std::vector<AnimationChannel> channels;
  float start, end;
};
}
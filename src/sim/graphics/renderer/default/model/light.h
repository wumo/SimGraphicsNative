#pragma once
#include "sim/graphics/base/vkcommon.h"

namespace sim::graphics::renderer::defaultRenderer {
struct Light {
  glm::vec4 location;
  glm::vec4 prop;
};

struct Lighting {
  float exposure{4.5f};
  float gamma{2.2f};
};
}

#pragma once
#include "sim/graphics/renderer/basic/framegraph/render_pass.h"

namespace sim::graphics::renderer::basic {
class OceanRenderPass {

public:
  OceanRenderPass(Device &device, DebugMarker &debugMarker);

private:
  Device &device;
  DebugMarker &debugMarker;
};
}
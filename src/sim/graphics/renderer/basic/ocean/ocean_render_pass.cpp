#include "ocean_render_pass.h"
namespace sim::graphics::renderer::basic {

OceanRenderPass::OceanRenderPass(Device &device, DebugMarker &debugMarker)
  : device(device), debugMarker(debugMarker) {}
}
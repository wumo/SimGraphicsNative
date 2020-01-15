#pragma once
#include "sim/graphics/base/vkcommon.h"
#include "frame_graph_builder.h"

namespace sim::graphics::renderer::basic {
class RenderPass {

public:
  explicit RenderPass(FrameGraphBuilder &builder);

  virtual void compile();

  virtual void execute();
};
}
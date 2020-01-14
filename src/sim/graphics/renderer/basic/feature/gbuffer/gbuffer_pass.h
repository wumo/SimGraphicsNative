#pragma once
#include "../../framegraph/render_pass.h"

namespace sim::graphics::renderer::basic {
class GBufferPass :public RenderPass{

public:
  GBufferPass(RenderPassBuilder &builder);
  void compile() override;
  void execute() override;
};
}
#pragma once
#include "../../framegraph/render_pass.h"

namespace sim::graphics::renderer::basic {
class DeferredPass: public RenderPass {

public:
  explicit DeferredPass(RenderPassBuilder &builder);
  void compile() override;
  void execute() override;
};
}
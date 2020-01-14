#include "gbuffer_pass.h"

namespace sim::graphics::renderer::basic {
void GBufferPass::compile() { RenderPass::compile(); }
void GBufferPass::execute() { RenderPass::execute(); }
GBufferPass::GBufferPass(RenderPassBuilder &builder): RenderPass(builder) {


}
}
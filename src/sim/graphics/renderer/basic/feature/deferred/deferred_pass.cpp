#include "deferred_pass.h"
namespace sim::graphics::renderer::basic {

void DeferredPass::compile() { RenderPass::compile(); }
void DeferredPass::execute() { RenderPass::execute(); }
DeferredPass::DeferredPass(RenderPassBuilder &builder): RenderPass(builder) {

}
}
#pragma once
#include "../../framegraph/render_pass.h"
#include "sim/graphics/base/pipeline/descriptors.h"

namespace sim::graphics::renderer::basic {
class GBufferPass: public RenderPass {

public:
  explicit GBufferPass(FrameGraphBuilder &builder);
  void compile() override;
  void execute() override;

private:
  using flag = vk::DescriptorBindingFlagBitsEXT;
  using shader = vk::ShaderStageFlagBits;

  struct SetDef: DescriptorSetDef {
    __uniform__(
      cam, shader::eVertex | shader::eFragment | shader::eTessellationControl |
             shader::eTessellationEvaluation);
    __buffer__(primitives, shader::eVertex | shader::eTessellationControl);
    __buffer__(meshInstances, shader::eVertex | shader::eTessellationControl);
    __buffer__(transforms, shader::eVertex | shader::eTessellationControl);
    __buffer__(
      material, shader::eVertex | shader::eFragment | shader::eTessellationControl);
    __samplers__(
      textures, flag{},
      shader::eVertex | shader::eFragment | shader::eTessellationControl |
        shader::eTessellationEvaluation);
  } setDef;

private:
  sPtr<TextureReference> position, normal, albedo, pbr, emissive, depth;

  sPtr<DescriptorSetReference> set_;
};
}
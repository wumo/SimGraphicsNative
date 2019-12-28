#include "pipeline.h"

namespace sim::graphics {
using shader = vk::ShaderStageFlagBits;

auto PipelineLayoutMaker::createUnique(const vk::Device &device) const
  -> vk::UniquePipelineLayout {
  vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
  pipelineLayoutInfo.setLayoutCount = uint32_t(setLayouts.size());
  pipelineLayoutInfo.pSetLayouts = setLayouts.data();
  pipelineLayoutInfo.pushConstantRangeCount = uint32_t(pushConstantRanges.size());
  pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();
  return std::move(device.createPipelineLayoutUnique(pipelineLayoutInfo));
}

auto PipelineLayoutMaker::addSetLayout(const vk::DescriptorSetLayout &layout)
  -> PipelineLayoutMaker & {
  setLayouts.push_back(layout);
  _setDefs.push_back(nullptr);
  return *this;
}
auto PipelineLayoutMaker::updateSetLayout(
  uint32_t set, const vk::DescriptorSetLayout &layout,
  const DescriptorSetLayoutMaker *setDef) -> PipelineLayoutMaker & {
  setLayouts.at(set) = layout;
  _setDefs.at(set) = setDef;
  return *this;
}

auto PipelineLayoutMaker::pushConstantRange(
  const vk::PushConstantRange &pushConstantRange) -> PipelineLayoutMaker & {
  pushConstantRanges.push_back(pushConstantRange);
  return *this;
}

auto PipelineLayoutMaker::setDefs() const
  -> const std::vector<const DescriptorSetLayoutMaker *> & {
  return _setDefs;
}

GraphicsPipelineMaker::GraphicsPipelineMaker(
  const vk::Device &device, uint32_t width, uint32_t height)
  : device{device} {
  _inputAssemblyState.topology = vk::PrimitiveTopology::eTriangleList;
  _viewport = vk::Viewport{0.0f, 0.0f, float(width), float(height), 0.0f, 1.0f};
  _scissor = vk::Rect2D{{0, 0}, {width, height}};
  _rasterizationState.lineWidth = 1.0f;

  _depthStencilState.depthTestEnable = VK_TRUE;
  _depthStencilState.depthWriteEnable = VK_TRUE;
  _depthStencilState.depthCompareOp = vk::CompareOp::eLess;
  _depthStencilState.depthBoundsTestEnable = VK_FALSE;
  _depthStencilState.back.failOp = vk::StencilOp::eKeep;
  _depthStencilState.back.passOp = vk::StencilOp::eKeep;
  _depthStencilState.back.compareOp = vk::CompareOp::eAlways;
  _depthStencilState.stencilTestEnable = VK_FALSE;
  _depthStencilState.front = _depthStencilState.back;
}

auto GraphicsPipelineMaker::createUnique(
  const vk::PipelineCache &pipelineCache, const vk::PipelineLayout &pipelineLayout,
  const vk::RenderPass &renderPass) -> vk::UniquePipeline {
  _colorBlendState.attachmentCount = uint32_t(colorBlendAttachments.size());
  _colorBlendState.pAttachments = colorBlendAttachments.data();

  vk::PipelineViewportStateCreateInfo viewportState{{}, 1, &_viewport, 1, &_scissor};

  vk::PipelineVertexInputStateCreateInfo vertexInputState;
  vertexInputState.vertexAttributeDescriptionCount =
    uint32_t(_vertexAttributeDescriptions.size());
  vertexInputState.pVertexAttributeDescriptions = _vertexAttributeDescriptions.data();
  vertexInputState.vertexBindingDescriptionCount =
    uint32_t(_vertexBindingDescriptions.size());
  vertexInputState.pVertexBindingDescriptions = _vertexBindingDescriptions.data();

  vk::PipelineDynamicStateCreateInfo dynamicState{
    {}, uint32_t(_dynamicState.size()), _dynamicState.data()};

  vk::GraphicsPipelineCreateInfo pipelineInfo;
  pipelineInfo.stageCount = uint32_t(_modules.size());
  pipelineInfo.pStages = _modules.data();
  pipelineInfo.pVertexInputState = &vertexInputState;
  pipelineInfo.pInputAssemblyState = &_inputAssemblyState;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &_rasterizationState;
  pipelineInfo.pMultisampleState = &_multisampleState;
  pipelineInfo.pDepthStencilState = &_depthStencilState;
  pipelineInfo.pColorBlendState = &_colorBlendState;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.pTessellationState = &_tesselationState;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass = _subpass;

  return device.createGraphicsPipelineUnique(pipelineCache, pipelineInfo);
}

auto GraphicsPipelineMaker::clearShaders() -> GraphicsPipelineMaker & {
  shaders.clear();
  _modules.clear();
  return *this;
}

auto GraphicsPipelineMaker::shader(
  const vk::ShaderStageFlagBits &stageFlagBits, const std::string &filename,
  const vk::SpecializationInfo *pSpecializationInfo) -> GraphicsPipelineMaker & {
  shaders.emplace_back(device, filename, pSpecializationInfo);
  _modules.emplace_back(
    vk::PipelineShaderStageCreateFlags{}, stageFlagBits, shaders.back().module(), "main",
    pSpecializationInfo);
  return *this;
}

auto GraphicsPipelineMaker::shader(
  const vk::ShaderStageFlagBits &stageFlagBits, const uint32_t *opcodes, size_t size,
  const vk::SpecializationInfo *pSpecializationInfo) -> GraphicsPipelineMaker & {
  shaders.emplace_back(device, opcodes, size, pSpecializationInfo);
  _modules.emplace_back(
    vk::PipelineShaderStageCreateFlags{}, stageFlagBits, shaders.back().module(), "main",
    pSpecializationInfo);
  return *this;
}

auto GraphicsPipelineMaker::subpass(const uint32_t &subpass) -> GraphicsPipelineMaker & {
  _subpass = subpass;
  return *this;
}

auto GraphicsPipelineMaker::colorBlendState(
  const vk::PipelineColorBlendStateCreateInfo &value) -> GraphicsPipelineMaker & {
  _colorBlendState = value;
  return *this;
}

auto GraphicsPipelineMaker::blendColorAttachment(const vk::Bool32 &enable)
  -> BlendColorAttachmentMaker {
  colorBlendAttachments.emplace_back();
  auto &blend = colorBlendAttachments.back();
  blend.blendEnable = enable;
  blend.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
  blend.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
  blend.colorBlendOp = vk::BlendOp::eAdd;
  blend.srcAlphaBlendFactor = vk::BlendFactor::eSrcAlpha;
  blend.dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
  blend.alphaBlendOp = vk::BlendOp::eAdd;
  using flag = vk::ColorComponentFlagBits;
  blend.colorWriteMask = flag::eR | flag::eG | flag::eB | flag::eA;
  return BlendColorAttachmentMaker{*this, uint32_t(colorBlendAttachments.size() - 1)};
}

GraphicsPipelineMaker::BlendColorAttachmentMaker::BlendColorAttachmentMaker(
  GraphicsPipelineMaker &pipelineMaker, uint32_t index)
  : maker{pipelineMaker}, index{index} {}

auto GraphicsPipelineMaker::BlendColorAttachmentMaker::enable(const vk::Bool32 &value)
  -> BlendColorAttachmentMaker & {
  maker.colorBlendAttachments[index].blendEnable = value;
  return *this;
}

auto GraphicsPipelineMaker::BlendColorAttachmentMaker::srcColorBlendFactor(
  const vk::BlendFactor &value) -> BlendColorAttachmentMaker & {
  maker.colorBlendAttachments[index].srcColorBlendFactor = value;
  return *this;
}

auto GraphicsPipelineMaker::BlendColorAttachmentMaker::dstColorBlendFactor(
  const vk::BlendFactor &value) -> BlendColorAttachmentMaker & {
  maker.colorBlendAttachments[index].dstColorBlendFactor = value;
  return *this;
}

auto GraphicsPipelineMaker::BlendColorAttachmentMaker::colorBlendOp(
  const vk::BlendOp &value) -> BlendColorAttachmentMaker & {
  maker.colorBlendAttachments[index].colorBlendOp = value;
  return *this;
}

auto GraphicsPipelineMaker::BlendColorAttachmentMaker::srcAlphaBlendFactor(
  const vk::BlendFactor &value) -> BlendColorAttachmentMaker & {
  maker.colorBlendAttachments[index].srcAlphaBlendFactor = value;
  return *this;
}

auto GraphicsPipelineMaker::BlendColorAttachmentMaker::dstAlphaBlendFactor(
  const vk::BlendFactor &value) -> BlendColorAttachmentMaker & {
  maker.colorBlendAttachments[index].dstAlphaBlendFactor = value;
  return *this;
}

auto GraphicsPipelineMaker::BlendColorAttachmentMaker::alphaBlendOp(
  const vk::BlendOp &value) -> BlendColorAttachmentMaker & {
  maker.colorBlendAttachments[index].alphaBlendOp = value;
  return *this;
}

auto GraphicsPipelineMaker::BlendColorAttachmentMaker::colorWriteMask(
  const vk::ColorComponentFlags &value) -> BlendColorAttachmentMaker & {
  maker.colorBlendAttachments[index].colorWriteMask = value;
  return *this;
}

auto GraphicsPipelineMaker::logicOpEnable(const vk::Bool32 &value)
  -> GraphicsPipelineMaker & {
  _colorBlendState.logicOpEnable = value;
  return *this;
}

auto GraphicsPipelineMaker::logicOp(const vk::LogicOp &value) -> GraphicsPipelineMaker & {
  _colorBlendState.logicOp = value;
  return *this;
}

auto GraphicsPipelineMaker::blendConstants(
  const float &r, const float &g, const float &b, const float &a)
  -> GraphicsPipelineMaker & {
  auto bc = _colorBlendState.blendConstants;
  bc[0] = r;
  bc[1] = g;
  bc[2] = b;
  bc[3] = a;
  return *this;
}

auto GraphicsPipelineMaker::vertexAttribute(
  const uint32_t &binding, const uint32_t &location, const vk::Format &format,
  const uint32_t &offset) -> GraphicsPipelineMaker & {
  _vertexAttributeDescriptions.emplace_back(location, binding, format, offset);
  return *this;
}

auto GraphicsPipelineMaker::vertexAttribute(
  const vk::VertexInputAttributeDescription &desc) -> GraphicsPipelineMaker & {
  _vertexAttributeDescriptions.push_back(desc);
  return *this;
}

auto GraphicsPipelineMaker::vertexBinding(
  const uint32_t &binding, const uint32_t &stride, const vk::VertexInputRate &inputRate)
  -> GraphicsPipelineMaker & {
  _vertexBindingDescriptions.emplace_back(binding, stride, inputRate);
  return *this;
}

auto GraphicsPipelineMaker::vertexBinding(const vk::VertexInputBindingDescription &desc)
  -> GraphicsPipelineMaker & {
  _vertexBindingDescriptions.push_back(desc);
  return *this;
}

auto GraphicsPipelineMaker::vertexBinding(
  const uint32_t &binding, const uint32_t &stride,
  const std::vector<vk::VertexInputAttributeDescription> &desc,
  const vk::VertexInputRate &inputRate) -> GraphicsPipelineMaker & {
  _vertexBindingDescriptions.emplace_back(binding, stride, inputRate);
  for(const auto &item: desc)
    vertexAttribute(item);
  return *this;
}

auto GraphicsPipelineMaker::inputAssemblyState(
  const vk::PipelineInputAssemblyStateCreateInfo &value) -> GraphicsPipelineMaker & {
  _inputAssemblyState = value;
  return *this;
}

auto GraphicsPipelineMaker::topology(const vk::PrimitiveTopology &topology)
  -> GraphicsPipelineMaker & {
  _inputAssemblyState.topology = topology;
  return *this;
}

auto GraphicsPipelineMaker::primitiveRestartEnable(
  const vk::Bool32 &primitiveRestartEnable) -> GraphicsPipelineMaker & {
  _inputAssemblyState.primitiveRestartEnable = primitiveRestartEnable;
  return *this;
}

auto GraphicsPipelineMaker::viewport(const vk::Viewport &value)
  -> GraphicsPipelineMaker & {
  _viewport = value;
  return *this;
}

auto GraphicsPipelineMaker::scissor(const vk::Rect2D &value) -> GraphicsPipelineMaker & {
  _scissor = value;
  return *this;
}

auto GraphicsPipelineMaker::rasterizationState(
  const vk::PipelineRasterizationStateCreateInfo &value) -> GraphicsPipelineMaker & {
  _rasterizationState = value;
  return *this;
}

auto GraphicsPipelineMaker::depthClampEnable(const vk::Bool32 &value)
  -> GraphicsPipelineMaker & {
  _rasterizationState.depthClampEnable = value;
  return *this;
}

auto GraphicsPipelineMaker::rasterizerDiscardEnable(const vk::Bool32 &value)
  -> GraphicsPipelineMaker & {
  _rasterizationState.rasterizerDiscardEnable = value;
  return *this;
}

auto GraphicsPipelineMaker::polygonMode(const vk::PolygonMode &value)
  -> GraphicsPipelineMaker & {
  _rasterizationState.polygonMode = value;
  return *this;
}

auto GraphicsPipelineMaker::cullMode(const vk::CullModeFlags &value)
  -> GraphicsPipelineMaker & {
  _rasterizationState.cullMode = value;
  return *this;
}

auto GraphicsPipelineMaker::frontFace(const vk::FrontFace &value)
  -> GraphicsPipelineMaker & {
  _rasterizationState.frontFace = value;
  return *this;
}

auto GraphicsPipelineMaker::depthBiasEnable(const vk::Bool32 &value)
  -> GraphicsPipelineMaker & {
  _rasterizationState.depthBiasEnable = value;
  return *this;
}

auto GraphicsPipelineMaker::depthBiasConstantFactor(const float &value)
  -> GraphicsPipelineMaker & {
  _rasterizationState.depthBiasConstantFactor = value;
  return *this;
}

auto GraphicsPipelineMaker::depthBiasClamp(const float &value)
  -> GraphicsPipelineMaker & {
  _rasterizationState.depthBiasClamp = value;
  return *this;
}

auto GraphicsPipelineMaker::depthBiasSlopeFactor(const float &value)
  -> GraphicsPipelineMaker & {
  _rasterizationState.depthBiasSlopeFactor = value;
  return *this;
}

auto GraphicsPipelineMaker::lineWidth(const float &value) -> GraphicsPipelineMaker & {
  _rasterizationState.lineWidth = value;
  return *this;
}

auto GraphicsPipelineMaker::multisampleState(
  const vk::PipelineMultisampleStateCreateInfo &value) -> GraphicsPipelineMaker & {
  _multisampleState = value;
  return *this;
}

auto GraphicsPipelineMaker::rasterizationSamples(const vk::SampleCountFlagBits &value)
  -> GraphicsPipelineMaker & {
  _multisampleState.rasterizationSamples = value;
  return *this;
}

auto GraphicsPipelineMaker::sampleShadingEnable(const vk::Bool32 &value)
  -> GraphicsPipelineMaker & {
  _multisampleState.sampleShadingEnable = value;
  return *this;
}

auto GraphicsPipelineMaker::minSampleShading(const float &value)
  -> GraphicsPipelineMaker & {
  _multisampleState.minSampleShading = value;
  return *this;
}

auto GraphicsPipelineMaker::pSampleMask(const vk::SampleMask *value)
  -> GraphicsPipelineMaker & {
  _multisampleState.pSampleMask = value;
  return *this;
}

auto GraphicsPipelineMaker::alphaToCoverageEnable(const vk::Bool32 &value)
  -> GraphicsPipelineMaker & {
  _multisampleState.alphaToCoverageEnable = value;
  return *this;
}

auto GraphicsPipelineMaker::alphaToOneEnable(const vk::Bool32 &value)
  -> GraphicsPipelineMaker & {
  _multisampleState.alphaToOneEnable = value;
  return *this;
}

auto GraphicsPipelineMaker::depthStencilState(
  const vk::PipelineDepthStencilStateCreateInfo &value) -> GraphicsPipelineMaker & {
  _depthStencilState = value;
  return *this;
}

auto GraphicsPipelineMaker::depthTestEnable(const vk::Bool32 &value)
  -> GraphicsPipelineMaker & {
  _depthStencilState.depthTestEnable = value;
  return *this;
}

auto GraphicsPipelineMaker::depthWriteEnable(const vk::Bool32 &value)
  -> GraphicsPipelineMaker & {
  _depthStencilState.depthWriteEnable = value;
  return *this;
}

auto GraphicsPipelineMaker::depthCompareOp(const vk::CompareOp &value)
  -> GraphicsPipelineMaker & {
  _depthStencilState.depthCompareOp = value;
  return *this;
}

auto GraphicsPipelineMaker::depthBoundsTestEnable(const vk::Bool32 &value)
  -> GraphicsPipelineMaker & {
  _depthStencilState.depthBoundsTestEnable = value;
  return *this;
}

auto GraphicsPipelineMaker::stencilTestEnable(const vk::Bool32 &value)
  -> GraphicsPipelineMaker & {
  _depthStencilState.stencilTestEnable = value;
  return *this;
}

auto GraphicsPipelineMaker::front(const vk::StencilOpState &value)
  -> GraphicsPipelineMaker & {
  _depthStencilState.front = value;
  return *this;
}

auto GraphicsPipelineMaker::back(const vk::StencilOpState &value)
  -> GraphicsPipelineMaker & {
  _depthStencilState.back = value;
  return *this;
}

auto GraphicsPipelineMaker::minDepthBounds(const float &value)
  -> GraphicsPipelineMaker & {
  _depthStencilState.minDepthBounds = value;
  return *this;
}

auto GraphicsPipelineMaker::maxDepthBounds(const float &value)
  -> GraphicsPipelineMaker & {
  _depthStencilState.maxDepthBounds = value;
  return *this;
}

auto GraphicsPipelineMaker::dynamicState(const vk::DynamicState &value)
  -> GraphicsPipelineMaker & {
  _dynamicState.push_back(value);
  return *this;
}

auto GraphicsPipelineMaker::tesselationState(uint32_t patchControlPoints)
  -> GraphicsPipelineMaker & {
  _tesselationState.patchControlPoints = patchControlPoints;
  return *this;
}

auto GraphicsPipelineMaker::clearColorBlendAttachments() -> GraphicsPipelineMaker & {
  colorBlendAttachments.clear();
  return *this;
}

auto GraphicsPipelineMaker::clearDynamicStates() -> GraphicsPipelineMaker & {
  _dynamicState.clear();
  return *this;
}

auto GraphicsPipelineMaker::clearVertexAttributeDescriptions()
  -> GraphicsPipelineMaker & {
  _vertexAttributeDescriptions.clear();
  return *this;
}

auto GraphicsPipelineMaker::clearVertexBindingDescriptions() -> GraphicsPipelineMaker & {
  _vertexBindingDescriptions.clear();
  return *this;
}

ComputePipelineMaker::ComputePipelineMaker(const vk::Device &device): device(device) {}

auto ComputePipelineMaker::shader(
  ShaderModule &shader, const vk::SpecializationInfo *pSpecializationInfo) -> void {
  _stage.module = shader.module();
  _stage.pName = "main";
  _stage.stage = vk::ShaderStageFlagBits::eCompute;
  _stage.pSpecializationInfo = pSpecializationInfo;
}

void ComputePipelineMaker::shader(
  const uint32_t *opcodes, size_t size,
  const vk::SpecializationInfo *pSpecializationInfo) {
  _shader = u<ShaderModule>(device, opcodes, size, pSpecializationInfo);

  _stage.module = _shader->module();
  _stage.pName = "main";
  _stage.stage = vk::ShaderStageFlagBits::eCompute;
  _stage.pSpecializationInfo = _shader->specialization();
}

auto ComputePipelineMaker::createUnique(
  const vk::PipelineCache &pipelineCache, const vk::PipelineLayout &pipelineLayout) const
  -> vk::UniquePipeline {
  vk::ComputePipelineCreateInfo pipelineInfo;
  pipelineInfo.stage = _stage;
  pipelineInfo.layout = pipelineLayout;
  return device.createComputePipelineUnique(pipelineCache, pipelineInfo);
}

}

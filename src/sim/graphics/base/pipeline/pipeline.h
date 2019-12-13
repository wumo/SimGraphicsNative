#pragma once
#include "sim/graphics/base/vkcommon.h"
#include "shader.h"

namespace sim::graphics {
class PipelineLayoutMaker {
public:
  vk::UniquePipelineLayout createUnique(const vk::Device &device) const;
  PipelineLayoutMaker &descriptorSetLayout(const vk::DescriptorSetLayout &layout);
  PipelineLayoutMaker &descriptorSetLayout(
    uint32_t set, const vk::DescriptorSetLayout &layout);
  PipelineLayoutMaker &descriptorSetLayouts(
    const std::vector<vk::DescriptorSetLayout> &layouts);
  PipelineLayoutMaker &pushConstantRange(
    const vk::ShaderStageFlags &stageFlags, const uint32_t &offset, const uint32_t &size);
  PipelineLayoutMaker &pushConstantRange(const vk::PushConstantRange &stageFlags);

private:
  std::vector<vk::DescriptorSetLayout> setLayouts;
  std::vector<vk::PushConstantRange> pushConstantRanges;
};

class PipelineLayoutDef {
protected:
  PipelineLayoutMaker maker;
  uint32_t set{0};

public:
  vk::UniquePipelineLayout pipelineLayout{};
  void init(const vk::Device &device) { pipelineLayout = maker.createUnique(device); }
};

template<typename T>
class DescriptorSetLayoutUpdater {
public:
  DescriptorSetLayoutUpdater(PipelineLayoutMaker &maker, uint32_t set)
    : maker(maker), _set{set} {}

  void operator()(const T &def) {
    maker.descriptorSetLayout(_set, *def.descriptorSetLayout);
  }

  uint32_t set() const { return _set; }

private:
  PipelineLayoutMaker &maker;
  uint32_t _set;
};

#define __set__(field, def) \
public:                     \
  DescriptorSetLayoutUpdater<def> field{maker, (maker.descriptorSetLayout({}), set++)};

class GraphicsPipelineMaker {
public:
  class BlendColorAttachmentMaker {
  public:
    explicit BlendColorAttachmentMaker(
      GraphicsPipelineMaker &pipelineMaker, uint32_t index);
    BlendColorAttachmentMaker &enable(const vk::Bool32 &value);
    BlendColorAttachmentMaker &srcColorBlendFactor(const vk::BlendFactor &value);
    BlendColorAttachmentMaker &dstColorBlendFactor(const vk::BlendFactor &value);
    BlendColorAttachmentMaker &colorBlendOp(const vk::BlendOp &value);
    BlendColorAttachmentMaker &srcAlphaBlendFactor(const vk::BlendFactor &value);
    BlendColorAttachmentMaker &dstAlphaBlendFactor(const vk::BlendFactor &value);
    BlendColorAttachmentMaker &alphaBlendOp(const vk::BlendOp &value);
    BlendColorAttachmentMaker &colorWriteMask(const vk::ColorComponentFlags &value);

  private:
    GraphicsPipelineMaker &maker;
    uint32_t index;
  };

  GraphicsPipelineMaker(const vk::Device &device, uint32_t width, uint32_t height);
  vk::UniquePipeline createUnique(
    const vk::Device &device, const vk::PipelineCache &pipelineCache,
    const vk::PipelineLayout &pipelineLayout, const vk::RenderPass &renderPass);
  GraphicsPipelineMaker &clearShaders();
  GraphicsPipelineMaker &shader(
    const vk::ShaderStageFlagBits &stageFlagBits, const std::string &filename,
    const vk::SpecializationInfo *pSpecializationInfo = nullptr);
  GraphicsPipelineMaker &shader(
    const vk::ShaderStageFlagBits &stageFlagBits, const uint32_t *opcodes, size_t size,
    const vk::SpecializationInfo *pSpecializationInfo = nullptr);
  GraphicsPipelineMaker &subpass(const uint32_t &subpass);

  GraphicsPipelineMaker &colorBlendState(
    const vk::PipelineColorBlendStateCreateInfo &value);
  BlendColorAttachmentMaker blendColorAttachment(const vk::Bool32 &enable);
  GraphicsPipelineMaker &logicOpEnable(const vk::Bool32 &value);
  GraphicsPipelineMaker &logicOp(const vk::LogicOp &value);
  GraphicsPipelineMaker &blendConstants(
    const float &r, const float &g, const float &b, const float &a);

  GraphicsPipelineMaker &vertexAttribute(
    const uint32_t &binding, const uint32_t &location, const vk::Format &format,
    const uint32_t &offset);
  GraphicsPipelineMaker &vertexAttribute(const vk::VertexInputAttributeDescription &desc);
  GraphicsPipelineMaker &vertexBinding(
    const uint32_t &binding, const uint32_t &stride,
    const vk::VertexInputRate &inputRate = vk::VertexInputRate::eVertex);
  GraphicsPipelineMaker &vertexBinding(const vk::VertexInputBindingDescription &desc);
  GraphicsPipelineMaker &vertexBinding(
    const uint32_t &binding, const uint32_t &stride,
    const std::vector<vk::VertexInputAttributeDescription> &desc,
    const vk::VertexInputRate &inputRate = vk::VertexInputRate::eVertex);

  GraphicsPipelineMaker &inputAssemblyState(
    const vk::PipelineInputAssemblyStateCreateInfo &value);
  GraphicsPipelineMaker &topology(const vk::PrimitiveTopology &topology);
  GraphicsPipelineMaker &primitiveRestartEnable(const vk::Bool32 &primitiveRestartEnable);

  GraphicsPipelineMaker &viewport(const vk::Viewport &value);
  GraphicsPipelineMaker &scissor(const vk::Rect2D &value);

  GraphicsPipelineMaker &rasterizationState(
    const vk::PipelineRasterizationStateCreateInfo &value);
  GraphicsPipelineMaker &depthClampEnable(const vk::Bool32 &value);
  GraphicsPipelineMaker &rasterizerDiscardEnable(const vk::Bool32 &value);
  GraphicsPipelineMaker &polygonMode(const vk::PolygonMode &value);
  GraphicsPipelineMaker &cullMode(const vk::CullModeFlags &value);
  GraphicsPipelineMaker &frontFace(const vk::FrontFace &value);
  GraphicsPipelineMaker &depthBiasEnable(const vk::Bool32 &value);
  GraphicsPipelineMaker &depthBiasConstantFactor(const float &value);
  GraphicsPipelineMaker &depthBiasClamp(const float &value);
  GraphicsPipelineMaker &depthBiasSlopeFactor(const float &value);
  GraphicsPipelineMaker &lineWidth(const float &value);

  GraphicsPipelineMaker &multisampleState(
    const vk::PipelineMultisampleStateCreateInfo &value);
  GraphicsPipelineMaker &rasterizationSamples(const vk::SampleCountFlagBits &value);
  GraphicsPipelineMaker &sampleShadingEnable(const vk::Bool32 &value);
  GraphicsPipelineMaker &minSampleShading(const float &value);
  GraphicsPipelineMaker &pSampleMask(const vk::SampleMask *value);
  GraphicsPipelineMaker &alphaToCoverageEnable(const vk::Bool32 &value);
  GraphicsPipelineMaker &alphaToOneEnable(const vk::Bool32 &value);

  GraphicsPipelineMaker &depthStencilState(
    const vk::PipelineDepthStencilStateCreateInfo &value);
  GraphicsPipelineMaker &depthTestEnable(const vk::Bool32 &value);
  GraphicsPipelineMaker &depthWriteEnable(const vk::Bool32 &value);
  GraphicsPipelineMaker &depthCompareOp(const vk::CompareOp &value);
  GraphicsPipelineMaker &depthBoundsTestEnable(const vk::Bool32 &value);
  GraphicsPipelineMaker &stencilTestEnable(const vk::Bool32 &value);
  GraphicsPipelineMaker &front(const vk::StencilOpState &value);
  GraphicsPipelineMaker &back(const vk::StencilOpState &value);
  GraphicsPipelineMaker &minDepthBounds(const float &value);
  GraphicsPipelineMaker &maxDepthBounds(const float &value);

  GraphicsPipelineMaker &dynamicState(const vk::DynamicState &value);

  GraphicsPipelineMaker &clearColorBlendAttachments();
  GraphicsPipelineMaker &clearDynamicStates();
  GraphicsPipelineMaker &clearVertexAttributeDescriptions();
  GraphicsPipelineMaker &clearVertexBindingDescriptions();

private:
  const vk::Device &device;
  vk::PipelineInputAssemblyStateCreateInfo _inputAssemblyState;
  vk::Viewport _viewport;
  vk::Rect2D _scissor;
  vk::PipelineRasterizationStateCreateInfo _rasterizationState;
  vk::PipelineMultisampleStateCreateInfo _multisampleState;
  vk::PipelineDepthStencilStateCreateInfo _depthStencilState;
  vk::PipelineColorBlendStateCreateInfo _colorBlendState;
  std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments;
  std::vector<vk::PipelineShaderStageCreateInfo> _modules;
  std::vector<vk::VertexInputAttributeDescription> _vertexAttributeDescriptions;
  std::vector<vk::VertexInputBindingDescription> _vertexBindingDescriptions;
  std::vector<vk::DynamicState> _dynamicState;
  uint32_t _subpass = 0;
  std::vector<ShaderModule> shaders;
};

class ComputePipelineMaker {
public:
  void shader(ShaderModule &shader, const vk::SpecializationInfo *pSpecializationInfo);
  ComputePipelineMaker &module(const vk::PipelineShaderStageCreateInfo &value);
  vk::UniquePipeline createUnique(
    const vk::Device &device, const vk::PipelineCache &pipelineCache,
    const vk::PipelineLayout &pipelineLayout) const;

private:
  vk::PipelineShaderStageCreateInfo _stage;
};
}

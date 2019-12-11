#include "render_pass.h"

namespace sim::graphics {
RenderPassMaker::AttachmentMaker::AttachmentMaker(RenderPassMaker &maker, uint32_t index)
  : maker{maker}, _index{index} {}

auto RenderPassMaker::AttachmentMaker::flags(const vk::AttachmentDescriptionFlags &value)
  -> AttachmentMaker & {
  maker.attachmentDescriptions[_index].flags = value;
  return *this;
}

auto RenderPassMaker::AttachmentMaker::format(const vk::Format &value)
  -> AttachmentMaker & {
  maker.attachmentDescriptions[_index].format = value;
  return *this;
}

auto RenderPassMaker::AttachmentMaker::samples(const vk::SampleCountFlagBits &value)
  -> AttachmentMaker & {
  maker.attachmentDescriptions[_index].samples = value;
  return *this;
}

auto RenderPassMaker::AttachmentMaker::loadOp(const vk::AttachmentLoadOp &value)
  -> AttachmentMaker & {
  maker.attachmentDescriptions[_index].loadOp = value;
  return *this;
}

auto RenderPassMaker::AttachmentMaker::storeOp(const vk::AttachmentStoreOp &value)
  -> AttachmentMaker & {
  maker.attachmentDescriptions[_index].storeOp = value;
  return *this;
}

auto RenderPassMaker::AttachmentMaker::stencilLoadOp(const vk::AttachmentLoadOp &value)
  -> AttachmentMaker & {
  maker.attachmentDescriptions[_index].stencilLoadOp = value;
  return *this;
}

auto RenderPassMaker::AttachmentMaker::stencilStoreOp(const vk::AttachmentStoreOp &value)
  -> AttachmentMaker & {
  maker.attachmentDescriptions[_index].stencilStoreOp = value;
  return *this;
}

auto RenderPassMaker::AttachmentMaker::initialLayout(const vk::ImageLayout &value)
  -> AttachmentMaker & {
  maker.attachmentDescriptions[_index].initialLayout = value;
  return *this;
}

auto RenderPassMaker::AttachmentMaker::finalLayout(const vk::ImageLayout &value)
  -> AttachmentMaker & {
  maker.attachmentDescriptions[_index].finalLayout = value;
  return *this;
}

auto RenderPassMaker::AttachmentMaker::index() const -> uint32_t { return _index; }

auto RenderPassMaker::attachment(const vk::Format &format) -> AttachmentMaker {
  auto desc = attachmentDescriptions.empty() ? vk::AttachmentDescription{} :
                                               attachmentDescriptions.back();
  desc.format = format;
  return attachment(desc);
}

auto RenderPassMaker::attachment(const vk::AttachmentDescription &desc)
  -> AttachmentMaker {
  attachmentDescriptions.push_back(desc);
  return AttachmentMaker(*this, uint32_t(attachmentDescriptions.size() - 1));
}

RenderPassMaker::SubpassMaker::SubpassMaker(
  vk::PipelineBindPoint bindpoint, uint32_t index)
  : bindpoint{bindpoint}, _index{index} {}

auto RenderPassMaker::subpass(vk::PipelineBindPoint bindPoint) -> SubpassMaker & {
  subpasses.emplace_back(bindPoint, uint32_t(subpasses.size()));
  return subpasses.back();
}

auto RenderPassMaker::SubpassMaker::color(
  const uint32_t &attachment, const vk::ImageLayout &layout) -> SubpassMaker & {
  colors.emplace_back(attachment, layout);
  return *this;
}

auto RenderPassMaker::SubpassMaker::input(
  const uint32_t &attachment, const vk::ImageLayout &layout) -> SubpassMaker & {
  inputs.emplace_back(attachment, layout);
  return *this;
}

auto RenderPassMaker::SubpassMaker::depthStencil(
  const uint32_t &attachment, const vk::ImageLayout &layout) -> SubpassMaker & {
  depth = {attachment, layout};
  return *this;
}

RenderPassMaker::SubpassMaker &RenderPassMaker::SubpassMaker::resolve(
  const uint32_t &attachment, const vk::ImageLayout &layout) {
  resolves.emplace_back(attachment, layout);
  return *this;
}

RenderPassMaker::SubpassMaker &RenderPassMaker::SubpassMaker::preserve(
  const uint32_t &attachment) {
  preserves.emplace_back(attachment);
  return *this;
}

auto RenderPassMaker::SubpassMaker::index() const -> uint32_t { return _index; }

RenderPassMaker::SubpassMaker::operator vk::SubpassDescription() const {
  return vk::SubpassDescription(
    {}, bindpoint, uint32_t(inputs.size()), inputs.data(), uint32_t(colors.size()),
    colors.data(), resolves.data(), depth.has_value() ? &depth.value() : nullptr,
    uint32_t(preserves.size()), preserves.data());
}

RenderPassMaker::DependencyMaker::DependencyMaker(
  RenderPassMaker &renderPassMaker, uint32_t index)
  : maker{renderPassMaker}, index{index} {}

auto RenderPassMaker::dependency(const uint32_t &srcSubpass, const uint32_t &dstSubpass)
  -> DependencyMaker {
  vk::SubpassDependency desc;
  desc.srcSubpass = srcSubpass;
  desc.dstSubpass = dstSubpass;
  subpassDependencies.push_back(desc);
  return DependencyMaker(*this, uint32_t(subpassDependencies.size() - 1));
}

auto RenderPassMaker::DependencyMaker::srcStageMask(const vk::PipelineStageFlags &value)
  -> DependencyMaker & {
  maker.subpassDependencies[index].srcStageMask = value;
  return *this;
}

auto RenderPassMaker::DependencyMaker::dstStageMask(const vk::PipelineStageFlags &value)
  -> DependencyMaker & {
  maker.subpassDependencies[index].dstStageMask = value;
  return *this;
}

auto RenderPassMaker::DependencyMaker::srcAccessMask(const vk::AccessFlags &value)
  -> DependencyMaker & {
  maker.subpassDependencies[index].srcAccessMask = value;
  return *this;
}

auto RenderPassMaker::DependencyMaker::dstAccessMask(const vk::AccessFlags &value)
  -> DependencyMaker & {
  maker.subpassDependencies[index].dstAccessMask = value;
  return *this;
}

auto RenderPassMaker::DependencyMaker::dependencyFlags(const vk::DependencyFlags &value)
  -> DependencyMaker & {
  maker.subpassDependencies[index].dependencyFlags = value;
  return *this;
}

auto RenderPassMaker::createUnique(const vk::Device &device) const
  -> vk::UniqueRenderPass {
  std::vector<vk::SubpassDescription> subpassDescs;
  for(auto &subpass: subpasses)
    subpassDescs.push_back(vk::SubpassDescription(subpass));

  vk::RenderPassCreateInfo renderPassInfo;
  renderPassInfo.attachmentCount = uint32_t(attachmentDescriptions.size());
  renderPassInfo.pAttachments = attachmentDescriptions.data();
  renderPassInfo.subpassCount = uint32_t(subpassDescs.size());
  renderPassInfo.pSubpasses = subpassDescs.data();
  renderPassInfo.dependencyCount = uint32_t(subpassDependencies.size());
  renderPassInfo.pDependencies = subpassDependencies.data();
  return device.createRenderPassUnique(renderPassInfo);
}
}

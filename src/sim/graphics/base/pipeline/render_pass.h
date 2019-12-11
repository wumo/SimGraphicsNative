#pragma once
#include "sim/graphics/base/vkcommon.h"
#include <optional>

namespace sim::graphics {
class RenderPassMaker {
public:
  class AttachmentMaker {
  public:
    explicit AttachmentMaker(RenderPassMaker &maker, uint32_t index);
    auto flags(const vk::AttachmentDescriptionFlags &value) -> AttachmentMaker &;
    auto format(const vk::Format &value) -> AttachmentMaker &;
    auto samples(const vk::SampleCountFlagBits &value) -> AttachmentMaker &;
    auto loadOp(const vk::AttachmentLoadOp &value) -> AttachmentMaker &;
    auto storeOp(const vk::AttachmentStoreOp &value) -> AttachmentMaker &;
    auto stencilLoadOp(const vk::AttachmentLoadOp &value) -> AttachmentMaker &;
    auto stencilStoreOp(const vk::AttachmentStoreOp &value) -> AttachmentMaker &;
    auto initialLayout(const vk::ImageLayout &value) -> AttachmentMaker &;
    auto finalLayout(const vk::ImageLayout &value) -> AttachmentMaker &;
    auto index() const -> uint32_t;

  private:
    RenderPassMaker &maker;
    uint32_t _index;
  };

  class SubpassMaker {
  public:
    explicit SubpassMaker(vk::PipelineBindPoint bindPoint, uint32_t index);
    auto color(
      const uint32_t &attachment,
      const vk::ImageLayout &layout = vk::ImageLayout::eColorAttachmentOptimal)
      -> SubpassMaker &;
    auto input(
      const uint32_t &attachment,
      const vk::ImageLayout &layout = vk::ImageLayout::eShaderReadOnlyOptimal)
      -> SubpassMaker &;
    auto depthStencil(
      const uint32_t &attachment,
      const vk::ImageLayout &layout = vk::ImageLayout::eDepthStencilAttachmentOptimal)
      -> SubpassMaker &;
    auto resolve(
      const uint32_t &attachment,
      const vk::ImageLayout &layout = vk::ImageLayout::eColorAttachmentOptimal)
      -> SubpassMaker &;
    auto preserve(const uint32_t &attachment) -> SubpassMaker &;
    auto index() const -> uint32_t;
    explicit operator vk::SubpassDescription() const;

  private:
    const vk::PipelineBindPoint bindpoint;
    const uint32_t _index;
    std::vector<vk::AttachmentReference> inputs;
    std::vector<vk::AttachmentReference> colors;
    std::vector<vk::AttachmentReference> resolves;
    std::optional<vk::AttachmentReference> depth;
    std::vector<uint32_t> preserves;
  };

  class DependencyMaker {
  public:
    explicit DependencyMaker(RenderPassMaker &renderPassMaker, uint32_t index);
    auto srcStageMask(const vk::PipelineStageFlags &value) -> DependencyMaker &;
    auto dstStageMask(const vk::PipelineStageFlags &value) -> DependencyMaker &;
    auto srcAccessMask(const vk::AccessFlags &value) -> DependencyMaker &;
    auto dstAccessMask(const vk::AccessFlags &value) -> DependencyMaker &;
    auto dependencyFlags(const vk::DependencyFlags &value) -> DependencyMaker &;

  private:
    RenderPassMaker &maker;
    uint32_t index;
  };

  /**copy the last defined vk::AttachmentDescription or create new.*/
  auto attachment(const vk::Format &format) -> AttachmentMaker;
  auto attachment(const vk::AttachmentDescription &desc) -> AttachmentMaker;

  /**
		 * Start a subpass description.After this you can can call
		 * subpassColorAttachment many times and subpassDepthStencilAttachment once.
		 * @param bindPoint
		 */
  auto subpass(vk::PipelineBindPoint bindPoint) -> SubpassMaker &;

  auto createUnique(const vk::Device &device) const -> vk::UniqueRenderPass;
  auto dependency(const uint32_t &srcSubpass, const uint32_t &dstSubpass)
    -> DependencyMaker;

private:
  std::vector<vk::AttachmentDescription> attachmentDescriptions;
  std::vector<SubpassMaker> subpasses;
  std::vector<vk::SubpassDependency> subpassDependencies;
};
}

#pragma once
#include "sim/graphics/base/vulkan_base.h"
#include "sim/graphics/base/pipeline/descriptors.h"

namespace sim::graphics::renderer::basic {

struct TextureDesc {
  vk::Extent2D extent;
  vk::Format format{vk::Format::eUndefined};
  vk::SampleCountFlagBits samples{vk::SampleCountFlagBits::e1};
  vk::AttachmentLoadOp loadOp{vk::AttachmentLoadOp::eDontCare};
  vk::AttachmentStoreOp storeOp{vk::AttachmentStoreOp::eDontCare};
  vk::AttachmentLoadOp stencilLoadOp{vk::AttachmentLoadOp::eDontCare};
  vk::AttachmentStoreOp stencilStoreOp{vk::AttachmentStoreOp::eDontCare};
  vk::ImageLayout initialLayout{vk::ImageLayout::eUndefined};
  vk::ImageLayout finalLayout{vk::ImageLayout::eUndefined};
};

struct TextureReference {};

struct SubpassReference {
} SubpassExternal;

struct DescriptorSetReference {};

class FrameGraphBuilder {
public:
  FrameGraphBuilder(Swapchain &swapchain);
  sPtr<TextureReference> createTexture(const TextureDesc &desc);

  void writeColor(
    sPtr<TextureReference> attachment,
    const vk::ImageLayout &layout = vk::ImageLayout::eColorAttachmentOptimal);
  void writeDepth(
    sPtr<TextureReference> attachment,
    const vk::ImageLayout &layout = vk::ImageLayout::eDepthStencilAttachmentOptimal);
  void readInput(
    sPtr<TextureReference> attachment,
    const vk::ImageLayout &layout = vk::ImageLayout::eShaderReadOnlyOptimal);
  void writeResolve(
    sPtr<TextureReference> attachment,
    const vk::ImageLayout &layout = vk::ImageLayout::eColorAttachmentOptimal);
  void preserve(sPtr<TextureReference> attachment);

  void dependency(
    SubpassReference pass, const vk::PipelineStageFlags &srcStageMask,
    const vk::PipelineStageFlags &dstStageMask, const vk::AccessFlags &srcAccessMask,
    const vk::AccessFlags &dstAccessMask,
    const vk::DependencyFlags &flags = vk::DependencyFlagBits::eByRegion);

  sPtr<DescriptorSetReference> createDescriptorSet(DescriptorSetDef &setDef);

  vk::Format getSwapchainFormat();

  Swapchain &swapchain() const;

  vk::SampleCountFlagBits sampleCount();

private:
  Swapchain &swapchain_;
  vk::SampleCountFlagBits sampleCount_{vk::SampleCountFlagBits ::e1};
};
}
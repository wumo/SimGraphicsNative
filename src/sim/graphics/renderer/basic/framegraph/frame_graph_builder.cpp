#include "frame_graph_builder.h"
namespace sim::graphics::renderer::basic {
FrameGraphBuilder::FrameGraphBuilder(Swapchain &swapchain): swapchain_(swapchain) {}
Swapchain &FrameGraphBuilder::swapchain() const { return swapchain_; }
vk::SampleCountFlagBits FrameGraphBuilder::sampleCount() { return sampleCount_; }

sPtr<TextureReference> FrameGraphBuilder::createTexture(const TextureDesc &desc) {}

void FrameGraphBuilder::writeColor(
  sPtr<TextureReference> attachment, const vk::ImageLayout &layout) {}
void FrameGraphBuilder::writeDepth(
  sPtr<TextureReference> attachment, const vk::ImageLayout &layout) {}
void FrameGraphBuilder::readInput(
  sPtr<TextureReference> attachment, const vk::ImageLayout &layout) {}
void FrameGraphBuilder::writeResolve(
  sPtr<TextureReference> attachment, const vk::ImageLayout &layout) {}
void FrameGraphBuilder::preserve(sPtr<TextureReference> attachment) {}
void FrameGraphBuilder::dependency(
  SubpassReference pass, const vk::PipelineStageFlags &srcStageMask,
  const vk::PipelineStageFlags &dstStageMask, const vk::AccessFlags &srcAccessMask,
  const vk::AccessFlags &dstAccessMask, const vk::DependencyFlags &flags) {}

sPtr<DescriptorSetReference> FrameGraphBuilder::createDescriptorSet(
  DescriptorSetDef &setDef) {}
}
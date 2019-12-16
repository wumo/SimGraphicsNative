#include "sim/graphics/renderer/default/model/default_model_manager.h"
#include "deferred.h"

namespace sim::graphics::renderer::defaultRenderer {
using namespace std::string_literals;
using namespace glm;
using layout = vk::ImageLayout;
using stage = vk::PipelineStageFlagBits;
using access = vk::AccessFlagBits;
using bindpoint = vk::PipelineBindPoint;
using descriptor = vk::DescriptorType;

void Deferred::updateModels() {
  if(config.enableImageBasedLighting) { mm->generateIBLTextures(); }

  mm->updateDescriptorSet(gBufferDef.texture);
  gBufferDef.update(gBufferSet);
  mm->updateInstances();
  mm->updateTransformsAndMaterials();

  mm->printSceneInfo();
}

void Deferred::render(
  std::function<void(uint32_t, float)> &updater, uint32_t imageIndex,
  float elapsedDuration) {

  auto &cb = graphicsCmdBuffers[imageIndex];
  updater(imageIndex, elapsedDuration);

  //  mm->updateInstances();
  mm->uploadPixels(cb);
  mm->updateTransformsAndMaterials();

  std::array<vk::ClearValue, 9> clearValues{
    vk::ClearColorValue{std::array{0.0f, 0.0f, 0.0f, 0.0f}},
    vk::ClearColorValue{std::array{0.0f, 0.0f, 0.0f, 0.0f}},
    vk::ClearColorValue{std::array{0.0f, 0.0f, 0.0f, 0.0f}},
    vk::ClearColorValue{std::array{0.0f, 0.0f, 0.0f, 0.0f}},
    vk::ClearColorValue{std::array{0.0f, 0.0f, 0.0f, 0.0f}},
    vk::ClearColorValue{std::array{0.0f, 0.0f, 0.0f, 0.0f}},
    vk::ClearDepthStencilValue{1.0f, 0},
    vk::ClearColorValue{std::array{0.0f, 0.0f, 0.0f, 0.0f}},
    vk::ClearColorValue{std::array{0.0f, 0.0f, 0.0f, 0.0f}}};
  vk::RenderPassBeginInfo renderPassBeginInfo{
    *renderPass, *framebuffers[imageIndex],
    vk::Rect2D{{0, 0}, {extent.width, extent.height}}, uint32_t(clearValues.size()),
    clearValues.data()};
  cb.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
  vk::Viewport viewport{0, 0, float(extent.width), float(extent.height), 0.0f, 1.0f};
  cb.setViewport(0, viewport);
  vk::Rect2D scissor{{0, 0}, {extent.width, extent.height}};
  cb.setScissor(0, scissor);

  const vk::DeviceSize zeroOffset{0};
  cb.bindVertexBuffers(0, mm->Buffer.vertices->buffer(), zeroOffset);
  cb.bindIndexBuffer(mm->Buffer.indices->buffer(), zeroOffset, vk::IndexType::eUint32);
  cb.bindDescriptorSets(
    bindpoint::eGraphics, *gBuffer.pipelineLayout, 0, gBufferSet, nullptr);

  debugMarker.begin(cb, "Subpass gBuffer creation");
  cb.bindPipeline(
    bindpoint::eGraphics,
    config.displayWireFrame ? *gBufferWireframe.pipeline : *gBuffer.pipeline);
  cb.drawIndexedIndirect(
    mm->drawCMDs->buffer(), mm->tri.offset * sizeof(vk::DrawIndexedIndirectCommand),
    mm->tri.size, sizeof(vk::DrawIndexedIndirectCommand));

  cb.bindPipeline(bindpoint::eGraphics, *debugLines.pipeline);
  cb.drawIndexedIndirect(
    mm->drawCMDs->buffer(), mm->debugLine.offset * sizeof(vk::DrawIndexedIndirectCommand),
    mm->debugLine.size, sizeof(vk::DrawIndexedIndirectCommand));
  debugMarker.end(cb);

  debugMarker.begin(cb, "Subpass shading ");
  cb.nextSubpass(vk::SubpassContents::eInline);
  cb.bindPipeline(bindpoint::eGraphics, *shading.pipeline);
  cb.bindDescriptorSets(
    bindpoint::eGraphics, *shading.pipelineLayout, 0, deferredSet, nullptr);
  if(config.enableImageBasedLighting)
    cb.bindDescriptorSets(
      bindpoint::eGraphics, *shading.pipelineLayout, 1, iblSet, nullptr);
  cb.draw(3, 1, 0, 0);
  debugMarker.end(cb);

  debugMarker.begin(cb, "Subpass translucent draw");
  cb.nextSubpass(vk::SubpassContents::eInline);

  cb.bindPipeline(bindpoint::eGraphics, *translucent.pipeline);
  cb.bindDescriptorSets(
    bindpoint::eGraphics, *gBuffer.pipelineLayout, 0, gBufferSet, nullptr);

  cb.drawIndexedIndirect(
    mm->drawCMDs->buffer(),
    mm->translucent.offset * sizeof(vk::DrawIndexedIndirectCommand), mm->translucent.size,
    sizeof(vk::DrawIndexedIndirectCommand));
  debugMarker.end(cb);
  cb.endRenderPass();

  //  if(config.gui) gui->drawGui(imageIndex, cb);
}
}

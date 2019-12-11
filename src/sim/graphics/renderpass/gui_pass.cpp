#include <sim/graphics/base/device.h>
#include <sim/graphics/pipeline/render_pass.h>
#include <sim/graphics/base/swapchain.h>
#include <imgui.h>
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "gui_pass.h"

namespace sim::graphics {
using layout = vk::ImageLayout;
using stage = vk::PipelineStageFlagBits;
using access = vk::AccessFlagBits;
using bindpoint = vk::PipelineBindPoint;

GuiPass::GuiPass(
  GLFWwindow *window, vk::Instance &vkInstance, Device &device, Swapchain &swapchain,
  vk::DescriptorPool &descriptorPool)
  : window{window},
    vkInstance{vkInstance},
    device{device},
    swapchain{swapchain},
    descriptorPool{descriptorPool} {
  createGuiRenderPass();
  createGuiPipeline();
}

void GuiPass::createGuiRenderPass() {
  RenderPassMaker maker;
  auto color = maker.attachment(swapchain.getImageFormat())
                 .samples(vk::SampleCountFlagBits::e1)
                 .loadOp(vk::AttachmentLoadOp::eDontCare)
                 .storeOp(vk::AttachmentStoreOp::eStore)
                 .stencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                 .stencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                 .initialLayout(layout::eUndefined)
                 .finalLayout(layout::eColorAttachmentOptimal)
                 .index();

  gui.subpass = maker.subpass(bindpoint::eGraphics).color(color).index();

  // A dependency to reset the layout of both attachments.
  maker.dependency(VK_SUBPASS_EXTERNAL, gui.subpass)
    .srcStageMask(stage::eBottomOfPipe)
    .dstStageMask(stage::eColorAttachmentOutput)
    .srcAccessMask(access::eMemoryRead)
    .dstAccessMask(access::eColorAttachmentWrite)
    .dependencyFlags(vk::DependencyFlagBits::eByRegion);

  maker.dependency(gui.subpass, VK_SUBPASS_EXTERNAL)
    .srcStageMask(stage::eColorAttachmentOutput)
    .dstStageMask(stage::eBottomOfPipe)
    .srcAccessMask(access::eColorAttachmentWrite)
    .dstAccessMask(access::eMemoryRead)
    .dependencyFlags(vk::DependencyFlagBits::eByRegion);

  gui.renderPass = maker.createUnique(device.getDevice());
}

void GuiPass::createGuiPipeline() {

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  auto &io = ImGui::GetIO();
  (void)io;
  gui.pipelineCache = device.getDevice().createPipelineCacheUnique({});
  ImGui_ImplVulkan_InitInfo initInfo{vkInstance,
                                     device.getPhysicalDevice(),
                                     device.getDevice(),
                                     device.getGraphics().index,
                                     device.getGraphics().queue,
                                     *gui.pipelineCache,
                                     descriptorPool,
                                     swapchain.getImageCount(),
                                     swapchain.getImageCount()};

  ImGui_ImplGlfw_InitForVulkan(window, true);
  ImGui_ImplVulkan_Init(&initInfo, *gui.renderPass);
  ImGui::StyleColorsDark();

  device.executeImmediately(
    [&](vk::CommandBuffer cb) { ImGui_ImplVulkan_CreateFontsTexture(cb); });
}

void GuiPass::resizeGui(vk::Extent2D extent) {
  gui.extent = extent;
  std::vector<vk::ImageView> _attachments = {{}};
  vk::FramebufferCreateInfo info{{},
                                 *gui.renderPass,
                                 uint32_t(_attachments.size()),
                                 _attachments.data(),
                                 extent.width,
                                 extent.height,
                                 1};

  gui.framebuffers.resize(swapchain.getImageCount());
  for(auto i = 0u; i < swapchain.getImageCount(); i++) {
    _attachments.back() = *swapchain.getImageViews()[i];
    gui.framebuffers[i] = device.getDevice().createFramebufferUnique(info);
  }
}

void GuiPass::drawGui(uint32_t imageIndex, vk::CommandBuffer &cb) {

  std::array<vk::ClearValue, 1> clearValues{
    vk::ClearColorValue{std::array{0.0f, 0.0f, 0.0f, 1.0f}}};

  vk::RenderPassBeginInfo renderPassBeginInfo{
    *gui.renderPass, *gui.framebuffers[imageIndex],
    vk::Rect2D{{0, 0}, {gui.extent.width, gui.extent.height}},
    uint32_t(clearValues.size()), clearValues.data()};
  cb.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  bool show_demo_window{true};
  ImGui::ShowDemoWindow(&show_demo_window);

  ImGui::Render();

  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cb);

  cb.endRenderPass();
}

GuiPass::~GuiPass() {
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}
}
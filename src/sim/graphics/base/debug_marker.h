#pragma once
#include "vkcommon.h"

namespace sim::graphics {
class DebugMarker {
public:
  DebugMarker() = default;

  DebugMarker(const vk::Device &device): device{&device} {}

  void name(vk::Buffer object, const char *markerName) {
    name(uint64_t(VkBuffer(object)), vk::DebugReportObjectTypeEXT::eBuffer, markerName);
  }

  void name(vk::Image object, const char *markerName) {
    name(uint64_t(VkImage(object)), vk::DebugReportObjectTypeEXT::eImage, markerName);
  }

  void name(vk::Pipeline object, const char *markerName) {
    name(
      uint64_t(VkPipeline(object)), vk::DebugReportObjectTypeEXT::ePipeline, markerName);
  }

  void name(vk::DescriptorSet object, const char *markerName) const {
    name(
      uint64_t(VkDescriptorSet(object)), vk::DebugReportObjectTypeEXT::eDescriptorSet,
      markerName);
  }

  void name(
    uint64_t object, vk::DebugReportObjectTypeEXT objectType,
    const char *markerName) const {
    if(VULKAN_HPP_DEFAULT_DISPATCHER.vkDebugMarkerSetObjectNameEXT) {
      vk::DebugMarkerObjectNameInfoEXT nameInfo;
      nameInfo.object = object;
      nameInfo.objectType = objectType;
      nameInfo.pObjectName = markerName;

      VULKAN_HPP_DEFAULT_DISPATCHER.vkDebugMarkerSetObjectNameEXT(
        *device, reinterpret_cast<const VkDebugMarkerObjectNameInfoEXT *>(&nameInfo));
    }
  }

  void tag(
    uint64_t object, vk::DebugReportObjectTypeEXT objectType, uint64_t name,
    size_t tagSize, const void *tag) const {
    if(VULKAN_HPP_DEFAULT_DISPATCHER.vkDebugMarkerSetObjectTagEXT) {
      vk::DebugMarkerObjectTagInfoEXT tagInfo;
      tagInfo.object = object;
      tagInfo.objectType = objectType;
      tagInfo.tagName = name;
      tagInfo.tagSize = tagSize;
      tagInfo.pTag = tag;
      VULKAN_HPP_DEFAULT_DISPATCHER.vkDebugMarkerSetObjectTagEXT(
        *device, reinterpret_cast<const VkDebugMarkerObjectTagInfoEXT *>(&tagInfo));
    }
  }

  void begin(
    vk::CommandBuffer commandBuffer, const char *markerName,
    glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f}) const {
    if(VULKAN_HPP_DEFAULT_DISPATCHER.vkCmdDebugMarkerBeginEXT) {
      vk::DebugMarkerMarkerInfoEXT markerInfo;
      memcpy(markerInfo.color, &color[0], sizeof(float) * 4);
      markerInfo.pMarkerName = markerName;
      VULKAN_HPP_DEFAULT_DISPATCHER.vkCmdDebugMarkerBeginEXT(
        commandBuffer, reinterpret_cast<const VkDebugMarkerMarkerInfoEXT *>(&markerInfo));
    }
  }

  void insert(
    vk::CommandBuffer commandBuffer, const char *markerName, glm::vec4 color) const {
    if(VULKAN_HPP_DEFAULT_DISPATCHER.vkCmdDebugMarkerInsertEXT) {
      vk::DebugMarkerMarkerInfoEXT markerInfo;
      memcpy(markerInfo.color, &color[0], sizeof(float) * 4);
      markerInfo.pMarkerName = markerName;
      VULKAN_HPP_DEFAULT_DISPATCHER.vkCmdDebugMarkerInsertEXT(
        commandBuffer, reinterpret_cast<const VkDebugMarkerMarkerInfoEXT *>(&markerInfo));
    }
  }

  void end(vk::CommandBuffer commandBuffer) const {
    if(VULKAN_HPP_DEFAULT_DISPATCHER.vkCmdDebugMarkerEndEXT)
      VULKAN_HPP_DEFAULT_DISPATCHER.vkCmdDebugMarkerEndEXT(commandBuffer);
  }

private:
  const vk::Device *device;
};
}

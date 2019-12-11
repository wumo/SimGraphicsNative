#include "raytracing_pipeline.h"

namespace sim::graphics::renderer::defaultRenderer {
using shader = vk::ShaderStageFlagBits;

RayTracingPipelineMaker::RayTracingPipelineMaker(Device &vkContext)
  : device{vkContext.getDevice()}, allocator{vkContext.allocator()} {}

RayTracingPipelineMaker &RayTracingPipelineMaker::rayGenGroup(
  const std::string &filename, const vk::SpecializationInfo *pSpecializationInfo) {
  errorIf(
    !shaders.empty() || hitGroupBegun, "ray gen shader should be the only first shader.");
  addShaderAndGroup(shader::eRaygenNV, {device, filename}, pSpecializationInfo);
  return *this;
}
RayTracingPipelineMaker &RayTracingPipelineMaker::rayGenGroup(
  const uint32_t *opcodes, size_t size,
  const vk::SpecializationInfo *pSpecializationInfo) {
  errorIf(
    !shaders.empty() || hitGroupBegun, "ray gen shader should be the only first shader.");
  addShaderAndGroup(shader::eRaygenNV, {device, opcodes, size}, pSpecializationInfo);
  return *this;
}

RayTracingPipelineMaker &RayTracingPipelineMaker::missGroup(
  const std::string &filename, const vk::SpecializationInfo *pSpecializationInfo) {
  errorIf(hitGroupBegun, "miss shader should be before hitgroup shaders");
  addShaderAndGroup(shader::eMissNV, {device, filename}, pSpecializationInfo);
  numMissGroups++;
  return *this;
}
RayTracingPipelineMaker &RayTracingPipelineMaker::missGroup(
  const uint32_t *opcodes, size_t size,
  const vk::SpecializationInfo *pSpecializationInfo) {
  errorIf(hitGroupBegun, "miss shader should be before hitgroup shaders");
  addShaderAndGroup(shader::eMissNV, {device, opcodes, size}, pSpecializationInfo);
  numMissGroups++;
  return *this;
}

void RayTracingPipelineMaker::addShaderAndGroup(
  const vk::ShaderStageFlagBits &stage, ShaderModule &&shader,
  const vk::SpecializationInfo *pSpecializationInfo) {
  modules.emplace_back(std::move(shader));
  auto shaderIndex = add(stage, modules.back().module(), pSpecializationInfo);
  vk::RayTracingShaderGroupCreateInfoNV groupInfo{
    vk::RayTracingShaderGroupTypeNV::eGeneral, shaderIndex, VK_SHADER_UNUSED_NV,
    VK_SHADER_UNUSED_NV, VK_SHADER_UNUSED_NV};
  groups.emplace_back(groupInfo);
}
RayTracingPipelineMaker &RayTracingPipelineMaker::hitGroup(ShaderGroup group) {
  auto closestHit = group.closestHit ?
                      add(
                        shader::eClosestHitNV, group.closestHit->module(),
                        group.closestHit->specialization()) :
                      VK_SHADER_UNUSED_NV;
  auto anyHit =
    group.anyHit ?
      add(shader::eAnyHitNV, group.anyHit->module(), group.anyHit->specialization()) :
      VK_SHADER_UNUSED_NV;
  auto intersection = group.intersection ?
                        add(
                          shader::eIntersectionNV, group.intersection->module(),
                          group.intersection->specialization()) :
                        VK_SHADER_UNUSED_NV;
  vk::RayTracingShaderGroupCreateInfoNV groupInfo{
    group.intersection ? vk::RayTracingShaderGroupTypeNV::eProceduralHitGroup :
                         vk::RayTracingShaderGroupTypeNV::eTrianglesHitGroup,
    VK_SHADER_UNUSED_NV, closestHit, anyHit, intersection};
  groups.emplace_back(groupInfo);
  return *this;
}

uint32_t RayTracingPipelineMaker::add(
  const vk::ShaderStageFlagBits &stageFlagBits, vk::ShaderModule &shader,
  const vk::SpecializationInfo *pSpecializationInfo) {
  vk::PipelineShaderStageCreateInfo info{
    {}, stageFlagBits, shader, "main", pSpecializationInfo};
  shaders.emplace_back(info);
  return uint32_t(shaders.size() - 1);
}

RayTracingPipelineMaker &RayTracingPipelineMaker::maxRecursionDepth(
  uint32_t maxRecursionDepth) {
  this->_maxRecursionDepth = maxRecursionDepth;
  return *this;
}

std::tuple<UniqueRayTracingPipeline, ShaderBindingTable>
RayTracingPipelineMaker::createUnique(
  const vk::PhysicalDeviceRayTracingPropertiesNV &rayTracingProperties,
  const vk::PipelineLayout &pipelineLayout, const vk::PipelineCache &pipelineCache) {
  vk::RayTracingPipelineCreateInfoNV info{{},
                                          uint32_t(shaders.size()),
                                          shaders.data(),
                                          uint32_t(groups.size()),
                                          groups.data(),
                                          _maxRecursionDepth,
                                          pipelineLayout};
  auto pipeline = device.createRayTracingPipelineNVUnique(pipelineCache, info, nullptr);
  auto shaderGroupHandleSize = rayTracingProperties.shaderGroupHandleSize;
  auto sbtSize = shaderGroupHandleSize * groups.size();
  auto sbtBuffer = u<HostRayTracingBuffer>(allocator, sbtSize);
  device.getRayTracingShaderGroupHandlesNV(
    *pipeline, 0, uint32_t(groups.size()), sbtSize, sbtBuffer->ptr());
  auto bindStride = shaderGroupHandleSize;
  ShaderBindingTable sbt{std::move(sbtBuffer),
                         0,
                         1 * shaderGroupHandleSize,
                         bindStride,
                         (1 + numMissGroups) * shaderGroupHandleSize,
                         bindStride};
  return std::make_tuple(std::move(pipeline), std::move(sbt));
}

}
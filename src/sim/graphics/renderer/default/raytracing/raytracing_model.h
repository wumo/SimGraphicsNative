#pragma once
#include "sim/graphics/base/pipeline/shader.h"
#include "sim/graphics/renderer/default/model/default_model_manager.h"
#include <cstring>

namespace sim::graphics::renderer::defaultRenderer {
class RayTracingModel: public DefaultModelManager {
  friend class RayTracing;
  using UniqueAccelerationStructureNV =
    vk::UniqueHandle<vk::AccelerationStructureNV, vk::DispatchLoaderDynamic>;
  struct AccelerationStructure {
    uPtr<DeviceRayTracingBuffer> buffer;
    vk::AccelerationStructureInfoNV info;
    UniqueAccelerationStructureNV as;
    uint64_t handle;

    uPtr<DeviceRayTracingBuffer> scratchBuffer;
  };

  struct VkGeometryInstance {
    glm::mat3x4 transform{};
    uint32_t instanceCustomIndex : 24;
    uint32_t mask : 8;
    uint32_t instanceOffset : 24;
    uint32_t flags : 8;
    uint64_t accelerationStructureHandle;
    VkGeometryInstance() = default;
    VkGeometryInstance(
      glm::mat4 transform, uint32_t instanceCustomIndex, uint32_t mask,
      uint32_t instanceOffset, const vk::GeometryInstanceFlagsNV &flag,
      uint64_t accelerationStructureHandle)
      : instanceCustomIndex{instanceCustomIndex},
        mask{mask},
        instanceOffset{instanceOffset},
        flags{static_cast<uint32_t>(flag)},
        accelerationStructureHandle{accelerationStructureHandle} {
      auto t = glm::transpose(transform);
      void *src = &t; // vs 2019 bug.
      memcpy(&this->transform, src, sizeof(this->transform));
      //    this->transform = {t[0], t[1], t[2]};
    }
  };

  struct BottomLevelAccelerationStructure: AccelerationStructure {
    std::vector<vk::GeometryNV> geometries;
  };

  struct TopLevelAccelerationStructure: AccelerationStructure {
    std::vector<VkGeometryInstance> instances;
    uPtr<HostRayTracingBuffer> instanceBuffer;
  };

public:
  explicit RayTracingModel(Device &context, const ModelConfig &config = {});

private:
  void createAS();
  void make_AS(
    AccelerationStructure &outAS, vk::AccelerationStructureTypeNV type,
    uint32_t instanceCount, uint32_t geometryCount, const vk::GeometryNV *geometries);
  void updateRTGeometry();
  void updateInstances() override;
  void buildAS();
  void updateAS(vk::CommandBuffer cb);

  vk::MemoryRequirements2 getASMemReq(
    vk::AccelerationStructureNV &,
    vk::AccelerationStructureMemoryRequirementsTypeNV) const;

  const bool mergeMeshes{false}; //buggy, disabled by default.
  const bool rebuildBlas{false};
  const vk::Device &vkDevice;

  std::vector<uint32_t> meshBlas;
  std::vector<uint32_t> modelBlas;
  std::vector<BottomLevelAccelerationStructure> blases;
  TopLevelAccelerationStructure tlas;

  uPtr<HostIndirectBuffer> drawCMDs_buffer;
  uint32_t sizeDebugTri{0}, sizeDebugLine{0};
};
}

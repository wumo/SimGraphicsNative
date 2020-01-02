#pragma once
#include "sim/graphics/base/vkcommon.h"
#include "sim/graphics/base/device.h"

namespace sim::graphics {
class BufferBase {
public:
  __only_move__(BufferBase);
  BufferBase(
    const VmaAllocator &allocator, const vk::BufferUsageFlags &usageFlags,
    vk::DeviceSize sizeInBytes, VmaMemoryUsage memoryUsage);

  BufferBase(
    const VmaAllocator &allocator, const vk::BufferUsageFlags &usageFlags,
    vk::DeviceSize sizeInBytes, VmaAllocationCreateInfo allocInfo);

  void barrier(
    const vk::CommandBuffer &cb, vk::PipelineStageFlags srcStageMask,
    vk::PipelineStageFlags dstStageMask, vk::DependencyFlags dependencyFlags,
    vk::AccessFlags srcAccessMask, vk::AccessFlags dstAccessMask,
    uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex);

  const vk::Buffer &buffer() const;

  auto deviceMem() const {
    return std::make_tuple(allocationInfo.deviceMemory, allocationInfo.offset);
  }

protected:
  struct VmaBuffer {
    const VmaAllocator &allocator;
    VmaAllocation allocation{VK_NULL_HANDLE};
    vk::Buffer buffer{nullptr};
  };

  using UniquePtr = std::unique_ptr<VmaBuffer, std::function<void(VmaBuffer *)>>;
  UniquePtr vmaBuffer;

  VmaAllocationInfo allocationInfo{};
  bool mappable{false};
};

class DeviceBuffer: public BufferBase {
public:
  DeviceBuffer(
    const VmaAllocator &allocator, vk::BufferUsageFlags usageFlags, vk::DeviceSize size)
    : BufferBase{allocator, usageFlags, size, VMA_MEMORY_USAGE_GPU_ONLY} {}

  /**
		 * For a device local buffer, copy memory to the buffer object immediately.
		 * Note that this will stall the pipeline!
		 */
  void upload(
    Device &device, const void *value, vk::DeviceSize sizeInBytes,
    vk::DeviceSize dstOffsetInBytes = 0);

  template<class Type, class Allocator>
  void upload(
    Device &device, const std::vector<Type, Allocator> &value,
    vk::DeviceSize dstOffsetInBytes = 0) {
    upload(device, value.data(), value.size() * sizeof(Type), dstOffsetInBytes);
  }

  template<typename T>
  void upload(Device &device, const T &value, vk::DeviceSize dstOffsetInBytes = 0) {
    upload(device, &value, sizeof(value), dstOffsetInBytes);
  }
};

class HostCoherentBuffer: public BufferBase {
public:
  HostCoherentBuffer(
    const VmaAllocator &allocator, vk::BufferUsageFlags usageFlags, vk::DeviceSize size,
    VmaMemoryUsage memoryUsage)
    : BufferBase{allocator,
                 usageFlags,
                 size,
                 {VMA_ALLOCATION_CREATE_MAPPED_BIT, memoryUsage,
                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT}} {}

  void updateRaw(
    const void *value, vk::DeviceSize sizeInBytes, vk::DeviceSize dstOffsetInBytes = 0);

  template<class Type, class Allocator>
  void updateVector(
    const std::vector<Type, Allocator> &value, uint32_t dstOffsetInBytes = 0,
    uint32_t srcSize = 0, uint32_t srcOffsetInBytes = 0) {
    updateRaw(
      (void *)(value.data() + srcOffsetInBytes),
      vk::DeviceSize((srcSize ? srcSize : value.size()) * sizeof(Type)),
      dstOffsetInBytes);
  }

  template<class Type>
  void updateSingle(const Type &value, uint32_t offsetInBytes = 0) {
    updateRaw((void *)&value, vk::DeviceSize(sizeof(Type)), offsetInBytes);
  }

  template<class T>
  T *map() {
    void *mapped;
    vmaMapMemory(vmaBuffer->allocator, vmaBuffer->allocation, &mapped);
    return static_cast<T *>(mapped);
  }

  void unmap() { vmaUnmapMemory(vmaBuffer->allocator, vmaBuffer->allocation); }

  void flush();
  void invalidate();

  template<class T = void>
  T *ptr() {
    return static_cast<T *>(allocationInfo.pMappedData);
  }
};

class UploadBuffer: public HostCoherentBuffer {
public:
  UploadBuffer(
    const VmaAllocator &allocator, vk::BufferUsageFlags usageFlags, vk::DeviceSize size)
    : HostCoherentBuffer{allocator, usageFlags, size, VMA_MEMORY_USAGE_CPU_ONLY} {}

  UploadBuffer(const VmaAllocator &allocator, vk::DeviceSize size)
    : UploadBuffer{allocator, vk::BufferUsageFlagBits::eTransferSrc, size} {}
};

class ReadBackBuffer: public HostCoherentBuffer {
public:
  ReadBackBuffer(
    const VmaAllocator &allocator, vk::BufferUsageFlags usageFlags, vk::DeviceSize size)
    : HostCoherentBuffer{allocator, usageFlags, size, VMA_MEMORY_USAGE_GPU_TO_CPU} {}
};

class HostBuffer: public HostCoherentBuffer {
public:
  HostBuffer(
    const VmaAllocator &allocator, vk::BufferUsageFlags usageFlags, vk::DeviceSize size)
    : HostCoherentBuffer{allocator, usageFlags, size, VMA_MEMORY_USAGE_CPU_TO_GPU} {}
};

class IndexBuffer: public DeviceBuffer {
public:
  IndexBuffer(const VmaAllocator &allocator, vk::DeviceSize size)
    : DeviceBuffer{allocator,
                   vk::BufferUsageFlagBits::eIndexBuffer |
                     vk::BufferUsageFlagBits::eTransferDst |
                     vk::BufferUsageFlagBits::eStorageBuffer,
                   size} {}

  template<class Type, class Allocator>
  IndexBuffer(Device &device, const std::vector<Type, Allocator> &value)
    : IndexBuffer{device.allocator(), value.size() * sizeof(Type)} {
    upload(device, value);
  }
};

class HostIndexBuffer: public HostBuffer {
public:
  HostIndexBuffer(const VmaAllocator &allocator, vk::DeviceSize size)
    : HostBuffer{allocator, vk::BufferUsageFlagBits::eIndexBuffer, size} {}

  template<class Type, class Allocator>
  HostIndexBuffer(
    const VmaAllocator &allocator, const std::vector<Type, Allocator> &value)
    : HostIndexBuffer{allocator, value.size() * sizeof(Type)} {
    updateVector(value);
  }
};

class IndexStorageBuffer: public DeviceBuffer {
public:
  IndexStorageBuffer(const VmaAllocator &allocator, vk::DeviceSize size)
    : DeviceBuffer{allocator,
                   vk::BufferUsageFlagBits::eIndexBuffer |
                     vk::BufferUsageFlagBits::eStorageBuffer |
                     vk::BufferUsageFlagBits::eTransferDst,
                   size} {}

  template<class Type, class Allocator>
  IndexStorageBuffer(Device &device, const std::vector<Type, Allocator> &value)
    : IndexStorageBuffer{device.allocator(), value.size() * sizeof(Type)} {
    upload(device, value);
  }
};

class HostIndexStorageBuffer: public HostBuffer {
public:
  HostIndexStorageBuffer(const VmaAllocator &allocator, vk::DeviceSize size)
    : HostBuffer{
        allocator,
        vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eStorageBuffer,
        size} {}

  template<class Type, class Allocator>
  HostIndexStorageBuffer(
    const VmaAllocator &allocator, const std::vector<Type, Allocator> &value)
    : HostIndexStorageBuffer{allocator, value.size() * sizeof(Type)} {
    updateVector(value);
  }
};

class VertexBuffer: public DeviceBuffer {
public:
  VertexBuffer(const VmaAllocator &allocator, vk::DeviceSize size)
    : DeviceBuffer{allocator,
                   vk::BufferUsageFlagBits::eVertexBuffer |
                     vk::BufferUsageFlagBits::eTransferDst |
                     vk::BufferUsageFlagBits::eStorageBuffer,
                   size} {}

  template<class Type, class Allocator>
  VertexBuffer(Device &device, const std::vector<Type, Allocator> &value)
    : VertexBuffer{device.allocator(), value.size() * sizeof(Type)} {
    upload(device, value);
  }
};

class HostVertexBuffer: public HostBuffer {
public:
  HostVertexBuffer(const VmaAllocator &allocator, vk::DeviceSize size)
    : HostBuffer{allocator, vk::BufferUsageFlagBits::eVertexBuffer, size} {}

  template<class Type, class Allocator>
  HostVertexBuffer(
    const VmaAllocator &allocator, const std::vector<Type, Allocator> &value)
    : HostVertexBuffer{allocator, value.size() * sizeof(Type)} {
    updateVector(value);
  }
};

class VertexStorageBuffer: public DeviceBuffer {
public:
  VertexStorageBuffer(const VmaAllocator &allocator, vk::DeviceSize size)
    : DeviceBuffer{allocator,
                   vk::BufferUsageFlagBits::eVertexBuffer |
                     vk::BufferUsageFlagBits::eStorageBuffer |
                     vk::BufferUsageFlagBits::eTransferDst,
                   size} {}

  template<class Type, class Allocator>
  VertexStorageBuffer(Device &device, const std::vector<Type, Allocator> &value)
    : VertexStorageBuffer{device.allocator(), value.size() * sizeof(Type)} {
    upload(device, value);
  }
};

class HostVertexStorageBuffer: public HostBuffer {
public:
  HostVertexStorageBuffer(const VmaAllocator &allocator, vk::DeviceSize size)
    : HostBuffer{
        allocator,
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer,
        size} {}

  template<class Type, class Allocator>
  HostVertexStorageBuffer(
    const VmaAllocator &allocator, const std::vector<Type, Allocator> &value)
    : HostVertexStorageBuffer{allocator, value.size() * sizeof(Type)} {
    updateVector(value);
  }
};

class UniformBuffer: public DeviceBuffer {
public:
  UniformBuffer(const VmaAllocator &allocator, vk::DeviceSize size)
    : DeviceBuffer{
        allocator,
        vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst,
        size} {}
};

class HostUniformBuffer: public HostBuffer {
public:
  HostUniformBuffer(const VmaAllocator &allocator, vk::DeviceSize size)
    : HostBuffer{allocator, vk::BufferUsageFlagBits::eUniformBuffer, size} {}

  template<class T>
  HostUniformBuffer(const VmaAllocator &allocator, const T &value)
    : HostUniformBuffer{allocator, sizeof(T)} {
    updateSingle(value);
  }

  template<class Type, class Allocator>
  HostUniformBuffer(
    const VmaAllocator &allocator, const std::vector<Type, Allocator> &value)
    : HostUniformBuffer{allocator, value.size() * sizeof(Type)} {
    updateVector(value);
  }
};

class StorageBuffer: public DeviceBuffer {
public:
  StorageBuffer(const VmaAllocator &allocator, vk::DeviceSize size)
    : DeviceBuffer{
        allocator,
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
        size} {}

  template<class Type, class Allocator>
  StorageBuffer(Device &device, const std::vector<Type, Allocator> &value)
    : StorageBuffer{device.allocator(), value.size() * sizeof(Type)} {
    upload(device, value);
  }
};

class HostStorageBuffer: public HostBuffer {
public:
  HostStorageBuffer(const VmaAllocator &allocator, vk::DeviceSize size)
    : HostBuffer{allocator, vk::BufferUsageFlagBits::eStorageBuffer, size} {}

  template<class Type>
  HostStorageBuffer(const VmaAllocator &allocator, const Type &value)
    : HostStorageBuffer{allocator, sizeof(Type)} {
    updateSingle(value);
  }

  template<class Type, class Allocator>
  HostStorageBuffer(
    const VmaAllocator &allocator, const std::vector<Type, Allocator> &value)
    : HostStorageBuffer{allocator, value.size() * sizeof(Type)} {
    updateVector(value);
  }
};

class IndirectBuffer: public DeviceBuffer {
public:
  IndirectBuffer(const VmaAllocator &allocator, vk::DeviceSize size)
    : DeviceBuffer{
        allocator,
        vk::BufferUsageFlagBits::eIndirectBuffer | vk::BufferUsageFlagBits::eTransferDst,
        size} {}

  template<class Type, class Allocator>
  IndirectBuffer(Device &device, const std::vector<Type, Allocator> &value)
    : IndirectBuffer{device.allocator(), value.size() * sizeof(Type)} {
    upload(device, value);
  }
};

class HostIndirectBuffer: public HostBuffer {
public:
  HostIndirectBuffer(const VmaAllocator &allocator, vk::DeviceSize size)
    : HostBuffer{allocator, vk::BufferUsageFlagBits::eIndirectBuffer, size} {}

  template<class Type>
  HostIndirectBuffer(const VmaAllocator &allocator, const Type &value)
    : HostIndirectBuffer{allocator, sizeof(Type)} {
    updateSingle(value);
  }

  template<class Type, class Allocator>
  HostIndirectBuffer(
    const VmaAllocator &allocator, const std::vector<Type, Allocator> &value)
    : HostIndirectBuffer{allocator, value.size() * sizeof(Type)} {
    updateVector(value);
  }
};

class HostRayTracingBuffer: public HostBuffer {
public:
  HostRayTracingBuffer(const VmaAllocator &allocator, vk::DeviceSize size)
    : HostBuffer{allocator, vk::BufferUsageFlagBits::eRayTracingNV, size} {}

  template<class Type, class Allocator>
  HostRayTracingBuffer(
    const VmaAllocator &allocator, const std::vector<Type, Allocator> &value)
    : HostRayTracingBuffer{allocator, value.size() * sizeof(Type)} {
    update(value);
  }
};

class DeviceRayTracingBuffer: public BufferBase {
public:
  DeviceRayTracingBuffer(
    const VmaAllocator &allocator, vk::DeviceSize size, uint32_t memoryTypeBits)
    : BufferBase{allocator,
                 vk::BufferUsageFlagBits::eRayTracingNV,
                 size,
                 {{}, {}, {}, {}, memoryTypeBits}} {}
};
}

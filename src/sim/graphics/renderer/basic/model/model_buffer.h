#pragma once
#include "sim/util/range.h"
#include "sim/graphics/base/vkcommon.h"
#include "sim/graphics/base/resource/buffers.h"

namespace sim::graphics::renderer::basic {
using namespace sim::util;

template<typename T>
struct Allocation {
  uint32_t offset;
  T *ptr;
};

template<typename T>
class DeviceVertexBuffer {
public:
  explicit DeviceVertexBuffer(const VmaAllocator &allocator, uint32_t maxNum)
    : maxNum{maxNum}, _count{0} {
    data = u<VertexBuffer>(allocator, maxNum * sizeof(T));
  }

  Range add(Device &device, const T *ubo, uint32_t num) {
    errorIf(_count + num >= this->maxNum, "exceeding max number of data");
    auto offset = _count;
    data->upload(device, ubo, num * sizeof(T), offset * sizeof(T));
    _count += num;
    return {offset, num};
  }
  uint32_t count() const { return _count; }
  vk::Buffer buffer() { return data->buffer(); }

private:
  uPtr<VertexBuffer> data;
  uint32_t maxNum, _count;
};

class DeviceIndexBuffer {

public:
  DeviceIndexBuffer(const VmaAllocator &allocator, uint32_t maxNum)
    : maxNum{maxNum}, _count{0} {
    data = u<IndexBuffer>(allocator, maxNum * sizeof(uint32_t));
  }

  Range add(Device &device, const uint32_t *ubo, uint32_t num) {
    errorIf(_count + num >= this->maxNum, "exceeding max number of data");
    auto offset = _count;
    data->upload(device, ubo, num * sizeof(uint32_t), offset * sizeof(uint32_t));
    _count += num;
    return {offset, num};
  }
  uint32_t count() const { return _count; }
  vk::Buffer buffer() { return data->buffer(); }

private:
  uPtr<IndexBuffer> data;
  uint32_t maxNum{-1u}, _count;
};

template<typename T>
struct HostUBOBuffer {
  uPtr<HostUniformBuffer> data;

  explicit HostUBOBuffer(const VmaAllocator &allocator) {
    data = u<HostUniformBuffer>(allocator, sizeof(T));
  }

  void update(Device &device, T ubo) { data->updateSingle(ubo); }
  vk::Buffer buffer() { return data->buffer(); }
};

template<typename T>
struct HostManagedStorageUBOBuffer {
  uPtr<HostStorageBuffer> data;
  std::vector<uint32_t> freeSlots;
  uint32_t maxNum;
  HostManagedStorageUBOBuffer(const VmaAllocator &allocator, uint32_t maxNum)
    : maxNum{maxNum} {
    data = u<HostStorageBuffer>(allocator, maxNum * sizeof(T));
    freeSlots.reserve(maxNum);
    for(int32_t i = maxNum; i > 0; --i)
      freeSlots.push_back(i - 1);
  }

  Allocation<T> allocate() {
    errorIf(freeSlots.empty(), "Buffer is full!");
    auto offset = freeSlots.back();
    freeSlots.pop_back();
    return {offset, data->ptr<T>() + offset};
  }

  void deallocate(Allocation<T> allocation) {
    errorIf(allocation.ptr != data->ptr<T>() + allocation.offset, "Invalid allocation!");
    freeSlots.push_back(allocation.offset);
  }

  void update(Device &device, uint32_t offset, T ubo) {
    errorIf(offset >= this->maxNum, "exceeding max number of data");
    data->updateSingle(ubo, offset * sizeof(T));
  }
  vk::Buffer buffer() { return data->buffer(); }

  uint32_t count() { return maxNum - freeSlots.size(); }
};

struct HostIndirectUBOBuffer {
  using CMDType = vk::DrawIndexedIndirectCommand;
  uPtr<HostIndirectBuffer> data;
  uint32_t maxNum, _count{0};
  HostIndirectUBOBuffer(const VmaAllocator &allocator, uint32_t maxNum): maxNum{maxNum} {
    data = u<HostIndirectBuffer>(allocator, maxNum * sizeof(CMDType));
  }

  auto allocate() -> Allocation<CMDType> {
    errorIf(_count >= this->maxNum, "exceeding max number of data");
    auto offset = _count++;
    return {offset, data->ptr<CMDType>() + offset};
  }

  auto deallocate(Allocation<CMDType> allocation) {
    errorIf(
      allocation.ptr != data->ptr<CMDType>() + allocation.offset, "Invalid allocation!");
    error("Not implemented");
  }

  void update(Device &device, uint32_t offset, vk::DrawIndexedIndirectCommand ubo) {
    errorIf(offset >= _count, "exceeding allocated number of data");
    data->updateSingle(ubo, offset * sizeof(vk::DrawIndexedIndirectCommand));
  }
  vk::Buffer buffer() { return data->buffer(); }

  uint32_t count() { return _count; }
};

}
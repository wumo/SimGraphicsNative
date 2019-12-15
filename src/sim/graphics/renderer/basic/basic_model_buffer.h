#pragma once
#include "sim/graphics/renderer/basic/model/basic_model.h"
#include "sim/graphics/base/vkcommon.h"
#include "sim/graphics/base/resource/buffers.h"

namespace sim::graphics::renderer::basic {

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

struct PrimitivesBuffer {
  uint32_t maxNumVertex{-1u};
  uint32_t maxNumIndex{-1u};
  PrimitivesBuffer(
    const VmaAllocator &allocator, uint32_t maxNumVertex, uint32_t maxNumIndex)
    : maxNumVertex{maxNumVertex}, maxNumIndex{maxNumIndex} {};
  virtual ~PrimitivesBuffer() = default;

  virtual Primitive add(
    uint32_t vertexCount, uint32_t indexCount, const AABB &aabb,
    const PrimitiveTopology &topology) = 0;
  virtual Primitive add(
    Device &device, const Vertex *_vertices, uint32_t vertexCount,
    const uint32_t *_indices, uint32_t indexCount, const AABB &aabb,
    const PrimitiveTopology &topology) = 0;
  virtual void update(
    Device &device, Range _vertex, Range _index, const Vertex *vertices,
    uint32_t vertexCount, const uint32_t *indices, uint32_t indexCount) = 0;

  virtual vk::Buffer vertexBuffer() = 0;
  virtual vk::Buffer indexBuffer() = 0;
  virtual uint32_t vertexCount() const = 0;
  virtual uint32_t indexCount() const = 0;
};

struct HostPrimitivesBuffer: public PrimitivesBuffer {
  uint32_t _vertexCount{0};
  uPtr<HostVertexStorageBuffer> _vertices;
  uint32_t _indexCount{0};
  uPtr<HostIndexStorageBuffer> _indices;

  HostPrimitivesBuffer(
    const VmaAllocator &allocator, uint32_t maxNumVertex, uint32_t maxNumIndex);

  Primitive add(
    uint32_t vertexCount, uint32_t indexCount, const AABB &aabb,
    const PrimitiveTopology &topology) override;
  Primitive add(
    Device &device, const Vertex *vertices, uint32_t vertexCount, const uint32_t *indices,
    uint32_t indexCount, const AABB &aabb, const PrimitiveTopology &topology) override;
  void update(
    Device &device, Range _vertex, Range _index, const Vertex *vertices,
    uint32_t vertexCount, const uint32_t *indices, uint32_t indexCount) override;
  vk::Buffer vertexBuffer() override;
  vk::Buffer indexBuffer() override;
  uint32_t vertexCount() const override;
  uint32_t indexCount() const override;
};

struct DevicePrimitivesBuffer {
  uint32_t _vertexCount{0};
  uint32_t _indexCount{0};
  uint32_t maxNumVertex{-1u};
  uint32_t maxNumIndex{-1u};
  struct VBO {
    uPtr<VertexBuffer> _vertices;
    uPtr<IndexBuffer> _indices;
  };
  std::vector<VBO> vbo;

  DevicePrimitivesBuffer(
    const VmaAllocator &allocator, uint32_t maxNumVertex, uint32_t maxNumIndex,
    uint32_t numFrame);

  Primitive add(
    uint32_t vertexCount, uint32_t indexCount, const AABB &aabb,
    const PrimitiveTopology &topology);
  vk::Buffer vertexBuffer(uint32_t imageIndex);
  vk::Buffer indexBuffer(uint32_t imageIndex);
  uint32_t vertexCount() const;
  uint32_t indexCount() const;
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
struct HostStorageUBOBuffer {
  uPtr<HostStorageBuffer> data;
  uint32_t maxNum;
  HostStorageUBOBuffer(const VmaAllocator &allocator, uint32_t maxNum): maxNum{maxNum} {
    data = u<HostStorageBuffer>(allocator, maxNum * sizeof(T));
  }

  void update(Device &device, uint32_t index, T ubo) {
    errorIf(index >= this->maxNum, "exceeding max number of data");
    data->updateSingle(ubo, index * sizeof(T));
  }
  vk::Buffer buffer() { return data->buffer(); }
};

template<typename T>
struct HostIndirectUBOBuffer {
  uPtr<HostIndirectBuffer> data;
  uint32_t maxNum;
  HostIndirectUBOBuffer(const VmaAllocator &allocator, uint32_t maxNum): maxNum{maxNum} {
    data = u<HostIndirectBuffer>(allocator, maxNum * sizeof(T));
  }

  void update(Device &device, uint32_t index, T ubo) {
    errorIf(index >= this->maxNum, "exceeding max number of data");
    data->updateSingle(ubo, index * sizeof(T));
  }
  vk::Buffer buffer() { return data->buffer(); }
};

}
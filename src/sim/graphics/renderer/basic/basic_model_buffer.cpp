#include "basic_model_buffer.h"
namespace sim::graphics::renderer::basic {

HostPrimitivesBuffer::HostPrimitivesBuffer(
  const VmaAllocator &allocator, uint32_t maxNumVertex, uint32_t maxNumIndex)
  : PrimitivesBuffer{allocator, maxNumVertex, maxNumIndex} {
  _vertices = u<HostVertexStorageBuffer>(allocator, maxNumVertex * sizeof(Vertex));
  _indices = u<HostIndexStorageBuffer>(allocator, maxNumIndex * sizeof(uint32_t));
}
Primitive HostPrimitivesBuffer::add(
  uint32_t vertexCount, uint32_t indexCount, const AABB &aabb,
  const PrimitiveTopology &topology) {
  return Primitive();
}
Primitive HostPrimitivesBuffer::add(
  Device &device, const Vertex *vertices, const uint32_t vertexCount,
  const uint32_t *indices, const uint32_t indexCount, const AABB &aabb,
  const PrimitiveTopology &topology) {
  errorIf(_vertexCount + vertexCount > maxNumVertex, "exceeding max number of vertices!");
  errorIf(_indexCount + indexCount > maxNumIndex, "exceeding max number of indices!");

  _vertices->updateRaw(
    vertices, vertexCount * sizeof(Vertex), _vertexCount * sizeof(Vertex));
  _indices->updateRaw(
    indices, indexCount * sizeof(uint32_t), _indexCount * sizeof(uint32_t));
  Range vertex{_vertexCount, vertexCount}, index{_indexCount, indexCount};
  _vertexCount += vertexCount;
  _indexCount += indexCount;
  return {index, vertex, {}, {}, aabb, topology};
}
void HostPrimitivesBuffer::update(
  Device &device, Range _vertex, Range _index, const Vertex *vertices,
  const uint32_t vertexCount, const uint32_t *indices, const uint32_t indexCount) {
  assert(_vertex.offset + _vertex.size <= _vertexCount);
  assert(_index.offset + _index.size <= _indexCount);
  assert(vertexCount < _vertex.size);
  assert(indexCount < _index.size);

  _vertices->updateRaw(
    vertices, vertexCount * sizeof(Vertex), _vertex.offset * sizeof(Vertex));
  _indices->updateRaw(
    indices, indexCount * sizeof(uint32_t), _index.offset * sizeof(uint32_t));
}
vk::Buffer HostPrimitivesBuffer::vertexBuffer() { return _vertices->buffer(); }
vk::Buffer HostPrimitivesBuffer::indexBuffer() { return _indices->buffer(); }
uint32_t HostPrimitivesBuffer::vertexCount() const { return _vertexCount; }
uint32_t HostPrimitivesBuffer::indexCount() const { return _indexCount; }

DevicePrimitivesBuffer::DevicePrimitivesBuffer(
  const VmaAllocator &allocator, uint32_t maxNumVertex, uint32_t maxNumIndex,
  uint32_t numFrame)
  : maxNumVertex{maxNumVertex}, maxNumIndex{maxNumIndex} {
  vbo.resize(numFrame);
  for(int i = 0; i < numFrame; ++i)
    vbo[i] = {u<VertexBuffer>(allocator, maxNumVertex * sizeof(Vertex)),
              u<IndexBuffer>(allocator, maxNumIndex * sizeof(uint32_t))};
}
Primitive DevicePrimitivesBuffer::add(
  uint32_t vertexCount, uint32_t indexCount, const AABB &aabb,
  const PrimitiveTopology &topology) {
  errorIf(_vertexCount + vertexCount > maxNumVertex, "exceeding max number of vertices!");
  errorIf(_indexCount + indexCount > maxNumIndex, "exceeding max number of indices!");
  Range vertex{_vertexCount, vertexCount}, index{_indexCount, indexCount};
  _vertexCount += vertexCount;
  _indexCount += indexCount;
  return {index, vertex, {}, {}, aabb, topology, DynamicType ::Dynamic};
}
vk::Buffer DevicePrimitivesBuffer::vertexBuffer(uint32_t imageIndex) {
  return vbo[imageIndex]._vertices->buffer();
}
vk::Buffer DevicePrimitivesBuffer::indexBuffer(uint32_t imageIndex) {
  return vbo[imageIndex]._indices->buffer();
}
uint32_t DevicePrimitivesBuffer::vertexCount() const { return _vertexCount; }
uint32_t DevicePrimitivesBuffer::indexCount() const { return _indexCount; }

}
#pragma once
#include "basic_model.h"
#include "sim/graphics/base/debug_marker.h"

namespace sim::graphics::renderer::basic {
class DrawQueue {
public:
  enum class DrawType : uint32_t {
    OpaqueTriangles = 0u,
    OpaqueLines = 1u,
    TransparentTriangles = 2u,
    TransparentLines = 3u
  };
  DrawQueue(
    const VmaAllocator &allocator, uint32_t maxNumMeshes, uint32_t maxNumLineMeshes,
    uint32_t maxNumTransparentMeshes, uint32_t maxNumTransparentLineMeshes);

  auto allocate(const Ptr<Primitive> &primitive, const Ptr<Material> &material)
    -> Allocation<vk::DrawIndexedIndirectCommand>;

  vk::Buffer buffer(DrawType drawType);
  uint32_t count(DrawType drawType);

  void mark(DebugMarker &debugMarker);

private:
  static uint32_t index(Ptr<Primitive> primitive, Ptr<Material> material);

private:
  std::array<uPtr<HostIndirectUBOBuffer>, 4> queues;
};
}
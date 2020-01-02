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
    TransparentLines = 3u,
    Terrain = 4u,
    MAX = 5u
  };
  DrawQueue(
    const VmaAllocator &allocator, uint32_t maxNumMeshes, uint32_t maxNumLineMeshes,
    uint32_t maxNumTransparentMeshes, uint32_t maxNumTransparentLineMeshes,
    uint32_t maxNumTerrainMeshes, uint32_t maxNumFrames, uint32_t maxNumDynamicMeshes,
    uint32_t maxNumDynamicLineMeshes, uint32_t maxNumDynamicTransparentMeshes,
    uint32_t maxNumDynamicTransparentLineMeshes, uint32_t maxNumDynamicTerrainMeshes);

  auto allocate(const Ptr<Primitive> &primitive, const Ptr<Material> &material)
    -> std::vector<Allocation<vk::DrawIndexedIndirectCommand>>;

  vk::Buffer buffer(DrawType drawType);
  uint32_t count(DrawType drawType);

  vk::Buffer buffer(DrawType drawType, uint32_t frame);
  uint32_t count(DrawType drawType, uint32_t frame);

  void mark(DebugMarker &debugMarker);

private:
  static uint32_t index(Ptr<Primitive> primitive, Ptr<Material> material);

private:
  using DrawQueues =
    std::array<uPtr<HostIndirectUBOBuffer>, static_cast<uint32_t>(DrawType::MAX)>;
  DrawQueues staticDrawQueues;
  std::vector<DrawQueues> dynamicDrawQueues;
  uint32_t numFrame;
};
}
#include "draw_queue.h"
namespace sim::graphics::renderer::basic {

DrawQueue::DrawQueue(
  const VmaAllocator &allocator, uint32_t maxNumMeshes, uint32_t maxNumLineMeshes,
  uint32_t maxNumTransparentMeshes, uint32_t maxNumTransparentLineMeshes) {
  queues = {u<HostIndirectUBOBuffer>(allocator, maxNumMeshes),
            u<HostIndirectUBOBuffer>(allocator, maxNumLineMeshes),
            u<HostIndirectUBOBuffer>(allocator, maxNumTransparentMeshes),
            u<HostIndirectUBOBuffer>(allocator, maxNumTransparentLineMeshes)};
}

auto DrawQueue::allocate(const Ptr<Primitive> &primitive, const Ptr<Material> &material)
  -> Allocation<vk::DrawIndexedIndirectCommand> {
  auto idx = index(primitive, material);
  return queues[idx]->allocate();
}

uint32_t DrawQueue::index(Ptr<Primitive> primitive, Ptr<Material> material) {
  auto base = 0u;
  switch(material->type()) {
    case MaterialType::eBRDF:
    case MaterialType::eBRDFSG: break;
    case MaterialType::eReflective:
    case MaterialType::eRefractive:
    case MaterialType::eNone: base = 0u; break;
    case MaterialType::eTranslucent: base = 2u; break;
  }
  switch(primitive->topology()) {
    case PrimitiveTopology::Triangles: base += 0u; break;
    case PrimitiveTopology::Lines: base += 1u; break;
    case PrimitiveTopology::Procedural: error("Not supported"); break;
  }
  return base;
}

void DrawQueue::mark(DebugMarker &debugMarker) {
  debugMarker.name(queues[0]->buffer(), "drawOpaqueCMDs buffer");
  debugMarker.name(queues[1]->buffer(), "drawLineCMDs buffer");
  debugMarker.name(queues[2]->buffer(), "drawTransparentCMDs buffer");
  debugMarker.name(queues[3]->buffer(), "drawTransparentLineCMDs buffer");
}
vk::Buffer DrawQueue::buffer(DrawQueue::DrawType drawType) {
  return queues[static_cast<uint32_t>(drawType)]->buffer();
}
uint32_t DrawQueue::count(DrawType drawType) {
  return queues[static_cast<uint32_t>(drawType)]->count();
}
}
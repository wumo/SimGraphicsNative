#include "draw_queue.h"
namespace sim::graphics::renderer::basic {

DrawQueue::DrawQueue(
  const VmaAllocator &allocator, uint32_t maxNumMeshes, uint32_t maxNumLineMeshes,
  uint32_t maxNumTransparentMeshes, uint32_t maxNumTransparentLineMeshes,
  uint32_t maxNumTerrainMeshes, uint32_t maxNumFrames, uint32_t maxNumDynamicMeshes,
  uint32_t maxNumDynamicLineMeshes, uint32_t maxNumDynamicTransparentMeshes,
  uint32_t maxNumDynamicTransparentLineMeshes, uint32_t maxNumDynamicTerrainMeshes)
  : numFrame{maxNumFrames} {
  staticDrawQueues = {u<HostIndirectUBOBuffer>(allocator, maxNumMeshes),
                      u<HostIndirectUBOBuffer>(allocator, maxNumLineMeshes),
                      u<HostIndirectUBOBuffer>(allocator, maxNumTransparentMeshes),
                      u<HostIndirectUBOBuffer>(allocator, maxNumTransparentLineMeshes),
                      u<HostIndirectUBOBuffer>(allocator, maxNumTerrainMeshes)};
  dynamicDrawQueues.reserve(numFrame);
  for(int i = 0; i < numFrame; ++i) {
    dynamicDrawQueues.push_back(
      {u<HostIndirectUBOBuffer>(allocator, maxNumDynamicMeshes),
       u<HostIndirectUBOBuffer>(allocator, maxNumDynamicLineMeshes),
       u<HostIndirectUBOBuffer>(allocator, maxNumDynamicTransparentMeshes),
       u<HostIndirectUBOBuffer>(allocator, maxNumDynamicTransparentLineMeshes),
       u<HostIndirectUBOBuffer>(allocator, maxNumDynamicTerrainMeshes)});
  }
}

auto DrawQueue::allocate(const Ptr<Primitive> &primitive, const Ptr<Material> &material)
  -> std::vector<Allocation<vk::DrawIndexedIndirectCommand>> {
  auto idx = index(primitive, material);
  std::vector<Allocation<vk::DrawIndexedIndirectCommand>> results;
  switch(primitive.get().type()) {
    case DynamicType::Static: results.push_back(staticDrawQueues[idx]->allocate()); break;
    case DynamicType::Dynamic:
      results.reserve(numFrame);
      for(int i = 0; i < numFrame; ++i)
        results.push_back(dynamicDrawQueues[i][idx]->allocate());
      break;
  }
  return std::move(results);
}

uint32_t DrawQueue::index(Ptr<Primitive> primitive, Ptr<Material> material) {
  auto base = 0u;
  switch(material->type()) {
    case MaterialType::eBRDF:
    case MaterialType::eBRDFSG:
    case MaterialType::eReflective:
    case MaterialType::eRefractive:
    case MaterialType::eNone:
    case MaterialType::eTerrain: base = 0u; break;
    case MaterialType::eTranslucent: base = 2u; break;
  }
  switch(primitive->topology()) {
    case PrimitiveTopology::Triangles: base += 0u; break;
    case PrimitiveTopology::Lines: base += 1u; break;
    case PrimitiveTopology::Patches: base = 4u; break;
    case PrimitiveTopology::Procedural: error("Not supported"); break;
  }
  return base;
}

void DrawQueue::mark(DebugMarker &debugMarker) {
  debugMarker.name(staticDrawQueues[0]->buffer(), "drawOpaqueCMDs buffer");
  debugMarker.name(staticDrawQueues[1]->buffer(), "drawLineCMDs buffer");
  debugMarker.name(staticDrawQueues[2]->buffer(), "drawTransparentCMDs buffer");
  debugMarker.name(staticDrawQueues[3]->buffer(), "drawTransparentLineCMDs buffer");
  debugMarker.name(staticDrawQueues[4]->buffer(), "drawTerrainCMDs buffer");
  for(int i = 0; i < numFrame; ++i) {
    debugMarker.name(
      dynamicDrawQueues[i][0]->buffer(),
      toString("dynamicDrawQueues ", i, " drawOpaqueCMDs buffer").c_str());
    debugMarker.name(
      dynamicDrawQueues[i][1]->buffer(),
      toString("dynamicDrawQueues ", i, " drawLineCMDs buffer").c_str());
    debugMarker.name(
      dynamicDrawQueues[i][2]->buffer(),
      toString("dynamicDrawQueues ", i, " drawTransparentCMDs buffer").c_str());
    debugMarker.name(
      dynamicDrawQueues[i][3]->buffer(),
      toString("dynamicDrawQueues ", i, " drawTransparentLineCMDs buffer").c_str());
    debugMarker.name(
      dynamicDrawQueues[i][4]->buffer(),
      toString("dynamicDrawQueues ", i, " drawTerrainCMDs buffer").c_str());
  }
}
vk::Buffer DrawQueue::buffer(DrawType drawType) {
  return staticDrawQueues[static_cast<uint32_t>(drawType)]->buffer();
}
uint32_t DrawQueue::count(DrawType drawType) {
  return staticDrawQueues[static_cast<uint32_t>(drawType)]->count();
}

vk::Buffer DrawQueue::buffer(DrawType drawType, uint32_t frame) {
  return dynamicDrawQueues[frame][static_cast<uint32_t>(drawType)]->buffer();
}
uint32_t DrawQueue::count(DrawType drawType, uint32_t frame) {
  return dynamicDrawQueues[frame][static_cast<uint32_t>(drawType)]->count();
}
}
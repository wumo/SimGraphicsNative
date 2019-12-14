#include "vertex.h"
namespace sim::graphics::renderer::basic {

uint32_t Vertex::stride() { return sizeof(Vertex); }

uint32_t Vertex::locations() { return 3; }

std::vector<vk::VertexInputAttributeDescription> Vertex::attributes(
  uint32_t binding, uint32_t baseLocation) {
  using f = vk::Format;
  return {{baseLocation + 0, binding, f::eR32G32B32Sfloat, offsetOf(Vertex, position)},
          {baseLocation + 1, binding, f::eR32G32B32Sfloat, offsetOf(Vertex, normal)},
          {baseLocation + 2, binding, f::eR32G32Sfloat, offsetOf(Vertex, uv)}};
}
}
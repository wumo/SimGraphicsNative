#include "sim/graphics/renderer/default/deferred/deferred.h"
#include "sim/graphics/util/materials.h"
#include "sim/graphics/renderer/default/util/panning_camera.h"

using namespace sim::graphics::renderer::defaultRenderer;

using namespace sim::graphics;
using namespace glm;
using namespace material;

auto main(int argc, const char **argv) -> int {
  Deferred app{};

  auto &camera = app.camera();
  camera.location = {0, 100, 100};

  app.light(0).location = {0, 100, 100, 0};

  ConstPtr<Mesh> box1mesh;

  std::vector<Vertex> vertices{
    {{10, 10, 10}, {1, 0, 0}, {0, 0}},     {{10, -10, 10}, {1, 0, 0}, {1, 0}},
    {{10, -10, -10}, {1, 0, 0}, {1, 1}},   {{10, 10, -10}, {1, 0, 0}, {0, 1}},
    {{-10, -10, 10}, {-1, 0, 0}, {0, 0}},  {{-10, 10, 10}, {-1, 0, 0}, {1, 0}},
    {{-10, 10, -10}, {-1, 0, 0}, {1, 1}},  {{-10, -10, -10}, {-1, 0, 0}, {0, 1}},
    {{-10, 10, 10}, {0, 0, 1}, {0, 0}},    {{-10, -10, 10}, {0, 0, 1}, {1, 0}},
    {{10, -10, 10}, {0, 0, 1}, {1, 1}},    {{10, 10, 10}, {0, 0, 1}, {0, 1}},
    {{10, 10, -10}, {0, 0, -1}, {0, 0}},   {{10, -10, -10}, {0, 0, -1}, {1, 0}},
    {{-10, -10, -10}, {0, 0, -1}, {1, 1}}, {{-10, 10, -10}, {0, 0, -1}, {0, 1}},
    {{10, 10, 10}, {0, 1, 0}, {0, 0}},     {{10, 10, -10}, {0, 1, 0}, {1, 0}},
    {{-10, 10, -10}, {0, 1, 0}, {1, 1}},   {{-10, 10, 10}, {0, 1, 0}, {0, 1}},
    {{-10, -10, 10}, {0, -1, 0}, {0, 0}},  {{-10, -10, -10}, {0, -1, 0}, {1, 0}},
    {{10, -10, -10}, {0, -1, 0}, {1, 1}},  {{10, -10, 10}, {0, -1, 0}, {0, 1}},
  };
  std::vector<uint32_t> indices{
    0,  1,  2,  0,  2,  3,  4,  5,  6,  4,  6,  7,  8,  9,  10, 8,  10, 11,
    12, 13, 14, 12, 14, 15, 16, 17, 18, 16, 18, 19, 20, 21, 22, 20, 22, 23,
  };

  std::vector<std::vector<uint32_t>> unique_vertex{{0, 11, 16}, {1, 10, 23}, {2, 13, 22},
                                                   {3, 12, 17}, {4, 9, 20},  {5, 8, 19},
                                                   {6, 15, 18}, {7, 14, 21}};

  auto &mm = app.modelManager();
  box1mesh = mm.newMesh(indices, vertices);
  auto box1Material = mm.newMaterial(Blue, PBR{0.3f, 0.4f}, MaterialFlag::eNone);
  auto box1 = mm.newModel(
    [&](ModelBuilder &builder) { builder.addMeshPart(box1mesh, box1Material); });
  mm.newModelInstance(box1, Transform{{-30.0, 0, 0}});

  app.updateModels();
  app.run([&](uint32_t imageIndex, float dt) {
    static float time = 0;
    time += dt;
    for(auto i = 0u; i < unique_vertex.size(); i++) {
      auto &same_vertices = unique_vertex[i];
      auto vertex = vertices[same_vertices[0]];

      auto dir = normalize(vertex.position);
      vertex.position =
        (2 + sin(time + i * pi<float>() / 4)) / 2 * sqrt(10.f * 10 * 3) * dir;

      for(auto vertexIdx: same_vertices)
        vertices[vertexIdx] = vertex;
    }
    mm.updateVertices(box1mesh, 0, vertices);
  });
}
#include "sim/graphics/renderer/default/deferred/deferred.h"
#include "sim/graphics/util/materials.h"

using namespace sim::graphics::renderer::defaultRenderer;

using namespace sim::graphics;
using namespace glm;
using namespace material;

auto main(int argc, const char **argv) -> int {
  Deferred app{};

  auto &camera = app.camera();
  camera.location = {0, 100, 100};

  app.light(0).location = {0, 100, 100, 0};

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

  auto &mm = app.modelManager();
  auto box1 = mm.newModel([&](ModelBuilder &builder) {
    auto mesh =
      mm.newMeshes([&](MeshBuilder &builder) { builder.from(vertices, indices); });

    builder.addMeshPart(
      mesh[0], mm.newMaterial(Green, PBR{0.3f, 0.4f}, MaterialFlag::eNone));
  });
  mm.newModelInstance(box1);

  std::vector<glm::vec3> positions{
    {10, 10, 10},   {10, -10, 10},   {10, -10, -10},  {10, 10, -10},  {-10, -10, 10},
    {-10, 10, 10},  {-10, 10, -10},  {-10, -10, -10}, {-10, 10, 10},  {-10, -10, 10},
    {10, -10, 10},  {10, 10, 10},    {10, 10, -10},   {10, -10, -10}, {-10, -10, -10},
    {-10, 10, -10}, {10, 10, 10},    {10, 10, -10},   {-10, 10, -10}, {-10, 10, 10},
    {-10, -10, 10}, {-10, -10, -10}, {10, -10, -10},  {10, -10, 10},
  };
  auto box2 = mm.newModel([&](ModelBuilder &builder) {
    auto mesh =
      mm.newMeshes([&](MeshBuilder &builder) { builder.from(positions, indices); });

    builder.addMeshPart(
      mesh[0], mm.newMaterial(Red, PBR{0.3f, 0.4f}, MaterialFlag::eNone));
  });
  mm.newModelInstance(box2, Transform{{30.0, 0, 0}});

  auto box3 = mm.newModel([&](ModelBuilder &builder) {
    auto mesh = mm.newMesh(indices, vertices);

    builder.addMeshPart(mesh, mm.newMaterial(Blue, PBR{0.3f, 0.4f}, MaterialFlag::eNone));
  });
  mm.newModelInstance(box3, Transform{{-30.0, 0, 0}});

  auto box4 = mm.newModel([&](ModelBuilder &builder) {
    auto mesh = mm.newMesh(indices, positions);

    builder.addMeshPart(
      mesh, mm.newMaterial(Yellow, PBR{0.3f, 0.4f}, MaterialFlag::eNone));
  });
  mm.newModelInstance(box4, Transform{{-60.0, 0, 0}});

  app.updateModels();
  app.run();
}
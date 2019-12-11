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
  camera.isPerspective = false;
  camera.zNear = -1000;
  camera.zFar = 1000;
  camera.location = {0, 0, 100};
  camera.focus = {0, 0, 0};
  app.light(0).location = {100, 100, 100, 0};

  auto &mm = app.modelManager();

  auto axisModel = mm.newModel([&](ModelBuilder &builder) {
    auto mesh = mm.newMeshes([](MeshBuilder &builder) {
      builder.line({0, 0, 0}, {100, 0, 0});
      builder.newMesh();
      builder.line({0, 0, 0}, {0, 100, 0});
      builder.newMesh();
      builder.line({0, 0, 0}, {0, 0, 100});
    });
    auto root = builder.addNode();
    builder.addMeshPart(root, mesh[0], mm.newMaterial(Red));
    builder.addMeshPart(root, mesh[1], mm.newMaterial(Green));
    builder.addMeshPart(root, mesh[2], mm.newMaterial(Blue));
  });
  mm.newModelInstance(axisModel);

  auto planeModel = mm.newModel([&](ModelBuilder &builder) {
    auto mesh = mm.newMeshes([](MeshBuilder &builder) {
      builder.rectangle({100, 100, 0}, {100, 0, 0}, {0, 100, 0});
    });
    builder.addMeshPart(mesh[0], mm.newMaterial(White));
  });
  mm.newModelInstance(planeModel);

  app.updateModels();
  app.run();
}
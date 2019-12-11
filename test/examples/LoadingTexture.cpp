#include "sim/graphics/renderer/default/deferred/deferred.h"
#include "sim/graphics/util/materials.h"

using namespace sim::graphics::renderer::defaultRenderer;
using namespace sim::graphics;
using namespace material;

auto main(int argc, const char **argv) -> int {
  Deferred app{};

  app.light(0).location = {0, 10, 10, 0};

  auto &mm = app.modelManager();

  auto rectangleMesh = mm.newMeshes([&](MeshBuilder &builder) {
    builder.rectangle({}, {5.f, 0.f, 0.f}, {0.f, 5.f, 0.f});
  });

  auto sandMaterial = mm.newMaterial("assets/private/textures/sand.png");
  auto sandRectangleModel = mm.newModel(
    [&](ModelBuilder &builder) { builder.addMeshPart(rectangleMesh[0], sandMaterial); });
  auto sandRectangle = mm.newModelInstance(sandRectangleModel);

  app.updateModels();
  app.run();
}
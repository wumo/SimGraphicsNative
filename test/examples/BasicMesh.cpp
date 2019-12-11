#include "sim/graphics/renderer/default/deferred/deferred.h"
#include "sim/graphics/util/materials.h"

using namespace sim::graphics::renderer::defaultRenderer;
using namespace sim::graphics;
using namespace material;

auto main(int argc, const char **argv) -> int {
  Deferred app{};

  app.light(0).location = {0, 10, 10, 0};

  auto &mm = app.modelManager();
  /**
   * mesh -> shape
   * material -> color/metal/glass, etc.
   * model -> prototype
   * modelInstance -> instance of some prototype
   */
  auto mesh = mm.newMeshes([&](MeshBuilder &builder) {
    builder.box({0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 1.0);
  });
  auto greenMaterial = mm.newMaterial(Green, PBR{0.3f, 0.4f});
  auto greenBoxModel = mm.newModel(
    [&](ModelBuilder &builder) { builder.addMeshPart(mesh[0], greenMaterial); });
  auto greenBox1 = mm.newModelInstance(greenBoxModel);

  app.updateModels();
  app.run();
}
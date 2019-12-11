#include "sim/graphics/renderer/default/deferred/deferred.h"
#include "sim/graphics/util/materials.h"

using namespace sim::graphics::renderer::defaultRenderer;
using namespace sim::graphics;
using namespace material;

auto main(int argc, const char **argv) -> int {

  // resolution 1920x1080 and title "Getting..."
  Deferred app{{640, 480, "Getting Started"}};

  auto &camera = app.camera();
  camera.location = {10, 10, 10};

  auto &light = app.light(0);
  light.location = {0, 100, 100, 0};

  auto &mm = app.modelManager();
  /**
   * mesh -> shape
   * material -> color/metal/glass, etc.
   * model -> prototype
   * modelInstance -> instance of some prototype
   */
  auto boxMesh = mm.newMeshes([&](MeshBuilder &builder) {
    builder.box({0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 1.0);
  });
  auto greenMaterial = mm.newMaterial(material::Green, PBR{0.3f, 0.4f});
  auto greenBoxModel = mm.newModel(
    [&](ModelBuilder &builder) { builder.addMeshPart(boxMesh[0], greenMaterial); });
  auto greenBox1 = mm.newModelInstance(greenBoxModel);

  // after adding model instances, you have to call app.updateModels() to enable
  // drawing these instances.
  app.updateModels();
  app.run();
}
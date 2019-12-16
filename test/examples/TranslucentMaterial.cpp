#include "sim/graphics/renderer/default/deferred/deferred.h"
#include "sim/graphics/util/materials.h"
#include "sim/graphics/renderer/default/util/panning_camera.h"
#include "sim/graphics/util/fps_meter.h"
#include <sim/util/syntactic_sugar.h>
using namespace sim::graphics::renderer::defaultRenderer;
using namespace sim;
using namespace sim::graphics;
using namespace material;
using namespace glm;

auto main(int argc, const char **argv) -> int {
  Deferred app{};
  auto &camera = app.camera();
  auto &light = app.light(0);
  camera.location = {0, 20, -100};
  camera.focus = {0, 0, 0};
  light.location = {100, 100, 100, 0};

  auto &mm = app.modelManager();
  auto axisModel = mm.newModel([&](ModelBuilder &builder) {
    auto mesh = mm.newMeshes([](MeshBuilder &builder) {
      builder.line({0, 0, 0}, {10, 0, 0});
      builder.newMesh();
      builder.line({0, 0, 0}, {0, 10, 0});
      builder.newMesh();
      builder.line({0, 0, 0}, {0, 0, 10});
    });
    auto root = builder.addNode();
    builder.addMeshPart(root, mesh[0], mm.newMaterial(Red));
    builder.addMeshPart(root, mesh[1], mm.newMaterial(Green));
    builder.addMeshPart(root, mesh[2], mm.newMaterial(Blue));
  });
  mm.newModelInstance(axisModel);

  auto ballModel = mm.newModel([&](ModelBuilder &builder) {
    auto mesh = mm.newMeshes([](MeshBuilder &builder) { builder.sphere({0, 0, 0}, 10); });
    builder.addMeshPart(
      mesh[0], mm.newMaterial(Green, PBR{0.3f, 0.4f}, MaterialFlag::eNone));
  });
  mm.newModelInstance(ballModel, Transform{{0, 0, 100}});

  //transparent color
  auto blueTrans = mm.newTex(glm::vec4{0, 0, 1, 0.5});
  auto pbrTex = mm.newTex(PBR{0.3f, 0.4f});
  auto wallModel = mm.newModel([&](ModelBuilder &builder) {
    auto mesh = mm.newMeshes([](MeshBuilder &builder) {
      builder.rectangle({}, {50, 0, 0}, {0, 50, 0});
    });
    builder.addMeshPart(
      mesh[0], mm.newMaterial(blueTrans, pbrTex, MaterialFlag::eTranslucent));
  });
  mm.newModelInstance(wallModel);

  auto wall2 = mm.newModelInstance(wallModel, Transform{{0, 0, 10}});

  auto redTrans = mm.newTex(glm::vec4{1, 0, 0, 0.5});
  wall2->node(0).meshParts[0].material =
    mm.newMaterial(redTrans, pbrTex, MaterialFlag::eTranslucent);

  app.updateModels();

  PanningCamera panningCamera(app.camera());
  FPSMeter mFPSMeter;
  app.run([&](uint32_t imageIndex, float dt) {
    panningCamera.updateCamera(app.input);
    mFPSMeter.update(dt);
    auto frameStats =
      toString(" ", int32_t(mFPSMeter.FPS()), " FPS (", mFPSMeter.FrameTime(), " ms)");
    auto fullTitle = "Test  " + frameStats;
    app.setWindowTitle(fullTitle);
  });
}

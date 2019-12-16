#include "sim/graphics/renderer/default/deferred/deferred.h"
#include "sim/graphics/util/materials.h"
#include "sim/graphics/renderer/default/util/panning_camera.h"
#include "sim/graphics/util/fps_meter.h"
#include <sim/util/syntactic_sugar.h>

using namespace sim;
using namespace sim::graphics;
using namespace sim::graphics::renderer::defaultRenderer;
using namespace material;
using namespace glm;

auto main(int argc, const char **argv) -> int {
  DeferredConfig c{};
  c.vsync = true;
  c.enableImageBasedLighting = false;
  Deferred app{c};
  auto &camera = app.camera();
  auto &light = app.light(0);
  camera.location = {1.9f, 2.f, 2.f};
  camera.focus = {0, 0, 0};
  light.location = {100, 100, 100, 0};

  auto &mm = app.modelManager();
  mm.newModelInstance(
    mm.newModel([&](ModelBuilder &builder) {
        auto mesh = mm.newMeshes([&](MeshBuilder &meshBuilder) {
          meshBuilder.rectangle({}, {100.f, 0.f, 0.f}, {0.f, 0.f, 100.f});
        })[0];
        builder.addMeshPart(
          mesh, mm.newMaterial(vec4(White, 0.2f), {}, MaterialFlag::eTranslucent));
      })
      .get());
  std::string name = "Buggy";
  auto model = mm.loadGLTF("assets/private/gltf/" + name + "/glTF/" + name + ".gltf");
  mm.loadEnvironmentMap("assets/private/environments/papermill.ktx");
  //  auto instance = mm.newModelInstance(model, Transform{{}, vec3{100.f}});
  auto instance = mm.newModelInstance(model);

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

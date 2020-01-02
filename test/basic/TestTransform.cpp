
#include "sim/graphics/renderer/basic/basic_renderer.h"
#include "sim/graphics/renderer/basic/util/panning_camera.h"
#include "sim/graphics/util/fps_meter.h"
#include "sim/util/syntactic_sugar.h"

using namespace sim;
using namespace sim::graphics;
using namespace sim::graphics::renderer::basic;
using namespace glm;

auto main(int argc, const char **argv) -> int {
  Config config{};
  config.sampleCount = 4;
  config.vsync = false;
  BasicRenderer app{config, {}, {}, {true, false}};

  auto &mm = app.sceneManager();

  auto &camera = mm.camera();
  camera.setLocation({2.f, 2.f, 2.f});
  mm.addLight();

  vec3 center{0.f, 10.f, 0.f};

  auto primitives =
    mm.newPrimitives(PrimitiveBuilder(mm)
                       .rectangle(center, {2.f, 0.f, 0.f}, {0.f, 2.f, 0.f})
                       .newPrimitive()
                       .rectangle(
                         {}, {0, 0.f, 100.f},
                         {
                           100.f,
                           0,
                           0,
                         })
                       .newPrimitive());
  Transform t{-center};

  auto rectMaterial = mm.newMaterial();
  rectMaterial->setColorFactor({0.f, 1.f, 0.f, 1.f});
  auto rectMesh = mm.newMesh(primitives[0], rectMaterial);
  auto rectNode = mm.newNode();
  Node::addMesh(rectNode, rectMesh);
  auto rectModel = mm.newModel({rectNode});
  auto rect = mm.newModelInstance(rectModel, t);

  auto planeMaterial = mm.newMaterial(MaterialType::eTranslucent);
  planeMaterial->setColorFactor({1.f, 1.f, 1.f, 0.5f});
  auto planeMesh = mm.newMesh(primitives[1], planeMaterial);
  auto planeNode = mm.newNode();
  Node::addMesh(planeNode, planeMesh);
  auto planeModel = mm.newModel({planeNode});
  auto plane = mm.newModelInstance(planeModel);

  mm.debugInfo();

  PanningCamera panningCamera(camera);
  sim::graphics::FPSMeter mFPSMeter;
  app.run([&](uint32_t imageIndex, float elapsedDuration) {
    mFPSMeter.update(elapsedDuration);
    panningCamera.updateCamera(app.input);
    auto frameStats = sim::toString(
      " ", int32_t(mFPSMeter.FPS()), " FPS (", mFPSMeter.FrameTime(), " ms)");
    auto fullTitle = "Test  " + frameStats;
    app.setWindowTitle(fullTitle);
  });
}

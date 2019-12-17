
#include "sim/graphics/renderer/basic/basic_renderer.h"
#include "sim/graphics/renderer/basic/util/panning_camera.h"
#include "sim/graphics/util/fps_meter.h"
#include "sim/util/syntactic_sugar.h"

using namespace sim::graphics;
using namespace sim::graphics::renderer::basic;
using namespace glm;

auto main(int argc, const char **argv) -> int {

  BasicRenderer app{{}, {}, {true, false}};

  auto &mm = app.modelManager();

  auto &camera = mm.camera();
  camera.setLocation({2.f, 2.f, 2.f});
  mm.addLight();

  auto boxPrimitive = mm.newPrimitive(
    PrimitiveBuilder(mm).box({}, {1.f, 0.f, 0.f}, {0.f, 1.f, 0.f}, 1.f).newPrimitive());

  sim::println(boxPrimitive->aabb());

  auto boxMaterial = mm.newMaterial();
  boxMaterial->setColorFactor({0.f, 1.f, 0.f, 1.f});

  auto boxMesh = mm.newMesh(boxPrimitive, boxMaterial);
  auto boxNode = mm.newNode();
  Node::addMesh(boxNode, boxMesh);

  auto redMaterial = mm.newMaterial();
  redMaterial->setColorFactor({1.f, 0.f, 0.f, 1.f});
  auto boxMesh2 = mm.newMesh(boxPrimitive, redMaterial);
  auto boxNode2 = mm.newNode({
    {1.f, 0.f, 0.f},
  });
  Node::addMesh(boxNode2, boxMesh2);

  Node::addChild(boxNode, boxNode2);

  auto boxModel = mm.newModel({boxNode});

  auto box = mm.newModelInstance(boxModel);

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

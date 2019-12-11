#include "sim/graphics/renderer/default/deferred/deferred.h"
#include "testScene.h"
#include "sim/graphics/renderer/default/util/panning_camera.h"
#include "sim/graphics/util/fps_meter.h"
using namespace sim::graphics;
using namespace sim::graphics::renderer::defaultRenderer;
auto main(int argc, const char **argv) -> int {
  //#ifndef NDEBUG
  //  bool validate = false;
  //#else
  //  bool validate = true;
  //#endif
  auto validate = true;
  DeferredConfig config{
    {1920, 1080, "Deferred Test", false, true, 4},
    true,
    false,
  };
  Deferred app(config, ModelConfig{}, {true});
  app.modelManager().loadEnvironmentMap("assets/private/environments/papermill.ktx");
  auto updateFunc = buildScene(app.camera(), app.light(0), app.modelManager());
  app.updateModels();
  PanningCamera panningCamera(app.camera());
  FPSMeter mFPSMeter;
  app.run([&](uint32_t imageIndex, float dt) {
    mFPSMeter.update(dt);
    panningCamera.updateCamera(app.input);
    auto frameStats =
      toString(" ", int32_t(mFPSMeter.FPS()), " FPS (", mFPSMeter.FrameTime(), " ms)");
    auto fullTitle = "Test  " + frameStats;
    app.setWindowTitle(fullTitle);
    updateFunc(dt);
  });
}

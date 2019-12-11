#include "sim/graphics/renderer/default/raytracing/raytracing.h"
#include "testScene.h"
#include "sim/graphics/renderer/default/util/panning_camera.h"
#include "sim/graphics/util/fps_meter.h"

using namespace sim::graphics;
using namespace sim::graphics::material;
using namespace sim::graphics::renderer::defaultRenderer;
auto main(int argc, const char **argv) -> int {
  RayTracingConfig config{{
                            1920,
                            1080,
                            "Raytracing Test",
                            false,
                            true,
                          },
                          true,
                          false,
                          10};
  RayTracing app(config, ModelConfig{}, {true});
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

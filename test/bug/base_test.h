#pragma once

#include <sim/util/syntactic_sugar.h>
#include "sim/graphics/renderer/default/deferred/deferred.h"
#include "sim/graphics/renderer/default/raytracing/raytracing.h"
#include "sim/graphics/renderer/default/util/panning_camera.h"
#include "sim/graphics/util/fps_meter.h"
#include "sim/graphics/util/materials.h"

using namespace sim;
using namespace sim::graphics;
using namespace sim::graphics::renderer::defaultRenderer;
using namespace material;
using namespace glm;
class TestCaseDeferred {
public:
  uPtr<Deferred> app;

  virtual auto config(
    DeferredConfig &c, ModelConfig &modelConfig, DebugConfig &debugConfig) -> void{};
  virtual auto setup() -> void{};
  virtual auto update(float elapsedDuration) -> void{};

  auto test() {
    auto validate = true;
    DeferredConfig c{
      {1920, 1080, "Example", false, false, 4, 2},
      false,
    };
    ModelConfig modelConfig{};
    DebugConfig debugConfig{validate};
    config(c, modelConfig, debugConfig);
    app = u<Deferred>(c, modelConfig, debugConfig);
    auto &camera = app->camera();
    auto &light = app->light(0);
    camera.location = {0, 20, -100};
    camera.focus = {0, 0, 0};
    light.location = {100, 100, 100, 0};

    setup();

    app->updateModels();

    PanningCamera panningCamera(app->camera());
    FPSMeter mFPSMeter;
    app->run([&](uint32_t imageIndex, float dt) {
      mFPSMeter.update(dt);
      panningCamera.updateCamera(app->input);
      auto frameStats =
        toString(" ", int32_t(mFPSMeter.FPS()), " FPS (", mFPSMeter.FrameTime(), " ms)");
      auto fullTitle = "Test  " + frameStats;
      app->setWindowTitle(fullTitle);
      update(dt);
    });
  }
};

class TestCaseRaytracing {
public:
  uPtr<RayTracing> app;

  virtual auto config(
    RayTracingConfig &c, ModelConfig &modelConfig, DebugConfig &debugConfig) -> void{};
  virtual auto setup() -> void{};
  virtual auto update(float dt) -> void{};

  auto test() {
    auto validate = true;
    RayTracingConfig c{{1920, 1080, "Raytracing Example", true, true}, true, false, 10};
    ModelConfig modelConfig{};
    DebugConfig debugConfig{validate};
    config(c, modelConfig, debugConfig);
    app = u<RayTracing>(c, modelConfig, debugConfig);
    auto &camera = app->camera();
    auto &light = app->light(0);
    camera.location = {0, 20, -100};
    camera.focus = {0, 0, 0};
    light.location = {100, 100, 100, 0};

    setup();

    app->updateModels();

    PanningCamera panningCamera(app->camera());
    FPSMeter mFPSMeter;
    app->run([&](uint32_t imageIndex, float dt) {
      mFPSMeter.update(dt);
      panningCamera.updateCamera(app->input);
      auto frameStats =
        toString(" ", int32_t(mFPSMeter.FPS()), " FPS (", mFPSMeter.FrameTime(), " ms)");
      auto fullTitle = "Test  " + frameStats;
      app->setWindowTitle(fullTitle);
      update(dt);
    });
  }
};

#include "sim/graphics/renderer/basic/basic_renderer.h"
#include "sim/graphics/renderer/basic/util/panning_camera.h"
#include "sim/graphics/util/fps_meter.h"
#include "sim/graphics/util/colors.h"

using namespace sim;
using namespace sim::graphics;
using namespace sim::graphics::renderer::basic;
using namespace glm;
using namespace sim::graphics::material;

auto main(int argc, const char **argv) -> int {
  Config config{};
  config.sampleCount = 4;
  config.vsync = false;
  BasicRenderer app{config, {}, {}, {true, false}};

  auto &mm = app.modelManager();

  auto &camera = mm.camera();
  camera.setLocation({2.f, 2.f, 2.f});
  mm.addLight(LightType ::Directional, {-1, -1, -1});

  std::string name = "CesiumMilkTruck";
  auto path = "assets/private/gltf/" + name + "/glTF/" + name + ".gltf";
  //  std::string path = "assets/private/models/DamagedHelmet.glb";
  auto model = mm.loadModel(path);
  auto aabb = model->aabb();
  println(aabb);
  auto range = aabb.max - aabb.min;
  auto scale = 1 / std::max(std::max(range.x, range.y), range.z);
  auto center = aabb.center();
  auto halfRange = aabb.halfRange();

  auto width = 100;
  vec3 origin{-width / 2, 0, -width / 2};
  for(int nx = 0; nx < width; ++nx)
    for(int ny = 0; ny < width; ++ny) {
      Transform t{{origin + -center * scale + vec3{nx, 0, ny}}, glm::vec3{scale}};
      //  t.translation = -center;
      auto instance = mm.newModelInstance(model, t);
    }

  auto envCube = mm.newCubeTexture("assets/private/environments/noga_2k.ktx");
  mm.useEnvironmentMap(envCube);

  mm.debugInfo();

  PanningCamera panningCamera(camera);
  bool pressed{false};
  bool rotate{false};
  sim::graphics::FPSMeter mFPSMeter;
  app.run([&](uint32_t imageIndex, float elapsedDuration) {
    mFPSMeter.update(elapsedDuration);
    panningCamera.updateCamera(app.input);
    auto frameStats = sim::toString(
      " ", int32_t(mFPSMeter.FPS()), " FPS (", mFPSMeter.FrameTime(), " ms)");
    auto fullTitle = "Test  " + frameStats;
    app.setWindowTitle(fullTitle);
    for(auto &animation: model->animations())
      animation.animateAll(elapsedDuration);
    if(app.input.keyPressed[KeyW]) pressed = true;
    else if(pressed) {
      mm.setWireframe(!mm.wireframe());
      pressed = false;
    }
  });
}

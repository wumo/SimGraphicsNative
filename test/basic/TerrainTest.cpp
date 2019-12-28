
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
  FeatureConfig featureConfig{FeatureConfig::Value::Tesselation};
  //  FeatureConfig featureConfig{};
  BasicRenderer app{config, {}, featureConfig, {true, false}};

  auto &mm = app.modelManager();

  auto &camera = mm.camera();
  camera.setLocation({2.f, 2.f, 2.f});
//  mm.addLight(LightType ::Directional, {-1, -1, -1});

  std::string name = "DamagedHelmet";
  auto path = "assets/private/gltf/" + name + "/glTF/" + name + ".gltf";
  auto model = mm.loadModel(path);
  auto aabb = model->aabb();
  println(aabb);
  auto range = aabb.max - aabb.min;
  auto scale = 1 / std::max(std::max(range.x, range.y), range.z);
  auto center = aabb.center();
  auto halfRange = aabb.halfRange();
  Transform t{{-center * scale}, glm::vec3{scale}};
  //  t.translation = -center;
  auto instance = mm.newModelInstance(model, t);

  auto &tm = mm.terrrainManager();
  tm.loadSingle(
    "assets/private/terrain/TreasureIsland", "Height.png", "Normal.png", "Albedo.png",
    {{-50, -10, 50}, {50, 10, -50}}, 10, 10, 538.33f / 2625, 16.f);

  //  auto envCube = mm.newCubeTexture("assets/private/environments/noga_2k.ktx");
  //  mm.useEnvironmentMap(envCube);

  mm.useSky();
  auto kPi = glm::pi<float>();
  float sun_zenith_angle_radians_{0};
  float sun_azimuth_angle_radians_{kPi / 2};
  mm.setSunPosition(sun_zenith_angle_radians_, sun_azimuth_angle_radians_);

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
    if(app.input.keyPressed[KeyW]) pressed = true;
    else if(pressed) {
      mm.setWireframe(!mm.wireframe());
      pressed = false;
    }
  });
}


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
  config.numFrame = 3;
  config.sampleCount = 4;
  config.vsync = false;
  FeatureConfig featureConfig{FeatureConfig::Value::Tesselation};
  //  FeatureConfig featureConfig{};
  BasicRenderer app{config, {}, featureConfig, {true, false}};

  auto &mm = app.sceneManager();

  auto &camera = mm.camera();
  camera.setLocation({40.f, 40.f, 40.f});
  mm.addLight(LightType ::Directional, {1, -1, 1});

  auto primitives =
    mm.newPrimitives(PrimitiveBuilder(mm).axis({}, 20.f, 0.1f, 0.5f, 50).newPrimitive());
  auto yellowMat = mm.newMaterial();
  yellowMat->setColorFactor({Yellow, 1.f});
  auto redMat = mm.newMaterial();
  redMat->setColorFactor({Red, 1.f});
  auto greenMat = mm.newMaterial();
  greenMat->setColorFactor({Green, 1.f});
  auto blueMat = mm.newMaterial();
  blueMat->setColorFactor({Blue, 1.f});

  auto originMesh = mm.newMesh(primitives[0], yellowMat);
  auto xMesh = mm.newMesh(primitives[1], redMat);
  auto yMesh = mm.newMesh(primitives[2], greenMat);
  auto zMesh = mm.newMesh(primitives[3], blueMat);
  auto axisNode = mm.newNode();
  Node::addMesh(axisNode, originMesh);
  Node::addMesh(axisNode, xMesh);
  Node::addMesh(axisNode, yMesh);
  Node::addMesh(axisNode, zMesh);
  auto axisModel = mm.newModel({axisNode});
  auto axis = mm.newModelInstance(axisModel);

  std::string name = "DamagedHelmet";
  auto path = "assets/private/gltf/" + name + "/glTF/" + name + ".gltf";
  auto model = mm.loadModel(path);
  auto aabb = model->aabb();
  println(aabb);
  auto range = aabb.max - aabb.min;
  auto scale = 1 / std::max(std::max(range.x, range.y), range.z);
  auto center = aabb.center();
  auto halfRange = aabb.halfRange();
  Transform t{vec3{0, 10, 0} + vec3{-center * scale}, glm::vec3{scale}};
  //  t.translation = -center;
  auto instance = mm.newModelInstance(model, t);

  auto &tm = mm.terrainManager();
  auto seaLevelRatio = 538.33f / 2625;
  tm.loadPatches(
    "assets/private/terrain/TreasureIsland", "Height", "Normal", "Albedo", 8, 8,
    {{-50, 0, 50}, {50, 20, -50}}, 10, 10, 40.f);

  auto &ocean = mm.oceanManager();
  auto field = ocean.newField(125.f, 128);
  ocean.updateWind({0.8f, 0.6f}, 60.f);
  ocean.updateWaveAmplitude(10.f);

  field->setTransform({vec3{0, 20 * seaLevelRatio, 0}, {100 / 128.f, 1, 100 / 128.f}});

  auto &sky = mm.skyManager();
  sky.init();
  auto kPi = glm::pi<float>();
  float sun_zenith_angle_radians_{kPi / 4};
  float sun_azimuth_angle_radians_{kPi * 3 / 4};
  sky.setSunDirection(sun_zenith_angle_radians_, sun_azimuth_angle_radians_);

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


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

  auto &mm = app.sceneManager();

  auto &camera = mm.camera();
  camera.changeZFar(1e10);
  camera.setLocation({40.f, 40.f, 40.f});
  //  mm.addLight(LightType ::Directional, {-1, -1, -1});

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
  tm.loadSingle(
    "assets/private/terrain/TreasureIsland", "Height.png", "Normal.png", "Albedo.png",
    {{-100, 0, 100}, {1'00, 20, -100}}, 10, 10, 40.f, false);
  //  tm.loadPatches(
  //    "assets/private/terrain/TreasureIsland", "Height", "Normal", "Albedo", 8, 8,
  //    {{-50, 0, 50}, {50, 20, -50}}, 10, 10, 40.f);

  tm.staticSeaLevel({{-100, 0, 100}, {1'00, 20, -100}}, 450.f / 2000);

  //  auto envCube = mm.newCubeTexture("assets/private/environments/noga_2k.ktx");
  //  mm.useEnvironmentMap(envCube);

  auto &sky = mm.skyManager();
  sky.init(1);

  auto kPi = glm::pi<float>();
  float seasonAngle = kPi / 4;
  float sunAngle = 0;
  float angularVelocity = kPi / 20;
  auto sunDirection = [&](float dt) {
    sunAngle += angularVelocity * dt;
    if(sunAngle > 2 * kPi) sunAngle = 0;

    return -vec3{cos(sunAngle), abs(sin(sunAngle) * sin(seasonAngle)),
                 -sin(sunAngle) * cos(seasonAngle)};
  };
  sky.setSunDirection(sunDirection(0));
  sky.setEarthCenter({0, -sky.earthRadius() / sky.lengthUnitInMeters() - 100, 0});

  mm.debugInfo();

  PanningCamera panningCamera(camera);
  bool pressed{false};
  bool rotate{false};
  sim::graphics::FPSMeter mFPSMeter;
  app.run([&](uint32_t imageIndex, float elapsedDuration) {
    mFPSMeter.update(elapsedDuration);
    panningCamera.updateCamera(app.input);
    auto frameStats = sim::toString(
      " ", int32_t(mFPSMeter.FPS()), " FPS (", mFPSMeter.FrameTime(),
      " ms), camera pos:", glm::to_string(camera.location()));
    auto fullTitle = "Test  " + frameStats;
    app.setWindowTitle(fullTitle);
    if(app.input.keyPressed[KeyW]) pressed = true;
    else if(pressed) {
      mm.setWireframe(!mm.wireframe());
      pressed = false;
    }
    sky.setSunDirection(sunDirection(elapsedDuration));
  });
}


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

  auto &mm = app.sceneManager();

  auto &camera = mm.camera();
  camera.setLocation({2.f, 2.f, 2.f});
  //  mm.addLight(LightType ::Directional, {-1, -1, -1});

  std::string name = "DamagedHelmet";
  auto path = "assets/private/gltf/" + name + "/glTF/" + name + ".gltf";
  //  std::string path = "assets/private/models/DamagedHelmet.glb";
  //  std::string path="assets/private/models/s9_mini_drone/scene.gltf";
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

  auto primitives = mm.newPrimitives(
    PrimitiveBuilder(mm)
      .boxLine(center, {halfRange.x, 0.f, 0.f}, {0.f, halfRange.y, 0.f}, halfRange.z)
      .newPrimitive(PrimitiveTopology::Lines)
      .axis({}, 2.f, 0.01f, 0.05f, 50)
      .newPrimitive()
      .sphere({1, 0, 0}, 0.5)
      .newPrimitive());

  sim::println(primitives[0]->aabb());

  auto boxMaterial = mm.newMaterial();
  boxMaterial->setColorFactor({1.f, 0.f, 0.f, 1.f});
  auto boxMesh = mm.newMesh(primitives[0], boxMaterial);
  auto boxNode = mm.newNode();
  Node::addMesh(boxNode, boxMesh);
  auto boxModel = mm.newModel({boxNode});
  auto box = mm.newModelInstance(boxModel, t);

  auto yellowMat = mm.newMaterial();
  yellowMat->setColorFactor({Yellow, 1.f});
  auto redMat = mm.newMaterial();
  redMat->setColorFactor({Red, 1.f});
  auto greenMat = mm.newMaterial();
  greenMat->setColorFactor({Green, 1.f});
  auto blueMat = mm.newMaterial();
  blueMat->setColorFactor({Blue, 1.f});

  auto originMesh = mm.newMesh(primitives[1], yellowMat);
  auto xMesh = mm.newMesh(primitives[2], redMat);
  auto yMesh = mm.newMesh(primitives[3], greenMat);
  auto zMesh = mm.newMesh(primitives[4], blueMat);
  auto axisNode = mm.newNode();
  Node::addMesh(axisNode, originMesh);
  Node::addMesh(axisNode, xMesh);
  Node::addMesh(axisNode, yMesh);
  Node::addMesh(axisNode, zMesh);
  auto axisModel = mm.newModel({axisNode});
  auto axis = mm.newModelInstance(axisModel);

  auto shpereMaterial = mm.newMaterial(MaterialType::eBRDF);
  shpereMaterial->setColorFactor({0.f, 0.f, 1.f, 1.f});
  shpereMaterial->setPbrFactor({0.f, 0.5f, 0.8f, 1.f});
  auto shpereMesh = mm.newMesh(primitives[5], shpereMaterial);
  auto shpereNode = mm.newNode();
  Node::addMesh(shpereNode, shpereMesh);
  auto shpereModel = mm.newModel({shpereNode});
  auto shpere = mm.newModelInstance(shpereModel);

  //  auto envCube = mm.newCubeTexture("assets/private/environments/noga_2k.ktx");
  //  mm.useEnvironmentMap(envCube);

  auto &sky = mm.skyManager();
  sky.init(1);

  auto kPi = glm::pi<float>();

  float seasonAngle = kPi / 4;
  float sunAngle = 0;
  float angularVelocity = kPi / 10;
  auto sunDirection = [&](float dt) {
    sunAngle += angularVelocity * dt;
    if(sunAngle > kPi) sunAngle = 0;

    return -vec3{cos(sunAngle), sin(sunAngle) * sin(seasonAngle),
                 -sin(sunAngle) * cos(seasonAngle)};
  };

  sky.setSunDirection(sunDirection(1));
  sky.setEarthCenter({0, -sky.earthRadius() / sky.lengthUnitInMeters() -100, 0});

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
    if(rotate) {
      t.rotation =
        t.rotation * angleAxis(pi<float>() * elapsedDuration * 0.2f, vec3{0.f, 1.f, 0.f});
      box->setTransform(t);
      instance->setTransform(t);
    }
    auto model = instance->model();
    for(auto &animation: model->animations())
      animation.animateAll(elapsedDuration);
    if(app.input.keyPressed[KeyW]) pressed = true;
    else if(pressed) {
      mm.setWireframe(!mm.wireframe());
      pressed = false;
    }

    //    sky.setSunDirection(sunDirection(elapsedDuration));

    //    sun_zenith_angle_radians_ += 1.f / 100 * elapsedDuration;
    //    if(sun_zenith_angle_radians_ > kPi / 2) sun_zenith_angle_radians_ = -kPi / 2;
    //    sun_azimuth_angle_radians_ += 1.f / 100 * elapsedDuration;
    //    sun_azimuth_angle_radians_ = std::fmod(sun_azimuth_angle_radians_, 2 * kPi);
    //    sky.setSunDirection(sun_zenith_angle_radians_, sun_azimuth_angle_radians_);
  });
}

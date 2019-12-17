
#include "sim/graphics/renderer/basic/basic_renderer.h"
#include "sim/graphics/renderer/basic/util/panning_camera.h"
#include "sim/graphics/util/fps_meter.h"
#include "sim/graphics/util/materials.h"

using namespace sim;
using namespace sim::graphics;
using namespace sim::graphics::renderer::basic;
using namespace glm;
using namespace sim::graphics::material;

auto main(int argc, const char **argv) -> int {
  Config config{};
  config.sampleCount = 4;
  config.vsync = false;
  BasicRenderer app{config, {}, {true, false}};

  auto &mm = app.modelManager();

  auto &camera = mm.camera();
  camera.setLocation({2.f, 2.f, 2.f});
  mm.addLight(LightType ::Directional, {-1, -1, -1});

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

  auto gridPrimitive = mm.newPrimitive(
    PrimitiveBuilder(mm).grid(100, 100).newPrimitive(PrimitiveTopology::Terrain));
  gridPrimitive->setAabb({{0, -3, 0}, {0, 7, 0}});

  auto gridMaterial = mm.newMaterial(MaterialType::eTerrain);
  gridMaterial->setColorFactor({0.8f, 0.8f, 0.8f, 1.f});
  auto gridMesh = mm.newMesh(gridPrimitive, gridMaterial);
  auto gridNode = mm.newNode();
  Node::addMesh(gridNode, gridMesh);
  auto gridModel = mm.newModel({gridNode});
  auto grid = mm.newModelInstance(gridModel);

  auto albedoTex = mm.newTexture("assets/private/terrain/TreasureIsland/Albedo.png");
  auto normalTex = mm.newTexture("assets/private/terrain/TreasureIsland/Normal.png");
  auto heightTex = mm.newGrayTexture("assets/private/terrain/TreasureIsland/Height.png");

  gridMaterial->setColorTex(albedoTex);
  gridMaterial->setNormalTex(normalTex);
  gridMaterial->setHeightTex(heightTex);

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
    if(app.input.keyPressed[KeyW]) pressed = true;
    else if(pressed) {
      mm.setWireframe(!mm.wireframe());
      pressed = false;
    }
  });
}

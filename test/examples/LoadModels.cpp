#include "sim/graphics/renderer/default/deferred/deferred.h"
#include "sim/graphics/util/materials.h"
#include "sim/graphics/renderer/default/util/panning_camera.h"

using namespace sim::graphics::renderer::defaultRenderer;

using namespace sim::graphics;

auto main(int argc, const char **argv) -> int {
  Deferred app{};

  auto &camera = app.camera();
  camera.location = {0, 100, 100};

  app.light(0).location = {0, 100, 100, 0};

  auto &mm = app.modelManager();
  auto sandMat = mm.newMaterial("assets/private/textures/sand.png");
  auto jetModel = mm.loadGLTF("assets/private/models/desert.glb");
  auto jet1 = mm.newModelInstance(jetModel);
  PanningCamera panningCamera(app.camera());
  for(auto i = 0u; i < jet1->numNodes(); ++i)
    for(auto &part: jet1->node(i).meshParts)
      part.material = sandMat;

  app.updateModels();
  app.run([&](uint32_t imageIndex, float dt) { panningCamera.updateCamera(app.input); });
}
#include "base_test.h"

class PBRBlink: public TestCaseDeferred {
public:
  void config(
    DeferredConfig &c, ModelConfig &modelConfig, DebugConfig &debugConfig) override {
    c.enableImageBasedLighting = true;
  }

private:
  Ptr<ModelInstance> jet;
  void setup() override {
    auto &mm = app->modelManager();
    auto jetModel = mm.loadGLTF("assets/private/models/DamagedHelmet.glb");
    mm.loadEnvironmentMap("assets/private/environments/papermill.ktx");
    jet = mm.newModelInstance(jetModel, Transform{{}, vec3{20.f}});
  }

  void update(float elapsedDuration) override {
    auto angle = radians(elapsedDuration * 20);
    auto quat = angleAxis(angle, glm::vec3{0.f, 1.f, 0.f});
    jet->transform()->rotation = quat * jet->transform()->rotation;
    auto &mm = app->modelManager();
    //    mm.transformChange(jet->transform().index());
  }
};
auto main(int argc, const char **argv) -> int { PBRBlink().test(); }
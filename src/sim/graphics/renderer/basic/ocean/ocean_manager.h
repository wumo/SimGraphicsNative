#pragma once
namespace sim::graphics::renderer::basic {
class BasicSceneManager;
class OceanManager {
public:
  explicit OceanManager(BasicSceneManager &mm);

private:
  BasicSceneManager &mm;
};
}

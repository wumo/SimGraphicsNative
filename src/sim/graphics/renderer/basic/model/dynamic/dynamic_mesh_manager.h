#pragma once
#include "../basic_model.h"
#include "../draw_queue.h"

namespace sim ::graphics::renderer::basic {
class BasicSceneManager;
class DynamicMeshManager {
public:
  explicit DynamicMeshManager(BasicSceneManager &scene);

private:
  friend class BasicSceneManager;

  struct {
    
    uPtr<DrawQueue> drawQueue;

  } Buffer;

private:
  BasicSceneManager &scene;
};
}
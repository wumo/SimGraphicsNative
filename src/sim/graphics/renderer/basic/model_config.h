#pragma once
#include <cstdint>

namespace sim::graphics::renderer::basic {
struct ModelConfig {
  /**max number of static vertices and indices*/
  uint32_t maxNumVertex{1000'0000}, maxNumIndex{1000'0000};
  /**max number of static vertices and indices*/
  uint32_t maxNumDynamicVertex{1'0000}, maxNumDynamicIndex{1'0000};
  /**max number of static model instances*/
  uint32_t maxNumTransform{10'0000};
  /**max number of static materials*/
  uint32_t maxNumMaterial{1'0000};
  /**max number of primitives*/
  uint32_t maxNumPrimitives{1'0000};
  /**max number of static mesh instances*/
  uint32_t maxNumMeshes{100'0000}, maxNumLineMeshes{1'000},
    maxNumTransparentMeshes{1'000}, maxNumTransparentLineMeshes{1'000};
  /**max number of terrain mesh instances*/
  uint32_t maxNumTerranMeshes{1'000};
  /**max number of texture including 2d and cube map.*/
  uint32_t maxNumTexture{1000};
  /**max number of lights*/
  uint32_t maxNumLights{1};
};
}
#pragma once
namespace sim::graphics::renderer::defaultRenderer {
struct ModelConfig {
  /**max number of device local vertices*/
  uint32_t maxNumVertex{10000000}; // 320MB
  /**max number of device local indices.*/
  uint32_t maxNumIndex{10000000}; // 400MB
  /**max number of meshes*/
  uint32_t maxNumMeshes{1000000};
  /**max number of indirect draw calls for Triangles*/
  uint32_t maxNumMeshInstances{1000000}; // 200MB
  /**max number of transforms*/
  uint32_t maxNumTransform{1000000}; // 480MB
  /**max number of materials*/
  uint32_t maxNumMaterial{1000000}; // 60MB
  uint32_t maxNumTexture{1000};
  uint32_t texWidth{512}, texHeight{512};
  // total 1188MB
  float lineWidth{1.f};

  uint32_t numLights{1};

  uint32_t maxNumSet = 100;
  uint32_t maxNumUniformBuffer = 100;
  uint32_t maxNumCombinedImageSampler = 1000;
  uint32_t maxNumInputAttachment = 100;
  uint32_t maxNumStorageBuffer = 100;
  uint32_t maxNumStorageImage = 2;
};
}
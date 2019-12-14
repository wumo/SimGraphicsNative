#pragma once
#include "sim/graphics/base/resource/images.h"
#include "sim/graphics/base/resource/buffers.h"
#include "sim/graphics/base/device.h"
#include "sim/graphics/renderer/basic/model/basic_model.h"

namespace sim::graphics::renderer::basic {

enum class EnvMap { Irradiance, PreFiltered };

struct EnvMaps {
  uPtr<TextureImageCube> irradiance, preFiltered;
};

class EnvMapGenerator {
public:
  explicit EnvMapGenerator(Device &device): device{device} {}

  uPtr<TextureImage2D> generateBRDFLUT();
  EnvMaps generateEnvMap(TextureImageCube &envCube);

private:
  void generateEnvMap(
    EnvMap envMap, TextureImageCube &cubeMap, TextureImageCube &envCube,
    HostVertexBuffer &vbo, HostIndexBuffer &ibo, const Primitive &primitive);

private:
  Device &device;
};
}

#pragma once
#include "sim/graphics/base/vkcommon.h"
#include <glslang/SPIRV/spirv.hpp>
#include <utility>

namespace sim::graphics {
class SpecializationMaker {
public:
  template<typename T>
  SpecializationMaker &entry(const T &data) {
    auto size = sizeof(T);
    entries.emplace_back(entries.size(), offset, size);
    auto p = reinterpret_cast<const std::byte *>(&data);
    for(size_t i = 0; i < size; i++)
      datum.emplace_back(*(p + i));
    offset += uint32_t(size);
    return *this;
  }

  vk::SpecializationInfo create() {
    return {uint32_t(entries.size()), entries.data(), datum.size(), datum.data()};
  }

private:
  std::vector<vk::SpecializationMapEntry> entries;
  std::vector<std::byte> datum;
  uint32_t offset{0};
};

struct Variable {
  // The name of the variable from the GLSL/HLSL
  std::string debugName;

  // The internal name (integer) of the variable
  int name;

  // The location in the binding.
  int location;

  // The binding in the descriptor set or I/O channel.
  int binding;

  // The descriptor set (for uniforms)
  int set;
  int instruction;

  // Storage class of the variable, eg. spv::StorageClass::Uniform
  spv::StorageClass storageClass;
};

class ShaderModule {
public:
  ShaderModule(
    const vk::Device &device, const std::string &filename,
    const vk::SpecializationInfo *pSpecializationInfo = nullptr);
  ShaderModule(
    const vk::Device &device, const uint32_t *opcodes, size_t size,
    const vk::SpecializationInfo *pSpecializationInfo = nullptr);
  vk::ShaderModule &module();
  void setSpecialization(vk::SpecializationInfo *pSpecializationInfo);
  const vk::SpecializationInfo *specialization() const;

private:
  struct State {
    vk::UniqueShaderModule module;
    const vk::SpecializationInfo *pSpecializationInfo{nullptr};
  } state;
};
}

#include "shader.h"
#include <unordered_map>
#include <fstream>
#include "sim/util/syntactic_sugar.h"

namespace sim::graphics {
ShaderModule::ShaderModule(
  const vk::Device &device, const std::string &filename,
  const vk::SpecializationInfo *pSpecializationInfo) {
  auto file = std::ifstream(filename, std::ios::ate | std::ios::binary);
  errorIf(!file.good(), "failed to read file!", filename);

  auto length = size_t(file.tellg());
  std::vector<uint32_t> opcodes;
  opcodes.resize(length / sizeof(uint32_t));
  file.seekg(0, std::ios::beg);
  file.read(reinterpret_cast<char *>(opcodes.data()), opcodes.size() * sizeof(uint32_t));
  file.close();

  vk::ShaderModuleCreateInfo shaderModuleInfo;
  shaderModuleInfo.codeSize = opcodes.size() * sizeof(uint32_t);
  shaderModuleInfo.pCode = reinterpret_cast<const uint32_t *>(opcodes.data());
  state.module = device.createShaderModuleUnique(shaderModuleInfo);
  state.pSpecializationInfo = pSpecializationInfo;
}

ShaderModule::ShaderModule(
  const vk::Device &device, const uint32_t *opcodes, size_t size,
  const vk::SpecializationInfo *pSpecializationInfo) {
  vk::ShaderModuleCreateInfo shaderModuleInfo;
  shaderModuleInfo.codeSize = size * sizeof(uint32_t);
  shaderModuleInfo.pCode = opcodes;
  state.module = device.createShaderModuleUnique(shaderModuleInfo);
  state.pSpecializationInfo = pSpecializationInfo;
}

void ShaderModule::setSpecialization(vk::SpecializationInfo *pSpecializationInfo) {
  state.pSpecializationInfo = pSpecializationInfo;
}

const vk::SpecializationInfo *ShaderModule::specialization() const {
  return state.pSpecializationInfo;
}

vk::ShaderModule &ShaderModule::module() { return *state.module; }
}

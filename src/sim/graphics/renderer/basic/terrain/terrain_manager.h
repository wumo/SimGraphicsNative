#pragma once
namespace sim::graphics::renderer::basic {
class TerrainManager {
public:
  explicit TerrainManager(BasicModelManager &mm);

  void loadPatches(
    const std::string &terrainFolder, const std::string &heightMapPrefix,
    const std::string &normalMapPrefix, const std::string &albedoMapPrefix,
    uint32_t patchNumX, uint32_t patchNumY, glm::vec3 origin, float patchScale,
    float minHeight, float maxHeight, float seaLevel);

  void loadSingle(
    const std::string &terrainFolder, const std::string &heightMap,
    const std::string &normalMap, const std::string &albedoMap, const AABB &aabb,
    uint32_t numVertexX, uint32_t numVertexY, float seaLevelRatio);

private:
  BasicModelManager &mm;
};
}
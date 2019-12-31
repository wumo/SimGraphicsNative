#pragma once
namespace sim::graphics::renderer::basic {
class TerrainManager {
public:
  explicit TerrainManager(BasicModelManager &mm);

  /**
   * World Machine tiled map
   *
   * Note: "Tiled Build Options" should check "Share edge vertices" box to prevent seam hole
   * between patches.
   */
  void loadPatches(
    const std::string &terrainFolder, const std::string &heightMapPrefix,
    const std::string &normalMapPrefix, const std::string &albedoMapPrefix,
    uint32_t patchNumX, uint32_t patchNumY, const AABB &aabb, uint32_t numVertexX,
    uint32_t numVertexY, float seaLevelRatio, float tesselationLevel = 64.0f);

  void loadSingle(
    const std::string &terrainFolder, const std::string &heightMap,
    const std::string &normalMap, const std::string &albedoMap, const AABB &aabb,
    uint32_t numVertexX, uint32_t numVertexY, float seaLevelRatio,
    float tesselationWidth = 64.0f);

private:
  BasicModelManager &mm;
};
}
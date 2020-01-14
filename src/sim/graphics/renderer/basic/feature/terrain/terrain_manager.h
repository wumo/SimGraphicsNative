#pragma once
namespace sim::graphics::renderer::basic {
class BasicSceneManager;
class TerrainManager {
public:
  explicit TerrainManager(BasicSceneManager &mm);

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
    uint32_t numVertexY, float tesselationLevel = 64.0f, bool lod = false);

  void loadSingle(
    const std::string &terrainFolder, const std::string &heightMap,
    const std::string &normalMap, const std::string &albedoMap, const AABB &aabb,
    uint32_t numVertexX, uint32_t numVertexY, float tesselationWidth = 64.0f,
    bool lod = false);

  void staticSeaLevel(const AABB &aabb, float seaLevelRatio);

private:
  BasicSceneManager &mm;
};
}
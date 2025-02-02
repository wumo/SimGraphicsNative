#include <sim/graphics/renderer/basic/basic_scene_manager.h>
#include "terrain_manager.h"

namespace sim::graphics::renderer::basic {
using namespace glm;
TerrainManager::TerrainManager(sim::graphics::renderer::basic::BasicSceneManager &mm)
  : mm(mm) {}

void TerrainManager::loadPatches(
  const std::string &terrainFolder, const std::string &heightMapPrefix,
  const std::string &normalMapPrefix, const std::string &albedoMapPrefix,
  uint32_t patchNumX, uint32_t patchNumY, const AABB &aabb, uint32_t numVertexX,
  uint32_t numVertexY, float tesselationLevel, bool lod) {

  auto range = aabb.range();

  auto &_min = aabb.min;
  auto &_max = aabb.max;
  float unitX = range.x / patchNumX;
  float unitZ = range.z / patchNumY;

  for(int nx = 0; nx < patchNumX; ++nx) {
    for(int ny = 0; ny < patchNumY; ++ny) {
      std::string heightMap = toString(heightMapPrefix, "_", nx, "_", ny, ".png");
      std::string normalMap = toString(normalMapPrefix, "_", nx, "_", ny, ".png");
      std::string albedoMap = toString(albedoMapPrefix, "_", nx, "_", ny, ".png");
      loadSingle(
        terrainFolder, heightMap, normalMap, albedoMap,
        {_min + vec3{(patchNumY - 1 - ny) * unitX, _min.y, -nx * unitZ},
         _min + vec3{(patchNumY - ny) * unitX, _max.y, -(nx + 1) * unitZ}},
        numVertexX, numVertexY, tesselationLevel, lod);
    }
  }
}

void TerrainManager::loadSingle(
  const std::string &terrainFolder, const std::string &heightMap,
  const std::string &normalMap, const std::string &albedoMap, const AABB &aabb,
  uint32_t numVertexX, uint32_t numVertexY, float tesselationWidth, bool lod) {
  vec3 center = aabb.center();

  auto gridPrimitive = mm.newPrimitive(
    PrimitiveBuilder(mm)
      .gridPatch(
        numVertexX, numVertexY, {center.x, 0, center.z}, {0, 0, 1}, {1, 0, 0},
        aabb.range().z / numVertexX, aabb.range().x / numVertexX)
      .newPrimitive(PrimitiveTopology::Patches));
  gridPrimitive->setAabb(aabb);
  gridPrimitive->setLod(lod);
  gridPrimitive->setTesselationLevel(
    lod ? glm::clamp(tesselationWidth, 1.f, 64.f) : tesselationWidth);

  auto gridMaterial = mm.newMaterial(MaterialType::eTerrain);
  auto gridMesh = mm.newMesh(gridPrimitive, gridMaterial);
  auto gridNode = mm.newNode();
  Node::addMesh(gridNode, gridMesh);
  auto gridModel = mm.newModel({gridNode});
  auto grid = mm.newModelInstance(gridModel);

  auto heightTex = mm.newGrayTexture(terrainFolder + "/" + heightMap);
  auto normalTex = mm.newTexture(terrainFolder + "/" + normalMap);
  auto albedoTex = mm.newTexture(terrainFolder + "/" + albedoMap);

  gridMaterial->setColorTex(albedoTex);
  gridMaterial->setNormalTex(normalTex);
  gridMaterial->setHeightTex(heightTex);
}

void TerrainManager::staticSeaLevel(const AABB &aabb, float seaLevelRatio) {
  vec3 center = aabb.center();
  float seaLevelHeight = clamp(seaLevelRatio, 0.f, 1.f) * aabb.range().y;
  auto horizonPrimitive =
    mm.newPrimitive(PrimitiveBuilder(mm)
                      .rectangle(
                        {center.x, aabb.min.y + seaLevelHeight, center.z},
                        {0, 0, aabb.halfRange().z}, {aabb.halfRange().x, 0, 0})
                      .newPrimitive());

  auto horizonMaterial = mm.newMaterial(MaterialType::eTranslucent);
  horizonMaterial->setColorFactor({39 / 255.f, 93 / 255.f, 121 / 255.f, 0.5f});
  auto horizonMesh = mm.newMesh(horizonPrimitive, horizonMaterial);
  auto horizonNode = mm.newNode();
  Node::addMesh(horizonNode, horizonMesh);
  auto horizonModel = mm.newModel({horizonNode});
  auto horizon = mm.newModelInstance(horizonModel);
}
}
#include "sim/graphics/renderer/default/model/default_model_manager.h"
#include "deferred_model.h"
#include "sim/graphics/util/materials.h"
namespace sim::graphics::renderer::defaultRenderer {
using namespace glm;
using namespace material;

DeferredModel::DeferredModel(
  Device &context, DebugMarker &debugMarker, const ModelConfig &config, bool enableIBL)
  : DefaultModelManager(context, config), debugMarker{debugMarker}, enableIBL{enableIBL} {
  auto size = config.maxNumMeshInstances * sizeof(vk::DrawIndexedIndirectCommand);
  drawCMDs = u<HostIndirectBuffer>(context.allocator(), size);
  idxMesh.resize(config.maxNumMeshes);

  size = config.maxNumMeshInstances * sizeof(vk::DrawIndexedIndirectCommand);
  if(enableIBL) loadSkybox();
}

void DeferredModel::loadSkybox() {
  auto skyboxModel = newModel([&](ModelBuilder &builder) {
    auto mesh = newMeshes([&](MeshBuilder &meshBuilder) {
      meshBuilder.box({}, {0.5f, 0, 0}, {0, 0.5f, 0}, 0.5f);
    })[0];
    builder.addMeshPart(mesh, newMaterial(Red));
  });
  auto skybox = newModelInstance(skyboxModel);
  auto &meshPart = skybox->node(0).meshParts[0];
  meshPart.visible = false;
  auto &mesh = meshPart.mesh.get();
  vk::DrawIndexedIndirectCommand drawCMD;
  drawCMD.instanceCount = 1;
  drawCMD.firstInstance = 0;
  drawCMD.indexCount = mesh.indexCount;
  drawCMD.firstIndex = mesh.indexOffset;
  drawCMD.vertexOffset = mesh.vertexOffset;

  drawSkyboxCMD = u<HostIndirectBuffer>(device.allocator(), drawCMD);

  SamplerMaker maker{};
  maker.addressModeU(vk::SamplerAddressMode::eClampToEdge)
    .addressModeV(vk::SamplerAddressMode::eClampToEdge)
    .addressModeW(vk::SamplerAddressMode::eClampToEdge)
    .maxLod(1.f)
    .maxAnisotropy(1.f)
    .borderColor(vk::BorderColor::eFloatOpaqueWhite);
  TextureImage2D brdfLUT{device, 512, 512, false, vk::Format::eR16G16Sfloat, true};
  brdfLUT.setSampler(maker.createUnique(device.getDevice()));
  debugMarker.name(brdfLUT.image(), "BRDF lut");
  Image.textures.push_back(std::move(brdfLUT));
  IBL.brdfLUT = Image.textures.size() - 1;

  TextureImageCube irradiance{device, 64, 64, calcMipLevels(64),
                              vk::Format::eR32G32B32A32Sfloat};
  debugMarker.name(irradiance.image(), "Irradiance EnvMap");
  maker.mipmapMode(vk::SamplerMipmapMode::eLinear)
    .maxLod(float(irradiance.getInfo().mipLevels));
  irradiance.setSampler(maker.createUnique(device.getDevice()));
  Image.cubeTextures.push_back(std::move(irradiance));
  IBL.irradiance = Image.cubeTextures.size() - 1;

  TextureImageCube prefiltered{device, 512, 512, calcMipLevels(512),
                               vk::Format::eR16G16B16A16Sfloat};
  debugMarker.name(prefiltered.image(), "PreFiltered EnvMap");
  maker.maxLod(float(prefiltered.getInfo().mipLevels));
  prefiltered.setSampler(maker.createUnique(device.getDevice()));
  IBL._shader_envMapInfo.prefilteredCubeMipLevels = prefiltered.getInfo().mipLevels;
  Image.cubeTextures.push_back(std::move(prefiltered));
  IBL.prefiltered = Image.cubeTextures.size() - 1;

  IBL.envMapInfoBuffer = u<HostUniformBuffer>(device.allocator(), IBL._shader_envMapInfo);
}

void DeferredModel::loadEnvironmentMap(const std::string &imagePath) {
  auto envTex = newCubeTex(imagePath, {}, true, 0);
  if(IBL.environment != envTex.texID) IBL.genEnvMap = true;
  IBL.environment = envTex.texID;
}

void DeferredModel::updateInstances() {
  DefaultModelManager::updateInstances();
  tri.size = 0;
  debugLine.size = 0;
  translucent.size = 0;

  visitMeshParts([&](MeshPart &meshPart) {
    switch(meshPart.mesh->primitive) {
      case Primitive::Triangles:
        switch(meshPart.material->flag) {
          case MaterialFlag::eBRDF:
          case MaterialFlag::eReflective:
          case MaterialFlag::eRefractive:
          case MaterialFlag::eNone: tri.size++; break;
          case MaterialFlag::eTranslucent: translucent.size++; break;
        }
        break;
      case Primitive::Lines: debugLine.size++; break;
      case Primitive::Procedural: break;
    }
  });

  tri.offset = 0;
  debugLine.offset = tri.endOffset();
  translucent.offset = debugLine.endOffset();
  auto triCMDs = drawCMDs->ptr<vk::DrawIndexedIndirectCommand>() + tri.offset;
  auto lineCMDs = drawCMDs->ptr<vk::DrawIndexedIndirectCommand>() + debugLine.offset;
  auto translucentCMDs =
    drawCMDs->ptr<vk::DrawIndexedIndirectCommand>() + translucent.offset;

  uint32_t idx{0}, idxTri{0}, idxLine{0}, idxTranslucent{0};
  visitMeshParts([&](MeshPart &meshPart) {
    auto &mesh = *meshPart.mesh;
    vk::DrawIndexedIndirectCommand drawCMD;
    drawCMD.instanceCount = meshPart.visible ? 1 : 0;
    drawCMD.firstInstance = idx;
    idx += 1;
    drawCMD.indexCount = mesh.indexCount;
    drawCMD.firstIndex = mesh.indexOffset;
    drawCMD.vertexOffset = mesh.vertexOffset;

    switch(mesh.primitive) {
      case Primitive::Triangles:
        switch(meshPart.material->flag) {
          case MaterialFlag::eBRDF:
          case MaterialFlag::eReflective:
          case MaterialFlag::eRefractive:
          case MaterialFlag::eNone: triCMDs[idxTri++] = drawCMD; break;
          case MaterialFlag::eTranslucent:
            translucentCMDs[idxTranslucent++] = drawCMD;
            break;
        }
        break;
      case Primitive::Lines: lineCMDs[idxLine++] = drawCMD; break;
      case Primitive::Procedural: break;
    }
  });
}

void DeferredModel::generateIBLTextures() {
  if(enableIBL) {
    generateBRDFLUT();
    generateEnvMap();
  }
}

void DeferredModel::printSceneInfo() {
  debugLog(
    "vertices: ", Geometry.vertexCount, " indices: ", Geometry.indexCount,
    " textures: ", Image.textures.size() + Image.cubeTextures.size(),
    " instances: ", tri.size + debugLine.size + translucent.size,
    " materials: ", Geometry.materials.size(),
    " transforms: ", Geometry.transforms.size());
}
}
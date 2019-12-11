#include "raytracing_model.h"

namespace sim::graphics::renderer::defaultRenderer {
using ASType = vk::AccelerationStructureTypeNV;
using access = vk::AccessFlagBits;
using stage = vk::PipelineStageFlagBits;
using memReqType = vk::AccelerationStructureMemoryRequirementsTypeNV;
using buildASFlag = vk::BuildAccelerationStructureFlagBitsNV;

RayTracingModel::RayTracingModel(Device &context, const ModelConfig &config)
  : DefaultModelManager(context, config), vkDevice{context.getDevice()} {
  tlas.instances.reserve(config.maxNumMeshInstances);
  auto size = config.maxNumMeshInstances * sizeof(VkGeometryInstance);
  tlas.instanceBuffer = u<HostRayTracingBuffer>(context.allocator(), size);

  meshBlas.resize(config.maxNumMeshes);
}

void RayTracingModel::createAS() {
  updateRTGeometry();
  for(auto &blas: blases)
    make_AS(
      blas, ASType::eBottomLevel, 0, uint32_t(blas.geometries.size()),
      blas.geometries.data());
  updateInstances();
  make_AS(tlas, ASType::eTopLevel, uint32_t(tlas.instances.size()), 0, nullptr);
  buildAS();
}

void RayTracingModel::updateRTGeometry() {
  blases.clear();
  blases.resize(Geometry.meshCountTri + Geometry.meshCountProc);
  if(mergeMeshes) {
    meshBlas.clear();
    meshBlas.resize(Geometry.meshes.size(), meshNoRef);
    modelBlas.clear();
    modelBlas.resize(Geometry.models.size(), meshNoRef);
  }
  auto blasIdx{0};
  for(size_t meshIdx = 0; meshIdx < Geometry.meshes.size(); meshIdx++) {
    auto &mesh = Geometry.meshes[meshIdx];
    switch(mesh.primitive) {
      case Primitive::Triangles: {
        uint32_t _blasIdx;
        if(mergeMeshes) switch(auto &ref = Geometry.meshRefByModel[meshIdx]) {
            case meshNoRef:
            case meshDupRef:
              _blasIdx = blasIdx;
              blasIdx++;
              break;
            default:
              if(modelBlas[ref] == meshNoRef) {
                modelBlas[ref] = blasIdx;
                _blasIdx = blasIdx;
                blasIdx++;
              } else
                _blasIdx = meshBlas[ref];
          }
        else {
          _blasIdx = blasIdx;
          blasIdx++;
        }
        blases[_blasIdx].geometries.emplace_back(
          vk::GeometryTypeNV::eTriangles,
          vk::GeometryDataNV{vk::GeometryTrianglesNV{
            Buffer.vertices->buffer(), mesh.vertexOffset * sizeof(Vertex),
            mesh.vertexCount, sizeof(Vertex), vk::Format::eR32G32B32Sfloat,
            Buffer.indices->buffer(), mesh.indexOffset * sizeof(uint32_t),
            mesh.indexCount, vk::IndexType::eUint32}},
          vk::GeometryFlagBitsNV::eOpaque);
        meshBlas[meshIdx] = _blasIdx;
        break;
      }
      case Primitive::Procedural:
        blases[blasIdx].geometries.emplace_back(
          vk::GeometryTypeNV::eAabbs,
          vk::GeometryDataNV{
            {},
            vk::GeometryAABBNV{Buffer.aabbs->buffer(), mesh.numAABBs, sizeof(AABB),
                               mesh.AABBoffset * sizeof(AABB)}},
          vk::GeometryFlagBitsNV::eOpaque);
        meshBlas[meshIdx] = blasIdx++;
        break;
      default: break;
    }
  }
  assert(blasIdx <= blases.size());
  blases.resize(blasIdx);
}

glm::mat4 calcTransform(uint32_t transformID, const std::vector<Transform> &transforms) {
  auto transform = transforms[transformID];
  auto model = transform.toMatrix();
  while(transform.getParentID() != -1) {
    transform = transforms[transform.getParentID()];
    auto parent = transform.toMatrix();
    model = parent * model;
  }
  return model;
}

void RayTracingModel::updateInstances() {
  DefaultModelManager::updateInstances();

  uint32_t sizeRT = 0;
  sizeDebugTri = 0;
  sizeDebugLine = 0;

  visitMeshParts([&](MeshPart &meshPart) {
    switch(meshPart.mesh->primitive) {
      case Primitive::Triangles:
        switch(meshPart.material->flag) {
          case MaterialFlag::eBRDF:
          case MaterialFlag::eReflective:
          case MaterialFlag::eRefractive: sizeRT++; break;
          case MaterialFlag::eNone: sizeDebugTri++; break;
          case MaterialFlag::eTranslucent: break;
        }
        break;
      case Primitive::Lines: sizeDebugLine++; break;
      case Primitive::Procedural: sizeRT++; break;
    }
  });

  uint32_t idx{0}, idxRT{0}, idxDebugTri{0}, idxLine{0};
  auto totalDebug = sizeDebugTri + sizeDebugLine;
  if(totalDebug > 0 && !drawCMDs_buffer) {
    auto size = config.maxNumMeshInstances * sizeof(vk::DrawIndexedIndirectCommand);
    drawCMDs_buffer = u<HostIndirectBuffer>(device.allocator(), size);
  }
  auto debugTriCMDs =
    drawCMDs_buffer ? drawCMDs_buffer->ptr<vk::DrawIndexedIndirectCommand>() : nullptr;
  auto lineCMDs = debugTriCMDs + sizeDebugTri;

  tlas.instances.clear();
  tlas.instances.resize(Instance.totalRef_tri + Instance.totalRef_proc);
  visitMeshParts([&](MeshPart &meshPart) {
    auto meshPtr = meshPart.mesh;
    auto &mesh = Geometry.meshes[meshPtr.index()];

    switch(mesh.primitive) {
      case Primitive::Lines: {
        vk::DrawIndexedIndirectCommand drawCMD;
        drawCMD.instanceCount = 1;
        drawCMD.indexCount = mesh.indexCount;
        drawCMD.firstIndex = mesh.indexOffset;
        drawCMD.vertexOffset = mesh.vertexOffset;
        drawCMD.firstInstance = idx;
        lineCMDs[idxLine++] = drawCMD;
        break;
      }
      case Primitive::Triangles: {
        uint32_t shaderOffset = 0;
        bool debugTri = false;
        switch(meshPart.material->flag) {
          case MaterialFlag::eBRDF: shaderOffset = 0; break;
          case MaterialFlag::eReflective: shaderOffset = 2; break;
          case MaterialFlag::eRefractive: shaderOffset = 4; break;
          case MaterialFlag::eNone: {
            debugTri = true;
            vk::DrawIndexedIndirectCommand drawCMD;
            drawCMD.instanceCount = 1;
            drawCMD.indexCount = mesh.indexCount;
            drawCMD.firstIndex = mesh.indexOffset;
            drawCMD.vertexOffset = mesh.vertexOffset;
            drawCMD.firstInstance = idx;
            debugTriCMDs[idxDebugTri++] = drawCMD;
            break;
          }
          case MaterialFlag::eTranslucent: debugTri = true; break;
        }
        if(debugTri) {

        } else {
          auto transform = calcTransform(meshPart.transform.index(), Geometry.transforms);
          tlas.instances[idxRT++] = {transform,
                                     idx,
                                     0xff,
                                     shaderOffset,
                                     vk::GeometryInstanceFlagBitsNV::eTriangleCullDisable,
                                     blases[meshBlas[meshPtr.index()]].handle};
        }
        break;
      }
      case Primitive::Procedural: {
        uint32_t shaderOffset = 0;
        switch(meshPart.material->flag) {
          case MaterialFlag::eBRDF: shaderOffset = 0; break;
          case MaterialFlag::eReflective: shaderOffset = 2; break;
          case MaterialFlag::eRefractive: shaderOffset = 4; break;
          case MaterialFlag::eNone: break;
          case MaterialFlag::eTranslucent: break;
        }

        auto transform = calcTransform(meshPart.transform.index(), Geometry.transforms);
        tlas.instances[idxRT++] = {transform,
                                   idx,
                                   0xff,
                                   shaderOffset + 6 * (1 + meshPtr->id),
                                   vk::GeometryInstanceFlagBitsNV::eTriangleCullDisable,
                                   blases[meshBlas[meshPtr.index()]].handle};
        break;
      }
    }
    idx++;
  });
  if(!tlas.instances.empty()) tlas.instanceBuffer->updateVector(tlas.instances);
}

void RayTracingModel::make_AS(
  AccelerationStructure &outAS, vk::AccelerationStructureTypeNV type,
  uint32_t instanceCount, uint32_t geometryCount, const vk::GeometryNV *geometries) {
  outAS.info = {type, buildASFlag::ePreferFastTrace, instanceCount, geometryCount,
                geometries};
  if(type != vk::AccelerationStructureTypeNV::eBottomLevel || rebuildBlas)
    outAS.info.flags |= buildASFlag::eAllowUpdate;

  outAS.as = vkDevice.createAccelerationStructureNVUnique({0, outAS.info}, nullptr);

  auto memReq = getASMemReq(*outAS.as, memReqType::eObject);
  outAS.buffer = u<DeviceRayTracingBuffer>(
    device.allocator(), memReq.memoryRequirements.size,
    memReq.memoryRequirements.memoryTypeBits);
  auto [mem, offset] = outAS.buffer->deviceMem();
  vk::BindAccelerationStructureMemoryInfoNV bindInfo{*outAS.as, mem, offset};
  vkDevice.bindAccelerationStructureMemoryNV(bindInfo);

  vkDevice.getAccelerationStructureHandleNV<uint64_t>(*outAS.as, outAS.handle);

  memReq = getASMemReq(*outAS.as, memReqType::eBuildScratch);
  outAS.scratchBuffer = u<DeviceRayTracingBuffer>(
    device.allocator(), memReq.memoryRequirements.size,
    memReq.memoryRequirements.memoryTypeBits);
}

void RayTracingModel::buildAS() {
  device.executeImmediately([&](vk::CommandBuffer cb) {
    vk::BufferMemoryBarrier barrier{
      access::eAccelerationStructureReadNV | access::eAccelerationStructureWriteNV,
      access::eAccelerationStructureReadNV | access::eAccelerationStructureWriteNV,
      VK_QUEUE_FAMILY_IGNORED,
      VK_QUEUE_FAMILY_IGNORED,
      {},
      0,
      VK_WHOLE_SIZE};
    for(auto &blas: blases) {
      cb.buildAccelerationStructureNV(
        blas.info, nullptr, 0, false, *blas.as, nullptr, blas.scratchBuffer->buffer(), 0);
      barrier.buffer = blas.scratchBuffer->buffer();
      cb.pipelineBarrier(
        stage::eAccelerationStructureBuildNV, stage::eAccelerationStructureBuildNV, {},
        nullptr, barrier, nullptr);
    }
    cb.buildAccelerationStructureNV(
      tlas.info, tlas.instanceBuffer->buffer(), 0, false, *tlas.as, nullptr,
      tlas.scratchBuffer->buffer(), 0);
    barrier.buffer = tlas.scratchBuffer->buffer();
    cb.pipelineBarrier(
      stage::eAccelerationStructureBuildNV, stage::eAccelerationStructureBuildNV, {},
      nullptr, barrier, nullptr);
  });
}

vk::MemoryRequirements2 RayTracingModel::getASMemReq(
  vk::AccelerationStructureNV &as, memReqType type) const {
  vk::AccelerationStructureMemoryRequirementsInfoNV memReqInfo{type, as};
  auto memReq = vkDevice.getAccelerationStructureMemoryRequirementsNV(memReqInfo);
  return memReq;
}

void RayTracingModel::updateAS(vk::CommandBuffer cb) {
  vk::BufferMemoryBarrier barrier{
    access::eAccelerationStructureReadNV | access::eAccelerationStructureWriteNV,
    access::eAccelerationStructureReadNV | access::eAccelerationStructureWriteNV,
    VK_QUEUE_FAMILY_IGNORED,
    VK_QUEUE_FAMILY_IGNORED,
    {},
    0,
    VK_WHOLE_SIZE};

  if(rebuildBlas)
    for(auto &blas: blases) {
      cb.buildAccelerationStructureNV(
        blas.info, nullptr, 0, true, *blas.as, *blas.as, blas.scratchBuffer->buffer(), 0);
      barrier.buffer = blas.scratchBuffer->buffer();
      cb.pipelineBarrier(
        stage::eAccelerationStructureBuildNV, stage::eAccelerationStructureBuildNV, {},
        nullptr, barrier, nullptr);
    }

  updateInstances();

  cb.buildAccelerationStructureNV(
    tlas.info, tlas.instanceBuffer->buffer(), 0, true, *tlas.as, *tlas.as,
    tlas.scratchBuffer->buffer(), 0);
  barrier.buffer = tlas.scratchBuffer->buffer();
  cb.pipelineBarrier(
    stage::eAccelerationStructureBuildNV, stage::eAccelerationStructureBuildNV, {},
    nullptr, barrier, nullptr);
}
}
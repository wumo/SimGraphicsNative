#include "buffers.h"

namespace sim::graphics {
BufferBase::BufferBase(
  const VmaAllocator &allocator, const vk::BufferUsageFlags &usageFlags,
  vk::DeviceSize sizeInBytes, VmaAllocationCreateInfo allocInfo) {
  std::vector<uint32_t> queueFamilyIndices = {0, 2};
  vk::BufferCreateInfo bufferInfo;
  bufferInfo.size = sizeInBytes;
  bufferInfo.usage = usageFlags;

  vmaBuffer = UniquePtr(new VmaBuffer{allocator}, [](VmaBuffer *ptr) {
    debugLog("deallocate buffer:", ptr->buffer);
    vmaDestroyBuffer(ptr->allocator, ptr->buffer, ptr->allocation);
    delete ptr;
  });
  auto result = vmaCreateBuffer(
    allocator, (VkBufferCreateInfo *)&bufferInfo, &allocInfo,
    reinterpret_cast<VkBuffer *>(&(vmaBuffer->buffer)), &vmaBuffer->allocation,
    &allocationInfo);
  errorIf(result != VK_SUCCESS, "failed to allocate buffer!");
  debugLog(
    "allocate buffer:", vmaBuffer->buffer, "[", allocationInfo.deviceMemory, "+",
    allocationInfo.offset, "]");
  VkMemoryPropertyFlags memFlags;
  vmaGetMemoryTypeProperties(vmaBuffer->allocator, allocationInfo.memoryType, &memFlags);
  if(
    (memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
    (memFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
    mappable = true;
  }
}

BufferBase::BufferBase(
  const VmaAllocator &allocator, const vk::BufferUsageFlags &usageFlags,
  vk::DeviceSize sizeInBytes, VmaMemoryUsage memoryUsage)
  : BufferBase(allocator, usageFlags, sizeInBytes, {{}, memoryUsage}) {}

const vk::Buffer &BufferBase::buffer() const { return vmaBuffer->buffer; }
void BufferBase::barrier(
  const vk::CommandBuffer &cb, vk::PipelineStageFlags srcStageMask,
  vk::PipelineStageFlags dstStageMask, vk::DependencyFlags dependencyFlags,
  vk::AccessFlags srcAccessMask, vk::AccessFlags dstAccessMask,
  uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex) {
  vk::BufferMemoryBarrier barrier{srcAccessMask,       dstAccessMask,
                                  srcQueueFamilyIndex, dstQueueFamilyIndex,
                                  vmaBuffer->buffer,   0,
                                  allocationInfo.size};
  cb.pipelineBarrier(
    srcStageMask, dstStageMask, dependencyFlags, nullptr, barrier, nullptr);
}

void DeviceBuffer::upload(
  Device &device, const void *value, vk::DeviceSize sizeInBytes,
  vk::DeviceSize dstOffsetInBytes) {
  if(sizeInBytes == 0) return;
  using buf = vk::BufferUsageFlagBits;
  using pfb = vk::MemoryPropertyFlagBits;
  auto tmp = HostBuffer{vmaBuffer->allocator, buf::eTransferSrc, sizeInBytes};
  tmp.updateRaw(value, sizeInBytes);
  device.executeImmediately([&](vk::CommandBuffer cb) {
    vk::BufferCopy copy{0, dstOffsetInBytes, sizeInBytes};
    cb.copyBuffer(tmp.buffer(), vmaBuffer->buffer, copy);
  });
}

void HostCoherentBuffer::updateRaw(
  const void *value, vk::DeviceSize sizeInBytes, vk::DeviceSize dstOffsetInBytes) {
  errorIf(!mappable, "This buffer is not mappable!");
  memcpy(ptr<std::byte>() + dstOffsetInBytes, value, size_t(sizeInBytes));
}

void HostCoherentBuffer::flush() {
  vmaFlushAllocation(vmaBuffer->allocator, vmaBuffer->allocation, 0, VK_WHOLE_SIZE);
}

void HostCoherentBuffer::invalidate() {
  vmaInvalidateAllocation(vmaBuffer->allocator, vmaBuffer->allocation, 0, VK_WHOLE_SIZE);
}

}

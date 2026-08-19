// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "gpuReadbackHelper.h"
#include "playerManager.h"
#include "handleMapService.h"
#include "log.h"

#include <cstring>

namespace gits {
namespace vulkan {

GpuReadbackHelper::GpuReadbackHelper(PlayerManager& playerManager) : m_Player(playerManager) {}

// ---------------------------------------------------------------------------
// Memory helpers
// ---------------------------------------------------------------------------

bool GpuReadbackHelper::IsHostVisible(uint64_t physDevKey, uint32_t memoryTypeIndex) {
  if (memoryTypeIndex == UINT32_MAX) {
    return false;
  }
  auto physDevice =
      reinterpret_cast<VkPhysicalDevice>(HandleMapService::Get().TryGetHandle(physDevKey));
  if (!physDevice) {
    return false;
  }
  VkPhysicalDeviceMemoryProperties props{};
  m_Player.GetInstanceDispatchTable(physDevice)
      .vkGetPhysicalDeviceMemoryProperties(physDevice, &props);
  if (memoryTypeIndex >= props.memoryTypeCount) {
    return false;
  }
  return (props.memoryTypes[memoryTypeIndex].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) !=
         0;
}

uint32_t GpuReadbackHelper::FindStagingMemoryTypeForPhysDevice(VkPhysicalDevice physDevice,
                                                               uint32_t memoryTypeBits) {
  VkPhysicalDeviceMemoryProperties props{};
  m_Player.GetInstanceDispatchTable(physDevice)
      .vkGetPhysicalDeviceMemoryProperties(physDevice, &props);

  constexpr VkMemoryPropertyFlags kRequired =
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  for (uint32_t i = 0; i < props.memoryTypeCount; ++i) {
    if ((memoryTypeBits & (1u << i)) &&
        (props.memoryTypes[i].propertyFlags & kRequired) == kRequired) {
      return i;
    }
  }
  // Fall back: HOST_VISIBLE without HOST_COHERENT (will need manual flush)
  for (uint32_t i = 0; i < props.memoryTypeCount; ++i) {
    if ((memoryTypeBits & (1u << i)) &&
        (props.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
      return i;
    }
  }
  return UINT32_MAX;
}

uint32_t GpuReadbackHelper::FindStagingMemoryType(uint64_t physDevKey, uint32_t memoryTypeBits) {
  auto physDevice =
      reinterpret_cast<VkPhysicalDevice>(HandleMapService::Get().TryGetHandle(physDevKey));
  if (!physDevice) {
    return UINT32_MAX;
  }
  return FindStagingMemoryTypeForPhysDevice(physDevice, memoryTypeBits);
}

bool GpuReadbackHelper::GetQueueFamilyProperties(uint64_t physDevKey,
                                                 std::vector<VkQueueFamilyProperties>& outProps) {
  outProps.clear();
  auto physDevice =
      reinterpret_cast<VkPhysicalDevice>(HandleMapService::Get().TryGetHandle(physDevKey));
  if (!physDevice) {
    return false;
  }
  auto& instDt = m_Player.GetInstanceDispatchTable(physDevice);
  uint32_t count = 0;
  instDt.vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &count, nullptr);
  if (count == 0) {
    return false;
  }
  outProps.resize(count);
  instDt.vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &count, outProps.data());
  // The driver may report back fewer families than the count query announced.
  outProps.resize(count);
  return !outProps.empty();
}

// ---------------------------------------------------------------------------
// QueryStagingBufferRequirements
//
// Create a throwaway VkBuffer with the requested size+usage, query its memory
// requirements via vkGetBufferMemoryRequirements, then immediately destroy the
// buffer.  The reported req.size (alignment-rounded) and req.memoryTypeBits
// (driver-allowed types) feed into the staging-buffer commands the recorder
// emits into the stream, so the second player's vkAllocateMemory +
// vkBindBufferMemory satisfy the actual driver requirements rather than
// guessing from the raw data length.
// ---------------------------------------------------------------------------
bool GpuReadbackHelper::QueryStagingBufferRequirements(uint64_t deviceKey,
                                                       VkDeviceSize size,
                                                       VkBufferUsageFlags usage,
                                                       VkMemoryRequirements& outReq) {
  VkBufferCreateInfo bci{};
  bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bci.size = size;
  bci.usage = usage;
  bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  return QueryBufferRequirements(deviceKey, bci, outReq);
}

// ---------------------------------------------------------------------------
// QueryBufferRequirements
// ---------------------------------------------------------------------------

bool GpuReadbackHelper::QueryBufferRequirements(uint64_t deviceKey,
                                                const VkBufferCreateInfo& createInfo,
                                                VkMemoryRequirements& outReq) {
  auto device = reinterpret_cast<VkDevice>(HandleMapService::Get().TryGetHandle(deviceKey));
  if (!device) {
    LOG_WARNING << "GpuReadbackHelper: QueryBufferRequirements: invalid device key=" << deviceKey;
    return false;
  }
  auto& dt = m_Player.GetDeviceDispatchTable(device);

  VkBuffer tempBuf = VK_NULL_HANDLE;
  if (dt.vkCreateBuffer(device, &createInfo, nullptr, &tempBuf) != VK_SUCCESS) {
    LOG_WARNING << "GpuReadbackHelper: QueryBufferRequirements: vkCreateBuffer failed";
    return false;
  }
  outReq = {};
  dt.vkGetBufferMemoryRequirements(device, tempBuf, &outReq);
  dt.vkDestroyBuffer(device, tempBuf, nullptr);
  return true;
}

bool GpuReadbackHelper::QueryImageRequirements(uint64_t deviceKey,
                                               const VkImageCreateInfo& createInfo,
                                               VkMemoryRequirements& outReq,
                                               bool& outRequiresDedicatedAllocation) {
  outRequiresDedicatedAllocation = false;
  auto device = reinterpret_cast<VkDevice>(HandleMapService::Get().TryGetHandle(deviceKey));
  if (!device) {
    LOG_WARNING << "GpuReadbackHelper: QueryImageRequirements: invalid device key=" << deviceKey;
    return false;
  }
  auto& dt = m_Player.GetDeviceDispatchTable(device);

  VkImage tempImg = VK_NULL_HANDLE;
  if (dt.vkCreateImage(device, &createInfo, nullptr, &tempImg) != VK_SUCCESS) {
    LOG_WARNING << "GpuReadbackHelper: QueryImageRequirements: vkCreateImage failed";
    return false;
  }
  outReq = {};
  // Prefer *2: it is the only entry point that can report
  // VkMemoryDedicatedRequirements - plain vkGetImageMemoryRequirements has no way to
  // signal that this image mandates a dedicated allocation
  // (VUID-vkBindImageMemory-image-01445). Both the 1.1 core and KHR entry points are
  // resolved unconditionally by the dispatch table (getProcAddr returns null for an
  // unsupported one), so fall back to the plain query only if neither is available.
  PFN_vkGetImageMemoryRequirements2 getReq2 = dt.vkGetImageMemoryRequirements2 != nullptr
                                                  ? dt.vkGetImageMemoryRequirements2
                                                  : dt.vkGetImageMemoryRequirements2KHR;
  if (getReq2 != nullptr) {
    VkImageMemoryRequirementsInfo2 info2{VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2};
    info2.image = tempImg;

    VkMemoryDedicatedRequirements dedicatedReq{VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS};
    VkMemoryRequirements2 req2{VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
    req2.pNext = &dedicatedReq;

    getReq2(device, &info2, &req2);
    outReq = req2.memoryRequirements;
    outRequiresDedicatedAllocation = (dedicatedReq.requiresDedicatedAllocation == VK_TRUE);
  } else {
    dt.vkGetImageMemoryRequirements(device, tempImg, &outReq);
  }
  dt.vkDestroyImage(device, tempImg, nullptr);
  return true;
}

uint32_t GpuReadbackHelper::FindCompatibleMemoryType(uint64_t physDevKey, uint32_t memoryTypeBits) {
  auto physDevice =
      reinterpret_cast<VkPhysicalDevice>(HandleMapService::Get().TryGetHandle(physDevKey));
  if (!physDevice) {
    return UINT32_MAX;
  }
  VkPhysicalDeviceMemoryProperties props{};
  m_Player.GetInstanceDispatchTable(physDevice)
      .vkGetPhysicalDeviceMemoryProperties(physDevice, &props);

  for (uint32_t i = 0; i < props.memoryTypeCount; ++i) {
    if ((memoryTypeBits & (1u << i)) &&
        (props.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
      return i;
    }
  }
  for (uint32_t i = 0; i < props.memoryTypeCount; ++i) {
    if (memoryTypeBits & (1u << i)) {
      return i;
    }
  }
  return UINT32_MAX;
}

VkDeviceAddress GpuReadbackHelper::QueryBufferDeviceAddress(uint64_t deviceKey,
                                                            uint64_t bufferKey) {
  auto device = reinterpret_cast<VkDevice>(HandleMapService::Get().TryGetHandle(deviceKey));
  GITS_ASSERT(device, "QueryBufferDeviceAddress: failed to get device by key");
  auto buffer = reinterpret_cast<VkBuffer>(HandleMapService::Get().TryGetHandle(bufferKey));
  GITS_ASSERT(buffer, "QueryBufferDeviceAddress: failed to get buffer by key");

  auto& dt = m_Player.GetDeviceDispatchTable(device);
  // A Vulkan 1.1 device carries only the extension entry point, KHR or the older EXT one.
  auto getBufferDeviceAddress = dt.vkGetBufferDeviceAddress      ? dt.vkGetBufferDeviceAddress
                                : dt.vkGetBufferDeviceAddressKHR ? dt.vkGetBufferDeviceAddressKHR
                                                                 : dt.vkGetBufferDeviceAddressEXT;
  GITS_ASSERT(getBufferDeviceAddress,
              "QueryBufferDeviceAddress: failed to get dispatchTable.getBufferDeviceAddress");

  VkBufferDeviceAddressInfo info{};
  info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
  info.buffer = buffer;
  return getBufferDeviceAddress(device, &info);
}

// ---------------------------------------------------------------------------
// AllocateStagingBuffer
// ---------------------------------------------------------------------------

bool GpuReadbackHelper::AllocateStagingBuffer(VkDevice device,
                                              VkPhysicalDevice physDevKey,
                                              VkDeviceSize size,
                                              VkBuffer& outBuf,
                                              VkDeviceMemory& outMem,
                                              void*& outMapped,
                                              VkBufferUsageFlags extraUsage,
                                              VkBufferCreateFlags extraCreateFlags) {
  auto& dt = m_Player.GetDeviceDispatchTable(device);

  VkBufferCreateInfo bci{};
  bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bci.flags = extraCreateFlags;
  bci.size = size;
  bci.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | extraUsage;
  bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (dt.vkCreateBuffer(device, &bci, nullptr, &outBuf) != VK_SUCCESS) {
    LOG_WARNING << "GpuReadbackHelper: vkCreateBuffer for staging failed";
    return false;
  }

  VkMemoryRequirements req{};
  dt.vkGetBufferMemoryRequirements(device, outBuf, &req);

  uint32_t memType = FindStagingMemoryTypeForPhysDevice(physDevKey, req.memoryTypeBits);
  if (memType == UINT32_MAX) {
    LOG_WARNING << "GpuReadbackHelper: no HOST_VISIBLE memory type for staging buffer";
    dt.vkDestroyBuffer(device, outBuf, nullptr);
    outBuf = VK_NULL_HANDLE;
    return false;
  }

  VkMemoryAllocateFlagsInfo allocFlagsInfo{};
  allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
  allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
  // If the capture/replay bit is set on the buffer, it must be set on the allocation too.
  const bool captureReplay =
      (extraCreateFlags & VK_BUFFER_CREATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT) != 0;
  if (captureReplay) {
    allocFlagsInfo.flags |= VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT;
  }

  VkMemoryAllocateInfo mai{};
  mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  if ((extraUsage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) || captureReplay) {
    mai.pNext = &allocFlagsInfo;
  }
  mai.allocationSize = req.size;
  mai.memoryTypeIndex = memType;

  if (dt.vkAllocateMemory(device, &mai, nullptr, &outMem) != VK_SUCCESS) {
    LOG_WARNING << "GpuReadbackHelper: vkAllocateMemory for staging failed";
    dt.vkDestroyBuffer(device, outBuf, nullptr);
    outBuf = VK_NULL_HANDLE;
    return false;
  }

  if (dt.vkBindBufferMemory(device, outBuf, outMem, 0) != VK_SUCCESS) {
    LOG_WARNING << "GpuReadbackHelper: vkBindBufferMemory for staging failed";
    dt.vkFreeMemory(device, outMem, nullptr);
    dt.vkDestroyBuffer(device, outBuf, nullptr);
    outBuf = VK_NULL_HANDLE;
    outMem = VK_NULL_HANDLE;
    return false;
  }

  if (dt.vkMapMemory(device, outMem, 0, VK_WHOLE_SIZE, 0, &outMapped) != VK_SUCCESS) {
    LOG_WARNING << "GpuReadbackHelper: vkMapMemory for staging failed";
    dt.vkFreeMemory(device, outMem, nullptr);
    dt.vkDestroyBuffer(device, outBuf, nullptr);
    outBuf = VK_NULL_HANDLE;
    outMem = VK_NULL_HANDLE;
    return false;
  }
  return true;
}

// ---------------------------------------------------------------------------
// SubmitOneShot
// ---------------------------------------------------------------------------

bool GpuReadbackHelper::SubmitOneShot(VkDevice device,
                                      VkQueue queue,
                                      VkCommandPool pool,
                                      std::function<void(VkCommandBuffer)> recordFn) {
  auto& dt = m_Player.GetDeviceDispatchTable(device);

  VkCommandBufferAllocateInfo ai{};
  ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  ai.commandPool = pool;
  ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  ai.commandBufferCount = 1;

  VkCommandBuffer cb = VK_NULL_HANDLE;
  if (dt.vkAllocateCommandBuffers(device, &ai, &cb) != VK_SUCCESS) {
    return false;
  }

  VkCommandBufferBeginInfo bi{};
  bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  dt.vkBeginCommandBuffer(cb, &bi);

  recordFn(cb);

  dt.vkEndCommandBuffer(cb);

  VkSubmitInfo si{};
  si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  si.commandBufferCount = 1;
  si.pCommandBuffers = &cb;
  VkResult result = dt.vkQueueSubmit(queue, 1, &si, VK_NULL_HANDLE);
  dt.vkQueueWaitIdle(queue);
  dt.vkFreeCommandBuffers(device, pool, 1, &cb);

  return result == VK_SUCCESS;
}

// ---------------------------------------------------------------------------
// ReadBuffer
// ---------------------------------------------------------------------------

bool GpuReadbackHelper::ReadBuffer(uint64_t deviceKey,
                                   uint64_t physDevKey,
                                   uint64_t queueKey,
                                   uint64_t commandPoolKey,
                                   uint64_t bufferKey,
                                   VkDeviceSize srcOffset,
                                   VkDeviceSize size,
                                   std::vector<uint8_t>& outData) {
  auto& hms = HandleMapService::Get();
  auto device = reinterpret_cast<VkDevice>(hms.TryGetHandle(deviceKey));
  auto physDevice = reinterpret_cast<VkPhysicalDevice>(hms.TryGetHandle(physDevKey));
  auto queue = reinterpret_cast<VkQueue>(hms.TryGetHandle(queueKey));
  auto pool = reinterpret_cast<VkCommandPool>(hms.TryGetHandle(commandPoolKey));
  auto buffer = reinterpret_cast<VkBuffer>(hms.TryGetHandle(bufferKey));

  if (!device || !physDevice || !queue || !pool || !buffer || size == 0) {
    return false;
  }

  auto& dt = m_Player.GetDeviceDispatchTable(device);

  VkBuffer stagingBuf = VK_NULL_HANDLE;
  VkDeviceMemory stagingMem = VK_NULL_HANDLE;
  void* mappedPtr = nullptr;

  if (!AllocateStagingBuffer(device, physDevice, size, stagingBuf, stagingMem, mappedPtr)) {
    return false;
  }

  bool ok = SubmitOneShot(device, queue, pool, [&](VkCommandBuffer cb) {
    // Barrier: ensure all prior writes to 'buffer' are visible for transfer read.
    VkBufferMemoryBarrier srcBarrier{};
    srcBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    srcBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
    srcBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    srcBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    srcBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    srcBarrier.buffer = buffer;
    srcBarrier.offset = 0;
    srcBarrier.size = VK_WHOLE_SIZE;
    dt.vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                            0, 0, nullptr, 1, &srcBarrier, 0, nullptr);

    VkBufferCopy region{srcOffset, 0, size};
    dt.vkCmdCopyBuffer(cb, buffer, stagingBuf, 1, &region);

    // Barrier: staging ? host-read.
    VkBufferMemoryBarrier dstBarrier{};
    dstBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    dstBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    dstBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    dstBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    dstBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    dstBarrier.buffer = stagingBuf;
    dstBarrier.offset = 0;
    dstBarrier.size = VK_WHOLE_SIZE;
    dt.vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 0,
                            nullptr, 1, &dstBarrier, 0, nullptr);
  });

  if (ok) {
    // Invalidate non-coherent memory before reading.
    VkMappedMemoryRange range{};
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.memory = stagingMem;
    range.offset = 0;
    range.size = VK_WHOLE_SIZE;
    dt.vkInvalidateMappedMemoryRanges(device, 1, &range);

    outData.resize(static_cast<size_t>(size));
    std::memcpy(outData.data(), mappedPtr, static_cast<size_t>(size));
  }

  dt.vkUnmapMemory(device, stagingMem);
  dt.vkDestroyBuffer(device, stagingBuf, nullptr);
  dt.vkFreeMemory(device, stagingMem, nullptr);

  return ok;
}

// ---------------------------------------------------------------------------
// StageBufferRegions / ReadStaged / FreeStaged / WaitQueueIdle
//
// In-command-buffer readback: the copy is recorded into the application's own command
// buffer, so it observes the input contents at the build's exact point in the
// submission. Read on the host once the submission completes.
// ---------------------------------------------------------------------------

bool GpuReadbackHelper::StageBufferRegions(uint64_t deviceKey,
                                           uint64_t physDevKey,
                                           uint64_t appCbKey,
                                           uint64_t srcBufferKey,
                                           const std::vector<CapturedBuildInputRegion>& regions,
                                           StagedInputReadback& outStaging) {
  outStaging = {};
  if (regions.empty()) {
    return false;
  }
  auto& hms = HandleMapService::Get();
  auto device = reinterpret_cast<VkDevice>(hms.TryGetHandle(deviceKey));
  auto physDevice = reinterpret_cast<VkPhysicalDevice>(hms.TryGetHandle(physDevKey));
  auto appCb = reinterpret_cast<VkCommandBuffer>(hms.TryGetHandle(appCbKey));
  auto srcBuffer = reinterpret_cast<VkBuffer>(hms.TryGetHandle(srcBufferKey));
  if (!device || !physDevice || !appCb || !srcBuffer) {
    return false;
  }

  // Pack regions sequentially into staging (region order == read order).
  VkDeviceSize total = 0;
  std::vector<VkBufferCopy> copies;
  copies.reserve(regions.size());
  for (const auto& r : regions) {
    if (r.RangeSize == 0) {
      continue;
    }
    copies.push_back({r.SrcOffset, total, r.RangeSize});
    total += r.RangeSize;
  }
  if (total == 0 || copies.empty()) {
    return false;
  }

  VkBuffer stagingBuf = VK_NULL_HANDLE;
  VkDeviceMemory stagingMem = VK_NULL_HANDLE;
  void* mapped = nullptr;
  if (!AllocateStagingBuffer(device, physDevice, total, stagingBuf, stagingMem, mapped)) {
    return false;
  }

  auto& dt = m_Player.GetDeviceDispatchTable(device);

  // Make prior writes to the source buffer visible to our transfer read.
  VkBufferMemoryBarrier preBarrier{};
  preBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  preBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
  preBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  preBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  preBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  preBarrier.buffer = srcBuffer;
  preBarrier.offset = 0;
  preBarrier.size = VK_WHOLE_SIZE;
  dt.vkCmdPipelineBarrier(appCb, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                          0, 0, nullptr, 1, &preBarrier, 0, nullptr);

  dt.vkCmdCopyBuffer(appCb, srcBuffer, stagingBuf, static_cast<uint32_t>(copies.size()),
                     copies.data());

  // Staging transfer-write -> host-read.
  VkBufferMemoryBarrier postBarrier{};
  postBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  postBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  postBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
  postBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  postBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  postBarrier.buffer = stagingBuf;
  postBarrier.offset = 0;
  postBarrier.size = VK_WHOLE_SIZE;
  dt.vkCmdPipelineBarrier(appCb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 0,
                          nullptr, 1, &postBarrier, 0, nullptr);

  outStaging.Device = device;
  outStaging.Buffer = stagingBuf;
  outStaging.Memory = stagingMem;
  outStaging.Mapped = mapped;
  outStaging.Size = total;
  return true;
}

bool GpuReadbackHelper::ReadStaged(const StagedInputReadback& staging,
                                   std::vector<uint8_t>& outData) {
  if (!staging.Device || !staging.Memory || !staging.Mapped || staging.Size == 0) {
    return false;
  }
  auto& dt = m_Player.GetDeviceDispatchTable(staging.Device);
  VkMappedMemoryRange range{};
  range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
  range.memory = staging.Memory;
  range.offset = 0;
  range.size = VK_WHOLE_SIZE;
  dt.vkInvalidateMappedMemoryRanges(staging.Device, 1, &range);
  outData.resize(static_cast<size_t>(staging.Size));
  std::memcpy(outData.data(), staging.Mapped, static_cast<size_t>(staging.Size));
  return true;
}

void GpuReadbackHelper::FreeStaged(const StagedInputReadback& staging) {
  if (!staging.Device) {
    return;
  }
  auto& dt = m_Player.GetDeviceDispatchTable(staging.Device);
  if (staging.Memory) {
    dt.vkUnmapMemory(staging.Device, staging.Memory);
  }
  if (staging.Buffer) {
    dt.vkDestroyBuffer(staging.Device, staging.Buffer, nullptr);
  }
  if (staging.Memory) {
    dt.vkFreeMemory(staging.Device, staging.Memory, nullptr);
  }
}

bool GpuReadbackHelper::WaitQueueIdle(uint64_t deviceKey, uint64_t queueKey) {
  auto& hms = HandleMapService::Get();
  auto device = reinterpret_cast<VkDevice>(hms.TryGetHandle(deviceKey));
  auto queue = reinterpret_cast<VkQueue>(hms.TryGetHandle(queueKey));
  if (!device || !queue) {
    return false;
  }
  auto& dt = m_Player.GetDeviceDispatchTable(device);
  return dt.vkQueueWaitIdle(queue) == VK_SUCCESS;
}

// ---------------------------------------------------------------------------
// ReadAccelerationStructureSerialized
// ---------------------------------------------------------------------------

bool GpuReadbackHelper::ReadAccelerationStructureSerialized(
    uint64_t deviceKey,
    uint64_t physDevKey,
    uint64_t queueKey,
    uint64_t commandPoolKey,
    uint64_t accelerationStructureKey,
    std::vector<uint8_t>& outData,
    VkDeviceAddress& outDeviceAddress,
    uint64_t& outOpaqueCaptureAddress,
    uint64_t& outMemoryOpaqueCaptureAddress) {
  auto& hms = HandleMapService::Get();
  auto device = reinterpret_cast<VkDevice>(hms.TryGetHandle(deviceKey));
  auto physDevice = reinterpret_cast<VkPhysicalDevice>(hms.TryGetHandle(physDevKey));
  auto queue = reinterpret_cast<VkQueue>(hms.TryGetHandle(queueKey));
  auto pool = reinterpret_cast<VkCommandPool>(hms.TryGetHandle(commandPoolKey));
  auto accelStruct =
      reinterpret_cast<VkAccelerationStructureKHR>(hms.TryGetHandle(accelerationStructureKey));

  if (!device || !physDevice || !queue || !pool || !accelStruct) {
    return false;
  }

  auto& dt = m_Player.GetDeviceDispatchTable(device);

  // Step 1: query the serialized size. The staging buffer below cannot be sized
  // before it is known.
  VkQueryPoolCreateInfo qpci{};
  qpci.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
  qpci.queryType = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_SIZE_KHR;
  qpci.queryCount = 1;

  VkQueryPool queryPool = VK_NULL_HANDLE;
  if (dt.vkCreateQueryPool(device, &qpci, nullptr, &queryPool) != VK_SUCCESS) {
    LOG_WARNING << "GpuReadbackHelper: vkCreateQueryPool for AS serialization size failed";
    return false;
  }

  bool queried = SubmitOneShot(device, queue, pool, [&](VkCommandBuffer cb) {
    dt.vkCmdResetQueryPool(cb, queryPool, 0, 1);
    dt.vkCmdWriteAccelerationStructuresPropertiesKHR(
        cb, 1, &accelStruct, VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_SIZE_KHR, queryPool,
        0);
  });

  uint64_t serializedSize = 0;
  VkResult queryResult = VK_ERROR_UNKNOWN;
  if (queried) {
    queryResult = dt.vkGetQueryPoolResults(device, queryPool, 0, 1, sizeof(serializedSize),
                                           &serializedSize, sizeof(serializedSize),
                                           VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
  }
  dt.vkDestroyQueryPool(device, queryPool, nullptr);

  if (!queried || queryResult != VK_SUCCESS || serializedSize == 0) {
    LOG_WARNING << "GpuReadbackHelper: failed to query AS serialized size for key="
                << accelerationStructureKey;
    return false;
  }

  // Step 2: allocate the staging buffer for the serialized bytes. SHADER_DEVICE_ADDRESS
  // + ACCELERATION_STRUCTURE_STORAGE are required because the serialize copy addresses
  // its destination by raw VkDeviceAddress. CAPTURE_REPLAY makes that address
  // reproducible, so the caller can hardcode it into the deserialize-restore commands.
  VkBuffer stagingBuf = VK_NULL_HANDLE;
  VkDeviceMemory stagingMem = VK_NULL_HANDLE;
  void* mappedPtr = nullptr;

  if (!AllocateStagingBuffer(device, physDevice, serializedSize, stagingBuf, stagingMem, mappedPtr,
                             VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                 VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                             VK_BUFFER_CREATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT)) {
    return false;
  }

  VkBufferDeviceAddressInfo addressInfo{};
  addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
  addressInfo.buffer = stagingBuf;
  auto vkGetBufferDeviceAddressUnified =
      dt.vkGetBufferDeviceAddress ? dt.vkGetBufferDeviceAddress : dt.vkGetBufferDeviceAddressKHR;
  VkDeviceAddress stagingAddress = vkGetBufferDeviceAddressUnified(device, &addressInfo);

  auto vkGetBufferOpaqueCaptureAddressUnified = dt.vkGetBufferOpaqueCaptureAddress
                                                    ? dt.vkGetBufferOpaqueCaptureAddress
                                                    : dt.vkGetBufferOpaqueCaptureAddressKHR;
  uint64_t stagingOpaqueAddress = vkGetBufferOpaqueCaptureAddressUnified(device, &addressInfo);

  // Both the buffer- and memory-side opaque addresses must be re-supplied on replay
  // to reproduce stagingAddress.
  VkDeviceMemoryOpaqueCaptureAddressInfo memAddressInfo{};
  memAddressInfo.sType = VK_STRUCTURE_TYPE_DEVICE_MEMORY_OPAQUE_CAPTURE_ADDRESS_INFO;
  memAddressInfo.memory = stagingMem;
  auto vkGetDeviceMemoryOpaqueCaptureAddressUnified =
      dt.vkGetDeviceMemoryOpaqueCaptureAddress ? dt.vkGetDeviceMemoryOpaqueCaptureAddress
                                               : dt.vkGetDeviceMemoryOpaqueCaptureAddressKHR;
  uint64_t stagingMemoryOpaqueAddress =
      vkGetDeviceMemoryOpaqueCaptureAddressUnified(device, &memAddressInfo);

  // Step 3: serialize the AS into the staging buffer.
  bool ok = SubmitOneShot(device, queue, pool, [&](VkCommandBuffer cb) {
    // Barrier: prior writes to the AS -> serialize copy.
    VkMemoryBarrier preBarrier{};
    preBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    preBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
    preBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dt.vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                            VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1,
                            &preBarrier, 0, nullptr, 0, nullptr);

    VkCopyAccelerationStructureToMemoryInfoKHR copyInfo{};
    copyInfo.sType = VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_TO_MEMORY_INFO_KHR;
    copyInfo.src = accelStruct;
    copyInfo.dst.deviceAddress = stagingAddress;
    copyInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_SERIALIZE_KHR;
    dt.vkCmdCopyAccelerationStructureToMemoryKHR(cb, &copyInfo);

    // Barrier: staging buffer write -> host read.
    VkBufferMemoryBarrier dstBarrier{};
    dstBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    dstBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
    dstBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    dstBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    dstBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    dstBarrier.buffer = stagingBuf;
    dstBarrier.offset = 0;
    dstBarrier.size = VK_WHOLE_SIZE;
    dt.vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                            VK_PIPELINE_STAGE_HOST_BIT, 0, 0, nullptr, 1, &dstBarrier, 0, nullptr);
  });

  if (ok) {
    VkMappedMemoryRange range{};
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.memory = stagingMem;
    range.offset = 0;
    range.size = VK_WHOLE_SIZE;
    dt.vkInvalidateMappedMemoryRanges(device, 1, &range);

    outData.resize(static_cast<size_t>(serializedSize));
    std::memcpy(outData.data(), mappedPtr, static_cast<size_t>(serializedSize));
    outDeviceAddress = stagingAddress;
    outOpaqueCaptureAddress = stagingOpaqueAddress;
    outMemoryOpaqueCaptureAddress = stagingMemoryOpaqueAddress;
  }

  dt.vkUnmapMemory(device, stagingMem);
  dt.vkDestroyBuffer(device, stagingBuf, nullptr);
  dt.vkFreeMemory(device, stagingMem, nullptr);

  return ok;
}

// ---------------------------------------------------------------------------
// QueryAccelerationStructureBuildSizes
// ---------------------------------------------------------------------------
bool GpuReadbackHelper::QueryAccelerationStructureBuildSizes(
    uint64_t deviceKey,
    const VkAccelerationStructureBuildGeometryInfoKHR& buildInfo,
    const uint32_t* pMaxPrimitiveCounts,
    VkAccelerationStructureBuildSizesInfoKHR& outSizes) {
  auto device = reinterpret_cast<VkDevice>(HandleMapService::Get().TryGetHandle(deviceKey));
  if (!device) {
    LOG_WARNING << "GpuReadbackHelper: QueryAccelerationStructureBuildSizes: invalid device key="
                << deviceKey;
    return false;
  }
  auto& dt = m_Player.GetDeviceDispatchTable(device);

  outSizes = {};
  outSizes.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
  dt.vkGetAccelerationStructureBuildSizesKHR(device,
                                             VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                             &buildInfo, pMaxPrimitiveCounts, &outSizes);
  return true;
}

// ---------------------------------------------------------------------------
// ReserveScratchBufferAddress
//
// Creates a throwaway buffer with capture/replay addressing and captures its
// addresses, so a restored build can use them for its scratch buffer.
// ---------------------------------------------------------------------------
bool GpuReadbackHelper::ReserveScratchBufferAddress(uint64_t deviceKey,
                                                    uint64_t physDevKey,
                                                    VkDeviceSize size,
                                                    VkDeviceAddress& outDeviceAddress,
                                                    uint64_t& outOpaqueCaptureAddress,
                                                    uint64_t& outMemoryOpaqueCaptureAddress) {
  auto& hms = HandleMapService::Get();
  auto device = reinterpret_cast<VkDevice>(hms.TryGetHandle(deviceKey));
  auto physDevice = reinterpret_cast<VkPhysicalDevice>(hms.TryGetHandle(physDevKey));
  if (!device || !physDevice) {
    LOG_WARNING << "GpuReadbackHelper: ReserveScratchBufferAddress: invalid device/physDevice key";
    return false;
  }
  auto& dt = m_Player.GetDeviceDispatchTable(device);

  VkBuffer scratchBuf = VK_NULL_HANDLE;
  VkDeviceMemory scratchMem = VK_NULL_HANDLE;
  void* mappedPtr = nullptr;
  if (!AllocateStagingBuffer(device, physDevice, size, scratchBuf, scratchMem, mappedPtr,
                             VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                             VK_BUFFER_CREATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT)) {
    return false;
  }

  VkBufferDeviceAddressInfo addressInfo{};
  addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
  addressInfo.buffer = scratchBuf;
  auto vkGetBufferDeviceAddressUnified =
      dt.vkGetBufferDeviceAddress ? dt.vkGetBufferDeviceAddress : dt.vkGetBufferDeviceAddressKHR;
  outDeviceAddress = vkGetBufferDeviceAddressUnified(device, &addressInfo);

  auto vkGetBufferOpaqueCaptureAddressUnified = dt.vkGetBufferOpaqueCaptureAddress
                                                    ? dt.vkGetBufferOpaqueCaptureAddress
                                                    : dt.vkGetBufferOpaqueCaptureAddressKHR;
  outOpaqueCaptureAddress = vkGetBufferOpaqueCaptureAddressUnified(device, &addressInfo);

  VkDeviceMemoryOpaqueCaptureAddressInfo memAddressInfo{};
  memAddressInfo.sType = VK_STRUCTURE_TYPE_DEVICE_MEMORY_OPAQUE_CAPTURE_ADDRESS_INFO;
  memAddressInfo.memory = scratchMem;
  auto vkGetDeviceMemoryOpaqueCaptureAddressUnified =
      dt.vkGetDeviceMemoryOpaqueCaptureAddress ? dt.vkGetDeviceMemoryOpaqueCaptureAddress
                                               : dt.vkGetDeviceMemoryOpaqueCaptureAddressKHR;
  outMemoryOpaqueCaptureAddress =
      vkGetDeviceMemoryOpaqueCaptureAddressUnified(device, &memAddressInfo);

  dt.vkUnmapMemory(device, scratchMem);
  // Kept alive so a subsequent reservation does not get the same address back.
  // ReleaseReservedAddresses tears them all down.
  m_ReservedAddressBuffers.push_back({device, scratchBuf, scratchMem});

  return true;
}

void GpuReadbackHelper::ReleaseReservedAddresses() {
  for (const auto& r : m_ReservedAddressBuffers) {
    auto& dt = m_Player.GetDeviceDispatchTable(r.Device);
    dt.vkDestroyBuffer(r.Device, r.Buffer, nullptr);
    dt.vkFreeMemory(r.Device, r.Memory, nullptr);
  }
  m_ReservedAddressBuffers.clear();
}

// ---------------------------------------------------------------------------
// ReadImage
// ---------------------------------------------------------------------------

namespace {

struct FormatBlockInfo {
  uint32_t BlockWidth{1};
  uint32_t BlockHeight{1};
  uint32_t BytesPerBlock{4};
  bool IsDepthStencil{false};
  uint32_t DepthBytes{0};   // 0 = no depth
  uint32_t StencilBytes{0}; // 0 = no stencil
  // Multi-planar YCbCr formats: copy requires one region per plane with
  // VK_IMAGE_ASPECT_PLANE_*_BIT and per-plane extent/stride.
  bool IsMultiPlanar{false};
  uint8_t PlaneCount{0};
  struct PlaneDesc {
    uint32_t WidthDivisor{1};  // plane width  = image width  / WidthDivisor
    uint32_t HeightDivisor{1}; // plane height = image height / HeightDivisor
    uint32_t BytesPerPixel{1};
  } Planes[3]{};
};

// warnOnUnknown is cleared by callers that only need the aspect mask: those run
// for every restored image, not just the ones actually read back, and the
// fallback block size they would be warned about does not affect their result.
static FormatBlockInfo GetFormatBlockInfo(VkFormat fmt, bool warnOnUnknown = true) {
  switch (fmt) {
  // ---- Single-channel ----
  case VK_FORMAT_R8_UNORM:
  case VK_FORMAT_R8_SNORM:
  case VK_FORMAT_R8_UINT:
  case VK_FORMAT_R8_SINT:
  case VK_FORMAT_R8_SRGB:
    return {1, 1, 1};
  case VK_FORMAT_R16_UNORM:
  case VK_FORMAT_R16_SNORM:
  case VK_FORMAT_R16_UINT:
  case VK_FORMAT_R16_SINT:
  case VK_FORMAT_R16_SFLOAT:
    return {1, 1, 2};
  case VK_FORMAT_R32_UINT:
  case VK_FORMAT_R32_SINT:
  case VK_FORMAT_R32_SFLOAT:
    return {1, 1, 4};
  // ---- Two-channel ----
  case VK_FORMAT_R8G8_UNORM:
  case VK_FORMAT_R8G8_SNORM:
  case VK_FORMAT_R8G8_UINT:
  case VK_FORMAT_R8G8_SINT:
  case VK_FORMAT_R8G8_SRGB:
    return {1, 1, 2};
  case VK_FORMAT_R16G16_SFLOAT:
  case VK_FORMAT_R16G16_UNORM:
  case VK_FORMAT_R16G16_SNORM:
  case VK_FORMAT_R16G16_UINT:
  case VK_FORMAT_R16G16_SINT:
    return {1, 1, 4};
  case VK_FORMAT_R32G32_SFLOAT:
  case VK_FORMAT_R32G32_UINT:
  case VK_FORMAT_R32G32_SINT:
    return {1, 1, 8};
  // ---- Four-channel (most common) ----
  case VK_FORMAT_R8G8B8A8_UNORM:
  case VK_FORMAT_R8G8B8A8_SNORM:
  case VK_FORMAT_R8G8B8A8_UINT:
  case VK_FORMAT_R8G8B8A8_SINT:
  case VK_FORMAT_R8G8B8A8_SRGB:
  case VK_FORMAT_B8G8R8A8_UNORM:
  case VK_FORMAT_B8G8R8A8_SNORM:
  case VK_FORMAT_B8G8R8A8_SRGB:
  case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
  case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
  case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
  case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
  case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
  case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
    return {1, 1, 4};
  case VK_FORMAT_R16G16B16A16_UNORM:
  case VK_FORMAT_R16G16B16A16_SNORM:
  case VK_FORMAT_R16G16B16A16_UINT:
  case VK_FORMAT_R16G16B16A16_SINT:
  case VK_FORMAT_R16G16B16A16_SFLOAT:
    return {1, 1, 8};
  case VK_FORMAT_R32G32B32A32_UINT:
  case VK_FORMAT_R32G32B32A32_SINT:
  case VK_FORMAT_R32G32B32A32_SFLOAT:
    return {1, 1, 16};
  // ---- BC compressed ----
  case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
  case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
  case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
  case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
  case VK_FORMAT_BC4_UNORM_BLOCK:
  case VK_FORMAT_BC4_SNORM_BLOCK:
    return {4, 4, 8};
  case VK_FORMAT_BC2_UNORM_BLOCK:
  case VK_FORMAT_BC2_SRGB_BLOCK:
  case VK_FORMAT_BC3_UNORM_BLOCK:
  case VK_FORMAT_BC3_SRGB_BLOCK:
  case VK_FORMAT_BC5_UNORM_BLOCK:
  case VK_FORMAT_BC5_SNORM_BLOCK:
  case VK_FORMAT_BC6H_UFLOAT_BLOCK:
  case VK_FORMAT_BC6H_SFLOAT_BLOCK:
  case VK_FORMAT_BC7_UNORM_BLOCK:
  case VK_FORMAT_BC7_SRGB_BLOCK:
    return {4, 4, 16};
  // ---- YCbCr packed single-plane ----
  // 4 bytes cover a 2-wide texel pair (macropixel).
  case VK_FORMAT_G8B8G8R8_422_UNORM:
  case VK_FORMAT_B8G8R8G8_422_UNORM:
    return {2, 1, 4};
  // ---- YCbCr multi-planar ----
  // 2-plane 4:2:0 (NV12 / P010 / P012 / P016):
  //   plane 0 = luma  (Y),    full resolution, bytesPerPixel=1 (8b) or 2 (10/12/16b)
  //   plane 1 = chroma (CbCr), half width+height, bytesPerPixel=2 or 4
  case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM: {
    FormatBlockInfo fi{};
    fi.IsMultiPlanar = true;
    fi.PlaneCount = 2;
    fi.Planes[0] = {1, 1, 1};
    fi.Planes[1] = {2, 2, 2};
    return fi;
  }
  case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
  case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
  case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM: {
    FormatBlockInfo fi{};
    fi.IsMultiPlanar = true;
    fi.PlaneCount = 2;
    fi.Planes[0] = {1, 1, 2};
    fi.Planes[1] = {2, 2, 4};
    return fi;
  }
  // 3-plane 4:2:0 (I420 / YV12 variants):
  //   plane 0 = Y (full), plane 1 = Cb (quarter area), plane 2 = Cr (quarter area)
  case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM: {
    FormatBlockInfo fi{};
    fi.IsMultiPlanar = true;
    fi.PlaneCount = 3;
    fi.Planes[0] = {1, 1, 1};
    fi.Planes[1] = {2, 2, 1};
    fi.Planes[2] = {2, 2, 1};
    return fi;
  }
  case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
  case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
  case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM: {
    FormatBlockInfo fi{};
    fi.IsMultiPlanar = true;
    fi.PlaneCount = 3;
    fi.Planes[0] = {1, 1, 2};
    fi.Planes[1] = {2, 2, 2};
    fi.Planes[2] = {2, 2, 2};
    return fi;
  }
  // 2-plane 4:2:2: chroma is half-width but full-height
  case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM: {
    FormatBlockInfo fi{};
    fi.IsMultiPlanar = true;
    fi.PlaneCount = 2;
    fi.Planes[0] = {1, 1, 1};
    fi.Planes[1] = {2, 1, 2};
    return fi;
  }
  case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
  case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
  case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM: {
    FormatBlockInfo fi{};
    fi.IsMultiPlanar = true;
    fi.PlaneCount = 2;
    fi.Planes[0] = {1, 1, 2};
    fi.Planes[1] = {2, 1, 4};
    return fi;
  }
  // 3-plane 4:2:2: chroma is half-width, full-height, per-channel
  case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM: {
    FormatBlockInfo fi{};
    fi.IsMultiPlanar = true;
    fi.PlaneCount = 3;
    fi.Planes[0] = {1, 1, 1};
    fi.Planes[1] = {2, 1, 1};
    fi.Planes[2] = {2, 1, 1};
    return fi;
  }
  case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
  case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
  case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM: {
    FormatBlockInfo fi{};
    fi.IsMultiPlanar = true;
    fi.PlaneCount = 3;
    fi.Planes[0] = {1, 1, 2};
    fi.Planes[1] = {2, 1, 2};
    fi.Planes[2] = {2, 1, 2};
    return fi;
  }
  // 3-plane 4:4:4: no chroma subsampling, all planes full resolution
  case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM: {
    FormatBlockInfo fi{};
    fi.IsMultiPlanar = true;
    fi.PlaneCount = 3;
    fi.Planes[0] = {1, 1, 1};
    fi.Planes[1] = {1, 1, 1};
    fi.Planes[2] = {1, 1, 1};
    return fi;
  }
  case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
  case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
  case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM: {
    FormatBlockInfo fi{};
    fi.IsMultiPlanar = true;
    fi.PlaneCount = 3;
    fi.Planes[0] = {1, 1, 2};
    fi.Planes[1] = {1, 1, 2};
    fi.Planes[2] = {1, 1, 2};
    return fi;
  }
  // ---- Depth / stencil ----
  case VK_FORMAT_D16_UNORM:
    return {1, 1, 0, true, 2, 0};
  case VK_FORMAT_X8_D24_UNORM_PACK32:
  case VK_FORMAT_D32_SFLOAT:
    return {1, 1, 0, true, 4, 0};
  case VK_FORMAT_S8_UINT:
    return {1, 1, 0, true, 0, 1};
  case VK_FORMAT_D16_UNORM_S8_UINT:
    return {1, 1, 0, true, 2, 1};
  case VK_FORMAT_D24_UNORM_S8_UINT:
    return {1, 1, 0, true, 4, 1};
  case VK_FORMAT_D32_SFLOAT_S8_UINT:
    return {1, 1, 0, true, 4, 1};
  default:
    if (warnOnUnknown) {
      LOG_WARNING << "GpuReadbackHelper: unknown format " << static_cast<uint32_t>(fmt)
                  << " for image readback, assuming 4 bytes/pixel";
    }
    return {1, 1, 4};
  }
}

// Returns total staging bytes needed and populates outRegions (one per subresource).
static VkDeviceSize ComputeImageStagingLayout(VkFormat format,
                                              const VkExtent3D& extent,
                                              uint32_t mipLevels,
                                              uint32_t arrayLayers,
                                              std::vector<VkBufferImageCopy>& outRegions) {
  const auto fi = GetFormatBlockInfo(format);
  VkDeviceSize offset = 0;

  auto addRegion = [&](VkImageAspectFlags aspect, uint32_t layer, uint32_t mip, uint32_t w,
                       uint32_t h, uint32_t d, uint32_t bw, uint32_t bh, uint32_t bytesPerBlock) {
    // Buffer offsets must be aligned to at least 4 bytes (spec requirement).
    const VkDeviceSize align = std::max(4u, bytesPerBlock);
    offset = (offset + align - 1) & ~(align - 1);

    VkBufferImageCopy region{};
    region.bufferOffset = offset;
    region.bufferRowLength = 0; // tight packing
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = aspect;
    region.imageSubresource.mipLevel = mip;
    region.imageSubresource.baseArrayLayer = layer;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {w, h, d};
    outRegions.push_back(region);

    uint32_t blocksX = (w + bw - 1) / bw;
    uint32_t blocksY = (h + bh - 1) / bh;
    offset += static_cast<VkDeviceSize>(blocksX) * blocksY * d * bytesPerBlock;
  };

  for (uint32_t layer = 0; layer < arrayLayers; ++layer) {
    for (uint32_t mip = 0; mip < mipLevels; ++mip) {
      uint32_t w = std::max(1u, extent.width >> mip);
      uint32_t h = std::max(1u, extent.height >> mip);
      uint32_t d = std::max(1u, extent.depth >> mip);

      if (fi.IsMultiPlanar) {
        static constexpr VkImageAspectFlags kPlaneAspect[3] = {
            VK_IMAGE_ASPECT_PLANE_0_BIT,
            VK_IMAGE_ASPECT_PLANE_1_BIT,
            VK_IMAGE_ASPECT_PLANE_2_BIT,
        };
        for (uint8_t p = 0; p < fi.PlaneCount; ++p) {
          uint32_t pw = std::max(1u, w / fi.Planes[p].WidthDivisor);
          uint32_t ph = std::max(1u, h / fi.Planes[p].HeightDivisor);
          addRegion(kPlaneAspect[p], layer, mip, pw, ph, d, 1, 1, fi.Planes[p].BytesPerPixel);
        }
      } else if (!fi.IsDepthStencil) {
        addRegion(VK_IMAGE_ASPECT_COLOR_BIT, layer, mip, w, h, d, fi.BlockWidth, fi.BlockHeight,
                  fi.BytesPerBlock);
      } else {
        if (fi.DepthBytes > 0) {
          addRegion(VK_IMAGE_ASPECT_DEPTH_BIT, layer, mip, w, h, d, 1, 1, fi.DepthBytes);
        }
        if (fi.StencilBytes > 0) {
          addRegion(VK_IMAGE_ASPECT_STENCIL_BIT, layer, mip, w, h, d, 1, 1, fi.StencilBytes);
        }
      }
    }
  }
  return offset;
}

} // anonymous namespace

VkImageAspectFlags AspectMaskForFormat(VkFormat format, bool disjoint) {
  const auto fi = GetFormatBlockInfo(format, /*warnOnUnknown=*/false);
  if (fi.IsDepthStencil) {
    VkImageAspectFlags aspect = 0;
    if (fi.DepthBytes > 0) {
      aspect |= VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    if (fi.StencilBytes > 0) {
      aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    return aspect;
  }
  // A barrier on a disjoint multi-planar image must name its plane(s)
  // (VUID-VkImageMemoryBarrier-image-01672 forbids COLOR there); a
  // non-disjoint one must use COLOR instead (VUID-VkImageMemoryBarrier-image-
  // 09242 forbids plane bits there).  Copy regions are unaffected by this:
  // each one already names a single plane bit, built separately from this
  // mask (VUID-vkCmdCopyBufferToImage-dstImage-07981).
  if (fi.IsMultiPlanar && disjoint) {
    VkImageAspectFlags aspect = VK_IMAGE_ASPECT_PLANE_0_BIT | VK_IMAGE_ASPECT_PLANE_1_BIT;
    if (fi.PlaneCount >= 3) {
      aspect |= VK_IMAGE_ASPECT_PLANE_2_BIT;
    }
    return aspect;
  }
  return VK_IMAGE_ASPECT_COLOR_BIT;
}

bool GpuReadbackHelper::ReadImage(uint64_t deviceKey,
                                  uint64_t physDevKey,
                                  uint64_t queueKey,
                                  uint64_t commandPoolKey,
                                  uint64_t imageKey,
                                  VkFormat format,
                                  const VkExtent3D& extent,
                                  uint32_t mipLevels,
                                  uint32_t arrayLayers,
                                  VkSampleCountFlagBits samples,
                                  VkImageLayout currentLayout,
                                  bool disjoint,
                                  std::vector<uint8_t>& outData,
                                  std::vector<VkBufferImageCopy>& outRegions) {
  // Multisampled images cannot be copied with vkCmdCopyImageToBuffer.
  if (samples != VK_SAMPLE_COUNT_1_BIT) {
    return false;
  }
  // Zero-size images have nothing to copy.
  if (extent.width == 0 || extent.height == 0 || extent.depth == 0) {
    return false;
  }

  auto& hms = HandleMapService::Get();
  auto device = reinterpret_cast<VkDevice>(hms.TryGetHandle(deviceKey));
  auto physDevice = reinterpret_cast<VkPhysicalDevice>(hms.TryGetHandle(physDevKey));
  auto queue = reinterpret_cast<VkQueue>(hms.TryGetHandle(queueKey));
  auto pool = reinterpret_cast<VkCommandPool>(hms.TryGetHandle(commandPoolKey));
  auto image = reinterpret_cast<VkImage>(hms.TryGetHandle(imageKey));

  if (!device || !physDevice || !queue || !pool || !image) {
    return false;
  }

  // Compute staging buffer layout.
  VkDeviceSize stagingSize =
      ComputeImageStagingLayout(format, extent, mipLevels, arrayLayers, outRegions);
  if (stagingSize == 0 || outRegions.empty()) {
    return false;
  }

  auto& dt = m_Player.GetDeviceDispatchTable(device);

  VkBuffer stagingBuf = VK_NULL_HANDLE;
  VkDeviceMemory stagingMem = VK_NULL_HANDLE;
  void* mappedPtr = nullptr;

  if (!AllocateStagingBuffer(device, physDevice, stagingSize, stagingBuf, stagingMem, mappedPtr)) {
    outRegions.clear();
    return false;
  }

  // Determine the aspect mask for layout transitions.
  const VkImageAspectFlags transitionAspect = AspectMaskForFormat(format, disjoint);

  bool ok = SubmitOneShot(device, queue, pool, [&](VkCommandBuffer cb) {
    // Transition image: currentLayout ? TRANSFER_SRC_OPTIMAL.  No ownership
    // transfer is attempted here: an EXCLUSIVE image's queue family must
    // already be the one this is submitted on, since a legal transfer needs
    // a release submitted on the owning queue and a matching acquire
    // submitted on this one, which a single one-shot submission cannot do
    // (see MaySubmitImageOperationOnQueueFamily, the caller that
    // enforces this precondition for depth/stencil images).
    VkImageMemoryBarrier toSrc{};
    toSrc.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toSrc.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
    toSrc.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    toSrc.oldLayout = currentLayout;
    toSrc.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    toSrc.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toSrc.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toSrc.image = image;
    toSrc.subresourceRange = {transitionAspect, 0, VK_REMAINING_MIP_LEVELS, 0,
                              VK_REMAINING_ARRAY_LAYERS};
    dt.vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                            0, 0, nullptr, 0, nullptr, 1, &toSrc);

    // Copy all subresources to staging buffer.
    dt.vkCmdCopyImageToBuffer(cb, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuf,
                              static_cast<uint32_t>(outRegions.size()), outRegions.data());

    // Transition image back: TRANSFER_SRC_OPTIMAL ? currentLayout.
    VkImageMemoryBarrier restore{};
    restore.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    restore.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    restore.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    restore.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    restore.newLayout = currentLayout;
    restore.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    restore.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    restore.image = image;
    restore.subresourceRange = {transitionAspect, 0, VK_REMAINING_MIP_LEVELS, 0,
                                VK_REMAINING_ARRAY_LAYERS};
    dt.vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                            0, 0, nullptr, 0, nullptr, 1, &restore);

    // Staging buffer barrier: TRANSFER_WRITE ? HOST_READ.
    VkBufferMemoryBarrier dstBarrier{};
    dstBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    dstBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    dstBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    dstBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    dstBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    dstBarrier.buffer = stagingBuf;
    dstBarrier.offset = 0;
    dstBarrier.size = VK_WHOLE_SIZE;
    dt.vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 0,
                            nullptr, 1, &dstBarrier, 0, nullptr);
  });

  if (ok) {
    VkMappedMemoryRange range{};
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.memory = stagingMem;
    range.offset = 0;
    range.size = VK_WHOLE_SIZE;
    dt.vkInvalidateMappedMemoryRanges(device, 1, &range);

    outData.resize(static_cast<size_t>(stagingSize));
    std::memcpy(outData.data(), mappedPtr, static_cast<size_t>(stagingSize));
  } else {
    outRegions.clear();
  }

  dt.vkUnmapMemory(device, stagingMem);
  dt.vkDestroyBuffer(device, stagingBuf, nullptr);
  dt.vkFreeMemory(device, stagingMem, nullptr);

  return ok;
}

VkDeviceSize GpuReadbackHelper::GetImageStagingLayout(VkFormat format,
                                                      const VkExtent3D& extent,
                                                      uint32_t mipLevels,
                                                      uint32_t arrayLayers,
                                                      std::vector<VkBufferImageCopy>& outRegions) {
  if (extent.width == 0 || extent.height == 0 || extent.depth == 0) {
    return 0;
  }
  return ComputeImageStagingLayout(format, extent, mipLevels, arrayLayers, outRegions);
}

} // namespace vulkan
} // namespace gits

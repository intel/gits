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
#include "enumToStrAuto.h"
#include "log.h"

#include <algorithm>
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
// AllocateStagingBufferShrinking
// ---------------------------------------------------------------------------

bool GpuReadbackHelper::AllocateStagingBufferShrinking(VkDevice device,
                                                       VkPhysicalDevice physDevice,
                                                       VkDeviceSize preferredSize,
                                                       VkDeviceSize minSize,
                                                       VkBuffer& outBuf,
                                                       VkDeviceMemory& outMem,
                                                       void*& outMapped,
                                                       VkDeviceSize& outSize) {
  for (VkDeviceSize size = std::max(preferredSize, minSize);; size /= 2) {
    const VkDeviceSize attempt = std::max(size, minSize);
    if (AllocateStagingBuffer(device, physDevice, attempt, outBuf, outMem, outMapped)) {
      outSize = attempt;
      return true;
    }
    if (attempt == minSize) {
      return false;
    }
  }
}

// ---------------------------------------------------------------------------
// SubmitOneShot
// ---------------------------------------------------------------------------

bool GpuReadbackHelper::SubmitOneShot(VkDevice device,
                                      VkQueue queue,
                                      VkCommandPool pool,
                                      std::function<void(VkCommandBuffer)> recordFn,
                                      const char* what) {
  auto& dt = m_Player.GetDeviceDispatchTable(device);

  VkCommandBufferAllocateInfo ai{};
  ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  ai.commandPool = pool;
  ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  ai.commandBufferCount = 1;

  VkCommandBuffer cb = VK_NULL_HANDLE;
  VkResult allocResult = dt.vkAllocateCommandBuffers(device, &ai, &cb);
  if (allocResult != VK_SUCCESS) {
    LOG_WARNING << "GpuReadbackHelper: vkAllocateCommandBuffers for " << what << " failed "
                << toStr(allocResult);
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
  VkResult waitResult = dt.vkQueueWaitIdle(queue);
  dt.vkFreeCommandBuffers(device, pool, 1, &cb);

  if (result != VK_SUCCESS) {
    LOG_WARNING << "GpuReadbackHelper: vkQueueSubmit for " << what << " failed " << toStr(result);
    return false;
  }
  if (waitResult != VK_SUCCESS) {
    LOG_WARNING << "GpuReadbackHelper: vkQueueWaitIdle after " << what << " failed "
                << toStr(waitResult);
    return false;
  }
  return true;
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
// QueryMicromapBuildSizes
// ---------------------------------------------------------------------------
bool GpuReadbackHelper::QueryMicromapBuildSizes(uint64_t deviceKey,
                                                const VkMicromapBuildInfoEXT& buildInfo,
                                                VkMicromapBuildSizesInfoEXT& outSizes) {
  auto device = reinterpret_cast<VkDevice>(HandleMapService::Get().TryGetHandle(deviceKey));
  if (!device) {
    LOG_WARNING << "GpuReadbackHelper: QueryMicromapBuildSizes: invalid device key=" << deviceKey;
    return false;
  }
  auto& dt = m_Player.GetDeviceDispatchTable(device);
  if (!dt.vkGetMicromapBuildSizesEXT) {
    LOG_WARNING << "GpuReadbackHelper: QueryMicromapBuildSizes: vkGetMicromapBuildSizesEXT is not "
                   "available on device key="
                << deviceKey;
    return false;
  }

  // The sizes depend only on pUsageCounts/ppUsageCounts, so zero the handle members in a local
  // copy - the decoded ones belong to the recorded process and must not reach the driver.
  VkMicromapBuildInfoEXT sizeQueryInfo = buildInfo;
  sizeQueryInfo.dstMicromap = VK_NULL_HANDLE;

  outSizes = {};
  outSizes.sType = VK_STRUCTURE_TYPE_MICROMAP_BUILD_SIZES_INFO_EXT;
  dt.vkGetMicromapBuildSizesEXT(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                &sizeQueryInfo, &outSizes);
  return true;
}

// ---------------------------------------------------------------------------
// ReserveFreshBufferAddress
//
// Creates a throwaway buffer with capture/replay addressing and captures its
// addresses, so a buffer the stream creates later can be placed at them.
// ---------------------------------------------------------------------------
bool GpuReadbackHelper::ReserveFreshBufferAddress(uint64_t deviceKey,
                                                  uint64_t physDevKey,
                                                  VkDeviceSize size,
                                                  VkDeviceAddress& outDeviceAddress,
                                                  uint64_t& outOpaqueCaptureAddress,
                                                  uint64_t& outMemoryOpaqueCaptureAddress) {
  auto& hms = HandleMapService::Get();
  auto device = reinterpret_cast<VkDevice>(hms.TryGetHandle(deviceKey));
  auto physDevice = reinterpret_cast<VkPhysicalDevice>(hms.TryGetHandle(physDevKey));
  if (!device || !physDevice) {
    LOG_WARNING << "GpuReadbackHelper: ReserveFreshBufferAddress: invalid device/physDevice key";
    return false;
  }
  auto& dt = m_Player.GetDeviceDispatchTable(device);

  VkBuffer buffer = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;
  void* mappedPtr = nullptr;
  if (!AllocateStagingBuffer(device, physDevice, size, buffer, memory, mappedPtr,
                             VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                             VK_BUFFER_CREATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT)) {
    return false;
  }

  VkBufferDeviceAddressInfo addressInfo{};
  addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
  addressInfo.buffer = buffer;
  auto vkGetBufferDeviceAddressUnified =
      dt.vkGetBufferDeviceAddress ? dt.vkGetBufferDeviceAddress : dt.vkGetBufferDeviceAddressKHR;
  outDeviceAddress = vkGetBufferDeviceAddressUnified(device, &addressInfo);

  auto vkGetBufferOpaqueCaptureAddressUnified = dt.vkGetBufferOpaqueCaptureAddress
                                                    ? dt.vkGetBufferOpaqueCaptureAddress
                                                    : dt.vkGetBufferOpaqueCaptureAddressKHR;
  outOpaqueCaptureAddress = vkGetBufferOpaqueCaptureAddressUnified(device, &addressInfo);

  VkDeviceMemoryOpaqueCaptureAddressInfo memAddressInfo{};
  memAddressInfo.sType = VK_STRUCTURE_TYPE_DEVICE_MEMORY_OPAQUE_CAPTURE_ADDRESS_INFO;
  memAddressInfo.memory = memory;
  auto vkGetDeviceMemoryOpaqueCaptureAddressUnified =
      dt.vkGetDeviceMemoryOpaqueCaptureAddress ? dt.vkGetDeviceMemoryOpaqueCaptureAddress
                                               : dt.vkGetDeviceMemoryOpaqueCaptureAddressKHR;
  outMemoryOpaqueCaptureAddress =
      vkGetDeviceMemoryOpaqueCaptureAddressUnified(device, &memAddressInfo);

  dt.vkUnmapMemory(device, memory);
  // Kept alive so a subsequent reservation does not get the same address back.
  // ReleaseReservedAddressesSince tears them down.
  m_ReservedAddressBuffers.push_back({device, buffer, memory});

  return true;
}

// ---------------------------------------------------------------------------
// ReserveCapturedBufferAddress
//
// Creates a live buffer and allocation at caller-supplied capture/replay addresses, so the
// driver cannot hand that range to a later reservation while the stream still has a buffer
// there. Mirrors the create info the stream uses for the same buffer.
// ---------------------------------------------------------------------------
bool GpuReadbackHelper::ReserveCapturedBufferAddress(uint64_t deviceKey,
                                                     VkDeviceSize size,
                                                     VkBufferUsageFlags usage,
                                                     VkDeviceSize allocationSize,
                                                     uint32_t memoryTypeIndex,
                                                     uint64_t opaqueCaptureAddress,
                                                     uint64_t memoryOpaqueCaptureAddress) {
  auto device = reinterpret_cast<VkDevice>(HandleMapService::Get().TryGetHandle(deviceKey));
  if (!device) {
    LOG_WARNING << "GpuReadbackHelper: ReserveCapturedBufferAddress: invalid device key";
    return false;
  }
  auto& dt = m_Player.GetDeviceDispatchTable(device);

  VkBufferOpaqueCaptureAddressCreateInfo opaqueAddrCI{};
  opaqueAddrCI.sType = VK_STRUCTURE_TYPE_BUFFER_OPAQUE_CAPTURE_ADDRESS_CREATE_INFO;
  opaqueAddrCI.opaqueCaptureAddress = opaqueCaptureAddress;

  VkBufferCreateInfo bci{};
  bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bci.pNext = &opaqueAddrCI;
  bci.flags = VK_BUFFER_CREATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT;
  bci.size = size;
  bci.usage = usage | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
  bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkBuffer buffer = VK_NULL_HANDLE;
  if (dt.vkCreateBuffer(device, &bci, nullptr, &buffer) != VK_SUCCESS) {
    return false;
  }

  VkMemoryOpaqueCaptureAddressAllocateInfo memOpaqueAddrCI{};
  memOpaqueAddrCI.sType = VK_STRUCTURE_TYPE_MEMORY_OPAQUE_CAPTURE_ADDRESS_ALLOCATE_INFO;
  memOpaqueAddrCI.opaqueCaptureAddress = memoryOpaqueCaptureAddress;

  VkMemoryAllocateFlagsInfo allocFlagsInfo{};
  allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
  allocFlagsInfo.pNext = &memOpaqueAddrCI;
  allocFlagsInfo.flags =
      VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT | VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT;

  VkMemoryAllocateInfo mai{};
  mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  mai.pNext = &allocFlagsInfo;
  mai.allocationSize = allocationSize;
  mai.memoryTypeIndex = memoryTypeIndex;

  VkDeviceMemory memory = VK_NULL_HANDLE;
  if (dt.vkAllocateMemory(device, &mai, nullptr, &memory) != VK_SUCCESS) {
    dt.vkDestroyBuffer(device, buffer, nullptr);
    return false;
  }
  if (dt.vkBindBufferMemory(device, buffer, memory, 0) != VK_SUCCESS) {
    dt.vkFreeMemory(device, memory, nullptr);
    dt.vkDestroyBuffer(device, buffer, nullptr);
    return false;
  }

  m_ReservedAddressBuffers.push_back({device, buffer, memory});
  return true;
}

size_t GpuReadbackHelper::MarkReservedAddresses() {
  return m_ReservedAddressBuffers.size();
}

void GpuReadbackHelper::ReleaseReservedAddressesSince(size_t mark) {
  if (mark >= m_ReservedAddressBuffers.size()) {
    return;
  }
  for (size_t i = mark; i < m_ReservedAddressBuffers.size(); ++i) {
    const auto& r = m_ReservedAddressBuffers[i];
    auto& dt = m_Player.GetDeviceDispatchTable(r.Device);
    dt.vkDestroyBuffer(r.Device, r.Buffer, nullptr);
    dt.vkFreeMemory(r.Device, r.Memory, nullptr);
  }
  m_ReservedAddressBuffers.resize(mark);
}

// ---------------------------------------------------------------------------
// ReadImages
// ---------------------------------------------------------------------------

namespace {

// Formats missing from the table are read back as if they had 4 bytes per texel.
VkDeviceSize ReadbackStagingLayout(VkFormat format,
                                   const VkExtent3D& extent,
                                   uint32_t mipLevels,
                                   uint32_t arrayLayers,
                                   std::vector<VkBufferImageCopy>& outRegions) {
  if (!GetFormatBlockInfo(format).Known) {
    LOG_WARNING << "GpuReadbackHelper: unknown format " << static_cast<uint32_t>(format)
                << " for image readback, assuming 4 bytes/pixel";
  }
  return ComputeImageStagingLayout(format, extent, mipLevels, arrayLayers, outRegions);
}

} // anonymous namespace

bool GpuReadbackHelper::ReadImages(uint64_t deviceKey,
                                   uint64_t physDevKey,
                                   uint64_t queueKey,
                                   uint64_t commandPoolKey,
                                   const std::vector<ImageReadbackRequest>& requests,
                                   const ImageReadbackSink& onImage) {
  // Trimmed below to the total to be read and raised
  // to the largest single image, so neither a tiny nor a huge set overshoots.
  constexpr VkDeviceSize kStagingBudget = 64ull * 1024 * 1024;

  auto& hms = HandleMapService::Get();
  auto device = reinterpret_cast<VkDevice>(hms.TryGetHandle(deviceKey));
  auto physDevice = reinterpret_cast<VkPhysicalDevice>(hms.TryGetHandle(physDevKey));
  auto queue = reinterpret_cast<VkQueue>(hms.TryGetHandle(queueKey));
  auto pool = reinterpret_cast<VkCommandPool>(hms.TryGetHandle(commandPoolKey));

  // onImage must fire exactly once per request, in request order: the caller streams one
  // token per manifest entry and the player pairs them up by position. Successes are
  // delivered as their batch completes, and the gaps left by requests that never made it
  // into a batch are filled in just before the next success.
  size_t nextToDeliver = 0;
  auto deliverGap = [&](size_t upTo) {
    while (nextToDeliver < upTo) {
      onImage(nextToDeliver++, nullptr, 0);
    }
  };
  auto deliver = [&](size_t index, const uint8_t* data, VkDeviceSize size) {
    deliverGap(index);
    onImage(index, data, size);
    nextToDeliver = index + 1;
  };

  if (!device || !physDevice || !queue || !pool) {
    deliverGap(requests.size());
    return false;
  }

  // Staging layout per request, computed once up front so the packing below knows every
  // size. Requests that cannot be copied at all - multisampled (vkCmdCopyImageToBuffer
  // rejects them), zero-extent, unresolvable handle, empty layout - never enter a batch
  // and are left to deliverGap.
  struct Prepared {
    size_t Index;
    VkImage Image;
    VkDeviceSize Size;
    VkDeviceSize Alignment;
    std::vector<VkBufferImageCopy> Regions;
  };
  std::vector<Prepared> prepared;
  prepared.reserve(requests.size());
  VkDeviceSize largest = 0;
  VkDeviceSize total = 0;

  for (size_t i = 0; i < requests.size(); ++i) {
    const auto& r = requests[i];
    if (r.Samples != VK_SAMPLE_COUNT_1_BIT || r.Extent.width == 0 || r.Extent.height == 0 ||
        r.Extent.depth == 0) {
      continue;
    }
    auto image = reinterpret_cast<VkImage>(hms.TryGetHandle(r.ImageKey));
    if (!image) {
      continue;
    }
    Prepared p;
    p.Index = i;
    p.Image = image;
    p.Size = ReadbackStagingLayout(r.Format, r.Extent, r.MipLevels, r.ArrayLayers, p.Regions);
    if (p.Size == 0 || p.Regions.empty()) {
      continue;
    }
    p.Alignment = StagingOffsetAlignment(r.Format);
    largest = std::max(largest, p.Size);
    total += p.Size;
    prepared.push_back(std::move(p));
  }

  if (prepared.empty()) {
    deliverGap(requests.size());
    return false;
  }

  VkBuffer stagingBuf = VK_NULL_HANDLE;
  VkDeviceMemory stagingMem = VK_NULL_HANDLE;
  void* mappedPtr = nullptr;
  VkDeviceSize stagingSize = 0;
  if (!AllocateStagingBufferShrinking(device, physDevice, std::min(kStagingBudget, total), largest,
                                      stagingBuf, stagingMem, mappedPtr, stagingSize)) {
    LOG_WARNING << "GpuReadbackHelper: no staging buffer for image readback - the largest image "
                   "alone needs "
                << largest << " bytes";
    deliverGap(requests.size());
    return false;
  }

  auto& dt = m_Player.GetDeviceDispatchTable(device);

  // Images packed at disjoint offsets of the one staging buffer, copied by a single
  // command buffer. Only the batch is waited on, not each image.
  struct BatchEntry {
    const Prepared* Item;
    VkDeviceSize Offset;
  };
  std::vector<BatchEntry> batch;
  std::vector<VkBufferImageCopy> regions; // rebased copy of one entry's regions
  size_t batchCount = 0;
  size_t readCount = 0;

  auto flush = [&]() {
    if (batch.empty()) {
      return;
    }
    ++batchCount;

    const bool ok = SubmitOneShot(
        device, queue, pool,
        [&](VkCommandBuffer cb) {
          // One barrier array for the whole batch, so a batch costs two barrier commands
          // rather than two per image.
          std::vector<VkImageMemoryBarrier> barriers;
          barriers.reserve(batch.size());
          for (const auto& e : batch) {
            const auto& r = requests[e.Item->Index];
            VkImageMemoryBarrier b{};
            b.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            b.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
            b.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            b.oldLayout = r.CurrentLayout;
            b.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            // No ownership transfer is attempted: an EXCLUSIVE image's queue family must
            // already be the one this is submitted on, since a legal transfer needs a
            // release submitted on the owning queue and a matching acquire submitted on
            // this one, which a single one-shot submission cannot do (see
            // MaySubmitImageOperationOnQueueFamily, the caller that enforces this
            // precondition for depth/stencil images).
            b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            b.image = e.Item->Image;
            b.subresourceRange = {AspectMaskForFormat(r.Format, r.Disjoint), 0,
                                  VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS};
            barriers.push_back(b);
          }
          dt.vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                  VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr,
                                  static_cast<uint32_t>(barriers.size()), barriers.data());

          for (const auto& e : batch) {
            regions = e.Item->Regions;
            for (auto& region : regions) {
              region.bufferOffset += e.Offset;
            }
            dt.vkCmdCopyImageToBuffer(cb, e.Item->Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                      stagingBuf, static_cast<uint32_t>(regions.size()),
                                      regions.data());
          }

          // The same barriers reversed: TRANSFER_SRC_OPTIMAL back to each image's layout.
          for (auto& b : barriers) {
            std::swap(b.oldLayout, b.newLayout);
            b.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            b.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
          }
          dt.vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                  VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr,
                                  static_cast<uint32_t>(barriers.size()), barriers.data());

          VkBufferMemoryBarrier dstBarrier{};
          dstBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
          dstBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
          dstBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
          dstBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
          dstBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
          dstBarrier.buffer = stagingBuf;
          dstBarrier.offset = 0;
          dstBarrier.size = VK_WHOLE_SIZE;
          dt.vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0,
                                  0, nullptr, 1, &dstBarrier, 0, nullptr);
        },
        "image content readback");

    if (ok) {
      VkMappedMemoryRange range{};
      range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
      range.memory = stagingMem;
      range.offset = 0;
      range.size = VK_WHOLE_SIZE;
      dt.vkInvalidateMappedMemoryRanges(device, 1, &range);
    }

    // The bytes stay in the staging buffer: onImage consumes them before the next batch
    // overwrites them, so peak host memory is the budget rather than the total.
    for (const auto& e : batch) {
      if (ok) {
        deliver(e.Item->Index, static_cast<const uint8_t*>(mappedPtr) + e.Offset, e.Item->Size);
        ++readCount;
      } else {
        deliver(e.Item->Index, nullptr, 0);
      }
    }
    batch.clear();
  };

  VkDeviceSize cursor = 0;
  for (const auto& item : prepared) {
    VkDeviceSize offset = (cursor + item.Alignment - 1) & ~(item.Alignment - 1);
    if (offset + item.Size > stagingSize) {
      flush();
      cursor = 0;
      offset = 0;
    }
    if (item.Size > stagingSize) {
      // Unreachable while stagingSize >= largest, but flush first either way so a pending
      // batch's lower indices are delivered before this one.
      flush();
      deliver(item.Index, nullptr, 0);
      continue;
    }
    batch.push_back({&item, offset});
    cursor = offset + item.Size;
  }
  flush();
  deliverGap(requests.size());

  dt.vkUnmapMemory(device, stagingMem);
  dt.vkDestroyBuffer(device, stagingBuf, nullptr);
  dt.vkFreeMemory(device, stagingMem, nullptr);

  LOG_INFO << "Vulkan subcapture: read back " << readCount << " of " << requests.size()
           << " image(s) in " << batchCount << " submission(s) through a "
           << (stagingSize / (1024 * 1024)) << " MB staging buffer";

  return readCount > 0;
}

VkDeviceSize GpuReadbackHelper::GetImageStagingLayout(VkFormat format,
                                                      const VkExtent3D& extent,
                                                      uint32_t mipLevels,
                                                      uint32_t arrayLayers,
                                                      std::vector<VkBufferImageCopy>& outRegions) {
  if (extent.width == 0 || extent.height == 0 || extent.depth == 0) {
    return 0;
  }
  return ReadbackStagingLayout(format, extent, mipLevels, arrayLayers, outRegions);
}

} // namespace vulkan
} // namespace gits

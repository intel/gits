// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "stateTrackingService.h" // for IGpuReadbackHelper

#include <functional>

namespace gits {
namespace vulkan {

class PlayerManager;

// Concrete implementation of IGpuReadbackHelper. Uses PlayerManager dispatch
// tables (GetDeviceDispatchTable / GetInstanceDispatchTable), same pattern as DX12 player.
class GpuReadbackHelper final : public IGpuReadbackHelper {
public:
  explicit GpuReadbackHelper(PlayerManager& playerManager);

  bool IsHostVisible(uint64_t physDevKey, uint32_t memoryTypeIndex) override;

  uint32_t FindStagingMemoryType(uint64_t physDevKey, uint32_t memoryTypeBits) override;

  bool GetQueueFamilyProperties(uint64_t physDevKey,
                                std::vector<VkQueueFamilyProperties>& outProps) override;

  bool QueryStagingBufferRequirements(uint64_t deviceKey,
                                      VkDeviceSize size,
                                      VkBufferUsageFlags usage,
                                      VkMemoryRequirements& outReq) override;

  bool QueryBufferRequirements(uint64_t deviceKey,
                               const VkBufferCreateInfo& createInfo,
                               VkMemoryRequirements& outReq) override;

  // Query a bound buffer because a parent trim may omit the original address command.
  VkDeviceAddress QueryBufferDeviceAddress(uint64_t deviceKey, uint64_t bufferKey);

  bool ReadBuffer(uint64_t deviceKey,
                  uint64_t physDevKey,
                  uint64_t queueKey,
                  uint64_t commandPoolKey,
                  uint64_t bufferKey,
                  VkDeviceSize srcOffset,
                  VkDeviceSize size,
                  std::vector<uint8_t>& outData) override;

  VkDeviceSize GetImageStagingLayout(VkFormat format,
                                     const VkExtent3D& extent,
                                     uint32_t mipLevels,
                                     uint32_t arrayLayers,
                                     std::vector<VkBufferImageCopy>& outRegions) override;

  bool StageBufferRegions(uint64_t deviceKey,
                          uint64_t physDevKey,
                          uint64_t appCbKey,
                          uint64_t srcBufferKey,
                          const std::vector<CapturedBuildInputRegion>& regions,
                          StagedInputReadback& outStaging) override;

  bool ReadStaged(const StagedInputReadback& staging, std::vector<uint8_t>& outData) override;

  void FreeStaged(const StagedInputReadback& staging) override;

  bool WaitQueueIdle(uint64_t deviceKey, uint64_t queueKey) override;

  bool ReadImage(uint64_t deviceKey,
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
                 std::vector<uint8_t>& outData,
                 std::vector<VkBufferImageCopy>& outRegions) override;

  bool ReadAccelerationStructureSerialized(uint64_t deviceKey,
                                           uint64_t physDevKey,
                                           uint64_t queueKey,
                                           uint64_t commandPoolKey,
                                           uint64_t accelerationStructureKey,
                                           std::vector<uint8_t>& outData,
                                           VkDeviceAddress& outDeviceAddress,
                                           uint64_t& outOpaqueCaptureAddress,
                                           uint64_t& outMemoryOpaqueCaptureAddress) override;

  bool QueryAccelerationStructureBuildSizes(
      uint64_t deviceKey,
      const VkAccelerationStructureBuildGeometryInfoKHR& buildInfo,
      const uint32_t* pMaxPrimitiveCounts,
      VkAccelerationStructureBuildSizesInfoKHR& outSizes) override;

  bool ReserveScratchBufferAddress(uint64_t deviceKey,
                                   uint64_t physDevKey,
                                   VkDeviceSize size,
                                   VkDeviceAddress& outDeviceAddress,
                                   uint64_t& outOpaqueCaptureAddress,
                                   uint64_t& outMemoryOpaqueCaptureAddress) override;

  void ReleaseReservedAddresses() override;

private:
  // Kept alive so their reserved capture/replay addresses are not handed back to a
  // later reservation in the same rebuild. Freed by ReleaseReservedAddresses.
  struct ReservedBuffer {
    VkDevice Device;
    VkBuffer Buffer;
    VkDeviceMemory Memory;
  };
  std::vector<ReservedBuffer> m_ReservedAddressBuffers;

  // Allocate a HOST_VISIBLE | HOST_COHERENT staging VkBuffer of 'size' bytes.
  // Maps it and stores the pointer in outMapped.  Returns false on failure.
  // extraUsage is OR'd into the buffer's usage flags, extraCreateFlags into
  // VkBufferCreateInfo::flags. The matching VK_MEMORY_ALLOCATE_DEVICE_ADDRESS[_
  // CAPTURE_REPLAY]_BIT is added to the allocation.
  bool AllocateStagingBuffer(VkDevice device,
                             VkPhysicalDevice physDevice,
                             VkDeviceSize size,
                             VkBuffer& outBuf,
                             VkDeviceMemory& outMem,
                             void*& outMapped,
                             VkBufferUsageFlags extraUsage = 0,
                             VkBufferCreateFlags extraCreateFlags = 0);

  // Allocate a one-shot command buffer, let the caller record into it, submit
  // it to queue and wait idle.  Frees the CB afterwards.  Returns false on error.
  bool SubmitOneShot(VkDevice device,
                     VkQueue queue,
                     VkCommandPool pool,
                     std::function<void(VkCommandBuffer)> recordFn);

  // Internal helper: find a HOST_VISIBLE staging memory type given a resolved
  // VkPhysicalDevice handle directly, avoiding a handle?key?handle round-trip.
  uint32_t FindStagingMemoryTypeForPhysDevice(VkPhysicalDevice physDevice, uint32_t memoryTypeBits);

  PlayerManager& m_Player;
};

} // namespace vulkan
} // namespace gits

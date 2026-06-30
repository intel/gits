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

  bool ReadBuffer(uint64_t deviceKey,
                  uint64_t physDevKey,
                  uint64_t queueKey,
                  uint64_t commandPoolKey,
                  uint64_t bufferKey,
                  VkDeviceSize size,
                  std::vector<uint8_t>& outData) override;

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

private:
  // Allocate a HOST_VISIBLE | HOST_COHERENT staging VkBuffer of 'size' bytes.
  // Maps it and stores the pointer in outMapped.  Returns false on failure.
  bool AllocateStagingBuffer(VkDevice device,
                             VkPhysicalDevice physDevice,
                             VkDeviceSize size,
                             VkBuffer& outBuf,
                             VkDeviceMemory& outMem,
                             void*& outMapped);

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

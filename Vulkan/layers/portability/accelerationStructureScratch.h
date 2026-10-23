// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "dispatchTablesHolder.h"

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace gits {
namespace vulkan {

// Scratch memory for acceleration structure builds, sized for the replay driver.
//
// The recorded scratch regions were sized for the recording driver, and another
// driver can need much more scratch memory for the same build. Scratch contents are
// undefined between builds, so the recorded scratch addresses can be replaced by
// regions of GITS-owned buffers. Every build recorded into a command buffer gets its
// own region, so builds can't overwrite each other's scratch. The regions are reused
// when the command buffer is recorded again and released when it is freed.
class AccelerationStructureScratch {
public:
  explicit AccelerationStructureScratch(DispatchTablesHolder& dispatchTablesHolder)
      : m_DispatchTablesHolder(dispatchTablesHolder) {}

  void SetPhysicalDevice(VkPhysicalDevice physicalDevice);
  void ReplaceScratch(VkDevice device,
                      VkCommandBuffer commandBuffer,
                      uint32_t infoCount,
                      VkAccelerationStructureBuildGeometryInfoKHR* infos,
                      const VkAccelerationStructureBuildRangeInfoKHR* const* rangeInfos);

  void AddCommandBuffers(VkCommandPool commandPool,
                         const VkCommandBuffer* commandBuffers,
                         uint32_t count);
  void ResetCommandBuffer(VkCommandBuffer commandBuffer);
  void ResetCommandPool(VkCommandPool commandPool);
  void FreeCommandBuffers(VkCommandPool commandPool,
                          const VkCommandBuffer* commandBuffers,
                          uint32_t count);
  void DestroyCommandPool(VkCommandPool commandPool);
  void DestroyDevice(VkDevice device);

private:
  struct Chunk {
    VkBuffer Buffer{VK_NULL_HANDLE};
    VkDeviceMemory Memory{VK_NULL_HANDLE};
    VkDeviceAddress Address{0};
    VkDeviceSize Size{0};
    VkDeviceSize Used{0};
  };
  struct Arena {
    VkDevice Device{VK_NULL_HANDLE};
    std::vector<Chunk> Chunks;
  };

  bool Allocate(VkDevice device, Arena& arena, VkDeviceSize size, VkDeviceAddress& address);
  bool CreateChunk(VkDevice device, VkDeviceSize size, Chunk& chunk);
  void Release(Arena& arena);

  DispatchTablesHolder& m_DispatchTablesHolder;
  VkPhysicalDevice m_PhysicalDevice{VK_NULL_HANDLE};
  VkDeviceSize m_Alignment{256};
  std::unordered_map<VkCommandBuffer, Arena> m_Arenas;
  std::unordered_map<VkCommandPool, std::unordered_set<VkCommandBuffer>> m_PoolCommandBuffers;
  uint64_t m_ReplacedBuildCount{0};
  VkDeviceSize m_TotalChunkSize{0};
};

} // namespace vulkan
} // namespace gits

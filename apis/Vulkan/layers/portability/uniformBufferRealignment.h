// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "dispatchTablesHolder.h"
#include "portabilityHelpers.h"

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace gits {
namespace vulkan {

// Uniform buffer descriptors whose offset is not a multiple of the replay device's
// minUniformBufferOffsetAlignment.
//
// A stream can contain such offsets when the recording driver tolerated them. Other
// drivers may then read wrong data or hang: Mesa ANV pushes uniform buffer ranges with
// commands that keep the read length in the low bits of the address, so a misaligned
// address inflates the length.
//
// The offsets are always reported. When realignment is enabled, such a descriptor is
// written with an aligned slot of a GITS-owned buffer instead, and every queue
// submission whose command buffers bind the descriptor set first copies the recorded
// range into the slot. The copy reads the range through a GITS-owned buffer bound to
// the memory of the recorded buffer, so the recorded buffer keeps its creation
// parameters. The copy is a command buffer added in front of the submission's command
// buffers, and the submission's semaphore waits are extended to the transfer stage
// so that the copy runs after them. The copy sees data written by the host or by
// earlier submissions, not data written by the same command buffers before the
// descriptor is used. Descriptor update templates, push descriptors, dynamic uniform
// buffers and sparse buffers are not handled.
class UniformBufferRealignment {
public:
  explicit UniformBufferRealignment(DispatchTablesHolder& dispatchTablesHolder)
      : m_DispatchTablesHolder(dispatchTablesHolder) {}

  void SetEnabled(bool enabled) {
    m_Enabled = enabled;
  }
  void SetPhysicalDevice(VkPhysicalDevice physicalDevice);
  void AddQueue(VkDevice device, uint32_t queueFamilyIndex, VkQueue queue);

  void AddMemory(VkDeviceMemory memory, const VkMemoryAllocateInfo& allocateInfo);
  void RemoveMemory(VkDeviceMemory memory);
  void AddBuffer(VkBuffer buffer, const VkBufferCreateInfo& createInfo);
  void BindBufferMemory(VkDevice device,
                        VkBuffer buffer,
                        VkDeviceMemory memory,
                        VkDeviceSize memoryOffset);
  void RemoveBuffer(VkBuffer buffer);

  void AddDescriptorSets(VkDescriptorPool pool, const VkDescriptorSet* sets, uint32_t count);
  void FreeDescriptorSets(const VkDescriptorSet* sets, uint32_t count);
  void ResetDescriptorPool(VkDescriptorPool pool);
  void UpdateDescriptorSets(VkDevice device,
                            uint32_t writeCount,
                            const VkWriteDescriptorSet* writes,
                            uint64_t key);
  // Called after the driver has copied the descriptors.
  void CopyDescriptorSets(VkDevice device, uint32_t copyCount, const VkCopyDescriptorSet* copies);

  void BeginCommandBuffer(VkCommandBuffer commandBuffer);
  void FreeCommandBuffers(const VkCommandBuffer* commandBuffers, uint32_t count);
  void BindDescriptorSets(VkCommandBuffer commandBuffer,
                          const VkDescriptorSet* sets,
                          uint32_t count);
  void ExecuteCommands(VkCommandBuffer commandBuffer,
                       const VkCommandBuffer* secondaryCommandBuffers,
                       uint32_t count);

  // Add the copy command buffers to the submissions. The submit infos point to storage
  // owned by this object until FinishSubmit is called on the same thread.
  void PrepareSubmit(VkQueue queue, uint32_t submitCount, VkSubmitInfo* submits);
  void PrepareSubmit2(VkQueue queue, uint32_t submitCount, VkSubmitInfo2* submits);
  void FinishSubmit(VkQueue queue);

  void DestroyDevice(VkDevice device);

private:
  using Chunk = portability::DeviceLocalBuffer;
  struct Slot {
    VkBuffer Buffer{VK_NULL_HANDLE};
    VkDeviceSize Offset{0};
    VkDeviceSize Size{0};
    uint32_t SizeClass{0}; // Classes above 64 KiB: dedicated chunk
    uint32_t ChunkIndex{0};
  };
  struct Entry {
    uint32_t Binding{0};
    uint32_t ArrayElement{0};
    VkBuffer Source{VK_NULL_HANDLE}; // alias of the recorded buffer
    VkDeviceSize SourceOffset{0};
    Slot Shadow;
  };
  struct MemoryInfo {
    uint32_t TypeIndex{0};
    VkDeviceSize Size{0};
    bool Dedicated{false};
  };
  // Uniform buffer which descriptors may read from, the copies read it through an
  // alias with transfer source usage bound to the same memory.
  struct SourceBuffer {
    VkDeviceSize Size{0};
    VkSharingMode SharingMode{VK_SHARING_MODE_EXCLUSIVE};
    std::vector<uint32_t> QueueFamilies;
    VkDevice Device{VK_NULL_HANDLE};
    VkDeviceMemory Memory{VK_NULL_HANDLE};
    VkDeviceSize MemoryOffset{0};
    VkBuffer Alias{VK_NULL_HANDLE};
    bool AliasFailed{false};
  };
  struct QueueInfo {
    VkDevice Device{VK_NULL_HANDLE};
    uint32_t Family{0};
  };
  struct PendingCopies {
    VkFence Fence{VK_NULL_HANDLE};
    std::vector<VkCommandBuffer> CommandBuffers;
  };
  struct FamilyPool {
    VkCommandPool Pool{VK_NULL_HANDLE};
    std::vector<VkCommandBuffer> FreeCommandBuffers;
    std::vector<PendingCopies> Pending;
  };

  bool AllocateSlot(VkDevice device, VkDeviceSize size, Slot& slot);
  void FreeSlot(const Slot& slot);
  bool CreateChunk(VkDevice device, VkDeviceSize size, Chunk& chunk);
  void ReleaseEntries(VkDescriptorSet set);
  VkBuffer GetAlias(SourceBuffer& source);
  // Record the copies for the descriptor sets bound by the command buffers.
  VkCommandBuffer RecordCopies(VkQueue queue, const std::vector<VkCommandBuffer>& commandBuffers);
  VkCommandBuffer AcquireCommandBuffer(VkDevice device, FamilyPool& familyPool);
  void Recycle(VkDevice device, FamilyPool& familyPool);
  void ReportMisaligned(VkDeviceSize offset, uint64_t key);

  DispatchTablesHolder& m_DispatchTablesHolder;
  bool m_Enabled{false};
  VkPhysicalDevice m_PhysicalDevice{VK_NULL_HANDLE};
  VkDeviceSize m_Alignment{0};
  VkDevice m_Device{VK_NULL_HANDLE};

  std::unordered_map<VkQueue, QueueInfo> m_Queues;
  std::unordered_map<VkDeviceMemory, MemoryInfo> m_Memories;
  std::unordered_map<VkBuffer, SourceBuffer> m_SourceBuffers;
  std::unordered_map<VkDescriptorSet, std::vector<Entry>> m_Entries;
  std::unordered_map<VkDescriptorSet, VkDescriptorPool> m_SetPools;
  std::unordered_map<VkDescriptorPool, std::unordered_set<VkDescriptorSet>> m_PoolSets;
  std::unordered_map<VkCommandBuffer, std::unordered_set<VkDescriptorSet>> m_BoundSets;

  // Shadow slots: power-of-two size classes, the smaller ones in fixed-size chunks,
  // the larger ones in dedicated chunks
  std::vector<std::vector<Chunk>> m_Chunks;
  std::vector<std::vector<std::pair<uint32_t, VkDeviceSize>>> m_FreeSlots;
  std::vector<Chunk> m_DedicatedChunks;
  std::unordered_map<uint32_t, std::vector<uint32_t>> m_FreeDedicatedChunks;
  VkDeviceSize m_ShadowBytes{0};

  std::unordered_map<uint32_t, FamilyPool> m_FamilyPools;
  std::vector<VkFence> m_FreeFences;

  uint64_t m_MisalignedCount{0};
  uint64_t m_RealignedCount{0};
  uint64_t m_CopySubmitCount{0};
  bool m_UnsupportedReported{false};
};

} // namespace vulkan
} // namespace gits

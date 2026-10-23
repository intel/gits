// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "uniformBufferRealignment.h"

#include "log.h"

#include <algorithm>
#include <map>

namespace gits {
namespace vulkan {

namespace {

constexpr VkDeviceSize CHUNK_SIZE = 1024ull * 1024;
constexpr VkDeviceSize MIN_SLOT_SIZE = 256;
constexpr uint32_t SIZE_CLASS_COUNT = 9; // 256 B .. 64 KiB
constexpr uint32_t DEDICATED = UINT32_MAX;

VkDeviceSize AlignUp(VkDeviceSize value, VkDeviceSize alignment) {
  return (value + alignment - 1) / alignment * alignment;
}

// Command buffer arrays of the submission in flight on this thread.
struct SubmitStorage {
  std::vector<std::vector<VkCommandBuffer>> CommandBuffers;
  std::vector<std::vector<VkCommandBufferSubmitInfo>> CommandBufferInfos;
  std::vector<VkCommandBuffer> CopyCommandBuffers;
};
thread_local SubmitStorage tl_Submit;

} // namespace

void UniformBufferRealignment::SetPhysicalDevice(VkPhysicalDevice physicalDevice) {
  m_PhysicalDevice = physicalDevice;
  m_Alignment = 0;
  VkInstanceLevelDispatchTable* dispatchTable =
      m_DispatchTablesHolder.GetInstanceDispatchTable(physicalDevice);
  if (dispatchTable == nullptr || dispatchTable->vkGetPhysicalDeviceProperties == nullptr) {
    return;
  }
  VkPhysicalDeviceProperties properties{};
  dispatchTable->vkGetPhysicalDeviceProperties(physicalDevice, &properties);
  m_Alignment = properties.limits.minUniformBufferOffsetAlignment;
}

void UniformBufferRealignment::AddQueue(VkDevice device, uint32_t queueFamilyIndex, VkQueue queue) {
  m_Queues[queue] = {device, queueFamilyIndex};
}

void UniformBufferRealignment::PrepareBufferCreateInfo(VkBufferCreateInfo& createInfo) const {
  if (m_Enabled && (createInfo.usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)) {
    createInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  }
}

void UniformBufferRealignment::AddBuffer(VkBuffer buffer, const VkBufferCreateInfo& createInfo) {
  if (createInfo.usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) {
    m_BufferSizes[buffer] = createInfo.size;
  }
}

void UniformBufferRealignment::RemoveBuffer(VkBuffer buffer) {
  if (m_BufferSizes.erase(buffer) == 0) {
    return;
  }
  // Descriptors of a destroyed buffer can't be used any more, stop copying from it.
  for (auto& [set, entries] : m_Entries) {
    for (auto it = entries.begin(); it != entries.end();) {
      if (it->Source == buffer) {
        FreeSlot(it->Shadow);
        it = entries.erase(it);
      } else {
        ++it;
      }
    }
  }
}

void UniformBufferRealignment::AddDescriptorSets(VkDescriptorPool pool,
                                                 const VkDescriptorSet* sets,
                                                 uint32_t count) {
  for (uint32_t i = 0; i < count; ++i) {
    m_SetPools[sets[i]] = pool;
    m_PoolSets[pool].insert(sets[i]);
  }
}

void UniformBufferRealignment::FreeDescriptorSets(const VkDescriptorSet* sets, uint32_t count) {
  for (uint32_t i = 0; i < count; ++i) {
    ReleaseEntries(sets[i]);
    auto poolIt = m_SetPools.find(sets[i]);
    if (poolIt != m_SetPools.end()) {
      m_PoolSets[poolIt->second].erase(sets[i]);
      m_SetPools.erase(poolIt);
    }
  }
}

void UniformBufferRealignment::ResetDescriptorPool(VkDescriptorPool pool) {
  auto it = m_PoolSets.find(pool);
  if (it == m_PoolSets.end()) {
    return;
  }
  for (VkDescriptorSet set : it->second) {
    ReleaseEntries(set);
    m_SetPools.erase(set);
  }
  m_PoolSets.erase(it);
}

void UniformBufferRealignment::ReleaseEntries(VkDescriptorSet set) {
  auto it = m_Entries.find(set);
  if (it == m_Entries.end()) {
    return;
  }
  for (const Entry& entry : it->second) {
    FreeSlot(entry.Shadow);
  }
  m_Entries.erase(it);
}

void UniformBufferRealignment::ReportMisaligned(VkDeviceSize offset, uint64_t key) {
  if (m_MisalignedCount++ != 0) {
    return;
  }
  if (m_Enabled) {
    LOG_INFO << "Portability: uniform buffer descriptor offsets which are not a multiple of the "
                "replay device's minUniformBufferOffsetAlignment ("
             << m_Alignment << ") read from aligned copies (first: offset " << offset << " at call "
             << key << ").";
  } else {
    LOG_WARNING << "Portability: uniform buffer descriptor offset " << offset << " at call " << key
                << " is not a multiple of the replay device's minUniformBufferOffsetAlignment ("
                << m_Alignment
                << "). The driver may read wrong data or hang; enable "
                   "Vulkan.Player.Portability.RealignUniformBuffers.";
  }
}

void UniformBufferRealignment::UpdateDescriptorSets(VkDevice device,
                                                    uint32_t writeCount,
                                                    const VkWriteDescriptorSet* writes,
                                                    uint64_t key) {
  if (m_Alignment == 0 || writes == nullptr) {
    return;
  }
  m_Device = device;
  for (uint32_t w = 0; w < writeCount; ++w) {
    const VkWriteDescriptorSet& write = writes[w];
    if (write.pBufferInfo == nullptr ||
        (write.descriptorType != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER &&
         write.descriptorType != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)) {
      continue;
    }
    auto* infos = const_cast<VkDescriptorBufferInfo*>(write.pBufferInfo);
    for (uint32_t i = 0; i < write.descriptorCount; ++i) {
      VkDescriptorBufferInfo& info = infos[i];
      const uint32_t element = write.dstArrayElement + i;

      // A new write replaces the descriptor's previous copy.
      auto entriesIt = m_Entries.find(write.dstSet);
      if (entriesIt != m_Entries.end()) {
        auto& entries = entriesIt->second;
        auto old = std::find_if(entries.begin(), entries.end(), [&](const Entry& entry) {
          return entry.Binding == write.dstBinding && entry.ArrayElement == element;
        });
        if (old != entries.end()) {
          FreeSlot(old->Shadow);
          entries.erase(old);
        }
      }

      if (info.buffer == VK_NULL_HANDLE || info.offset % m_Alignment == 0) {
        continue;
      }
      ReportMisaligned(info.offset, key);
      if (!m_Enabled) {
        continue;
      }
      auto sizeIt = m_BufferSizes.find(info.buffer);
      if (write.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
          sizeIt == m_BufferSizes.end() || info.offset >= sizeIt->second) {
        if (!m_UnsupportedReported) {
          m_UnsupportedReported = true;
          LOG_WARNING << "Portability: a misaligned uniform buffer descriptor at call " << key
                      << " can't be realigned (dynamic uniform buffer or unknown buffer).";
        }
        continue;
      }
      VkDeviceSize size = info.range == VK_WHOLE_SIZE ? sizeIt->second - info.offset : info.range;
      size = std::min(size, sizeIt->second - info.offset);

      Entry entry;
      entry.Binding = write.dstBinding;
      entry.ArrayElement = element;
      entry.Source = info.buffer;
      entry.SourceOffset = info.offset;
      if (!AllocateSlot(device, size, entry.Shadow)) {
        LOG_ERROR << "Portability: allocation of an aligned uniform buffer copy (" << size
                  << " bytes) failed.";
        continue;
      }
      info.buffer = entry.Shadow.Buffer;
      info.offset = entry.Shadow.Offset;
      info.range = size;
      m_Entries[write.dstSet].push_back(entry);
      ++m_RealignedCount;
    }
  }
}

void UniformBufferRealignment::BeginCommandBuffer(VkCommandBuffer commandBuffer) {
  auto it = m_BoundSets.find(commandBuffer);
  if (it != m_BoundSets.end()) {
    it->second.clear();
  }
}

void UniformBufferRealignment::FreeCommandBuffers(const VkCommandBuffer* commandBuffers,
                                                  uint32_t count) {
  for (uint32_t i = 0; commandBuffers != nullptr && i < count; ++i) {
    m_BoundSets.erase(commandBuffers[i]);
  }
}

void UniformBufferRealignment::BindDescriptorSets(VkCommandBuffer commandBuffer,
                                                  const VkDescriptorSet* sets,
                                                  uint32_t count) {
  if (!m_Enabled || sets == nullptr) {
    return;
  }
  for (uint32_t i = 0; i < count; ++i) {
    if (sets[i] != VK_NULL_HANDLE) {
      m_BoundSets[commandBuffer].insert(sets[i]);
    }
  }
}

void UniformBufferRealignment::PrepareSubmit(VkQueue queue,
                                             uint32_t submitCount,
                                             VkSubmitInfo* submits) {
  tl_Submit.CommandBuffers.clear();
  tl_Submit.CopyCommandBuffers.clear();
  if (!m_Enabled || m_RealignedCount == 0 || submits == nullptr) {
    return;
  }
  tl_Submit.CommandBuffers.resize(submitCount);
  for (uint32_t s = 0; s < submitCount; ++s) {
    VkSubmitInfo& submit = submits[s];
    std::vector<VkCommandBuffer> commandBuffers(submit.pCommandBuffers,
                                                submit.pCommandBuffers + submit.commandBufferCount);
    const VkCommandBuffer copies = RecordCopies(queue, commandBuffers);
    if (copies == VK_NULL_HANDLE) {
      continue;
    }
    auto& storage = tl_Submit.CommandBuffers[s];
    storage.reserve(commandBuffers.size() + 1);
    storage.push_back(copies);
    storage.insert(storage.end(), commandBuffers.begin(), commandBuffers.end());
    submit.pCommandBuffers = storage.data();
    submit.commandBufferCount = static_cast<uint32_t>(storage.size());
  }
}

void UniformBufferRealignment::PrepareSubmit2(VkQueue queue,
                                              uint32_t submitCount,
                                              VkSubmitInfo2* submits) {
  tl_Submit.CommandBufferInfos.clear();
  tl_Submit.CopyCommandBuffers.clear();
  if (!m_Enabled || m_RealignedCount == 0 || submits == nullptr) {
    return;
  }
  tl_Submit.CommandBufferInfos.resize(submitCount);
  for (uint32_t s = 0; s < submitCount; ++s) {
    VkSubmitInfo2& submit = submits[s];
    std::vector<VkCommandBuffer> commandBuffers;
    for (uint32_t i = 0; i < submit.commandBufferInfoCount; ++i) {
      commandBuffers.push_back(submit.pCommandBufferInfos[i].commandBuffer);
    }
    const VkCommandBuffer copies = RecordCopies(queue, commandBuffers);
    if (copies == VK_NULL_HANDLE) {
      continue;
    }
    auto& storage = tl_Submit.CommandBufferInfos[s];
    VkCommandBufferSubmitInfo copyInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
    copyInfo.commandBuffer = copies;
    storage.push_back(copyInfo);
    storage.insert(storage.end(), submit.pCommandBufferInfos,
                   submit.pCommandBufferInfos + submit.commandBufferInfoCount);
    submit.pCommandBufferInfos = storage.data();
    submit.commandBufferInfoCount = static_cast<uint32_t>(storage.size());
  }
}

void UniformBufferRealignment::FinishSubmit(VkQueue queue) {
  if (tl_Submit.CopyCommandBuffers.empty()) {
    return;
  }
  std::vector<VkCommandBuffer> copies;
  copies.swap(tl_Submit.CopyCommandBuffers);
  auto queueIt = m_Queues.find(queue);
  if (queueIt == m_Queues.end()) {
    return;
  }
  VkDeviceLevelDispatchTable* dispatchTable =
      m_DispatchTablesHolder.GetDeviceDispatchTable(queueIt->second.Device);
  VkFence fence = VK_NULL_HANDLE;
  if (!m_FreeFences.empty()) {
    fence = m_FreeFences.back();
    m_FreeFences.pop_back();
  } else {
    VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    if (dispatchTable->vkCreateFence(queueIt->second.Device, &fenceInfo, nullptr, &fence) !=
        VK_SUCCESS) {
      return;
    }
  }
  // An empty submission signals the fence once the copies (and everything before them)
  // have completed, then the command buffers can be reused.
  if (dispatchTable->vkQueueSubmit(queue, 0, nullptr, fence) != VK_SUCCESS) {
    m_FreeFences.push_back(fence);
    return;
  }
  m_FamilyPools[queueIt->second.Family].Pending.push_back({fence, std::move(copies)});
}

VkCommandBuffer UniformBufferRealignment::RecordCopies(
    VkQueue queue, const std::vector<VkCommandBuffer>& commandBuffers) {
  // Copies per (source, shadow) buffer pair, each shadow slot once.
  std::map<std::pair<VkBuffer, VkBuffer>, std::vector<VkBufferCopy>> copies;
  std::unordered_set<const Entry*> seen;
  for (VkCommandBuffer commandBuffer : commandBuffers) {
    auto boundIt = m_BoundSets.find(commandBuffer);
    if (boundIt == m_BoundSets.end()) {
      continue;
    }
    for (VkDescriptorSet set : boundIt->second) {
      auto entriesIt = m_Entries.find(set);
      if (entriesIt == m_Entries.end()) {
        continue;
      }
      for (const Entry& entry : entriesIt->second) {
        if (seen.insert(&entry).second) {
          copies[{entry.Source, entry.Shadow.Buffer}].push_back(
              {entry.SourceOffset, entry.Shadow.Offset, entry.Shadow.Size});
        }
      }
    }
  }
  if (copies.empty()) {
    return VK_NULL_HANDLE;
  }
  auto queueIt = m_Queues.find(queue);
  if (queueIt == m_Queues.end()) {
    if (!m_UnsupportedReported) {
      m_UnsupportedReported = true;
      LOG_WARNING << "Portability: uniform buffer copies skipped for an unknown queue.";
    }
    return VK_NULL_HANDLE;
  }
  const VkDevice device = queueIt->second.Device;
  VkDeviceLevelDispatchTable* dispatchTable = m_DispatchTablesHolder.GetDeviceDispatchTable(device);
  FamilyPool& familyPool = m_FamilyPools[queueIt->second.Family];
  if (familyPool.Pool == VK_NULL_HANDLE) {
    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueIt->second.Family;
    if (dispatchTable->vkCreateCommandPool(device, &poolInfo, nullptr, &familyPool.Pool) !=
        VK_SUCCESS) {
      return VK_NULL_HANDLE;
    }
  }
  const VkCommandBuffer commandBuffer = AcquireCommandBuffer(device, familyPool);
  if (commandBuffer == VK_NULL_HANDLE) {
    return VK_NULL_HANDLE;
  }

  VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  dispatchTable->vkBeginCommandBuffer(commandBuffer, &beginInfo);
  // Earlier work may still read the slots or write the sources.
  VkMemoryBarrier before{VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr, VK_ACCESS_MEMORY_WRITE_BIT,
                         VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT};
  dispatchTable->vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                      VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &before, 0, nullptr, 0,
                                      nullptr);
  for (const auto& [buffers, regions] : copies) {
    dispatchTable->vkCmdCopyBuffer(commandBuffer, buffers.first, buffers.second,
                                   static_cast<uint32_t>(regions.size()), regions.data());
  }
  VkMemoryBarrier after{VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr, VK_ACCESS_TRANSFER_WRITE_BIT,
                        VK_ACCESS_MEMORY_READ_BIT};
  dispatchTable->vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &after, 0, nullptr,
                                      0, nullptr);
  dispatchTable->vkEndCommandBuffer(commandBuffer);

  tl_Submit.CopyCommandBuffers.push_back(commandBuffer);
  ++m_CopySubmitCount;
  return commandBuffer;
}

VkCommandBuffer UniformBufferRealignment::AcquireCommandBuffer(VkDevice device,
                                                               FamilyPool& familyPool) {
  Recycle(device, familyPool);
  if (!familyPool.FreeCommandBuffers.empty()) {
    const VkCommandBuffer commandBuffer = familyPool.FreeCommandBuffers.back();
    familyPool.FreeCommandBuffers.pop_back();
    return commandBuffer;
  }
  VkDeviceLevelDispatchTable* dispatchTable = m_DispatchTablesHolder.GetDeviceDispatchTable(device);
  VkCommandBufferAllocateInfo allocateInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  allocateInfo.commandPool = familyPool.Pool;
  allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocateInfo.commandBufferCount = 1;
  VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
  if (dispatchTable->vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer) !=
      VK_SUCCESS) {
    return VK_NULL_HANDLE;
  }
  return commandBuffer;
}

void UniformBufferRealignment::Recycle(VkDevice device, FamilyPool& familyPool) {
  VkDeviceLevelDispatchTable* dispatchTable = m_DispatchTablesHolder.GetDeviceDispatchTable(device);
  for (auto it = familyPool.Pending.begin(); it != familyPool.Pending.end();) {
    if (dispatchTable->vkGetFenceStatus(device, it->Fence) != VK_SUCCESS) {
      ++it;
      continue;
    }
    dispatchTable->vkResetFences(device, 1, &it->Fence);
    m_FreeFences.push_back(it->Fence);
    familyPool.FreeCommandBuffers.insert(familyPool.FreeCommandBuffers.end(),
                                         it->CommandBuffers.begin(), it->CommandBuffers.end());
    it = familyPool.Pending.erase(it);
  }
}

bool UniformBufferRealignment::CreateChunk(VkDevice device, VkDeviceSize size, Chunk& chunk) {
  VkDeviceLevelDispatchTable* dispatchTable = m_DispatchTablesHolder.GetDeviceDispatchTable(device);
  VkInstanceLevelDispatchTable* instanceDispatchTable =
      m_DispatchTablesHolder.GetInstanceDispatchTable(m_PhysicalDevice);
  if (dispatchTable == nullptr || instanceDispatchTable == nullptr) {
    return false;
  }
  VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferInfo.size = size;
  bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  if (dispatchTable->vkCreateBuffer(device, &bufferInfo, nullptr, &chunk.Buffer) != VK_SUCCESS) {
    return false;
  }
  VkMemoryRequirements requirements{};
  dispatchTable->vkGetBufferMemoryRequirements(device, chunk.Buffer, &requirements);
  VkPhysicalDeviceMemoryProperties memoryProperties{};
  instanceDispatchTable->vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memoryProperties);
  uint32_t memoryType = UINT32_MAX;
  for (uint32_t t = 0; t < memoryProperties.memoryTypeCount && memoryType == UINT32_MAX; ++t) {
    if ((requirements.memoryTypeBits & (1u << t)) &&
        (memoryProperties.memoryTypes[t].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
      memoryType = t;
    }
  }
  VkMemoryAllocateInfo allocateInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  allocateInfo.allocationSize = requirements.size;
  allocateInfo.memoryTypeIndex = memoryType;
  if (memoryType == UINT32_MAX ||
      dispatchTable->vkAllocateMemory(device, &allocateInfo, nullptr, &chunk.Memory) !=
          VK_SUCCESS ||
      dispatchTable->vkBindBufferMemory(device, chunk.Buffer, chunk.Memory, 0) != VK_SUCCESS) {
    dispatchTable->vkDestroyBuffer(device, chunk.Buffer, nullptr);
    if (chunk.Memory != VK_NULL_HANDLE) {
      dispatchTable->vkFreeMemory(device, chunk.Memory, nullptr);
    }
    chunk = {};
    return false;
  }
  m_ShadowBytes += requirements.size;
  return true;
}

bool UniformBufferRealignment::AllocateSlot(VkDevice device, VkDeviceSize size, Slot& slot) {
  const VkDeviceSize minSlot = std::max(MIN_SLOT_SIZE, AlignUp(m_Alignment, MIN_SLOT_SIZE));
  uint32_t sizeClass = 0;
  while (sizeClass < SIZE_CLASS_COUNT && (minSlot << sizeClass) < size) {
    ++sizeClass;
  }
  slot.Size = size;
  if (sizeClass == SIZE_CLASS_COUNT) {
    // Large ranges get a dedicated chunk, kept for reuse once released.
    Chunk chunk;
    if (!CreateChunk(device, AlignUp(size, minSlot), chunk)) {
      return false;
    }
    slot.Buffer = chunk.Buffer;
    slot.Offset = 0;
    slot.SizeClass = DEDICATED;
    slot.ChunkIndex = static_cast<uint32_t>(m_DedicatedChunks.size());
    m_DedicatedChunks.push_back(chunk);
    return true;
  }
  if (m_Chunks.size() < SIZE_CLASS_COUNT) {
    m_Chunks.resize(SIZE_CLASS_COUNT);
    m_FreeSlots.resize(SIZE_CLASS_COUNT);
  }
  auto& freeSlots = m_FreeSlots[sizeClass];
  if (freeSlots.empty()) {
    Chunk chunk;
    if (!CreateChunk(device, CHUNK_SIZE, chunk)) {
      return false;
    }
    const uint32_t chunkIndex = static_cast<uint32_t>(m_Chunks[sizeClass].size());
    m_Chunks[sizeClass].push_back(chunk);
    const VkDeviceSize slotSize = minSlot << sizeClass;
    for (VkDeviceSize offset = CHUNK_SIZE; offset >= slotSize; offset -= slotSize) {
      freeSlots.push_back({chunkIndex, offset - slotSize});
    }
  }
  const auto [chunkIndex, offset] = freeSlots.back();
  freeSlots.pop_back();
  slot.Buffer = m_Chunks[sizeClass][chunkIndex].Buffer;
  slot.Offset = offset;
  slot.SizeClass = sizeClass;
  slot.ChunkIndex = chunkIndex;
  return true;
}

void UniformBufferRealignment::FreeSlot(const Slot& slot) {
  if (slot.Buffer == VK_NULL_HANDLE) {
    return;
  }
  if (slot.SizeClass == DEDICATED) {
    // The chunk may still be read by work in flight; it is destroyed with the device.
    return;
  }
  m_FreeSlots[slot.SizeClass].push_back({slot.ChunkIndex, slot.Offset});
}

void UniformBufferRealignment::DestroyDevice(VkDevice device) {
  if (m_MisalignedCount != 0) {
    LOG_INFO << "Portability: " << m_MisalignedCount
             << " uniform buffer descriptor write(s) used offsets which are not a multiple of "
                "minUniformBufferOffsetAlignment, "
             << m_RealignedCount << " realigned, " << m_CopySubmitCount
             << " submission(s) with copies, " << m_ShadowBytes << " bytes of copy memory.";
  }
  VkDeviceLevelDispatchTable* dispatchTable = m_DispatchTablesHolder.GetDeviceDispatchTable(device);
  if (dispatchTable != nullptr) {
    auto destroyChunk = [&](const Chunk& chunk) {
      dispatchTable->vkDestroyBuffer(device, chunk.Buffer, nullptr);
      dispatchTable->vkFreeMemory(device, chunk.Memory, nullptr);
    };
    for (const auto& chunks : m_Chunks) {
      std::for_each(chunks.begin(), chunks.end(), destroyChunk);
    }
    std::for_each(m_DedicatedChunks.begin(), m_DedicatedChunks.end(), destroyChunk);
    for (auto& [family, familyPool] : m_FamilyPools) {
      for (const PendingCopies& pending : familyPool.Pending) {
        dispatchTable->vkDestroyFence(device, pending.Fence, nullptr);
      }
      if (familyPool.Pool != VK_NULL_HANDLE) {
        dispatchTable->vkDestroyCommandPool(device, familyPool.Pool, nullptr);
      }
    }
    for (VkFence fence : m_FreeFences) {
      dispatchTable->vkDestroyFence(device, fence, nullptr);
    }
  }
  m_Chunks.clear();
  m_FreeSlots.clear();
  m_DedicatedChunks.clear();
  m_FamilyPools.clear();
  m_FreeFences.clear();
  m_Entries.clear();
  m_SetPools.clear();
  m_PoolSets.clear();
  m_BoundSets.clear();
  m_Queues.clear();
  m_BufferSizes.clear();
  m_MisalignedCount = 0;
  m_RealignedCount = 0;
  m_CopySubmitCount = 0;
  m_ShadowBytes = 0;
}

} // namespace vulkan
} // namespace gits

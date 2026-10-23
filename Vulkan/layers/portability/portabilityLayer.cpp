// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "portabilityLayer.h"

#include "configurationLib.h"
#include "portabilityHelpers.h"
#include "log.h"
#include "vulkanHelpers.h"

#include <algorithm>
#include <ios>

namespace gits {
namespace vulkan {

namespace {

// Number of set bits in a queue flags mask; used to prefer the target family
// that adds the fewest extra capabilities over the recorded one.
uint32_t PopCount(VkQueueFlags flags) {
  uint32_t count = 0;
  while (flags) {
    flags &= (flags - 1);
    ++count;
  }
  return count;
}

// Pick the best replay (target) family for a recorded (source) family.
// Preference order:
//   1. a target family whose capabilities are a superset of the source family's;
//   2. otherwise the target family with the largest capability overlap;
//   3. otherwise family 0.
// Distinct source families spread across
// the available target families instead of piling onto the lowest matching one.
uint32_t MatchFamily(const VkQueueFamilyProperties& source,
                     const std::vector<VkQueueFamilyProperties>& targets,
                     const std::vector<bool>& used) {
  const VkQueueFlags wanted = source.queueFlags;

  uint32_t bestSuperset = UINT32_MAX;
  bool bestSupersetUsed = true;
  uint32_t bestSupersetExtra = UINT32_MAX;
  uint32_t bestOverlap = UINT32_MAX;
  bool bestOverlapUsed = true;
  uint32_t bestOverlapCount = 0;

  for (uint32_t t = 0; t < targets.size(); ++t) {
    const VkQueueFlags flags = targets[t].queueFlags;
    const bool isUsed = used[t];
    if ((flags & wanted) == wanted) {
      const uint32_t extra = PopCount(flags & ~wanted);
      const bool better = bestSuperset == UINT32_MAX || (!isUsed && bestSupersetUsed) ||
                          (isUsed == bestSupersetUsed && extra < bestSupersetExtra);
      if (better) {
        bestSuperset = t;
        bestSupersetUsed = isUsed;
        bestSupersetExtra = extra;
      }
    }
    const uint32_t overlap = PopCount(flags & wanted);
    if (overlap > 0) {
      const bool better = bestOverlap == UINT32_MAX || (!isUsed && bestOverlapUsed) ||
                          (isUsed == bestOverlapUsed && overlap > bestOverlapCount);
      if (better) {
        bestOverlap = t;
        bestOverlapUsed = isUsed;
        bestOverlapCount = overlap;
      }
    }
  }

  if (bestSuperset != UINT32_MAX) {
    return bestSuperset;
  }
  if (bestOverlap != UINT32_MAX) {
    return bestOverlap;
  }
  return 0;
}

// Extract the base VkQueueFamilyProperties out of the VkQueueFamilyProperties2
// array used by the *QueueFamilyProperties2 query variants.
std::vector<VkQueueFamilyProperties> ToBaseProperties(const VkQueueFamilyProperties2* props,
                                                      uint32_t count) {
  std::vector<VkQueueFamilyProperties> out;
  if (props == nullptr) {
    return out;
  }
  out.reserve(count);
  for (uint32_t i = 0; i < count; ++i) {
    out.push_back(props[i].queueFamilyProperties);
  }
  return out;
}

uint64_t ConvertAddress(uint64_t address, VulkanCaptureReplayAddressForm form) {
  switch (form) {
  case VulkanCaptureReplayAddressForm::BITS_48:
    return portability::ToBits48Address(address);
  case VulkanCaptureReplayAddressForm::CANONICAL:
    return portability::ToCanonicalAddress(address);
  default:
    return address;
  }
}

} // namespace

thread_local PortabilityLayer::ConvertedAddress PortabilityLayer::tl_ConvertedAddress;

PortabilityLayer::PortabilityLayer(DispatchTablesHolder& dispatchTablesHolder)
    : Layer("Portability"),
      m_DispatchTablesHolder(dispatchTablesHolder),
      m_RemapQueueFamilies(Configurator::Get().vulkan.player.portability.remapQueueFamilies),
      m_ReplaceScratch(
          Configurator::Get().vulkan.player.portability.replaceAccelerationStructureScratch),
      m_Scratch(dispatchTablesHolder),
      m_RelocateSparseBuffers(Configurator::Get().vulkan.player.portability.relocateSparseBuffers),
      m_SparseBuffers(dispatchTablesHolder),
      m_RealignUniformBuffers(Configurator::Get().vulkan.player.portability.realignUniformBuffers),
      m_UniformBuffers(dispatchTablesHolder),
      m_RequestedAddressForm(
          Configurator::Get().vulkan.player.portability.captureReplayAddressForm) {
  if (m_ReplaceScratch && Configurator::Get().common.player.subcapture.enabled) {
    // A subcapture can't refer to the scratch buffers created by the layer.
    LOG_WARNING << "Portability: acceleration structure scratch replacement is disabled during "
                   "subcapture.";
    m_ReplaceScratch = false;
  }
  if (m_RelocateSparseBuffers && Configurator::Get().common.player.subcapture.enabled) {
    // A subcapture would record the relocated buffer without its recorded address.
    LOG_WARNING << "Portability: sparse buffer relocation is disabled during subcapture.";
    m_RelocateSparseBuffers = false;
  }
  if (m_RealignUniformBuffers && Configurator::Get().common.player.subcapture.enabled) {
    // A subcapture would record the descriptors pointing to the GITS-owned copies.
    LOG_WARNING << "Portability: uniform buffer realignment is disabled during subcapture.";
    m_RealignUniformBuffers = false;
  }
  // Misaligned uniform buffer descriptors are reported even when they aren't realigned.
  m_UniformBuffers.SetEnabled(m_RealignUniformBuffers);
}

// --- Building the remap from the queue family property queries ---------------

void PortabilityLayer::StoreSource(VkPhysicalDevice physicalDevice,
                                   const VkQueueFamilyProperties* props,
                                   uint32_t count) {
  if (!m_RemapQueueFamilies || props == nullptr || count == 0) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_Families[physicalDevice].source.assign(props, props + count);
}

void PortabilityLayer::StoreTargetAndBuild(VkPhysicalDevice physicalDevice,
                                           const VkQueueFamilyProperties* props,
                                           uint32_t count) {
  if (!m_RemapQueueFamilies || props == nullptr || count == 0) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  DeviceFamilies& families = m_Families[physicalDevice];
  families.target.assign(props, props + count);

  if (families.source.empty() || families.built) {
    return;
  }

  families.remap.resize(families.source.size());
  std::vector<bool> used(families.target.size(), false);
  bool identity = families.source.size() <= families.target.size();
  for (uint32_t s = 0; s < families.source.size(); ++s) {
    const uint32_t t = MatchFamily(families.source[s], families.target, used);
    families.remap[s] = t;
    used[t] = true;
    if (t != s) {
      identity = false;
    }
  }
  families.built = true;

  if (!identity) {
    LOG_INFO << "Portability: built queue family remap for physical device (source families="
             << families.source.size() << ", target families=" << families.target.size() << ").";
    for (uint32_t s = 0; s < families.remap.size(); ++s) {
      LOG_INFO << "Portability:   source family " << s << " -> target family " << families.remap[s];
    }
  }
}

void PortabilityLayer::Pre(vkGetPhysicalDeviceQueueFamilyPropertiesCommand& command) {
  // The recorded (source) properties are still in the array before the live
  // driver call overwrites it.
  StoreSource(command.m_physicalDevice.Value, command.m_pQueueFamilyProperties.Value,
              command.m_pQueueFamilyPropertyCount.Value ? *command.m_pQueueFamilyPropertyCount.Value
                                                        : 0);
}

void PortabilityLayer::Post(vkGetPhysicalDeviceQueueFamilyPropertiesCommand& command) {
  // After the live call the array holds the replay (target) properties. The
  // array was sized to the recorded family count. However, if the replay
  // device has more families than were recorded, the extras are not observed
  // and the remap cannot spread onto them.
  StoreTargetAndBuild(
      command.m_physicalDevice.Value, command.m_pQueueFamilyProperties.Value,
      command.m_pQueueFamilyPropertyCount.Value ? *command.m_pQueueFamilyPropertyCount.Value : 0);
}

void PortabilityLayer::Pre(vkGetPhysicalDeviceQueueFamilyProperties2Command& command) {
  const uint32_t count =
      command.m_pQueueFamilyPropertyCount.Value ? *command.m_pQueueFamilyPropertyCount.Value : 0;
  const auto base = ToBaseProperties(command.m_pQueueFamilyProperties.Value, count);
  StoreSource(command.m_physicalDevice.Value, base.data(), static_cast<uint32_t>(base.size()));
}

void PortabilityLayer::Post(vkGetPhysicalDeviceQueueFamilyProperties2Command& command) {
  const uint32_t count =
      command.m_pQueueFamilyPropertyCount.Value ? *command.m_pQueueFamilyPropertyCount.Value : 0;
  const auto base = ToBaseProperties(command.m_pQueueFamilyProperties.Value, count);
  StoreTargetAndBuild(command.m_physicalDevice.Value, base.data(),
                      static_cast<uint32_t>(base.size()));
}

void PortabilityLayer::Pre(vkGetPhysicalDeviceQueueFamilyProperties2KHRCommand& command) {
  const uint32_t count =
      command.m_pQueueFamilyPropertyCount.Value ? *command.m_pQueueFamilyPropertyCount.Value : 0;
  const auto base = ToBaseProperties(command.m_pQueueFamilyProperties.Value, count);
  StoreSource(command.m_physicalDevice.Value, base.data(), static_cast<uint32_t>(base.size()));
}

void PortabilityLayer::Post(vkGetPhysicalDeviceQueueFamilyProperties2KHRCommand& command) {
  const uint32_t count =
      command.m_pQueueFamilyPropertyCount.Value ? *command.m_pQueueFamilyPropertyCount.Value : 0;
  const auto base = ToBaseProperties(command.m_pQueueFamilyProperties.Value, count);
  StoreTargetAndBuild(command.m_physicalDevice.Value, base.data(),
                      static_cast<uint32_t>(base.size()));
}

// --- Activating the remap and merging colliding queue requests ---------------

void PortabilityLayer::Pre(vkCreateDeviceCommand& command) {
  VkDeviceCreateInfo* createInfo = command.m_pCreateInfo.Value;
  if (createInfo == nullptr) {
    return;
  }

  std::lock_guard<std::mutex> lock(m_Mutex);

  m_AddressForm = SelectAddressForm(command.m_physicalDevice.Value);
  if (m_ReplaceScratch) {
    m_Scratch.SetPhysicalDevice(command.m_physicalDevice.Value);
  }
  if (m_RelocateSparseBuffers) {
    m_SparseBuffers.SetPhysicalDevice(command.m_physicalDevice.Value);
  }
  m_UniformBuffers.SetPhysicalDevice(command.m_physicalDevice.Value);

  auto it = m_Families.find(command.m_physicalDevice.Value);
  if (it == m_Families.end() || !it->second.built) {
    return; // No remap available (e.g. the app never queried families); pass through.
  }
  const DeviceFamilies& families = it->second;

  // Creating another device from a different physical device overwrites remap
  if (m_Active && command.m_physicalDevice.Value != m_ActiveDevice) {
    LOG_WARNING << "Portability: a second logical device is being created from a different "
                   "physical device, but the queue-family remap is global and supports only "
                   "one device. The earlier remap will be overwritten, which can corrupt "
                   "replay of commands on the first device.";
  }

  // Select this device's remap as the active one used by later commands.
  m_FamilyRemap = families.remap;
  m_QueueRemap.clear();
  m_Active = false;
  for (uint32_t s = 0; s < m_FamilyRemap.size(); ++s) {
    if (m_FamilyRemap[s] != s) {
      m_Active = true;
    }
  }
  if (families.source.size() > families.target.size()) {
    m_Active = true;
  }
  if (!m_Active) {
    return; // Identity remap: nothing to rewrite.
  }
  m_ActiveDevice = command.m_physicalDevice.Value;

  // Merge the recorded VkDeviceQueueCreateInfo entries by target family.  Two
  // source families that remap onto the same target family must collapse into a
  // single entry, and the per-family queueCount must be clamped to what the
  // target family supports (extra queues alias onto the last available one).
  m_MergedQueueInfos.clear();
  m_MergedPriorities.clear();

  // target family -> index into m_MergedQueueInfos / m_MergedPriorities
  std::unordered_map<uint32_t, uint32_t> familyToEntry;

  // Number of recorded queues that had to alias onto an already-used target queue
  // because the target family exposes fewer queues than the stream requested.
  uint32_t aliasedQueues = 0;
  const uint32_t recordedQueueInfoCount = createInfo->queueCreateInfoCount;

  for (uint32_t i = 0; i < createInfo->queueCreateInfoCount; ++i) {
    const VkDeviceQueueCreateInfo& src = createInfo->pQueueCreateInfos[i];
    const uint32_t srcFamily = src.queueFamilyIndex;
    const uint32_t tgtFamily = RemapFamily(srcFamily);
    const uint32_t tgtMax =
        tgtFamily < families.target.size() ? families.target[tgtFamily].queueCount : 1;

    auto entryIt = familyToEntry.find(tgtFamily);
    uint32_t entryIdx;
    if (entryIt == familyToEntry.end()) {
      entryIdx = static_cast<uint32_t>(m_MergedQueueInfos.size());
      familyToEntry[tgtFamily] = entryIdx;
      VkDeviceQueueCreateInfo info{};
      info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      // Carry the recorded creation parameters: flags (e.g.
      // VK_DEVICE_QUEUE_CREATE_PROTECTED_BIT) and the pNext chain (e.g.
      // VkDeviceQueueGlobalPriorityCreateInfoKHR).  The pNext storage lives in the
      // decoded stream, which outlives the driver call, so the pointer is valid.
      info.pNext = src.pNext;
      info.flags = src.flags;
      info.queueFamilyIndex = tgtFamily;
      info.queueCount = 0;
      m_MergedQueueInfos.push_back(info);
      m_MergedPriorities.emplace_back();
    } else {
      entryIdx = entryIt->second;
      // A second source family collapsed onto this target family.  We keep the
      // first entry's flags/pNext; a single merged entry cannot honor conflicting
      // per-family creation parameters, so warn if they disagree.
      const VkDeviceQueueCreateInfo& kept = m_MergedQueueInfos[entryIdx];
      if (kept.flags != src.flags || kept.pNext != src.pNext) {
        LOG_WARNING << "Portability: source families merged onto target family " << tgtFamily
                    << " have differing queue-create flags/pNext; keeping the first and "
                       "dropping the rest.";
      }
    }

    const uint32_t base = m_MergedQueueInfos[entryIdx].queueCount;
    const uint32_t maxQueueIndex = tgtMax > 0 ? tgtMax - 1 : 0u;
    for (uint32_t q = 0; q < src.queueCount; ++q) {
      // Assign the next free queue index in the target family, clamping (and so
      // aliasing) when the recorded stream asked for more queues than exist.
      const uint32_t tgtQueue = std::min(base + q, maxQueueIndex);
      if (base + q > maxQueueIndex) {
        ++aliasedQueues;
      }
      const uint64_t key = (static_cast<uint64_t>(srcFamily) << 32) | q;
      const uint64_t value = (static_cast<uint64_t>(tgtFamily) << 32) | tgtQueue;
      m_QueueRemap[key] = value;
    }
    const uint32_t newCount = std::min(base + src.queueCount, tgtMax > 0 ? tgtMax : 1u);
    m_MergedQueueInfos[entryIdx].queueCount = newCount;

    // Keep a priorities array at least as long as queueCount.  Reuse the
    // recorded priorities where available, pad the rest with 1.0f.
    std::vector<float>& priorities = m_MergedPriorities[entryIdx];
    priorities.resize(newCount, 1.0f);
    if (src.pQueuePriorities != nullptr) {
      for (uint32_t q = 0; q < src.queueCount && base + q < newCount; ++q) {
        priorities[base + q] = src.pQueuePriorities[q];
      }
    }
  }

  for (uint32_t e = 0; e < m_MergedQueueInfos.size(); ++e) {
    m_MergedQueueInfos[e].pQueuePriorities = m_MergedPriorities[e].data();
  }

  LOG_INFO << "Portability: vkCreateDevice remapped " << recordedQueueInfoCount
           << " recorded queue-create-info(s) into " << m_MergedQueueInfos.size()
           << " target queue-create-info(s).";
  for (const auto& mergedQueueInfo : m_MergedQueueInfos) {
    LOG_INFO << "Portability:   target family " << mergedQueueInfo.queueFamilyIndex
             << " queueCount=" << mergedQueueInfo.queueCount;
  }
  if (aliasedQueues > 0) {
    LOG_WARNING << "Portability: " << aliasedQueues
                << " recorded queue(s) aliased onto fewer target queues; work that ran on "
                   "separate queues at capture time will serialize on replay.";
  }

  createInfo->queueCreateInfoCount = static_cast<uint32_t>(m_MergedQueueInfos.size());
  createInfo->pQueueCreateInfos = m_MergedQueueInfos.data();
}

// --- Replacing acceleration structure build scratch memory --------------------

void PortabilityLayer::Pre(vkCmdBuildAccelerationStructuresKHRCommand& command) {
  if (!m_ReplaceScratch || command.m_Skip) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  if (m_ActiveScratchDevice == VK_NULL_HANDLE) {
    return;
  }
  m_Scratch.ReplaceScratch(m_ActiveScratchDevice, command.m_commandBuffer.Value,
                           command.m_infoCount.Value, command.m_pInfos.Value,
                           command.m_ppBuildRangeInfos.Value);
}

void PortabilityLayer::Post(vkAllocateCommandBuffersCommand& command) {
  if (!m_ReplaceScratch || command.m_Return.Value != VK_SUCCESS ||
      command.m_pAllocateInfo.Value == nullptr) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_ActiveScratchDevice = command.m_device.Value;
  m_Scratch.AddCommandBuffers(command.m_pAllocateInfo.Value->commandPool,
                              command.m_pCommandBuffers.Value,
                              command.m_pAllocateInfo.Value->commandBufferCount);
}

void PortabilityLayer::Pre(vkBeginCommandBufferCommand& command) {
  if (!m_ReplaceScratch && !m_RealignUniformBuffers) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  if (m_RealignUniformBuffers) {
    m_UniformBuffers.BeginCommandBuffer(command.m_commandBuffer.Value);
  }
  if (m_ReplaceScratch) {
    m_Scratch.ResetCommandBuffer(command.m_commandBuffer.Value);
  }
}

void PortabilityLayer::Pre(vkResetCommandBufferCommand& command) {
  if (!m_ReplaceScratch) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_Scratch.ResetCommandBuffer(command.m_commandBuffer.Value);
}

void PortabilityLayer::Pre(vkResetCommandPoolCommand& command) {
  if (!m_ReplaceScratch) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_Scratch.ResetCommandPool(command.m_commandPool.Value);
}

void PortabilityLayer::Pre(vkFreeCommandBuffersCommand& command) {
  if (!m_ReplaceScratch && !m_RealignUniformBuffers) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  if (m_RealignUniformBuffers) {
    m_UniformBuffers.FreeCommandBuffers(command.m_pCommandBuffers.Value,
                                        command.m_commandBufferCount.Value);
  }
  if (m_ReplaceScratch) {
    m_Scratch.FreeCommandBuffers(command.m_commandPool.Value, command.m_pCommandBuffers.Value,
                                 command.m_commandBufferCount.Value);
  }
}

void PortabilityLayer::Pre(vkDestroyCommandPoolCommand& command) {
  if (!m_ReplaceScratch) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_Scratch.DestroyCommandPool(command.m_commandPool.Value);
}

void PortabilityLayer::Pre(vkDestroyDeviceCommand& command) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_UniformBuffers.DestroyDevice(command.m_device.Value);
  if (m_RelocateSparseBuffers) {
    m_SparseBuffers.DestroyDevice();
  }
  if (m_ReplaceScratch) {
    m_Scratch.DestroyDevice(command.m_device.Value);
  }
}

// --- Relocating sparse buffers -------------------------------------------------

void PortabilityLayer::Post(vkCreateBufferCommand& command) {
  if (!m_RelocateSparseBuffers && !m_RealignUniformBuffers) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  if (m_RealignUniformBuffers && command.m_Return.Value == VK_SUCCESS && command.m_pBuffer.Value &&
      command.m_pCreateInfo.Value) {
    m_UniformBuffers.AddBuffer(*command.m_pBuffer.Value, *command.m_pCreateInfo.Value);
  }
  if (m_RelocateSparseBuffers) {
    m_SparseBuffers.AddBuffer(command.m_device.Value,
                              command.m_Return.Value == VK_SUCCESS && command.m_pBuffer.Value
                                  ? *command.m_pBuffer.Value
                                  : VK_NULL_HANDLE);
  }
}

void PortabilityLayer::Pre(vkDestroyBufferCommand& command) {
  if (!m_RelocateSparseBuffers && !m_RealignUniformBuffers) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  if (m_RealignUniformBuffers) {
    m_UniformBuffers.RemoveBuffer(command.m_buffer.Value);
  }
  if (m_RelocateSparseBuffers) {
    m_SparseBuffers.RemoveBuffer(command.m_buffer.Value);
  }
}

void PortabilityLayer::Pre(vkFreeMemoryCommand& command) {
  if (!m_RealignUniformBuffers) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_UniformBuffers.RemoveMemory(command.m_memory.Value);
}

void PortabilityLayer::Post(vkBindBufferMemoryCommand& command) {
  if (!m_RealignUniformBuffers || command.m_Return.Value != VK_SUCCESS) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_UniformBuffers.BindBufferMemory(command.m_device.Value, command.m_buffer.Value,
                                    command.m_memory.Value, command.m_memoryOffset.Value);
}

void PortabilityLayer::Post(vkBindBufferMemory2Command& command) {
  if (!m_RealignUniformBuffers || command.m_Return.Value != VK_SUCCESS ||
      command.m_pBindInfos.Value == nullptr) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  for (uint32_t i = 0; i < command.m_bindInfoCount.Value; ++i) {
    const VkBindBufferMemoryInfo& bindInfo = command.m_pBindInfos.Value[i];
    m_UniformBuffers.BindBufferMemory(command.m_device.Value, bindInfo.buffer, bindInfo.memory,
                                      bindInfo.memoryOffset);
  }
}

void PortabilityLayer::Post(vkBindBufferMemory2KHRCommand& command) {
  if (!m_RealignUniformBuffers || command.m_Return.Value != VK_SUCCESS ||
      command.m_pBindInfos.Value == nullptr) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  for (uint32_t i = 0; i < command.m_bindInfoCount.Value; ++i) {
    const VkBindBufferMemoryInfo& bindInfo = command.m_pBindInfos.Value[i];
    m_UniformBuffers.BindBufferMemory(command.m_device.Value, bindInfo.buffer, bindInfo.memory,
                                      bindInfo.memoryOffset);
  }
}

void PortabilityLayer::Pre(vkCmdTraceRaysKHRCommand& command) {
  if (!m_RelocateSparseBuffers) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_SparseBuffers.TranslateRegion(command.m_pRaygenShaderBindingTable.Value);
  m_SparseBuffers.TranslateRegion(command.m_pMissShaderBindingTable.Value);
  m_SparseBuffers.TranslateRegion(command.m_pHitShaderBindingTable.Value);
  m_SparseBuffers.TranslateRegion(command.m_pCallableShaderBindingTable.Value);
}

void PortabilityLayer::Pre(vkCmdTraceRaysIndirectKHRCommand& command) {
  if (!m_RelocateSparseBuffers) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_SparseBuffers.TranslateRegion(command.m_pRaygenShaderBindingTable.Value);
  m_SparseBuffers.TranslateRegion(command.m_pMissShaderBindingTable.Value);
  m_SparseBuffers.TranslateRegion(command.m_pHitShaderBindingTable.Value);
  m_SparseBuffers.TranslateRegion(command.m_pCallableShaderBindingTable.Value);
}

// --- Realigning uniform buffer descriptors ----------------------------------

void PortabilityLayer::Post(vkGetDeviceQueueCommand& command) {
  if (!m_RealignUniformBuffers || command.m_pQueue.Value == nullptr) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_UniformBuffers.AddQueue(command.m_device.Value, command.m_queueFamilyIndex.Value,
                            *command.m_pQueue.Value);
}

void PortabilityLayer::Post(vkGetDeviceQueue2Command& command) {
  if (!m_RealignUniformBuffers || command.m_pQueue.Value == nullptr ||
      command.m_pQueueInfo.Value == nullptr) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_UniformBuffers.AddQueue(command.m_device.Value, command.m_pQueueInfo.Value->queueFamilyIndex,
                            *command.m_pQueue.Value);
}

void PortabilityLayer::Post(vkAllocateDescriptorSetsCommand& command) {
  if (!m_RealignUniformBuffers || command.m_Return.Value != VK_SUCCESS ||
      command.m_pAllocateInfo.Value == nullptr || command.m_pDescriptorSets.Value == nullptr) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_UniformBuffers.AddDescriptorSets(command.m_pAllocateInfo.Value->descriptorPool,
                                     command.m_pDescriptorSets.Value,
                                     command.m_pAllocateInfo.Value->descriptorSetCount);
}

void PortabilityLayer::Pre(vkFreeDescriptorSetsCommand& command) {
  if (!m_RealignUniformBuffers) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_UniformBuffers.FreeDescriptorSets(command.m_pDescriptorSets.Value,
                                      command.m_descriptorSetCount.Value);
}

void PortabilityLayer::Pre(vkResetDescriptorPoolCommand& command) {
  if (!m_RealignUniformBuffers) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_UniformBuffers.ResetDescriptorPool(command.m_descriptorPool.Value);
}

void PortabilityLayer::Pre(vkDestroyDescriptorPoolCommand& command) {
  if (!m_RealignUniformBuffers) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_UniformBuffers.ResetDescriptorPool(command.m_descriptorPool.Value);
}

void PortabilityLayer::Pre(vkUpdateDescriptorSetsCommand& command) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_UniformBuffers.UpdateDescriptorSets(command.m_device.Value,
                                        command.m_descriptorWriteCount.Value,
                                        command.m_pDescriptorWrites.Value, command.m_Key);
}

void PortabilityLayer::Post(vkUpdateDescriptorSetsCommand& command) {
  if (!m_RealignUniformBuffers || command.m_descriptorCopyCount.Value == 0) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_UniformBuffers.CopyDescriptorSets(command.m_device.Value, command.m_descriptorCopyCount.Value,
                                      command.m_pDescriptorCopies.Value);
}

void PortabilityLayer::Pre(vkCmdBindDescriptorSetsCommand& command) {
  if (!m_RealignUniformBuffers) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_UniformBuffers.BindDescriptorSets(command.m_commandBuffer.Value,
                                      command.m_pDescriptorSets.Value,
                                      command.m_descriptorSetCount.Value);
}

void PortabilityLayer::Pre(vkCmdBindDescriptorSets2Command& command) {
  const VkBindDescriptorSetsInfo* info = command.m_pBindDescriptorSetsInfo.Value;
  if (!m_RealignUniformBuffers || info == nullptr) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_UniformBuffers.BindDescriptorSets(command.m_commandBuffer.Value, info->pDescriptorSets,
                                      info->descriptorSetCount);
}

void PortabilityLayer::Pre(vkCmdBindDescriptorSets2KHRCommand& command) {
  const VkBindDescriptorSetsInfo* info = command.m_pBindDescriptorSetsInfo.Value;
  if (!m_RealignUniformBuffers || info == nullptr) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_UniformBuffers.BindDescriptorSets(command.m_commandBuffer.Value, info->pDescriptorSets,
                                      info->descriptorSetCount);
}

void PortabilityLayer::Pre(vkCmdExecuteCommandsCommand& command) {
  if (!m_RealignUniformBuffers) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_UniformBuffers.ExecuteCommands(command.m_commandBuffer.Value, command.m_pCommandBuffers.Value,
                                   command.m_commandBufferCount.Value);
}

void PortabilityLayer::Pre(vkQueueSubmitCommand& command) {
  if (!m_RealignUniformBuffers) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_UniformBuffers.PrepareSubmit(command.m_queue.Value, command.m_submitCount.Value,
                                 const_cast<VkSubmitInfo*>(command.m_pSubmits.Value));
}

void PortabilityLayer::Post(vkQueueSubmitCommand& command) {
  if (!m_RealignUniformBuffers) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_UniformBuffers.FinishSubmit(command.m_queue.Value);
}

void PortabilityLayer::Pre(vkQueueSubmit2Command& command) {
  if (!m_RealignUniformBuffers) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_UniformBuffers.PrepareSubmit2(command.m_queue.Value, command.m_submitCount.Value,
                                  const_cast<VkSubmitInfo2*>(command.m_pSubmits.Value));
}

void PortabilityLayer::Post(vkQueueSubmit2Command& command) {
  if (!m_RealignUniformBuffers) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_UniformBuffers.FinishSubmit(command.m_queue.Value);
}

void PortabilityLayer::Pre(vkQueueSubmit2KHRCommand& command) {
  if (!m_RealignUniformBuffers) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_UniformBuffers.PrepareSubmit2(command.m_queue.Value, command.m_submitCount.Value,
                                  const_cast<VkSubmitInfo2*>(command.m_pSubmits.Value));
}

void PortabilityLayer::Post(vkQueueSubmit2KHRCommand& command) {
  if (!m_RealignUniformBuffers) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_UniformBuffers.FinishSubmit(command.m_queue.Value);
}

// --- Converting memory opaque capture addresses ------------------------------

VulkanCaptureReplayAddressForm PortabilityLayer::SelectAddressForm(
    VkPhysicalDevice physicalDevice) const {
  if (m_RequestedAddressForm != VulkanCaptureReplayAddressForm::AUTO) {
    return m_RequestedAddressForm;
  }

  VkInstanceLevelDispatchTable* dispatchTable =
      m_DispatchTablesHolder.GetInstanceDispatchTable(physicalDevice);
  PFN_vkGetPhysicalDeviceProperties2 vkGetPhysicalDeviceProperties2Unified = nullptr;
  if (dispatchTable != nullptr) {
    vkGetPhysicalDeviceProperties2Unified = dispatchTable->vkGetPhysicalDeviceProperties2
                                                ? dispatchTable->vkGetPhysicalDeviceProperties2
                                                : dispatchTable->vkGetPhysicalDeviceProperties2KHR;
  }
  if (vkGetPhysicalDeviceProperties2Unified == nullptr) {
    LOG_WARNING << "Portability: cannot query the replay driver, memory opaque capture addresses "
                   "are passed unchanged.";
    return VulkanCaptureReplayAddressForm::KEEP;
  }

  VkPhysicalDeviceDriverProperties driverProperties{};
  driverProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;
  VkPhysicalDeviceProperties2 properties{};
  properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
  properties.pNext = &driverProperties;
  vkGetPhysicalDeviceProperties2Unified(physicalDevice, &properties);

  // Mesa ANV reports and accepts memory opaque capture addresses zero-extended to
  // 48 bits, the Intel Windows driver reports them canonical. Buffer device
  // addresses are canonical on both.
  VulkanCaptureReplayAddressForm form = VulkanCaptureReplayAddressForm::KEEP;
  if (driverProperties.driverID == VK_DRIVER_ID_INTEL_OPEN_SOURCE_MESA) {
    form = VulkanCaptureReplayAddressForm::BITS_48;
  } else if (driverProperties.driverID == VK_DRIVER_ID_INTEL_PROPRIETARY_WINDOWS) {
    form = VulkanCaptureReplayAddressForm::CANONICAL;
  }
  LOG_INFO << "Portability: memory opaque capture address form for driver \""
           << driverProperties.driverName << "\": " << form << ".";
  return form;
}

void PortabilityLayer::Pre(vkAllocateMemoryCommand& command) {
  tl_ConvertedAddress = {};
  const VkMemoryAllocateInfo* allocateInfo = command.m_pAllocateInfo.Value;
  if (allocateInfo == nullptr) {
    return;
  }
  auto* opaqueAddressInfo = const_cast<VkMemoryOpaqueCaptureAddressAllocateInfo*>(
      static_cast<const VkMemoryOpaqueCaptureAddressAllocateInfo*>(getPNextStructure(
          allocateInfo->pNext, VK_STRUCTURE_TYPE_MEMORY_OPAQUE_CAPTURE_ADDRESS_ALLOCATE_INFO)));
  if (opaqueAddressInfo == nullptr || opaqueAddressInfo->opaqueCaptureAddress == 0) {
    return;
  }

  std::lock_guard<std::mutex> lock(m_Mutex);
  const uint64_t recorded = opaqueAddressInfo->opaqueCaptureAddress;
  const uint64_t replayed = ConvertAddress(recorded, m_AddressForm);
  tl_ConvertedAddress = {recorded, replayed};
  if (replayed == recorded) {
    return;
  }
  if (m_ConvertedAddressCount++ == 0) {
    LOG_INFO << "Portability: converting memory opaque capture addresses to the " << m_AddressForm
             << " form (first: 0x" << std::hex << recorded << " -> 0x" << replayed << std::dec
             << ").";
  }
  opaqueAddressInfo->opaqueCaptureAddress = replayed;
}

void PortabilityLayer::Post(vkAllocateMemoryCommand& command) {
  const ConvertedAddress converted = tl_ConvertedAddress;
  tl_ConvertedAddress = {};
  if (m_RealignUniformBuffers && command.m_Return.Value == VK_SUCCESS &&
      command.m_pMemory.Value != nullptr && command.m_pAllocateInfo.Value != nullptr) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_UniformBuffers.AddMemory(*command.m_pMemory.Value, *command.m_pAllocateInfo.Value);
  }
  if (converted.Recorded == 0 || command.m_Return.Value == VK_SUCCESS) {
    return;
  }
  LOG_ERROR << "Portability: vkAllocateMemory failed for memory opaque capture address 0x"
            << std::hex << converted.Replayed << " (recorded 0x" << converted.Recorded << ")"
            << std::dec
            << ". The address range is not available on the replay device; the stream "
               "cannot be replayed with its recorded GPU virtual addresses.";
  if (m_ReplaceScratch) {
    // The driver chooses the addresses of the scratch memory, which can take a range
    // recorded allocations need.
    LOG_ERROR << "Portability: acceleration structure scratch memory allocated by GITS may "
                 "occupy the range, try replaying without "
                 "Vulkan.Player.Portability.ReplaceAccelerationStructureScratch.";
  }
}

// --- Applying the active remap ----------------------------------------------

uint32_t PortabilityLayer::RemapFamily(uint32_t family) const {
  if (!m_Active || family >= m_FamilyRemap.size()) {
    // Out-of-range values include VK_QUEUE_FAMILY_IGNORED / _EXTERNAL /
    // _FOREIGN_EXT, which must be passed through untouched.
    return family;
  }
  return m_FamilyRemap[family];
}

uint32_t PortabilityLayer::ClampQueueIndex(uint32_t targetFamily, uint32_t queueIndex) const {
  // Callers hold m_Mutex; use find() to avoid mutating m_Families from a const method.
  auto it = m_Families.find(m_ActiveDevice);
  if (it == m_Families.end() || targetFamily >= it->second.target.size()) {
    return queueIndex;
  }
  const uint32_t queueCount = it->second.target[targetFamily].queueCount;
  if (queueCount == 0 || queueIndex < queueCount) {
    return queueIndex;
  }
  LOG_WARNING << "Portability: a queue was requested at index " << queueIndex
              << " on target family " << targetFamily << " which exposes only " << queueCount
              << " queue(s); clamping to " << (queueCount - 1) << ".";
  return queueCount - 1;
}

uint32_t PortabilityLayer::RemapSharingIndices(uint32_t* indices, uint32_t count) const {
  if (indices == nullptr || count == 0) {
    return count;
  }
  uint32_t out = 0;
  for (uint32_t i = 0; i < count; ++i) {
    const uint32_t remapped = RemapFamily(indices[i]);
    bool duplicate = false;
    for (uint32_t j = 0; j < out; ++j) {
      if (indices[j] == remapped) {
        duplicate = true;
        break;
      }
    }
    if (!duplicate) {
      indices[out++] = remapped;
    }
  }
  return out;
}

template <typename Barrier>
void PortabilityLayer::RemapBarriers(Barrier* barriers, uint32_t count) const {
  if (barriers == nullptr) {
    return;
  }
  for (uint32_t i = 0; i < count; ++i) {
    barriers[i].srcQueueFamilyIndex = RemapFamily(barriers[i].srcQueueFamilyIndex);
    barriers[i].dstQueueFamilyIndex = RemapFamily(barriers[i].dstQueueFamilyIndex);
  }
}

void PortabilityLayer::Pre(vkGetDeviceQueueCommand& command) {
  if (!m_Active) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  const uint64_t key =
      (static_cast<uint64_t>(command.m_queueFamilyIndex.Value) << 32) | command.m_queueIndex.Value;
  auto it = m_QueueRemap.find(key);
  if (it != m_QueueRemap.end()) {
    command.m_queueFamilyIndex.Value = static_cast<uint32_t>(it->second >> 32);
    command.m_queueIndex.Value = static_cast<uint32_t>(it->second & 0xFFFFFFFF);
  } else {
    command.m_queueFamilyIndex.Value = RemapFamily(command.m_queueFamilyIndex.Value);
    command.m_queueIndex.Value =
        ClampQueueIndex(command.m_queueFamilyIndex.Value, command.m_queueIndex.Value);
  }
}

void PortabilityLayer::Pre(vkGetDeviceQueue2Command& command) {
  if (!m_Active || command.m_pQueueInfo.Value == nullptr) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  VkDeviceQueueInfo2* info = command.m_pQueueInfo.Value;
  const uint64_t key = (static_cast<uint64_t>(info->queueFamilyIndex) << 32) | info->queueIndex;
  auto it = m_QueueRemap.find(key);
  if (it != m_QueueRemap.end()) {
    info->queueFamilyIndex = static_cast<uint32_t>(it->second >> 32);
    info->queueIndex = static_cast<uint32_t>(it->second & 0xFFFFFFFF);
  } else {
    info->queueFamilyIndex = RemapFamily(info->queueFamilyIndex);
    info->queueIndex = ClampQueueIndex(info->queueFamilyIndex, info->queueIndex);
  }
}

void PortabilityLayer::Pre(vkCreateCommandPoolCommand& command) {
  if (!m_Active || command.m_pCreateInfo.Value == nullptr) {
    return;
  }
  command.m_pCreateInfo.Value->queueFamilyIndex =
      RemapFamily(command.m_pCreateInfo.Value->queueFamilyIndex);
}

void PortabilityLayer::Pre(vkCreateBufferCommand& command) {
  VkBufferCreateInfo* info = command.m_pCreateInfo.Value;
  if (m_RelocateSparseBuffers && info != nullptr) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_SparseBuffers.PrepareBuffer(command.m_device.Value, *info, [this](uint64_t address) {
      return ConvertAddress(address, m_AddressForm);
    });
  }
  if (!m_Active || info == nullptr || info->sharingMode != VK_SHARING_MODE_CONCURRENT) {
    return;
  }
  uint32_t* indices = const_cast<uint32_t*>(info->pQueueFamilyIndices);
  const uint32_t unique = RemapSharingIndices(indices, info->queueFamilyIndexCount);
  if (unique <= 1) {
    // A concurrent buffer shared by a single (collapsed) family is just an
    // exclusive buffer; concurrent sharing with one family is invalid.
    info->sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info->queueFamilyIndexCount = 0;
    info->pQueueFamilyIndices = nullptr;
  } else {
    info->queueFamilyIndexCount = unique;
  }
}

void PortabilityLayer::Pre(vkCreateImageCommand& command) {
  VkImageCreateInfo* info = command.m_pCreateInfo.Value;
  if (!m_Active || info == nullptr || info->sharingMode != VK_SHARING_MODE_CONCURRENT) {
    return;
  }
  uint32_t* indices = const_cast<uint32_t*>(info->pQueueFamilyIndices);
  const uint32_t unique = RemapSharingIndices(indices, info->queueFamilyIndexCount);
  if (unique <= 1) {
    info->sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info->queueFamilyIndexCount = 0;
    info->pQueueFamilyIndices = nullptr;
  } else {
    info->queueFamilyIndexCount = unique;
  }
}

void PortabilityLayer::Pre(vkCreateSwapchainKHRCommand& command) {
  VkSwapchainCreateInfoKHR* info = command.m_pCreateInfo.Value;
  if (!m_Active || info == nullptr || info->imageSharingMode != VK_SHARING_MODE_CONCURRENT) {
    return;
  }
  uint32_t* indices = const_cast<uint32_t*>(info->pQueueFamilyIndices);
  const uint32_t unique = RemapSharingIndices(indices, info->queueFamilyIndexCount);
  if (unique <= 1) {
    info->imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info->queueFamilyIndexCount = 0;
    info->pQueueFamilyIndices = nullptr;
  } else {
    info->queueFamilyIndexCount = unique;
  }
}

void PortabilityLayer::Pre(vkCmdPipelineBarrierCommand& command) {
  if (!m_Active) {
    return;
  }
  RemapBarriers(command.m_pBufferMemoryBarriers.Value, command.m_pBufferMemoryBarriers.Size);
  RemapBarriers(command.m_pImageMemoryBarriers.Value, command.m_pImageMemoryBarriers.Size);
}

void PortabilityLayer::Pre(vkCmdPipelineBarrier2Command& command) {
  if (!m_Active || command.m_pDependencyInfo.Value == nullptr) {
    return;
  }
  VkDependencyInfo* dep = command.m_pDependencyInfo.Value;
  RemapBarriers(const_cast<VkBufferMemoryBarrier2*>(dep->pBufferMemoryBarriers),
                dep->bufferMemoryBarrierCount);
  RemapBarriers(const_cast<VkImageMemoryBarrier2*>(dep->pImageMemoryBarriers),
                dep->imageMemoryBarrierCount);
}

void PortabilityLayer::Pre(vkCmdPipelineBarrier2KHRCommand& command) {
  if (!m_Active || command.m_pDependencyInfo.Value == nullptr) {
    return;
  }
  VkDependencyInfo* dep = command.m_pDependencyInfo.Value;
  RemapBarriers(const_cast<VkBufferMemoryBarrier2*>(dep->pBufferMemoryBarriers),
                dep->bufferMemoryBarrierCount);
  RemapBarriers(const_cast<VkImageMemoryBarrier2*>(dep->pImageMemoryBarriers),
                dep->imageMemoryBarrierCount);
}

// Do remap for the case where both indices carry a
// concrete recorded family that does not exist on the replay device.
void PortabilityLayer::Pre(vkCmdSetEvent2Command& command) {
  if (!m_Active || command.m_pDependencyInfo.Value == nullptr) {
    return;
  }
  VkDependencyInfo* dep = command.m_pDependencyInfo.Value;
  RemapBarriers(const_cast<VkBufferMemoryBarrier2*>(dep->pBufferMemoryBarriers),
                dep->bufferMemoryBarrierCount);
  RemapBarriers(const_cast<VkImageMemoryBarrier2*>(dep->pImageMemoryBarriers),
                dep->imageMemoryBarrierCount);
}

void PortabilityLayer::Pre(vkCmdSetEvent2KHRCommand& command) {
  if (!m_Active || command.m_pDependencyInfo.Value == nullptr) {
    return;
  }
  VkDependencyInfo* dep = command.m_pDependencyInfo.Value;
  RemapBarriers(const_cast<VkBufferMemoryBarrier2*>(dep->pBufferMemoryBarriers),
                dep->bufferMemoryBarrierCount);
  RemapBarriers(const_cast<VkImageMemoryBarrier2*>(dep->pImageMemoryBarriers),
                dep->imageMemoryBarrierCount);
}

void PortabilityLayer::Pre(vkCmdWaitEventsCommand& command) {
  if (!m_Active) {
    return;
  }
  RemapBarriers(command.m_pBufferMemoryBarriers.Value, command.m_pBufferMemoryBarriers.Size);
  RemapBarriers(command.m_pImageMemoryBarriers.Value, command.m_pImageMemoryBarriers.Size);
}

void PortabilityLayer::Pre(vkCmdWaitEvents2Command& command) {
  if (!m_Active) {
    return;
  }
  for (uint32_t i = 0; i < command.m_pDependencyInfos.Size; ++i) {
    VkDependencyInfo& dep = command.m_pDependencyInfos.Value[i];
    RemapBarriers(const_cast<VkBufferMemoryBarrier2*>(dep.pBufferMemoryBarriers),
                  dep.bufferMemoryBarrierCount);
    RemapBarriers(const_cast<VkImageMemoryBarrier2*>(dep.pImageMemoryBarriers),
                  dep.imageMemoryBarrierCount);
  }
}

void PortabilityLayer::Pre(vkCmdWaitEvents2KHRCommand& command) {
  if (!m_Active) {
    return;
  }
  for (uint32_t i = 0; i < command.m_pDependencyInfos.Size; ++i) {
    VkDependencyInfo& dep = command.m_pDependencyInfos.Value[i];
    RemapBarriers(const_cast<VkBufferMemoryBarrier2*>(dep.pBufferMemoryBarriers),
                  dep.bufferMemoryBarrierCount);
    RemapBarriers(const_cast<VkImageMemoryBarrier2*>(dep.pImageMemoryBarriers),
                  dep.imageMemoryBarrierCount);
  }
}

} // namespace vulkan
} // namespace gits

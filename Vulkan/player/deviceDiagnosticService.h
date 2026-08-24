// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "vulkanHeader2.h"

#include <atomic>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace gits {
namespace vulkan {

class PlayerManager;

// Enables replay-only device-loss diagnostics and owns all marker storage.
// Checkpoint marker addresses must stay valid until the command buffer has
// finished execution, so markers are retained for the lifetime of the player.
class DeviceDiagnosticService {
public:
  explicit DeviceDiagnosticService(PlayerManager& manager);

  void PrepareDeviceCreate(VkPhysicalDevice physicalDevice, VkDeviceCreateInfo* createInfo);
  void TrackDevice(VkPhysicalDevice physicalDevice,
                   VkDevice device,
                   const VkDeviceCreateInfo* createInfo);
  void UntrackDevice(VkDevice device);
  void TrackQueue(VkDevice device, VkQueue queue);
  void TrackCommandPool(VkDevice device, VkCommandPool commandPool, uint32_t queueFamilyIndex);
  void UntrackCommandPool(VkCommandPool commandPool);
  void TrackCommandBuffers(VkDevice device,
                           VkCommandPool commandPool,
                           uint32_t commandBufferCount,
                           const VkCommandBuffer* commandBuffers);
  void UntrackCommandBuffers(uint32_t commandBufferCount, const VkCommandBuffer* commandBuffers);

  void InsertCheckpoint(VkCommandBuffer commandBuffer,
                        uint64_t commandKey,
                        const char* commandName);
  void OnDeviceLost(const char* commandName,
                    uint64_t commandKey,
                    const void* dispatchableHandle = nullptr);

private:
  struct DeviceState {
    VkPhysicalDevice PhysicalDevice{VK_NULL_HANDLE};
    VkDevice Device{VK_NULL_HANDLE};
    bool CheckpointsEnabled{};
    bool DeviceFaultEnabled{};
    std::vector<VkQueueFlags> QueueFamilyFlags;
  };

  struct QueueState {
    VkDevice Device{VK_NULL_HANDLE};
    VkQueue Queue{VK_NULL_HANDLE};
  };

  struct CommandPoolState {
    VkDevice Device{VK_NULL_HANDLE};
    uint32_t QueueFamilyIndex{};
  };

  struct CommandBufferState {
    VkDevice Device{VK_NULL_HANDLE};
    VkCommandPool CommandPool{VK_NULL_HANDLE};
    // Resolved once at allocation time so InsertCheckpoint stays a single hash
    // lookup per command. Null when this buffer must not receive checkpoints.
    PFN_vkCmdSetCheckpointNV CmdSetCheckpoint{};
  };

  struct CheckpointMarker {
    uint64_t CommandKey{};
    const char* CommandName{};
  };

  bool IsExtensionSupported(VkPhysicalDevice physicalDevice, const char* extensionName);
  bool IsDeviceFaultSupported(VkPhysicalDevice physicalDevice);
  void EnableDeviceFaultFeature(VkDeviceCreateInfo* createInfo);
  void DumpCheckpoints(VkDevice lostDevice, VkQueue lostQueue);
  void DumpDeviceFaults(VkDevice lostDevice);
  const DeviceState* FindDevice(VkDevice device) const;
  VkDevice ResolveDevice(const void* dispatchableHandle) const;
  VkQueue ResolveQueue(const void* dispatchableHandle) const;
  const CheckpointMarker* FindMarker(const void* markerAddress) const;

  static bool HasExtension(const VkDeviceCreateInfo* createInfo, const char* extensionName);
  static bool HasDeviceFaultFeature(const VkDeviceCreateInfo* createInfo);

private:
  // A marker must stay alive until the command buffer holding it has finished
  // executing, so markers live in a slot ring that is allocated once. That keeps
  // the addresses stable and the memory bounded, and lets the slot be recovered
  // from the returned pointer without a side table. The ring only has to outlast
  // the commands in flight, which is orders of magnitude below this size.
  static constexpr size_t MarkerRingSize = 1u << 20;

  PlayerManager& m_Manager;
  bool m_CheckpointsRequested{};
  bool m_DeviceFaultRequested{};
  std::atomic<bool> m_DeviceLostHandled{false};
  std::vector<DeviceState> m_Devices;
  std::vector<QueueState> m_Queues;
  std::unordered_map<VkCommandPool, CommandPoolState> m_CommandPools;
  std::unordered_map<VkCommandBuffer, CommandBufferState> m_CommandBuffers;
  std::vector<CheckpointMarker> m_MarkerRing;
  size_t m_NextMarker{};

  static thread_local std::vector<const char*> tl_DeviceExtensionNames;
  static thread_local VkPhysicalDeviceFaultFeaturesEXT tl_DeviceFaultFeatures;
};

} // namespace vulkan
} // namespace gits

// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"

#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace gits {
namespace vulkan {

// The queue family remap is computed from vkGetPhysicalDeviceQueueFamilyProperties:
//   * the source queue family properties are produced in Pre hook;
//   * the target queue family properties are produced in Post hook.
// The layer builds a source->target family remap by capability matching.
//
// Assumption: a single physical+logical device.
class PortabilityLayer : public Layer {
public:
  PortabilityLayer();

  // Build the source->target remap.
  void Pre(vkGetPhysicalDeviceQueueFamilyPropertiesCommand& command) override;
  void Post(vkGetPhysicalDeviceQueueFamilyPropertiesCommand& command) override;
  void Pre(vkGetPhysicalDeviceQueueFamilyProperties2Command& command) override;
  void Post(vkGetPhysicalDeviceQueueFamilyProperties2Command& command) override;
  void Pre(vkGetPhysicalDeviceQueueFamilyProperties2KHRCommand& command) override;
  void Post(vkGetPhysicalDeviceQueueFamilyProperties2KHRCommand& command) override;

  // Activate the remap for the device.
  void Pre(vkCreateDeviceCommand& command) override;

  // Apply the remap.
  void Pre(vkGetDeviceQueueCommand& command) override;
  void Pre(vkGetDeviceQueue2Command& command) override;
  void Pre(vkCreateCommandPoolCommand& command) override;
  void Pre(vkCreateBufferCommand& command) override;
  void Pre(vkCreateImageCommand& command) override;
  void Pre(vkCreateSwapchainKHRCommand& command) override;
  void Pre(vkCmdPipelineBarrierCommand& command) override;
  void Pre(vkCmdPipelineBarrier2Command& command) override;
  void Pre(vkCmdPipelineBarrier2KHRCommand& command) override;
  void Pre(vkCmdSetEvent2Command& command) override;
  void Pre(vkCmdSetEvent2KHRCommand& command) override;
  void Pre(vkCmdWaitEventsCommand& command) override;
  void Pre(vkCmdWaitEvents2Command& command) override;
  void Pre(vkCmdWaitEvents2KHRCommand& command) override;

private:
  // Remap data accumulated per physical device while replaying the queue family
  // property queries.
  struct DeviceFamilies {
    std::vector<VkQueueFamilyProperties> source; // recorded (from the stream)
    std::vector<VkQueueFamilyProperties> target; // replay device (live driver)
    std::vector<uint32_t> remap;                 // source family -> target family
    bool built{false};
  };

  void StoreSource(VkPhysicalDevice physicalDevice,
                   const VkQueueFamilyProperties* props,
                   uint32_t count);
  void StoreTargetAndBuild(VkPhysicalDevice physicalDevice,
                           const VkQueueFamilyProperties* props,
                           uint32_t count);

  // Apply the active family remap to a single index.
  uint32_t RemapFamily(uint32_t family) const;
  uint32_t ClampQueueIndex(uint32_t targetFamily, uint32_t queueIndex) const;
  // Remap an in-place pQueueFamilyIndices array. Deduplicate and return the new count.
  uint32_t RemapSharingIndices(uint32_t* indices, uint32_t count) const;
  // Remap src/dst family ownership transfer indices of a barrier in place.
  template <typename Barrier>
  void RemapBarriers(Barrier* barriers, uint32_t count) const;

  std::mutex m_Mutex;
  std::unordered_map<VkPhysicalDevice, DeviceFamilies> m_Families;

  // A logical device was created with a non-identity remap
  bool m_Active{false};

  // Physical device owning the active remap
  VkPhysicalDevice m_ActiveDevice{VK_NULL_HANDLE};

  // Active source family -> target family
  std::vector<uint32_t> m_FamilyRemap;

  // (sourceFamily << 32 | sourceQueueIndex) -> (targetFamily << 32 | targetQueueIndex)
  std::unordered_map<uint64_t, uint64_t> m_QueueRemap;

  // Storage kept alive for the duration of the vkCreateDevice driver call.
  std::vector<VkDeviceQueueCreateInfo> m_MergedQueueInfos;
  std::vector<std::vector<float>> m_MergedPriorities;
};

} // namespace vulkan
} // namespace gits

// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "vulkanHeader2.h"

#include <cstdint>
#include <map>
#include <optional>
#include <utility>
#include <unordered_map>
#include <vector>

namespace gits {
namespace vulkan {

// Maps a stable Vulkan capture/replay VkDeviceAddress to objectKey.
// Untracked at destroy, unlike the object states, which are retained past it.
// Not scoped per VkDevice - raytracing address resolution assumes a single device.
class DeviceAddressTrackingService {
public:
  // Keep buffer ranges and acceleration-structure addresses in separate namespaces.
  void Track(VkDeviceAddress baseAddress, uint64_t objectKey, VkDeviceSize size);
  void Untrack(uint64_t objectKey);

  void TrackAccelerationStructure(VkDeviceAddress address, uint64_t objectKey);
  void UntrackAccelerationStructure(uint64_t objectKey);

  // Return the buffer range containing address and its byte offset.
  std::optional<std::pair<uint64_t, VkDeviceSize>> FindContaining(VkDeviceAddress address) const;

  std::optional<uint64_t> FindAccelerationStructure(VkDeviceAddress address) const;

private:
  struct BufferAddressRange {
    uint64_t ObjectKey{};
    VkDeviceSize Size{};
  };
  // Aliased buffers share a base address, so keep every alias: destroying one must not
  // unresolve the others. Ordered for the FindContaining predecessor walk.
  std::map<VkDeviceAddress, std::vector<BufferAddressRange>> m_AddressToBuffers;
  std::unordered_map<uint64_t, VkDeviceAddress> m_BufferToAddress;
  // Only grows - it merely bounds the FindContaining walk, so staleness costs a longer scan.
  VkDeviceSize m_MaxTrackedSize{};
  // Single owner suffices - two live acceleration structures never share an address.
  std::map<VkDeviceAddress, uint64_t> m_AccelerationStructureAddressToKey;
  std::unordered_map<uint64_t, VkDeviceAddress> m_AccelerationStructureKeyToAddress;
};

} // namespace vulkan
} // namespace gits

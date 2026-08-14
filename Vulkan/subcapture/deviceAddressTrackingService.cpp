// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "deviceAddressTrackingService.h"

#include "log.h"

#include <algorithm>
#include <ranges>

namespace gits {
namespace vulkan {

void DeviceAddressTrackingService::Track(VkDeviceAddress baseAddress,
                                         uint64_t objectKey,
                                         VkDeviceSize size) {
  if (baseAddress == 0 || objectKey == 0) {
    return;
  }
  if (size == 0) {
    // Never 0 per spec, so BufferState::BufferSize was never populated.
    LOG_TRACE << "Vulkan subcapture: not tracking device address " << baseAddress
              << " for buffer key=" << objectKey << " with unknown size";
    return;
  }
  // A re-track of the same key (address changed) drops the old range first.
  Untrack(objectKey);
  m_AddressToBuffers[baseAddress].push_back({objectKey, size});
  m_BufferToAddress[objectKey] = baseAddress;
  m_MaxTrackedSize = std::max(m_MaxTrackedSize, size);
}

void DeviceAddressTrackingService::Untrack(uint64_t objectKey) {
  auto it = m_BufferToAddress.find(objectKey);
  if (it == m_BufferToAddress.end()) {
    return;
  }
  auto addressIt = m_AddressToBuffers.find(it->second);
  if (addressIt != m_AddressToBuffers.end()) {
    // Drop only this key's range - any alias sharing the address stays resolvable.
    auto& aliases = addressIt->second;
    aliases.erase(std::remove_if(aliases.begin(), aliases.end(),
                                 [objectKey](const BufferAddressRange& range) {
                                   return range.ObjectKey == objectKey;
                                 }),
                  aliases.end());
    if (aliases.empty()) {
      m_AddressToBuffers.erase(addressIt);
    }
  }
  m_BufferToAddress.erase(it);
}

void DeviceAddressTrackingService::TrackAccelerationStructure(VkDeviceAddress address,
                                                              uint64_t objectKey) {
  if (address == 0 || objectKey == 0) {
    return;
  }
  UntrackAccelerationStructure(objectKey);
  auto [it, inserted] = m_AccelerationStructureAddressToKey.try_emplace(address, objectKey);
  if (!inserted) {
    // Evict the previous owner, or its later untrack would erase this entry.
    m_AccelerationStructureKeyToAddress.erase(it->second);
    it->second = objectKey;
  }
  m_AccelerationStructureKeyToAddress[objectKey] = address;
}

void DeviceAddressTrackingService::UntrackAccelerationStructure(uint64_t objectKey) {
  auto it = m_AccelerationStructureKeyToAddress.find(objectKey);
  if (it == m_AccelerationStructureKeyToAddress.end()) {
    return;
  }
  auto addressIt = m_AccelerationStructureAddressToKey.find(it->second);
  // Erase the forward entry only while this key still owns it.
  if (addressIt != m_AccelerationStructureAddressToKey.end() && addressIt->second == objectKey) {
    m_AccelerationStructureAddressToKey.erase(addressIt);
  }
  m_AccelerationStructureKeyToAddress.erase(it);
}

std::optional<std::pair<uint64_t, VkDeviceSize>> DeviceAddressTrackingService::FindContaining(
    VkDeviceAddress address) const {
  auto it = m_AddressToBuffers.upper_bound(address);
  while (it != m_AddressToBuffers.begin()) {
    --it;
    const VkDeviceSize offset = static_cast<VkDeviceSize>(address - it->first);
    // offset only grows from here, so nothing further down can span the address.
    if (offset >= m_MaxTrackedSize) {
      break;
    }
    // Most recent alias first, so a lookup past a small alias falls back to a larger one.
    for (const auto& range : std::ranges::reverse_view(it->second)) {
      if (offset < range.Size) {
        return std::make_pair(range.ObjectKey, offset);
      }
    }
  }
  return std::nullopt;
}

std::optional<uint64_t> DeviceAddressTrackingService::FindAccelerationStructure(
    VkDeviceAddress address) const {
  auto it = m_AccelerationStructureAddressToKey.find(address);
  if (it == m_AccelerationStructureAddressToKey.end()) {
    return std::nullopt;
  }
  return it->second;
}

} // namespace vulkan
} // namespace gits

// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "deviceAddressTrackingService.h"

namespace gits {
namespace vulkan {

void DeviceAddressTrackingService::Track(VkDeviceAddress baseAddress, uint64_t objectKey) {
  if (baseAddress == 0) {
    return;
  }
  m_AddressToKey[baseAddress] = objectKey;
}

std::optional<uint64_t> DeviceAddressTrackingService::Find(VkDeviceAddress address) const {
  auto it = m_AddressToKey.find(address);
  if (it == m_AddressToKey.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::optional<std::pair<uint64_t, VkDeviceSize>> DeviceAddressTrackingService::FindContaining(
    VkDeviceAddress address) const {
  auto it = m_AddressToKey.upper_bound(address);
  if (it == m_AddressToKey.begin()) {
    return std::nullopt;
  }
  --it;
  return std::make_pair(it->second, static_cast<VkDeviceSize>(address - it->first));
}

} // namespace vulkan
} // namespace gits

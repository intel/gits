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

namespace gits {
namespace vulkan {

// Maps a stable Vulkan capture/replay VkDeviceAddress to objectKey.
class DeviceAddressTrackingService {
public:
  void Track(VkDeviceAddress baseAddress, uint64_t objectKey);

  // Exact base-address lookup.
  std::optional<uint64_t> Find(VkDeviceAddress address) const;

  // Floor lookup: greatest tracked base address <= 'address', plus the offset from
  // it. The caller must bounds-check that offset against the object's known size.
  std::optional<std::pair<uint64_t, VkDeviceSize>> FindContaining(VkDeviceAddress address) const;

private:
  std::map<VkDeviceAddress, uint64_t> m_AddressToKey;
};

} // namespace vulkan
} // namespace gits

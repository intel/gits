// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace gits {
namespace vulkan {

// Removes every entry listed in `suppressed` from a Vulkan create-info name
// array (e.g. ppEnabledLayerNames / ppEnabledExtensionNames). The surviving
// pointers are copied into `storage`, and `count`/`names` are updated to
// reference it. `storage` must outlive the API call that consumes `names`.
//
// The original array is left untouched (it may be const stream/app memory), so
// this is a no-op unless something is actually suppressed. Returns the number
// of removed entries.
inline uint32_t RemoveSuppressedNames(const std::vector<std::string>& suppressed,
                                      uint32_t& count,
                                      const char* const*& names,
                                      std::vector<const char*>& storage) {
  if (suppressed.empty() || count == 0 || names == nullptr) {
    return 0;
  }

  storage.clear();
  storage.reserve(count);
  for (uint32_t i = 0; i < count; ++i) {
    const char* name = names[i];
    const bool drop =
        name != nullptr && std::any_of(suppressed.begin(), suppressed.end(),
                                       [name](const std::string& s) { return s == name; });
    if (!drop) {
      storage.push_back(name);
    }
  }

  const uint32_t removed = count - static_cast<uint32_t>(storage.size());
  if (removed > 0) {
    count = static_cast<uint32_t>(storage.size());
    names = storage.data();
  }
  return removed;
}

} // namespace vulkan
} // namespace gits

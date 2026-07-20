// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "vulkanHeader2.h"

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

// Enumerates a layer list via `driverEnumerate` (a callable with the same
// signature as vkEnumerate{Instance,Device}LayerProperties, minus the
// physical device handle, which the caller should already have bound), drops
// the layers named in `suppressed` and writes the filtered result into
// `pProperties`, honoring the Vulkan two-call enumeration idiom
// (pProperties == nullptr queries the count).
template <typename DriverEnumerate>
void ProduceFilteredLayers(const std::vector<std::string>& suppressed,
                           DriverEnumerate&& driverEnumerate,
                           uint32_t* pPropertyCount,
                           VkLayerProperties* pProperties) {
  if (pPropertyCount == nullptr) {
    return;
  }

  uint32_t count = 0;
  std::vector<VkLayerProperties> properties;
  if (driverEnumerate(&count, nullptr) == VK_SUCCESS && count > 0) {
    properties.resize(count);
    if (driverEnumerate(&count, properties.data()) == VK_SUCCESS) {
      properties.resize(count);
      properties.erase(std::remove_if(properties.begin(), properties.end(),
                                      [&suppressed](const VkLayerProperties& layer) {
                                        return std::any_of(suppressed.begin(), suppressed.end(),
                                                           [&layer](const std::string& name) {
                                                             return name == layer.layerName;
                                                           });
                                      }),
                       properties.end());
    }
  }

  if (pProperties == nullptr) {
    *pPropertyCount = static_cast<uint32_t>(properties.size());
  } else {
    const uint32_t writable = std::min(*pPropertyCount, static_cast<uint32_t>(properties.size()));
    for (uint32_t i = 0; i < writable; ++i) {
      pProperties[i] = properties[i];
    }
    *pPropertyCount = writable;
  }
}

} // namespace vulkan
} // namespace gits

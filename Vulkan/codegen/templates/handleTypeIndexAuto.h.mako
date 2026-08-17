// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "vulkanHeader2.h"

#include <cstddef>

namespace gits {
namespace vulkan {

<% handle_types = collect_unique_handle_types(handles) %>\
// Number of distinct Vulkan handle types. Each maps to one bucket in the
// recorder-side handle->key store (see HandleMapService), so that non-unique
// non-dispatchable handle values are disambiguated by object type.
inline constexpr std::size_t kHandleTypeCount = ${len(handle_types)};

// Sentinel returned by HandleTypeIndexFromObjectType for a VkObjectType that
// has no dedicated bucket (e.g. VK_OBJECT_TYPE_UNKNOWN).
inline constexpr std::size_t kInvalidHandleTypeIndex = static_cast<std::size_t>(-1);

// Compile-time, contiguous [0, kHandleTypeCount) bucket index for a Vulkan
// handle type. The primary template is intentionally left undefined so that
// instantiating it for a non-handle type is a compile error. Type aliases
// (e.g. VkDescriptorUpdateTemplateKHR) resolve to their canonical type's
// specialization automatically.
template <typename T>
constexpr std::size_t HandleTypeIndex();

% for i, h in enumerate(handle_types):
template <>
constexpr std::size_t HandleTypeIndex<${h.name}>() {
  return ${i};
}
% endfor

// Runtime VkObjectType -> bucket index, for type-erased uint64 handles that
// carry their concrete type in a sibling field (e.g.
// VkDebugUtilsObjectNameInfoEXT::objectHandle + ::objectType).
inline std::size_t HandleTypeIndexFromObjectType(VkObjectType type) {
  switch (type) {
% for i, h in enumerate(handle_types):
  case ${h.type}:
    return ${i};
% endfor
  default:
    return kInvalidHandleTypeIndex;
  }
}

} // namespace vulkan
} // namespace gits

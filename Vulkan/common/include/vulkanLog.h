// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "log.h"
#include "vulkanHeader.h"

#include <plog/Record.h>

typedef struct PNextPointer* PNextPointerTypeTag;

namespace gits {
namespace Vulkan {

template <typename T>
std::string ToStr(const T& value) {
  if constexpr (std::is_pointer_v<T>) {
    if (value == nullptr) {
      return "NULL";
    }
    std::ostringstream oss;
    oss << "0x" << std::hex << reinterpret_cast<uintptr_t>(value);
    return oss.str();
  } else if constexpr (std::is_enum_v<T>) {
    using UnderlyingType = std::underlying_type_t<T>;
    return std::to_string(static_cast<UnderlyingType>(value));
  } else if constexpr (std::is_arithmetic_v<T>) {
    return std::to_string(value);
  } else if constexpr (std::is_convertible_v<T, std::string>) {
    return std::string(value);
  } else {
    // Template-dependent static_assert: sizeof(T) == 0 ensures this assertion is only
    // evaluated during template instantiation, not during initial template parsing.
    // Using static_assert(false, ...) would cause GCC to fail compilation immediately
    // during the parsing phase, even if this branch is never instantiated.
    static_assert(sizeof(T) == 0, "ToStr specialization required for this type");
  }
  return "";
}

std::string ToStr(const char* str);
std::string ToStr(char* str);

template <typename T, size_t N>
std::string ToStr(const T (&arr)[N]) {
  std::ostringstream oss;
  oss << "[ ";
  for (size_t i = 0; i < N; ++i) {
    if (i > 0) {
      oss << ", ";
    }
    oss << ToStr(arr[i]);
  }
  oss << " ]";
  return oss.str();
}

std::string ToStr(PFN_vkAllocationFunction c);
std::string ToStr(PFN_vkReallocationFunction c);
std::string ToStr(PFN_vkDebugReportCallbackEXT c);
std::string ToStr(PFN_vkFreeFunction c);
std::string ToStr(PFN_vkInternalAllocationNotification c);
std::string ToStr(PFN_vkDebugUtilsMessengerCallbackEXT c);
std::string ToStr(PFN_vkDeviceMemoryReportCallbackEXT c);
//std::string ToStr(PFN_vkFaultCallbackFunction c);
std::string ToStr(PFN_vkVoidFunction c);
std::string ToStr(PFN_vkGetInstanceProcAddr c);
std::string ToStr(PFN_vkGetDeviceProcAddr c);
//std::string ToStr(PFN_GetPhysicalDeviceProcAddr c);
std::string ToStr(PNextPointerTypeTag pNext);

#include "vulkanLogAuto.inl"

} // namespace Vulkan
} // namespace gits

// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "log.h"
#include "vulkanHeader.h"

namespace gits {
namespace Vulkan {
using std::int64_t;
using std::uint64_t;

typedef struct PNextPointer* PNextPointerTypeTag;

class CVkLog : public gits::CLog {
public:
  CVkLog(LogLevel lvl, LogStyle style);

  CVkLog& operator<<(int32_t c);
  CVkLog& operator<<(int64_t c);
  CVkLog& operator<<(uint32_t c);
  CVkLog& operator<<(uint64_t c);
  CVkLog& operator<<(float c);
  CVkLog& operator<<(double c);
  CVkLog& operator<<(const uint32_t* c);
  CVkLog& operator<<(const char* c);
  CVkLog& operator<<(const void* c);
  CVkLog& operator<<(PFN_vkAllocationFunction c);
  CVkLog& operator<<(PFN_vkReallocationFunction c);
  CVkLog& operator<<(PFN_vkDebugReportCallbackEXT c);
  CVkLog& operator<<(PFN_vkFreeFunction c);
  CVkLog& operator<<(PFN_vkInternalAllocationNotification c);
  CVkLog& operator<<(PFN_vkDebugUtilsMessengerCallbackEXT c);
  CVkLog& operator<<(PFN_vkDeviceMemoryReportCallbackEXT c);
  CVkLog& operator<<(PFN_vkFaultCallbackFunction c);
  CVkLog& operator<<(PFN_vkVoidFunction c);
  CVkLog& operator<<(PFN_vkGetInstanceProcAddr c);
  CVkLog& operator<<(PFN_vkGetDeviceProcAddr c);
  //CVkLog& operator<<(PFN_GetPhysicalDeviceProcAddr c);
  CVkLog& operator<<(PNextPointerTypeTag pNext);
#if !defined GITS_PLATFORM_X11 || defined GITS_ARCH_X86
  // On Linux x64 and ARM, unsigned long is the same as uint64_t and it causes a redefinition error.
  CVkLog& operator<<(unsigned long c);
#endif
#include "vulkanLogAuto.inl"
};

// See common/include/log.h for explanations of these macros.
#define VkLog1(lvl)                                                                                \
  if (gits::ShouldLog(gits::LogLevel::lvl))                                                        \
  gits::Vulkan::CVkLog(gits::LogLevel::lvl, gits::LogStyle::NORMAL)
#define VkLog2(lvl, style)                                                                         \
  if (gits::ShouldLog(gits::LogLevel::lvl))                                                        \
  gits::Vulkan::CVkLog(gits::LogLevel::lvl, gits::LogStyle::style)
// Workaround for a MSVC bug, see https://stackoverflow.com/a/5134656/
#define EXPAND(x) x
// Magic to call different variants based on the number of arguments.
#define GET_OVERLOAD(PLACEHOLDER1, PLACEHOLDER2, NAME, ...) NAME
#define VkLog(...)                                          EXPAND(GET_OVERLOAD(__VA_ARGS__, VkLog2, VkLog1)(__VA_ARGS__))
} // namespace Vulkan
} // namespace gits

// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "log.h"
#include "log2.h"
#include "vulkanHeader.h"

#include <plog/Record.h>

namespace plog {
typedef struct PNextPointer* PNextPointerTypeTag;

Record& operator<<(Record& record, PFN_vkAllocationFunction c);
Record& operator<<(Record& record, PFN_vkReallocationFunction c);
Record& operator<<(Record& record, PFN_vkDebugReportCallbackEXT c);
Record& operator<<(Record& record, PFN_vkFreeFunction c);
Record& operator<<(Record& record, PFN_vkInternalAllocationNotification c);
Record& operator<<(Record& record, PFN_vkDebugUtilsMessengerCallbackEXT c);
Record& operator<<(Record& record, PFN_vkDeviceMemoryReportCallbackEXT c);
Record& operator<<(Record& record, PFN_vkFaultCallbackFunction c);
Record& operator<<(Record& record, PFN_vkVoidFunction c);
Record& operator<<(Record& record, PFN_vkGetInstanceProcAddr c);
Record& operator<<(Record& record, PFN_vkGetDeviceProcAddr c);
//Record& operator<<(Record& record, PFN_GetPhysicalDeviceProcAddr c);
Record& operator<<(Record& record, PNextPointerTypeTag pNext);

#include "vulkanLogAuto.inl"

} // namespace plog

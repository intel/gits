// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "config.h"
#include "vulkanLog.h"
#include "vulkanTools_lite.h"

namespace plog {

Record& operator<<(Record& record, PFN_vkAllocationFunction c) {
  if (c != nullptr) {
    record << (void*)c;
  } else {
    record << "0";
  }
  return record;
}

Record& operator<<(Record& record, PFN_vkReallocationFunction c) {
  if (c != nullptr) {
    record << (void*)c;
  } else {
    record << "0";
  }
  return record;
}

Record& operator<<(Record& record, PFN_vkDebugReportCallbackEXT c) {
  if (c != nullptr) {
    record << (void*)c;
  } else {
    record << "0";
  }
  return record;
}

Record& operator<<(Record& record, PFN_vkFreeFunction c) {
  if (c != nullptr) {
    record << (void*)c;
  } else {
    record << "0";
  }
  return record;
}

Record& operator<<(Record& record, PFN_vkInternalAllocationNotification c) {
  if (c != nullptr) {
    record << (void*)c;
  } else {
    record << "0";
  }
  return record;
}

Record& operator<<(Record& record, PFN_vkDebugUtilsMessengerCallbackEXT c) {
  if (c != nullptr) {
    record << (void*)c;
  } else {
    record << "0";
  }
  return record;
}

Record& operator<<(Record& record, PFN_vkDeviceMemoryReportCallbackEXT c) {
  if (c != nullptr) {
    record << (void*)c;
  } else {
    record << "0";
  }
  return record;
}

Record& operator<<(Record& record, PFN_vkFaultCallbackFunction c) {
  if (c != nullptr) {
    record << (void*)c;
  } else {
    record << "0";
  }
  return record;
}

Record& operator<<(Record& record, PFN_vkVoidFunction c) {
  if (c != nullptr) {
    record << (void*)c;
  } else {
    record << "0";
  }
  return record;
}

Record& operator<<(Record& record, PFN_vkGetInstanceProcAddr c) {
  if (c != nullptr) {
    record << (void*)c;
  } else {
    record << "0";
  }
  return record;
}

Record& operator<<(Record& record, PFN_vkGetDeviceProcAddr c) {
  if (c != nullptr) {
    record << (void*)c;
  } else {
    record << "0";
  }
  return record;
}

Record& operator<<(Record& record, PNextPointerTypeTag pNext) {
  using namespace gits;

  pNext = (PNextPointerTypeTag)gits::Vulkan::ignoreLoaderSpecificStructureTypes(pNext);

  if (pNext != nullptr) {
    if (Configurator::IsTraceDataOptPresent(TraceData::VK_STRUCTS)) {

      switch (*(VkStructureType*)pNext) {

#define PNEXT_WRAPPER(STRUCTURE_TYPE, structure, ...)                                              \
  case STRUCTURE_TYPE:                                                                             \
    record << *(const structure*)pNext;                                                            \
    break;

#include "vulkanPNextWrappers.inl"

      default:
        record << (const void*)pNext;
        break;
      }
    } else {
      record << (const void*)pNext;
    }
  } else {
    record << "0";
  }
  return record;
}

} // namespace plog

// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "vulkanLog.h"
#include "vulkanTools_lite.h"

namespace gits {
namespace Vulkan {

std::string ToStr(const char* str) {
  if (str == nullptr) {
    return "NULL";
  }
  return "\"" + std::string(str) + "\"";
}

std::string ToStr(char* str) {
  if (str == nullptr) {
    return "NULL";
  }
  return "\"" + std::string(str) + "\"";
}

std::string ToStr(PFN_vkAllocationFunction c) {
  std::ostringstream oss;
  if (c != nullptr) {
    oss << (void*)c;
  } else {
    oss << "0";
  }
  return oss.str();
}

std::string ToStr(PFN_vkReallocationFunction c) {
  std::ostringstream oss;
  if (c != nullptr) {
    oss << (void*)c;
  } else {
    oss << "0";
  }
  return oss.str();
}

std::string ToStr(PFN_vkDebugReportCallbackEXT c) {
  std::ostringstream oss;
  if (c != nullptr) {
    oss << (void*)c;
  } else {
    oss << "0";
  }
  return oss.str();
}

std::string ToStr(PFN_vkFreeFunction c) {
  std::ostringstream oss;
  if (c != nullptr) {
    oss << (void*)c;
  } else {
    oss << "0";
  }
  return oss.str();
}

std::string ToStr(PFN_vkInternalAllocationNotification c) {
  std::ostringstream oss;
  if (c != nullptr) {
    oss << (void*)c;
  } else {
    oss << "0";
  }
  return oss.str();
}

std::string ToStr(PFN_vkDebugUtilsMessengerCallbackEXT c) {
  std::ostringstream oss;
  if (c != nullptr) {
    oss << (void*)c;
  } else {
    oss << "0";
  }
  return oss.str();
}

std::string ToStr(PFN_vkDeviceMemoryReportCallbackEXT c) {
  std::ostringstream oss;
  if (c != nullptr) {
    oss << (void*)c;
  } else {
    oss << "0";
  }
  return oss.str();
}

std::string ToStr(PFN_vkVoidFunction c) {
  std::ostringstream oss;
  if (c != nullptr) {
    oss << (void*)c;
  } else {
    oss << "0";
  }
  return oss.str();
}

std::string ToStr(PFN_vkGetInstanceProcAddr c) {
  std::ostringstream oss;
  if (c != nullptr) {
    oss << (void*)c;
  } else {
    oss << "0";
  }
  return oss.str();
}

std::string ToStr(PFN_vkGetDeviceProcAddr c) {
  std::ostringstream oss;
  if (c != nullptr) {
    oss << (void*)c;
  } else {
    oss << "0";
  }
  return oss.str();
}

std::string ToStr(PNextPointerTypeTag pNext) {
  pNext = (PNextPointerTypeTag)gits::Vulkan::ignoreLoaderSpecificStructureTypes(pNext);

  std::ostringstream oss;
  if (pNext != nullptr) {
    switch (*(VkStructureType*)pNext) {

#define PNEXT_WRAPPER(STRUCTURE_TYPE, structure, ...)                                              \
  case STRUCTURE_TYPE:                                                                             \
    oss << ToStr(*(const structure*)pNext);                                                        \
    break;

#include "vulkanPNextWrappers.inl"

    default:
      oss << (const void*)pNext;
      break;
    }
  } else {
    oss << "0";
  }
  return oss.str();
}

} // namespace Vulkan
} // namespace gits

// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "vulkanLog.h"
#include "vulkanTools_lite.h"

namespace gits {
namespace Vulkan {

CVkLog::CVkLog(LogLevel lvl, LogStyle style) : CLog(lvl, style) {}

CVkLog& CVkLog::operator<<(int32_t c) {
  _buffer << c;
  return *this;
}

CVkLog& CVkLog::operator<<(int64_t c) {
  _buffer << c;
  return *this;
}

CVkLog& CVkLog::operator<<(uint32_t c) {
  _buffer << c;
  return *this;
}

CVkLog& CVkLog::operator<<(uint64_t c) {
  _buffer << c;
  return *this;
}

CVkLog& CVkLog::operator<<(float c) {
  _buffer << c;
  return *this;
}

CVkLog& CVkLog::operator<<(double c) {
  _buffer << c;
  return *this;
}

CVkLog& CVkLog::operator<<(const uint32_t* c) {
  if (c != nullptr) {
    _buffer << *c;
  } else {
    _buffer << "0";
  }
  return *this;
}

CVkLog& CVkLog::operator<<(const char* c) {
  if (c != nullptr) {
    _buffer << c;
  } else {
    _buffer << "0";
  }
  return *this;
}

CVkLog& CVkLog::operator<<(const void* c) {
  if (c != nullptr) {
    _buffer << c;
  } else {
    _buffer << "0";
  }
  return *this;
}

CVkLog& CVkLog::operator<<(PFN_vkAllocationFunction c) {
  if (c != nullptr) {
    _buffer << (void*)c;
  } else {
    _buffer << "0";
  }
  return *this;
}

CVkLog& CVkLog::operator<<(PFN_vkReallocationFunction c) {
  if (c != nullptr) {
    _buffer << (void*)c;
  } else {
    _buffer << "0";
  }
  return *this;
}

CVkLog& CVkLog::operator<<(PFN_vkDebugReportCallbackEXT c) {
  if (c != nullptr) {
    _buffer << (void*)c;
  } else {
    _buffer << "0";
  }
  return *this;
}

CVkLog& CVkLog::operator<<(PFN_vkFreeFunction c) {
  if (c != nullptr) {
    _buffer << (void*)c;
  } else {
    _buffer << "0";
  }
  return *this;
}

CVkLog& CVkLog::operator<<(PFN_vkInternalAllocationNotification c) {
  if (c != nullptr) {
    _buffer << (void*)c;
  } else {
    _buffer << "0";
  }
  return *this;
}

CVkLog& CVkLog::operator<<(PFN_vkDebugUtilsMessengerCallbackEXT c) {
  if (c != nullptr) {
    _buffer << (void*)c;
  } else {
    _buffer << "0";
  }
  return *this;
}

CVkLog& CVkLog::operator<<(PFN_vkDeviceMemoryReportCallbackEXT c) {
  if (c != nullptr) {
    _buffer << (void*)c;
  } else {
    _buffer << "0";
  }
  return *this;
}

CVkLog& CVkLog::operator<<(PFN_vkFaultCallbackFunction c) {
  if (c != nullptr) {
    _buffer << (void*)c;
  } else {
    _buffer << "0";
  }
  return *this;
}

CVkLog& CVkLog::operator<<(PFN_vkVoidFunction c) {
  if (c != nullptr) {
    _buffer << (void*)c;
  } else {
    _buffer << "0";
  }
  return *this;
}

CVkLog& CVkLog::operator<<(PFN_vkGetInstanceProcAddr c) {
  if (c != nullptr) {
    _buffer << (void*)c;
  } else {
    _buffer << "0";
  }
  return *this;
}

CVkLog& CVkLog::operator<<(PFN_vkGetDeviceProcAddr c) {
  if (c != nullptr) {
    _buffer << (void*)c;
  } else {
    _buffer << "0";
  }
  return *this;
}

CVkLog& CVkLog::operator<<(PNextPointerTypeTag pNext) {
  pNext = (PNextPointerTypeTag)ignoreLoaderSpecificStructureTypes(pNext);

  if (pNext != nullptr) {
    if (isTraceDataOptPresent(TraceData::VK_STRUCTS)) {

      switch (*(VkStructureType*)pNext) {

#define PNEXT_WRAPPER(STRUCTURE_TYPE, structure, ...)                                              \
  case STRUCTURE_TYPE:                                                                             \
    *this << *(const structure*)pNext;                                                             \
    break;

#include "vulkanPNextWrappers.inl"

      default:
        _buffer << (const void*)pNext;
        break;
      }
    } else {
      _buffer << (const void*)pNext;
    }
  } else {
    _buffer << "0";
  }
  return *this;
}

#if !defined GITS_PLATFORM_X11 || defined GITS_ARCH_X86
// On Linux x64 and ARM, unsigned long is the same as uint64_t and it causes a redefinition error.
CVkLog& CVkLog::operator<<(unsigned long c) {
  _buffer << c;
  return *this;
}
#endif

} // namespace Vulkan
} // namespace gits

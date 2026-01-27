// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "directx.h"
#include "log.h"
#include "to_string/toStr.h"
#include "ccodeParameters.h"
#include "ccodeStructsAuto.h"

#include <string>
#include <vector>
#include <sstream>

namespace gits {
namespace DirectX {
namespace ccode {

// Get the object name (e.g 615 -> "O615")
std::string objKeyToStr(unsigned key);
// Get the global ComPtr access to Ptr (e.g. 615 -> "g_O615.Get()")
// Note: Returns "nullptr" if the key is 0
std::string objKeyToPtrStr(unsigned key);

template <typename T>
std::string toHex(const T& value) {
  std::ostringstream oss;
  oss << "0x" << std::hex << std::uppercase << value;
  return oss.str();
}

template <typename T>
bool isZeroInitialized(const T& obj) {
  T zeroObj{};
  return memcmp(&obj, &zeroObj, sizeof(T)) == 0;
}

void toCpp(const LARGE_INTEGER& value, CppParameterInfo& info, CppParameterOutput& out);
void toCpp(const void* value, CppParameterInfo& info, CppParameterOutput& out);
void toCpp(const char* s, CppParameterInfo& info, CppParameterOutput& out);
void toCpp(const wchar_t* s, CppParameterInfo& info, CppParameterOutput& out);
void toCpp(const HMONITOR& value, CppParameterInfo& info, CppParameterOutput& out);
void toCpp(const HWND& value, CppParameterInfo& info, CppParameterOutput& out);
void toCpp(const D3D12_RECT& value, CppParameterInfo& info, CppParameterOutput& out);

template <size_t N>
void toCpp(const wchar_t (&s)[N], CppParameterInfo& info, CppParameterOutput& out) {
  return toCpp(s, info, out);
}

template <typename T>
void toCpp(const T* value, CppParameterInfo& info, CppParameterOutput& out) {
  if (!value || info.isSizeZero()) {
    out.initialization = "";
    out.decorator = "";
    out.value = "nullptr";
    return;
  }

  // Array of values
  std::string name = info.getIndexedName();
  if (info.size.value_or(0)) {
    std::ostringstream ss;
    // Declare array storage
    ss << info.type << " " << name << "[" << info.size.value() << "] = {};" << std::endl;
    for (unsigned i = 0; i < info.size; ++i) {
      ss << "// " << info.name << " " << i << std::endl;
      CppParameterInfo elementInfo(info.type, info.name, info);
      elementInfo.isArrayElement = true;
      elementInfo.index = i;
      CppParameterOutput elementOut;
      toCpp(value[i], elementInfo, elementOut);
      ss << elementOut.initialization;
    }
    out.initialization = ss.str();
    out.value = name;
    out.decorator = "";
    return;
  }

  // Pointer to single value
  GITS_ASSERT(info.isPtr, "Type is expected to be a pointer.");
  GITS_ASSERT(!info.size.has_value(), "Type is not expected to have size.");
  toCpp(*value, info, out);

  // Adjust decorator for pointer
  out.decorator = "&";
}

template <typename T>
void toCpp(T* value, CppParameterInfo& info, CppParameterOutput& out) {
  toCpp(const_cast<const T*>(value), info, out);
}

template <typename T>
void toCpp(const T& value, CppParameterInfo& info, CppParameterOutput& out) {
  GITS_ASSERT(
      info.isArrayElement || info.isPtr,
      "Basic types passed by value are expected to be either a pointer or part of an array");

  std::string name = info.getIndexedName();
  std::ostringstream ss;
  // Array storage is already declared
  if (!info.isArrayElement) {
    ss << info.type << " ";
  }
  // Basic type initialization
  ss << name << " = " << toStr(value) << ";" << std::endl;
  out.initialization = ss.str();
  out.value = name;
  out.decorator = "";
}

} // namespace ccode
} // namespace DirectX
} // namespace gits

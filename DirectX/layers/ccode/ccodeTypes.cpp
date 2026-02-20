// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "ccodeTypes.h"
#include "ccodeStream.h"
#include "command.h"
#include "log.h"
#include "to_string/toStr.h"

#include <sstream>

namespace gits {
namespace DirectX {
namespace ccode {

std::string objKeyToStr(unsigned key) {
  GITS_ASSERT(key);
  return "O" + keyToStr(key);
}

std::string objKeyToPtrStr(unsigned key) {
  if (!key) {
    return "nullptr";
  }
  return "g_" + objKeyToStr(key);
}

void toCpp(const LARGE_INTEGER& value, CppParameterInfo& info, CppParameterOutput& out) {
  std::string name = info.getIndexedName();
  std::ostringstream ss;
  if (!info.isArrayElement) {
    ss << info.type << " " << name << ";" << std::endl;
  }
  ss << name << ".QuadPart = " << value.QuadPart << ";" << std::endl;

  out.initialization = ss.str();
  out.value = name;
  out.decorator = "";
}

void toCpp(const void* value, CppParameterInfo& info, CppParameterOutput& out) {
  static unsigned count = 0;

  if (value == nullptr || info.size.value_or(0) == 0) {
    out.initialization = "";
    out.value = "nullptr";
    out.decorator = "";
    return;
  }

  // Store data in resource file
  auto& ccodeStream = ccode::CCodeStream::getInstance();
  ccodeStream.writeData(value, info.size.value());

  std::string dataName = "data" + std::to_string(count);
  std::ostringstream ss;
  ss << "std::vector<std::byte> " << dataName << "(" << info.size.value() << ");" << std::endl;
  ss << "DataService::Get().Read(" << dataName << ".data(), " << dataName << ".size());"
     << std::endl;

  out.initialization = ss.str();
  out.value = dataName + ".data()";
  out.decorator = "";

  count++;
}

void toCpp(const char* s, CppParameterInfo& info, CppParameterOutput& out) {
  out.initialization = "";
  out.value = "\"" + std::string(s) + '"';
  out.decorator = "";
}

void toCpp(const wchar_t* s, CppParameterInfo& info, CppParameterOutput& out) {
  out.initialization = "";
  out.value = "L\"" + toStr(s) + '"';
  out.decorator = "";
}

void toCpp(const HMONITOR& value, CppParameterInfo& info, CppParameterOutput& out) {
  auto ptrValue = reinterpret_cast<std::uintptr_t>(value);
  out.initialization = "";
  out.value = toHex(ptrValue);
  out.decorator = "(HMONITOR)";
}

void toCpp(const HWND& value, CppParameterInfo& info, CppParameterOutput& out) {
  auto ptrValue = reinterpret_cast<std::uintptr_t>(value);
  std::ostringstream ss;
  ss << info.type << " " << info.name << " = "
     << "WindowService::Get().GetHandle(" << ptrValue << ");" << std::endl;

  out.initialization = ss.str();
  out.value = info.name;
  out.decorator = "";
}

void toCpp(const D3D12_RECT& value, CppParameterInfo& info, CppParameterOutput& out) {
  std::string name = info.getIndexedName();
  std::ostringstream ss;
  if (!info.isArrayElement) {
    ss << info.type << " " << name << ";" << std::endl;
  }
  ss << name << ".left = " << toStr(value.left) << ";" << std::endl;
  ss << name << ".top = " << toStr(value.top) << ";" << std::endl;
  ss << name << ".right = " << toStr(value.right) << ";" << std::endl;
  ss << name << ".bottom = " << toStr(value.bottom) << ";" << std::endl;

  out.initialization = ss.str();
  out.value = name;
  out.decorator = "";
}

} // namespace ccode
} // namespace DirectX
} // namespace gits

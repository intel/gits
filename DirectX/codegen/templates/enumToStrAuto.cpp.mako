// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "enumToStrAuto.h"
#include <unordered_map>

namespace gits {
namespace DirectX {

static std::string enumToStr(const std::unordered_map<int, std::string>& enumMap, int value) {
  std::string result;
  bool first = true;
  for (const auto& [k, name] : enumMap) {
    if (value & k) {
      if (!first) {
        result += "|";
      }
      result += name;
      first = false;
    }
  }
  return result.empty() ? "Unknown" : result;
}

%for enum in enums:

std::string toStr(${enum.name} value) {
  std::string result = "Unknown";
  switch (value) {
%for value in enum.values:
  case ${value}:
    result = "${value}";
    break;
%endfor
  default:
    static std::unordered_map<int, std::string> enumMap{
%for value in enum.values:
      {${value}, "${value}"},
%endfor
    };
    result = enumToStr(enumMap, value);
    break;
  }
  return result;
}

%endfor

} // namespace DirectX
} // namespace gits

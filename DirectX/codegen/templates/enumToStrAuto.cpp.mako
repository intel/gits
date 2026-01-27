// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "enumToStrAuto.h"
#include <unordered_map>
#include <algorithm>

namespace gits {
namespace DirectX {

static std::string enumToStr(const std::vector<std::pair<int, std::string>>& enumValues, int value) {
  std::string result;
  bool first = true;
  int remainingValue = value;
  for (const auto& [k, name] : enumValues) {
    if (k != 0 && (remainingValue & k) == k) {
      if (!first) {
        result += "|";
      }
      result += name;
      first = false;
      remainingValue &= ~k; // Clear the bits
    }
  }
  return result.empty() ? "0" : result;
}

%for enum in enums:

std::string toStr(${enum.name} value) {
  std::string result = "unknown";
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
    // Sort map by keys in descending order for proper flag combination handling
    static const auto sortedMap = []() {
      std::vector<std::pair<int, std::string>> sorted(enumMap.begin(), enumMap.end());
      std::sort(sorted.begin(), sorted.end(), 
                [](const auto& a, const auto& b) { return a.first > b.first; });
      return sorted;
    }();
    result = enumToStr(sortedMap, value);
    break;
  }
  return result;
}

%endfor

} // namespace DirectX
} // namespace gits

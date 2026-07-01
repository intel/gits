// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "printBitmasksAuto.h"
#include <vector>
#include <unordered_map>

namespace gits {
namespace vulkan {

static std::string flagsToStr(const std::vector<std::pair<int, std::string>>& sortedFlagsMap, int value) {
  std::string result;
  bool first = true;
  int remainingValue = value;
  for (const auto& [k, name] : sortedFlagsMap) {
    if (k != 0 && (remainingValue & k) == k) {
      if (!first) {
        result += "|";
      }
      result += name;
      first = false;
      remainingValue &= ~k; // Clear the bits
    }
  }
  return result.empty() ? "0" : std::move(result);
}


%for bitmask in bitmasks:
<% define = get_define(bitmask.platform) %>\
% if define:
#ifdef ${define}
% endif

FastOStream& Print${bitmask.flag_name}(FastOStream& stream, const ${bitmask.flag_name} value) {
  std::string result = "unknown";
  switch (value) {
<% seen_values = set() %>\
%for flag_name, flag_value in bitmask.bits.items():
%if flag_value not in seen_values:
  <% seen_values.add(flag_value) %>\
  case ${flag_value}:
    result = "${flag_name}";
    break;
%endif
%endfor
  default:
    static std::unordered_map<int, std::string> flagsMap{
<% seen_values = set() %>\
%for flag_name, flag_value in bitmask.bits.items():
%if flag_value not in seen_values:
  <% seen_values.add(flag_value) %>\
  {${flag_value}, "${flag_name}"},
%endif
%endfor
    };
    // Sort map by keys in descending order for proper flag combination handling
    static const auto sortedFlagsMap = []() {
      std::vector<std::pair<int, std::string>> sorted(flagsMap.begin(), flagsMap.end());
      std::sort(sorted.begin(), sorted.end(), 
                [](const auto& a, const auto& b) { return a.first > b.first; });
      return sorted;
    }();
    result = flagsToStr(sortedFlagsMap, value);
    break;
  }
  stream << result;
  return stream;
}

FastOStream& Print${bitmask.flag_name}(FastOStream& stream, const ${bitmask.flag_name}* value) {
  if (value) {
    Print${bitmask.flag_name}(stream, *value);
  } else {
    stream << "nullptr";
  }
  return stream;
}

% if define:
#endif
% endif
%endfor

} // namespace vulkan
} // namespace gits

// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "printEnumsAuto.h"
#include <unordered_map>

namespace gits {
namespace DirectX {

static void enumToString(FastOStream& stream, 
                std::unordered_map<int, std::string> & enumMap, 
                int eVal) {
  unsigned cnt{0};

  for (const auto& [key, value] : enumMap) {
    if (eVal & key) {
      if (cnt != 0) {
        stream << "|";
      }
      stream << value;
      ++cnt;
    }
  }

  if (cnt == 0) {
    stream << "unknown";
  }
}

%for enum in enums:

FastOStream& operator<<(FastOStream& stream, ${enum.name} value) {
  switch (value) {
%for value in enum.values:
  case ${value}:
    stream << "${value}";
    break;
%endfor
  default:
    static std::unordered_map<int, std::string> enumMap{
%for value in enum.values:
      {${value}, "${value}"},
%endfor
    };
    enumToString(stream, enumMap, value);
    break;
  }
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const ${enum.name}* value) {
  if (value) {
    stream << *value;
  } else {
    stream << "nullptr";
  }
  return stream;
}

%endfor

} // namespace DirectX
} // namespace gits

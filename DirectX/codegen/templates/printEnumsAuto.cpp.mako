// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "printEnumsAuto.h"
#include "to_string/enumToStrAuto.h"

namespace gits {
namespace DirectX {

%for enum in enums:

FastOStream& operator<<(FastOStream& stream, ${enum.name} value) {
  return stream << toStr(value);
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

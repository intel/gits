// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "printCustom.h"
#include <unordered_map>

namespace gits {
namespace vulkan {

FastOStream& PrintObjectKey(FastOStream& stream, unsigned key) {
  if (!key) {
    stream << "nullptr";
  } else {
    stream << "O";
    stream << key;
  }
  return stream;
}

FastOStream& PrintString(FastOStream& stream, const char* s) {
  if (s) {
    stream << "\"" << s << "\"";
  } else {
    stream << "nullptr";
  }
  return stream;
}

FastOStream& PrintStringArray(FastOStream& stream, uint32_t count, const char* const* s) {
  if (!s) {
    return stream << "nullptr";
  }

  stream << "[";
  for (uint32_t i = 0; i < count; ++i) {
    if (i > 0) {
      stream << ", ";
    }
    PrintString(stream, s[i]);
  }
  stream << "]";
  return stream;
}

} // namespace vulkan
} // namespace gits

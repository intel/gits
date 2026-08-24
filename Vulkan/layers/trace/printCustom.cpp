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

FastOStream& PrintGitsObjectKey(FastOStream& stream, GITSKey key) {
  // NOTE: key == 0 is a valid VK_NULL_HANDLE for a plain handle argument (as opposed
  // to a null *pointer*, which callers already check for separately before printing
  // any object key at all) and must print as "O0", not "nullptr".
  //
  // Vulkan subcapture state restore mints object keys with no real captured key of
  // their own (a temporary command buffer, staging buffer/memory, a relocated
  // acceleration structure, etc. - see StateTrackingService::AllocateSyntheticKey()
  // and SubcaptureRecorder::CreateStateRestoreObjectKey()) from the same
  // STATE_RESTORE_KEY_MASK-flagged range PrintKey() uses for synthesized *command*
  // keys. Mirrors DirectX's keyToStr(): render them as "S<n>" too so they read
  // distinctly from real captured object keys, prefixed with "O" like any other
  // object key.
  stream << "O";
  if (IsStateRestoreKey(key)) {
    PrintKey(stream, key);
  } else {
    stream << key;
  }
  return stream;
}

FastOStream& PrintKey(FastOStream& stream, GITSKey key) {
  if (IsStateRestoreKey(key)) {
    stream << "S" << ExtractStateRestoreKey(key);
  } else {
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

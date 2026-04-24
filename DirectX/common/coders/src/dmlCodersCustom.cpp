// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "dmlCodersCustom.h"
#include "log.h"

#include <string.h>

#define DML_MAX_NAME_LENGTH 256

namespace gits {
namespace DirectX {
namespace dml {

void writeData(const char* src, unsigned srcSize, char* dst, unsigned& offset) {
  memcpy(&dst[offset], src, srcSize);
  offset += srcSize;
}

template <>
unsigned GetSizeT(const void* src, unsigned count) {
  // Treat void* as char* (used in DML_CONSTANT_DATA_GRAPH_NODE_DESC)
  return GetSizeT<std::byte>((std::byte*)src, count);
}

template <>
void EncodeT(const void* src, unsigned count, char* dst, unsigned& offset) {
  // Treat void* as std::byte* (used in DML_CONSTANT_DATA_GRAPH_NODE_DESC)
  EncodeT<std::byte>((std::byte*)src, count, dst, offset);
}

template <>
void DecodeT(const void* dst, unsigned count, char* src, unsigned& offset) {
  // Treat void* as std::byte* (used in DML_CONSTANT_DATA_GRAPH_NODE_DESC)
  DecodeT<std::byte>((std::byte*)dst, count, src, offset);
}

template <>
unsigned GetSizeT(const char* src, unsigned count) {
  GITS_ASSERT(count == 1);
  if (!src) {
    return 0;
  }
  return strnlen_s(src, DML_MAX_NAME_LENGTH) + 1;
}

template <>
void EncodeT(const char* src, unsigned count, char* dst, unsigned& offset) {
  GITS_ASSERT(count == 1);
  EncodeT<std::byte>((std::byte*)src, strnlen_s(src, DML_MAX_NAME_LENGTH) + 1, dst, offset);
}

template <>
void DecodeT(const char* dst, unsigned count, char* src, unsigned& offset) {
  GITS_ASSERT(count == 1);
  DecodeT<std::byte>((std::byte*)dst, strnlen_s(dst, DML_MAX_NAME_LENGTH) + 1, src, offset);
}

} // namespace dml
} // namespace DirectX
} // namespace gits

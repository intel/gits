// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "gits.h"

#include <cstdint>
#include <cstddef>

namespace gits {
namespace DirectX {
namespace dml {

void writeData(const char* src, unsigned srcSize, char* dst, unsigned& offset);

template <typename T>
const T* addPtrs(const T* lhs, char* rhs) {
  return reinterpret_cast<const T*>(std::uintptr_t(lhs) + std::uintptr_t(rhs));
}

template <typename T>
const T* castTo(const void* buffer) {
  return reinterpret_cast<const T*>(buffer);
}

template <typename T>
unsigned getSizeT(const T* src, unsigned count) {
  if (!src) {
    return 0;
  }
  return count * sizeof(T);
}

template <typename T>
void encodeT(const T* src, unsigned count, char* dst, unsigned& offset) {
  auto blobSize = getSizeT(src, count);
  if (!src) {
    GITS_ASSERT(0 && "Unexpected input");
  }
  writeData((char*)src, blobSize, dst, offset);
}

template <typename T>
void decodeT(const T* dst, unsigned count, char* src, unsigned& offset) {
  // Basic types (copyiable) have already been copied into dst and do not need to be decoded
  // Simply update the offset based on size
  offset += getSizeT(dst, count);
}

// Basic type specializations

template <>
unsigned getSizeT(const void* src, unsigned count);

template <>
void encodeT(const void* src, unsigned count, char* dst, unsigned& offset);

template <>
void decodeT(const void* dst, unsigned count, char* src, unsigned& offset);

template <>
unsigned getSizeT(const char* src, unsigned count);

template <>
void encodeT(const char* src, unsigned count, char* dst, unsigned& offset);

template <>
void decodeT(const char* dst, unsigned count, char* src, unsigned& offset);

} // namespace dml
} // namespace DirectX
} // namespace gits

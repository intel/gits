// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "log.h"

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
unsigned GetSizeT(const T* src, unsigned count) {
  if (!src) {
    return 0;
  }
  return count * sizeof(T);
}

template <typename T>
void EncodeT(const T* src, unsigned count, char* dst, unsigned& offset) {
  auto blobSize = GetSizeT(src, count);
  if (!src) {
    GITS_ASSERT(0 && "Unexpected input");
  }
  writeData((char*)src, blobSize, dst, offset);
}

template <typename T>
void DecodeT(const T* dst, unsigned count, char* src, unsigned& offset) {
  // Basic types (copyiable) have already been copied into dst and do not need to be decoded
  // Simply update the offset based on size
  offset += GetSizeT(dst, count);
}

// Basic type specializations

template <>
unsigned GetSizeT(const void* src, unsigned count);

template <>
void EncodeT(const void* src, unsigned count, char* dst, unsigned& offset);

template <>
void DecodeT(const void* dst, unsigned count, char* src, unsigned& offset);

template <>
unsigned GetSizeT(const char* src, unsigned count);

template <>
void EncodeT(const char* src, unsigned count, char* dst, unsigned& offset);

template <>
void DecodeT(const char* dst, unsigned count, char* src, unsigned& offset);

} // namespace dml
} // namespace DirectX
} // namespace gits

// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "arguments.h"
#include "printEnumsAuto.h"
#include "printUnionsAuto.h"
#include "printStructuresAuto.h"
#include "printStructuresCustom.h"
#include "fastOStream.h"

#include <iostream>

namespace gits {
namespace vulkan {

FastOStream& PrintObjectKey(FastOStream& stream, unsigned key);
FastOStream& PrintString(FastOStream& stream, const char* s);
FastOStream& PrintStringArray(FastOStream& stream, uint32_t count, const char* const* s);

template <typename T>
FastOStream& PrintArray(FastOStream& stream, unsigned dimension, T* data) {
  if (!data) {
    return stream << "nullptr";
  }

  stream << "{";
  for (unsigned i = 0; i < dimension; ++i) {
    if (i > 0) {
      stream << ", ";
    }
    stream << data[i];
  }
  stream << "}";
  return stream;
}

template <typename T, size_t N>
FastOStream& PrintStaticArray(FastOStream& stream, const T (&array)[N]) {
  return PrintArray(stream, N, array);
}

// Specialization for char arrays: print as a quoted string up to the first NUL.
template <size_t N>
FastOStream& PrintStaticArray(FastOStream& stream, const char (&array)[N]) {
  return PrintString(stream, array);
}

template <typename T, size_t ROWS, size_t COLS>
FastOStream& PrintStatic2DArray(FastOStream& stream, const T (&array)[ROWS][COLS]) {
  stream << "{";
  for (unsigned row = 0; row < ROWS; ++row) {
    PrintStaticArray(stream, array[row]);
    if (row < ROWS - 1) {
      stream << ", ";
    }
  }
  return stream << "}";
}

} // namespace vulkan
} // namespace gits

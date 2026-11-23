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
// Object/handle GITSKeys in trace output (HandleArgument, etc.).
FastOStream& PrintGitsObjectKey(FastOStream& stream, GITSKey key);
// Prints a command's own key (Command::m_Key), rendering state-restore-synthesized keys
// (see STATE_RESTORE_KEY_MASK in arguments.h) as "S<n>" so they read distinctly from real
// captured command keys in the trace log instead of colliding visually with them.
FastOStream& PrintKey(FastOStream& stream, GITSKey key);
FastOStream& PrintString(FastOStream& stream, const char* s);
FastOStream& PrintStringArray(FastOStream& stream, uint32_t count, const char* const* s);

template <typename T>
FastOStream& PrintArray(FastOStream& stream, size_t dimension, T* data) {
  if (!data) {
    return stream << "nullptr";
  }

  stream << "{";
  for (size_t i = 0; i < dimension; ++i) {
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

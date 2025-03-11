// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "directx.h"
#include "arguments.h"
#include "printEnumsAuto.h"
#include "printStructuresAuto.h"
#include "toStr.h"
#include "fastOStream.h"

#include <iostream>

namespace gits {
namespace DirectX {

FastOStream& printObjectKey(FastOStream& stream, unsigned key);
FastOStream& printString(FastOStream& stream, const wchar_t* s);
FastOStream& printString(FastOStream& stream, const char* s);

template <typename T>
FastOStream& printArray(FastOStream& stream, unsigned dimension, T* data) {
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
FastOStream& printStaticArray(FastOStream& stream, const T (&array)[N]) {
  return printArray(stream, N, array);
}

template <typename T, size_t ROWS, size_t COLS>
FastOStream& printStatic2DArray(FastOStream& stream, const T (&array)[ROWS][COLS]) {
  stream << "{";
  for (unsigned row = 0; row < ROWS; ++row) {
    printStaticArray(stream, array[row]);
    if (row < ROWS - 1) {
      stream << ", ";
    }
  }
  return stream << "}";
}

FastOStream& operator<<(FastOStream& stream, REFIID riid);
FastOStream& operator<<(FastOStream& stream, const UINT* value);
FastOStream& operator<<(FastOStream& stream, const BOOL* value);
FastOStream& operator<<(FastOStream& stream, const UINT8& value);
FastOStream& operator<<(FastOStream& stream, const UINT64* value);
FastOStream& operator<<(FastOStream& stream, const LARGE_INTEGER& value);
FastOStream& operator<<(FastOStream& stream, const D3D12_RECT& value);
FastOStream& operator<<(FastOStream& stream, const D3D12_RECT* value);
FastOStream& operator<<(FastOStream& stream, const POINT& value);
FastOStream& operator<<(FastOStream& stream, const DML_SCALAR_UNION& value);

} // namespace DirectX
} // namespace gits

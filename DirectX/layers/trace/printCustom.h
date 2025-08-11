// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "directx.h"
#define INTC_IGDEXT_D3D12
#include "igdext.h"
#include "igdext_tools.h"
#include "nvapi.h"
#include "arguments.h"
#include "printEnumsAuto.h"
#include "printStructuresAuto.h"
#include "to_string/toStr.h"
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

FastOStream& operator<<(FastOStream& stream, const INTCExtensionInfo& value);
FastOStream& operator<<(FastOStream& stream, const INTCExtensionInfo* value);
FastOStream& operator<<(FastOStream& stream, const INTCExtensionVersion& value);
FastOStream& operator<<(FastOStream& stream, const INTCDeviceInfo& value);
FastOStream& operator<<(FastOStream& stream, const INTCExtensionAppInfo& value);
FastOStream& operator<<(FastOStream& stream, const INTCAppInfoVersion& value);
FastOStream& operator<<(FastOStream& stream, const INTCExtensionAppInfo1& value);
FastOStream& operator<<(FastOStream& stream, const INTCExtensionAppInfo1* value);

FastOStream& operator<<(FastOStream& stream,
                        NVAPI_D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS_EX value);
FastOStream& operator<<(FastOStream& stream, NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_EX value);
FastOStream& operator<<(FastOStream& stream, NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_FORMAT value);
FastOStream& operator<<(FastOStream& stream,
                        NVAPI_D3D12_RAYTRACING_DISPLACEMENT_MICROMAP_FORMAT value);
FastOStream& operator<<(FastOStream& stream, NVAPI_D3D12_RAYTRACING_LSS_ENDCAP_MODE value);
FastOStream& operator<<(FastOStream& stream, NVAPI_D3D12_RAYTRACING_LSS_PRIMITIVE_FORMAT value);
FastOStream& operator<<(FastOStream& stream,
                        NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_BUILD_FLAGS value);
FastOStream& operator<<(FastOStream& stream,
                        NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_POSTBUILD_INFO_TYPE value);
FastOStream& operator<<(FastOStream& stream,
                        NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_FLAGS value);
FastOStream& operator<<(
    FastOStream& stream,
    NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_CLUSTER_FLAGS value);
FastOStream& operator<<(
    FastOStream& stream,
    NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_GEOMETRY_FLAGS value);
FastOStream& operator<<(FastOStream& stream,
                        NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_TYPE value);
FastOStream& operator<<(FastOStream& stream,
                        NVAPI_3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_INDEX_FORMAT value);
FastOStream& operator<<(FastOStream& stream,
                        NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_MODE value);
FastOStream& operator<<(FastOStream& stream,
                        NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_MOVE_TYPE value);
FastOStream& operator<<(
    FastOStream& stream,
    NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_ADDRESS_RESOLUTION_FLAGS value);

} // namespace DirectX
} // namespace gits

// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "arguments.h"
#include "printCustom.h"
#include "printEnumsAuto.h"
#include "printStructuresAuto.h"
#include "directx.h"

#include <iostream>

namespace gits {
namespace DirectX {

template <typename T>
FastOStream& operator<<(FastOStream& stream, PointerArgument<T>& arg) {
  return stream << const_cast<const T*>(arg.value);
}

template <template <typename> typename Arg, typename T>
FastOStream& operator<<(FastOStream& stream, Arg<T>& arg) {
  stream << arg.value;
  return stream;
}

template <typename T, int N>
FastOStream& operator<<(FastOStream& stream, StaticArrayArgument<T, N>& arg) {

  stream << "[";
  for (int i = 0; i < N; ++i) {
    if (i > 0) {
      stream << ", ";
    }
    stream << arg.value[i];
  }
  stream << "]";
  return stream;
}

template <typename T>
FastOStream& operator<<(FastOStream& stream, ArrayArgument<T>& arg) {
  if (!arg.value) {
    return stream << "nullptr";
  }

  stream << "[";
  for (int i = 0; i < arg.size; ++i) {
    if (i > 0) {
      stream << ", ";
    }
    stream << arg.value[i];
  }
  stream << "]";
  return stream;
}

template <typename T>
FastOStream& operator<<(FastOStream& stream, InterfaceArgument<T>& arg) {
  printObjectKey(stream, arg.key);
  return stream;
}

template <typename T>
FastOStream& operator<<(FastOStream& stream, InterfaceOutputArgument<T>& arg) {
  printObjectKey(stream, arg.key);
  return stream;
}

template <typename T>
FastOStream& operator<<(FastOStream& stream, InterfaceArrayArgument<T>& arg) {
  if (!arg.value) {
    return stream << "nullptr";
  }

  stream << "[";
  for (unsigned i = 0; i < arg.size; ++i) {
    if (i > 0) {
      stream << ", ";
    }
    printObjectKey(stream, arg.keys[i]);
  }
  stream << "]";
  return stream;
}

template <typename T>
FastOStream& operator<<(FastOStream& stream, ContextArgument<T>& arg) {
  printObjectKey(stream, arg.key);
  return stream;
}

template <typename T>
FastOStream& operator<<(FastOStream& stream, ContextOutputArgument<T>& arg) {
  printObjectKey(stream, arg.key);
  return stream;
}

FastOStream& operator<<(FastOStream& stream, BufferArgument& arg);
FastOStream& operator<<(FastOStream& stream, OutputBufferArgument& arg);
FastOStream& operator<<(FastOStream& stream, ShaderIdentifierArgument& arg);
FastOStream& operator<<(FastOStream& stream,
                        DescriptorHandleArrayArgument<D3D12_CPU_DESCRIPTOR_HANDLE>& arg);
FastOStream& operator<<(FastOStream& stream,
                        DescriptorHandleArgument<D3D12_CPU_DESCRIPTOR_HANDLE>& arg);
FastOStream& operator<<(FastOStream& stream,
                        DescriptorHandleArgument<D3D12_GPU_DESCRIPTOR_HANDLE>& arg);
FastOStream& operator<<(FastOStream& stream, LPCWSTR_Argument& arg);
FastOStream& operator<<(FastOStream& stream, LPCSTR_Argument& arg);
FastOStream& operator<<(FastOStream& stream, D3D12_GPU_VIRTUAL_ADDRESS_Argument& arg);
FastOStream& operator<<(FastOStream& stream, D3D12_GPU_VIRTUAL_ADDRESSs_Argument& arg);
FastOStream& operator<<(FastOStream& stream, D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument& arg);
FastOStream& operator<<(FastOStream& stream, D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument& arg);
FastOStream& operator<<(FastOStream& stream, D3D12_TEXTURE_COPY_LOCATION_Argument& arg);
FastOStream& operator<<(FastOStream& stream, D3D12_RESOURCE_BARRIERs_Argument& arg);
FastOStream& operator<<(FastOStream& stream, D3D12_INDEX_BUFFER_VIEW_Argument& arg);
FastOStream& operator<<(FastOStream& stream, D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument& arg);
FastOStream& operator<<(FastOStream& stream, D3D12_VERTEX_BUFFER_VIEWs_Argument& arg);
FastOStream& operator<<(FastOStream& stream, D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument& arg);
FastOStream& operator<<(FastOStream& stream, D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument& arg);
FastOStream& operator<<(FastOStream& stream, D3D12_PIPELINE_STATE_STREAM_DESC_Argument& arg);
FastOStream& operator<<(FastOStream& stream, D3D12_STATE_OBJECT_DESC_Argument& arg);
FastOStream& operator<<(FastOStream& stream, D3D12_SHADER_RESOURCE_VIEW_DESC_Argument& arg);
FastOStream& operator<<(FastOStream& stream, ArrayArgument<D3D12_RESIDENCY_PRIORITY>& arg);
FastOStream& operator<<(FastOStream& stream, D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument& arg);
FastOStream& operator<<(FastOStream& stream, D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument& arg);
FastOStream& operator<<(FastOStream& stream,
                        PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC>& arg);
FastOStream& operator<<(
    FastOStream& stream,
    ArrayArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg);
FastOStream& operator<<(
    FastOStream& stream,
    PointerArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg);
FastOStream& operator<<(FastOStream& stream,
                        PointerArgument<D3D12_UNORDERED_ACCESS_VIEW_DESC>& arg);
FastOStream& operator<<(FastOStream& stream, D3D12_BARRIER_GROUPs_Argument& arg);
FastOStream& operator<<(FastOStream& stream, PointerArgument<D3D12_DISPATCH_RAYS_DESC>& arg);
FastOStream& operator<<(FastOStream& stream, DML_BINDING_TABLE_DESC_Argument& arg);
FastOStream& operator<<(FastOStream& stream, DML_GRAPH_DESC_Argument& arg);
FastOStream& operator<<(FastOStream& stream, DML_BINDING_DESC_Argument& arg);
FastOStream& operator<<(FastOStream& stream, DML_BINDING_DESCs_Argument& arg);
FastOStream& operator<<(FastOStream& stream, DML_OPERATOR_DESC_Argument& arg);
FastOStream& operator<<(FastOStream& stream, xess_d3d12_init_params_t_Argument& arg);
FastOStream& operator<<(FastOStream& stream, xess_d3d12_execute_params_t_Argument& arg);
FastOStream& operator<<(FastOStream& stream, DML_CheckFeatureSupport_BufferArgument& arg);
FastOStream& operator<<(FastOStream& stream, DSTORAGE_QUEUE_DESC_Argument& arg);
FastOStream& operator<<(FastOStream& stream, DSTORAGE_REQUEST_Argument& arg);
FastOStream& operator<<(
    FastOStream& stream,
    PointerArgument<NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS>& arg);
FastOStream& operator<<(FastOStream& stream,
                        PointerArgument<NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS>& arg);
FastOStream& operator<<(
    FastOStream& stream,
    PointerArgument<NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS>& arg);

} // namespace DirectX
} // namespace gits

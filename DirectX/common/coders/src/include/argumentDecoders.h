// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "arguments.h"
#include "intelExtensions.h"
#include "nvapi.h"

namespace gits {
namespace DirectX {

template <typename T>
bool DecodeNullPtr(char* src, unsigned& offset, T& arg) {
  memcpy(&arg.Value, src + offset, sizeof(void*));
  offset += sizeof(void*);
  if (!arg.Value) {
    return true;
  }
  return false;
}

template <typename T>
void Decode(char* src, unsigned& offset, T& var) {
  unsigned size = sizeof(var);
  memcpy(&var, src + offset, size);
  offset += size;
}

template <typename T>
void Decode(char* src, unsigned& offset, Argument<T>& arg) {
  unsigned size = sizeof(arg.Value);
  memcpy(&arg.Value, src + offset, size);
  offset += size;
}

template <typename T>
void Decode(char* src, unsigned& offset, PointerArgument<T>& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }
  arg.Value = reinterpret_cast<T*>(const_cast<char*>(src + offset));
  offset += sizeof(T);
}

template <typename T>
void Decode(char* src, unsigned& offset, ArrayArgument<T>& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  memcpy(&arg.Size, src + offset, sizeof(arg.Size));
  offset += sizeof(arg.Size);

  arg.Value = reinterpret_cast<T*>(const_cast<char*>(src + offset));
  offset += sizeof(T) * arg.Size;
}

void Decode(char* src, unsigned& offset, BufferArgument& arg);

void Decode(char* src, unsigned& offset, OutputBufferArgument& arg);

template <typename T>
void Decode(char* src, unsigned& offset, InterfaceArgument<T>& arg) {
  unsigned size = sizeof(arg.Key);
  memcpy(&arg.Key, src + offset, size);
  offset += size;
}

template <typename T>
void Decode(char* src, unsigned& offset, InterfaceArrayArgument<T>& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  memcpy(&arg.Size, src + offset, sizeof(arg.Size));
  offset += sizeof(arg.Size);
  arg.Data.resize(arg.Size);
  arg.Value = arg.Data.data();

  arg.Keys.resize(arg.Size);
  memcpy(arg.Keys.data(), src + offset, arg.Size * sizeof(unsigned));
  offset += arg.Size * sizeof(unsigned);
}

template <typename T>
void Decode(char* src, unsigned& offset, InterfaceOutputArgument<T>& arg) {
  unsigned size = sizeof(arg.Key);
  memcpy(&arg.Key, src + offset, size);
  offset += size;
  arg.Value = &arg.Data;
}

template <typename T>
void Decode(char* src, unsigned& offset, DescriptorHandleArgument<T>& arg) {
  memcpy(&arg.Value, src + offset, sizeof(arg.Value));
  offset += sizeof(arg.Value);
  memcpy(&arg.InterfaceKey, src + offset, sizeof(arg.InterfaceKey));
  offset += sizeof(arg.InterfaceKey);
  memcpy(&arg.Index, src + offset, sizeof(arg.Index));
  offset += sizeof(arg.Index);
}

template <typename T>
void Decode(char* src, unsigned& offset, DescriptorHandleArrayArgument<T>& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  memcpy(&arg.Size, src + offset, sizeof(arg.Size));
  offset += sizeof(arg.Size);
  arg.Data.resize(arg.Size);
  arg.Value = arg.Data.data();

  arg.InterfaceKeys.resize(arg.Size);
  memcpy(arg.InterfaceKeys.data(), src + offset, arg.Size * sizeof(unsigned));
  offset += arg.Size * sizeof(unsigned);

  arg.Indexes.resize(arg.Size);
  memcpy(arg.Indexes.data(), src + offset, arg.Size * sizeof(unsigned));
  offset += arg.Size * sizeof(unsigned);
}

template <typename T>
void Decode(char* src, unsigned& offset, ContextArgument<T>& arg) {
  memcpy(&arg.Key, src + offset, sizeof(arg.Key));
  offset += sizeof(arg.Key);
}

template <typename T>
void Decode(char* src, unsigned& offset, ContextOutputArgument<T>& arg) {
  memcpy(&arg.Key, src + offset, sizeof(arg.Key));
  offset += sizeof(arg.Key);
  arg.Value = &arg.Data;
}

void Decode(char* src, unsigned& offset, LPCWSTR_Argument& arg);
void Decode(char* src, unsigned& offset, LPCSTR_Argument& arg);
void Decode(char* src, unsigned& offset, D3D12_GPU_VIRTUAL_ADDRESS_Argument& arg);
void Decode(char* src, unsigned& offset, D3D12_GPU_VIRTUAL_ADDRESSs_Argument& arg);
void Decode(char* src, unsigned& offset, ShaderIdentifierArgument& arg);
void Decode(char* src, unsigned& offset, D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument& arg);
void Decode(char* src, unsigned& offset, D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument& arg);
void Decode(char* src, unsigned& offset, D3D12_TEXTURE_COPY_LOCATION_Argument& arg);
void Decode(char* src, unsigned& offset, D3D12_RESOURCE_BARRIERs_Argument& arg);
void Decode(char* src, unsigned& offset, PointerArgument<D3D12_ROOT_SIGNATURE_DESC>& arg);
void Decode(char* src, unsigned& offset, PointerArgument<D3D12_VERSIONED_ROOT_SIGNATURE_DESC>& arg);
void Decode(char* src, unsigned& offset, PointerArgument<D3D12_COMMAND_SIGNATURE_DESC>& arg);
void Decode(char* src, unsigned& offset, D3D12_INDEX_BUFFER_VIEW_Argument& arg);
void Decode(char* src, unsigned& offset, D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument& arg);
void Decode(char* src, unsigned& offset, D3D12_VERTEX_BUFFER_VIEWs_Argument& arg);
void Decode(char* src, unsigned& offset, D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument& arg);
void Decode(char* src, unsigned& offset, D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument& arg);
void Decode(char* src, unsigned& offset, D3D12_STATE_OBJECT_DESC_Argument& arg);
void Decode(char* src, unsigned& offset, D3D12_PIPELINE_STATE_STREAM_DESC_Argument& arg);
void Decode(char* src, unsigned& offset, D3D12_BARRIER_GROUPs_Argument& arg);
void Decode(char* src, unsigned& offset, DML_BINDING_DESC_Argument& arg);
void Decode(char* src, unsigned& offset, DML_BINDING_DESCs_Argument& arg);
void Decode(char* src, unsigned& offset, DML_BINDING_TABLE_DESC_Argument& arg);
void Decode(char* src, unsigned& offset, DML_OPERATOR_DESC_Argument& arg);
void Decode(char* src, unsigned& offset, DML_GRAPH_DESC_Argument& arg);
void Decode(char* src, unsigned& offset, DML_CheckFeatureSupport_BufferArgument& arg);

void Decode(char* src,
            unsigned& offset,
            PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS>& arg);
void Decode(char* src,
            unsigned& offset,
            PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC>& arg);
void Decode(char* src, unsigned& offset, PointerArgument<D3D12_DISPATCH_RAYS_DESC>& arg);
void Decode(char* src,
            unsigned& offset,
            ArrayArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg);
void Decode(char* src,
            unsigned& offset,
            PointerArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg);
void Decode(char* src, unsigned& offset, PointerArgument<INTCExtensionInfo>& arg);
void Decode(char* src, unsigned& offset, PointerArgument<INTCExtensionAppInfo>& arg);
void Decode(char* src, unsigned& offset, PointerArgument<INTCExtensionAppInfo1>& arg);
void Decode(char* src,
            unsigned& offset,
            PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>& arg);
void Decode(char* src, unsigned& offset, PointerArgument<INTC_D3D12_HEAP_DESC>& arg);
void Decode(char* src, unsigned& offset, D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument& arg);
void Decode(char* src, unsigned& offset, D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument& arg);
void Decode(char* src, unsigned& offset, D3D12_SHADER_RESOURCE_VIEW_DESC_Argument& arg);
void Decode(char* src,
            unsigned& offset,
            ArrayArgument<D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_INFO>& arg);
void Decode(char* src, unsigned& offset, PointerArgument<INTC_D3D12_RESOURCE_DESC_0001>& arg);
void Decode(char* src, unsigned& offset, xess_d3d12_init_params_t_Argument& arg);
void Decode(char* src, unsigned& offset, xess_d3d12_execute_params_t_Argument& arg);
void Decode(char* src, unsigned& offset, DSTORAGE_QUEUE_DESC_Argument& arg);
void Decode(char* src, unsigned& offset, DSTORAGE_REQUEST_Argument& arg);
void Decode(char* src,
            unsigned& offset,
            PointerArgument<NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS>& arg);
void Decode(char* src,
            unsigned& offset,
            PointerArgument<NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS>& arg);
void Decode(char* src,
            unsigned& offset,
            PointerArgument<NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS>& arg);
void Decode(char* src, unsigned& offset, xell_frame_report_t_Argument& arg);
void Decode(char* src, unsigned& offset, xefg_swapchain_d3d12_init_params_t_Argument& arg);
void Decode(char* src, unsigned& offset, xefg_swapchain_d3d12_resource_data_t_Argument& arg);

} // namespace DirectX
} // namespace gits

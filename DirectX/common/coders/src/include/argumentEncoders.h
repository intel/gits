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
bool EncodeNullPtr(char* dest, unsigned& offset, const T& arg) {
  memcpy(dest + offset, &arg.Value, sizeof(void*));
  offset += sizeof(void*);
  if (!arg.Value) {
    return true;
  }
  return false;
}

template <typename T>
unsigned GetSize(const T& var) {
  return sizeof(var);
}

template <typename T>
void Encode(char* dest, unsigned& offset, const T& var) {
  unsigned size = GetSize(var);
  memcpy(dest + offset, &var, size);
  offset += size;
}

template <typename T>
unsigned GetSize(const Argument<T>& arg) {
  return sizeof(arg.Value);
}

template <typename T>
void Encode(char* dest, unsigned& offset, const Argument<T>& arg) {
  unsigned size = GetSize(arg);
  memcpy(dest + offset, &arg.Value, size);
  offset += size;
}

template <typename T>
unsigned GetSize(const PointerArgument<T>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(T);
}

template <typename T>
void Encode(char* dest, unsigned& offset, const PointerArgument<T>& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }
  memcpy(dest + offset, arg.Value, sizeof(T));
  offset += sizeof(T);
}

template <typename T>
unsigned GetSize(const ArrayArgument<T>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(arg.Size) + sizeof(T) * arg.Size;
}

template <typename T>
void Encode(char* dest, unsigned& offset, const ArrayArgument<T>& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }
  memcpy(dest + offset, &arg.Size, sizeof(arg.Size));
  offset += sizeof(arg.Size);
  memcpy(dest + offset, arg.Value, sizeof(T) * arg.Size);
  offset += sizeof(T) * arg.Size;
}

unsigned GetSize(const BufferArgument& arg);
void Encode(char* dest, unsigned& offset, const BufferArgument& arg);

unsigned GetSize(const OutputBufferArgument& arg);
void Encode(char* dest, unsigned& offset, const OutputBufferArgument& arg);

template <typename T>
unsigned GetSize(const InterfaceArgument<T>& arg) {
  return sizeof(arg.Key);
}

template <typename T>
void Encode(char* dest, unsigned& offset, const InterfaceArgument<T>& arg) {
  unsigned size = GetSize(arg);
  memcpy(dest + offset, &arg.Key, size);
  offset += size;
}

template <typename T>
unsigned GetSize(const InterfaceArrayArgument<T>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(arg.Size) + sizeof(unsigned) * arg.Size;
}

template <typename T>
void Encode(char* dest, unsigned& offset, const InterfaceArrayArgument<T>& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }
  memcpy(dest + offset, &arg.Size, sizeof(arg.Size));
  offset += sizeof(arg.Size);
  memcpy(dest + offset, arg.Keys.data(), sizeof(unsigned) * arg.Size);
  offset += sizeof(unsigned) * arg.Size;
}

template <typename T>
unsigned GetSize(const InterfaceOutputArgument<T>& arg) {
  return sizeof(arg.Key);
}

template <typename T>
void Encode(char* dest, unsigned& offset, const InterfaceOutputArgument<T>& arg) {
  unsigned size = GetSize(arg);
  memcpy(dest + offset, &arg.Key, size);
  offset += size;
}

template <typename T>
unsigned GetSize(const DescriptorHandleArgument<T>& arg) {
  return sizeof(T) + sizeof(arg.InterfaceKey) + sizeof(arg.Index);
}

template <typename T>
void Encode(char* dest, unsigned& offset, const DescriptorHandleArgument<T>& arg) {
  memcpy(dest + offset, &arg.Value, sizeof(T));
  offset += sizeof(T);
  memcpy(dest + offset, &arg.InterfaceKey, sizeof(arg.InterfaceKey));
  offset += sizeof(arg.InterfaceKey);
  memcpy(dest + offset, &arg.Index, sizeof(arg.Index));
  offset += sizeof(arg.Index);
}

template <typename T>
unsigned GetSize(const DescriptorHandleArrayArgument<T>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(arg.Size) + sizeof(unsigned) * arg.Size * 2;
}

template <typename T>
void Encode(char* dest, unsigned& offset, const DescriptorHandleArrayArgument<T>& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }
  memcpy(dest + offset, &arg.Size, sizeof(arg.Size));
  offset += sizeof(arg.Size);
  memcpy(dest + offset, arg.InterfaceKeys.data(), sizeof(unsigned) * arg.Size);
  offset += sizeof(unsigned) * arg.Size;
  memcpy(dest + offset, arg.Indexes.data(), sizeof(unsigned) * arg.Size);
  offset += sizeof(unsigned) * arg.Size;
}

template <typename T>
unsigned GetSize(const ContextArgument<T>& arg) {
  return sizeof(arg.Key);
}

template <typename T>
void Encode(char* dest, unsigned& offset, const ContextArgument<T>& arg) {
  memcpy(dest + offset, &arg.Key, sizeof(arg.Key));
  offset += sizeof(arg.Key);
}

template <typename T>
unsigned GetSize(const ContextOutputArgument<T>& arg) {
  return sizeof(arg.Key);
}

template <typename T>
void Encode(char* dest, unsigned& offset, const ContextOutputArgument<T>& arg) {
  memcpy(dest + offset, &arg.Key, sizeof(arg.Key));
  offset += sizeof(arg.Key);
}

unsigned GetSize(const LPCWSTR_Argument& arg);
void Encode(char* dest, unsigned& offset, const LPCWSTR_Argument& arg);

unsigned GetSize(const LPCSTR_Argument& arg);
void Encode(char* dest, unsigned& offset, const LPCSTR_Argument& arg);

unsigned GetSize(const D3D12_GPU_VIRTUAL_ADDRESS_Argument& arg);
void Encode(char* dest, unsigned& offset, const D3D12_GPU_VIRTUAL_ADDRESS_Argument& arg);

unsigned GetSize(const D3D12_GPU_VIRTUAL_ADDRESSs_Argument& arg);
void Encode(char* dest, unsigned& offset, const D3D12_GPU_VIRTUAL_ADDRESSs_Argument& arg);

unsigned GetSize(const ShaderIdentifierArgument& arg);
void Encode(char* dest, unsigned& offset, const ShaderIdentifierArgument& arg);

unsigned GetSize(const D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument& arg);
void Encode(char* dest, unsigned& offset, const D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument& arg);

unsigned GetSize(const D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument& arg);
void Encode(char* dest, unsigned& offset, const D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument& arg);

unsigned GetSize(const D3D12_TEXTURE_COPY_LOCATION_Argument& arg);
void Encode(char* dest, unsigned& offset, const D3D12_TEXTURE_COPY_LOCATION_Argument& arg);

unsigned GetSize(const D3D12_RESOURCE_BARRIERs_Argument& arg);
void Encode(char* dest, unsigned& offset, const D3D12_RESOURCE_BARRIERs_Argument& arg);

unsigned GetSize(const PointerArgument<D3D12_ROOT_SIGNATURE_DESC>& arg);
void Encode(char* dest, unsigned& offset, const PointerArgument<D3D12_ROOT_SIGNATURE_DESC>& arg);

unsigned GetSize(const PointerArgument<D3D12_VERSIONED_ROOT_SIGNATURE_DESC>& arg);
void Encode(char* dest,
            unsigned& offset,
            const PointerArgument<D3D12_VERSIONED_ROOT_SIGNATURE_DESC>& arg);

unsigned GetSize(const PointerArgument<D3D12_COMMAND_SIGNATURE_DESC>& arg);
void Encode(char* dest, unsigned& offset, const PointerArgument<D3D12_COMMAND_SIGNATURE_DESC>& arg);

unsigned GetSize(const D3D12_INDEX_BUFFER_VIEW_Argument& arg);
void Encode(char* dest, unsigned& offset, const D3D12_INDEX_BUFFER_VIEW_Argument& arg);

unsigned GetSize(const D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument& arg);
void Encode(char* dest, unsigned& offset, const D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument& arg);

unsigned GetSize(const D3D12_VERTEX_BUFFER_VIEWs_Argument& arg);
void Encode(char* dest, unsigned& offset, const D3D12_VERTEX_BUFFER_VIEWs_Argument& arg);

unsigned GetSize(const D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument& arg);
void Encode(char* dest, unsigned& offset, const D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument& arg);

unsigned GetSize(const D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument& arg);
void Encode(char* dest,
            unsigned& offset,
            const D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument& arg);

unsigned GetSize(const D3D12_STATE_OBJECT_DESC_Argument& arg);
void Encode(char* dest, unsigned& offset, const D3D12_STATE_OBJECT_DESC_Argument& arg);

unsigned GetSize(const D3D12_PIPELINE_STATE_STREAM_DESC_Argument& arg);
void Encode(char* dest, unsigned& offset, const D3D12_PIPELINE_STATE_STREAM_DESC_Argument& arg);

unsigned GetSize(const PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS>& arg);
void Encode(char* dest,
            unsigned& offset,
            const PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS>& arg);

unsigned GetSize(const PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC>& arg);
void Encode(char* dest,
            unsigned& offset,
            const PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC>& arg);

unsigned GetSize(const PointerArgument<D3D12_DISPATCH_RAYS_DESC>& arg);
void Encode(char* dest, unsigned& offset, const PointerArgument<D3D12_DISPATCH_RAYS_DESC>& arg);

unsigned GetSize(
    const ArrayArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg);
void Encode(char* dest,
            unsigned& offset,
            const ArrayArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg);

unsigned GetSize(
    const PointerArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg);
void Encode(
    char* dest,
    unsigned& offset,
    const PointerArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg);

unsigned GetSize(const D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument& arg);
void Encode(char* dest,
            unsigned& offset,
            const D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument& arg);

unsigned GetSize(const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument& arg);
void Encode(char* dest, unsigned& offset, const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument& arg);

unsigned GetSize(const D3D12_SHADER_RESOURCE_VIEW_DESC_Argument& arg);
void Encode(char* dest, unsigned& offset, const D3D12_SHADER_RESOURCE_VIEW_DESC_Argument& arg);

unsigned GetSize(const D3D12_BARRIER_GROUPs_Argument& arg);
void Encode(char* dest, unsigned& offset, const D3D12_BARRIER_GROUPs_Argument& arg);

unsigned GetSize(const ArrayArgument<D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_INFO>& arg);
void Encode(char* dest,
            unsigned& offset,
            const ArrayArgument<D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_INFO>& arg);

unsigned GetSize(const PointerArgument<D3D12_UNORDERED_ACCESS_VIEW_DESC>& arg);
void Encode(char* dest,
            unsigned& offset,
            const PointerArgument<D3D12_UNORDERED_ACCESS_VIEW_DESC>& arg);

unsigned GetSize(const DML_BINDING_DESC_Argument& arg);
void Encode(char* dest, unsigned& offset, const DML_BINDING_DESC_Argument& arg);

unsigned GetSize(const DML_BINDING_DESCs_Argument& arg);
void Encode(char* dest, unsigned& offset, const DML_BINDING_DESCs_Argument& arg);

unsigned GetSize(const DML_BINDING_TABLE_DESC_Argument& arg);
void Encode(char* dest, unsigned& offset, const DML_BINDING_TABLE_DESC_Argument& arg);

unsigned GetSize(const DML_OPERATOR_DESC_Argument& arg);
void Encode(char* dest, unsigned& offset, const DML_OPERATOR_DESC_Argument& arg);

unsigned GetSize(const DML_GRAPH_DESC_Argument& arg);
void Encode(char* dest, unsigned& offset, const DML_GRAPH_DESC_Argument& arg);

unsigned GetSize(const DML_CheckFeatureSupport_BufferArgument& arg);
void Encode(char* dest, unsigned& offset, const DML_CheckFeatureSupport_BufferArgument& arg);

unsigned GetSize(const PointerArgument<INTCExtensionAppInfo>& arg);
void Encode(char* dest, unsigned& offset, const PointerArgument<INTCExtensionAppInfo>& arg);

unsigned GetSize(const PointerArgument<INTCExtensionAppInfo1>& arg);
void Encode(char* dest, unsigned& offset, const PointerArgument<INTCExtensionAppInfo1>& arg);

unsigned GetSize(const PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>& arg);
void Encode(char* dest,
            unsigned& offset,
            const PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>& arg);

unsigned GetSize(const PointerArgument<INTC_D3D12_RESOURCE_DESC_0001>& arg);
void Encode(char* dest,
            unsigned& offset,
            const PointerArgument<INTC_D3D12_RESOURCE_DESC_0001>& arg);

unsigned GetSize(const PointerArgument<INTC_D3D12_HEAP_DESC>& arg);
void Encode(char* dest, unsigned& offset, const PointerArgument<INTC_D3D12_HEAP_DESC>& arg);

unsigned GetSize(const xess_d3d12_init_params_t_Argument& arg);
void Encode(char* dest, unsigned& offset, const xess_d3d12_init_params_t_Argument& arg);

unsigned GetSize(const xess_d3d12_execute_params_t_Argument& arg);
void Encode(char* dest, unsigned& offset, const xess_d3d12_execute_params_t_Argument& arg);

unsigned GetSize(const ArrayArgument<D3D12_META_COMMAND_DESC>& arg);
void Encode(char* dest, unsigned& offset, const ArrayArgument<D3D12_META_COMMAND_DESC>& arg);

unsigned GetSize(const DSTORAGE_QUEUE_DESC_Argument& arg);
void Encode(char* dest, unsigned& offset, const DSTORAGE_QUEUE_DESC_Argument& arg);

unsigned GetSize(const DSTORAGE_REQUEST_Argument& arg);
void Encode(char* dest, unsigned& offset, const DSTORAGE_REQUEST_Argument& arg);

unsigned GetSize(
    const PointerArgument<NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS>& arg);
void Encode(char* dest,
            unsigned& offset,
            const PointerArgument<NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS>& arg);

unsigned GetSize(const PointerArgument<NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS>& arg);
void Encode(char* dest,
            unsigned& offset,
            const PointerArgument<NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS>& arg);

unsigned GetSize(
    const PointerArgument<NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS>& arg);
void Encode(
    char* dest,
    unsigned& offset,
    const PointerArgument<NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS>& arg);

unsigned GetSize(const xell_frame_report_t_Argument& arg);
void Encode(char* dest, unsigned& offset, const xell_frame_report_t_Argument& arg);

unsigned GetSize(const xefg_swapchain_d3d12_init_params_t_Argument& arg);
void Encode(char* dest, unsigned& offset, const xefg_swapchain_d3d12_init_params_t_Argument& arg);

unsigned GetSize(const xefg_swapchain_d3d12_resource_data_t_Argument& arg);
void Encode(char* dest, unsigned& offset, const xefg_swapchain_d3d12_resource_data_t_Argument& arg);

} // namespace DirectX
} // namespace gits

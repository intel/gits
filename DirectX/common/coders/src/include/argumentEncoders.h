// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "command.h"
#include "intelExtensions.h"
#include "nvapi.h"

namespace gits {
namespace DirectX {

template <typename T>
bool encodeNullPtr(char* dest, unsigned& offset, const T& arg) {
  memcpy(dest + offset, &arg.value, sizeof(void*));
  offset += sizeof(void*);
  if (!arg.value) {
    return true;
  }
  return false;
}

template <typename T>
unsigned getSize(const T& var) {
  return sizeof(var);
}

template <typename T>
void encode(char* dest, unsigned& offset, const T& var) {
  unsigned size = getSize(var);
  memcpy(dest + offset, &var, size);
  offset += size;
}

template <typename T>
unsigned getSize(const Argument<T>& arg) {
  return sizeof(arg.value);
}

template <typename T>
void encode(char* dest, unsigned& offset, const Argument<T>& arg) {
  unsigned size = getSize(arg);
  memcpy(dest + offset, &arg.value, size);
  offset += size;
}

template <typename T>
unsigned getSize(const PointerArgument<T>& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(T);
}

template <typename T>
void encode(char* dest, unsigned& offset, const PointerArgument<T>& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }
  memcpy(dest + offset, arg.value, sizeof(T));
  offset += sizeof(T);
}

template <typename T>
unsigned getSize(const ArrayArgument<T>& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(arg.size) + sizeof(T) * arg.size;
}

template <typename T>
void encode(char* dest, unsigned& offset, const ArrayArgument<T>& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }
  memcpy(dest + offset, &arg.size, sizeof(arg.size));
  offset += sizeof(arg.size);
  memcpy(dest + offset, arg.value, sizeof(T) * arg.size);
  offset += sizeof(T) * arg.size;
}

unsigned getSize(const BufferArgument& arg);
void encode(char* dest, unsigned& offset, const BufferArgument& arg);

unsigned getSize(const OutputBufferArgument& arg);
void encode(char* dest, unsigned& offset, const OutputBufferArgument& arg);

template <typename T>
unsigned getSize(const InterfaceArgument<T>& arg) {
  return sizeof(arg.key);
}

template <typename T>
void encode(char* dest, unsigned& offset, const InterfaceArgument<T>& arg) {
  unsigned size = getSize(arg);
  memcpy(dest + offset, &arg.key, size);
  offset += size;
}

template <typename T>
unsigned getSize(const InterfaceArrayArgument<T>& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(arg.size) + sizeof(unsigned) * arg.size;
}

template <typename T>
void encode(char* dest, unsigned& offset, const InterfaceArrayArgument<T>& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }
  memcpy(dest + offset, &arg.size, sizeof(arg.size));
  offset += sizeof(arg.size);
  memcpy(dest + offset, arg.keys.data(), sizeof(unsigned) * arg.size);
  offset += sizeof(unsigned) * arg.size;
}

template <typename T>
unsigned getSize(const InterfaceOutputArgument<T>& arg) {
  return sizeof(arg.key);
}

template <typename T>
void encode(char* dest, unsigned& offset, const InterfaceOutputArgument<T>& arg) {
  unsigned size = getSize(arg);
  memcpy(dest + offset, &arg.key, size);
  offset += size;
}

template <typename T>
unsigned getSize(const DescriptorHandleArgument<T>& arg) {
  return sizeof(T) + sizeof(arg.interfaceKey) + sizeof(arg.index);
}

template <typename T>
void encode(char* dest, unsigned& offset, const DescriptorHandleArgument<T>& arg) {
  memcpy(dest + offset, &arg.value, sizeof(T));
  offset += sizeof(T);
  memcpy(dest + offset, &arg.interfaceKey, sizeof(arg.interfaceKey));
  offset += sizeof(arg.interfaceKey);
  memcpy(dest + offset, &arg.index, sizeof(arg.index));
  offset += sizeof(arg.index);
}

template <typename T>
unsigned getSize(const DescriptorHandleArrayArgument<T>& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(arg.size) + sizeof(unsigned) * arg.size * 2;
}

template <typename T>
void encode(char* dest, unsigned& offset, const DescriptorHandleArrayArgument<T>& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }
  memcpy(dest + offset, &arg.size, sizeof(arg.size));
  offset += sizeof(arg.size);
  memcpy(dest + offset, arg.interfaceKeys.data(), sizeof(unsigned) * arg.size);
  offset += sizeof(unsigned) * arg.size;
  memcpy(dest + offset, arg.indexes.data(), sizeof(unsigned) * arg.size);
  offset += sizeof(unsigned) * arg.size;
}

template <typename T>
unsigned getSize(const ContextArgument<T>& arg) {
  return sizeof(arg.key);
}

template <typename T>
void encode(char* dest, unsigned& offset, const ContextArgument<T>& arg) {
  memcpy(dest + offset, &arg.key, sizeof(arg.key));
  offset += sizeof(arg.key);
}

template <typename T>
unsigned getSize(const ContextOutputArgument<T>& arg) {
  return sizeof(arg.key);
}

template <typename T>
void encode(char* dest, unsigned& offset, const ContextOutputArgument<T>& arg) {
  memcpy(dest + offset, &arg.key, sizeof(arg.key));
  offset += sizeof(arg.key);
}

unsigned getSize(const LPCWSTR_Argument& arg);
void encode(char* dest, unsigned& offset, const LPCWSTR_Argument& arg);

unsigned getSize(const LPCSTR_Argument& arg);
void encode(char* dest, unsigned& offset, const LPCSTR_Argument& arg);

unsigned getSize(const D3D12_GPU_VIRTUAL_ADDRESS_Argument& arg);
void encode(char* dest, unsigned& offset, const D3D12_GPU_VIRTUAL_ADDRESS_Argument& arg);

unsigned getSize(const D3D12_GPU_VIRTUAL_ADDRESSs_Argument& arg);
void encode(char* dest, unsigned& offset, const D3D12_GPU_VIRTUAL_ADDRESSs_Argument& arg);

unsigned getSize(const ShaderIdentifierArgument& arg);
void encode(char* dest, unsigned& offset, const ShaderIdentifierArgument& arg);

unsigned getSize(const D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument& arg);
void encode(char* dest, unsigned& offset, const D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument& arg);

unsigned getSize(const D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument& arg);
void encode(char* dest, unsigned& offset, const D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument& arg);

unsigned getSize(const D3D12_TEXTURE_COPY_LOCATION_Argument& arg);
void encode(char* dest, unsigned& offset, const D3D12_TEXTURE_COPY_LOCATION_Argument& arg);

unsigned getSize(const D3D12_RESOURCE_BARRIERs_Argument& arg);
void encode(char* dest, unsigned& offset, const D3D12_RESOURCE_BARRIERs_Argument& arg);

unsigned getSize(const PointerArgument<D3D12_ROOT_SIGNATURE_DESC>& arg);
void encode(char* dest, unsigned& offset, const PointerArgument<D3D12_ROOT_SIGNATURE_DESC>& arg);

unsigned getSize(const PointerArgument<D3D12_VERSIONED_ROOT_SIGNATURE_DESC>& arg);
void encode(char* dest,
            unsigned& offset,
            const PointerArgument<D3D12_VERSIONED_ROOT_SIGNATURE_DESC>& arg);

unsigned getSize(const PointerArgument<D3D12_COMMAND_SIGNATURE_DESC>& arg);
void encode(char* dest, unsigned& offset, const PointerArgument<D3D12_COMMAND_SIGNATURE_DESC>& arg);

unsigned getSize(const D3D12_INDEX_BUFFER_VIEW_Argument& arg);
void encode(char* dest, unsigned& offset, const D3D12_INDEX_BUFFER_VIEW_Argument& arg);

unsigned getSize(const D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument& arg);
void encode(char* dest, unsigned& offset, const D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument& arg);

unsigned getSize(const D3D12_VERTEX_BUFFER_VIEWs_Argument& arg);
void encode(char* dest, unsigned& offset, const D3D12_VERTEX_BUFFER_VIEWs_Argument& arg);

unsigned getSize(const D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument& arg);
void encode(char* dest, unsigned& offset, const D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument& arg);

unsigned getSize(const D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument& arg);
void encode(char* dest,
            unsigned& offset,
            const D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument& arg);

unsigned getSize(const D3D12_STATE_OBJECT_DESC_Argument& arg);
void encode(char* dest, unsigned& offset, const D3D12_STATE_OBJECT_DESC_Argument& arg);

unsigned getSize(const D3D12_PIPELINE_STATE_STREAM_DESC_Argument& arg);
void encode(char* dest, unsigned& offset, const D3D12_PIPELINE_STATE_STREAM_DESC_Argument& arg);

unsigned getSize(const PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS>& arg);
void encode(char* dest,
            unsigned& offset,
            const PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS>& arg);

unsigned getSize(const PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC>& arg);
void encode(char* dest,
            unsigned& offset,
            const PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC>& arg);

unsigned getSize(const PointerArgument<D3D12_DISPATCH_RAYS_DESC>& arg);
void encode(char* dest, unsigned& offset, const PointerArgument<D3D12_DISPATCH_RAYS_DESC>& arg);

unsigned getSize(
    const ArrayArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg);
void encode(char* dest,
            unsigned& offset,
            const ArrayArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg);

unsigned getSize(
    const PointerArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg);
void encode(
    char* dest,
    unsigned& offset,
    const PointerArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg);

unsigned getSize(const D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument& arg);
void encode(char* dest,
            unsigned& offset,
            const D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument& arg);

unsigned getSize(const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument& arg);
void encode(char* dest, unsigned& offset, const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument& arg);

unsigned getSize(const D3D12_SHADER_RESOURCE_VIEW_DESC_Argument& arg);
void encode(char* dest, unsigned& offset, const D3D12_SHADER_RESOURCE_VIEW_DESC_Argument& arg);

unsigned getSize(const D3D12_BARRIER_GROUPs_Argument& arg);
void encode(char* dest, unsigned& offset, const D3D12_BARRIER_GROUPs_Argument& arg);

unsigned getSize(const PointerArgument<D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_INFO>& arg);
void encode(char* dest,
            unsigned& offset,
            const PointerArgument<D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_INFO>& arg);

unsigned getSize(const DML_BINDING_DESC_Argument& arg);
void encode(char* dest, unsigned& offset, const DML_BINDING_DESC_Argument& arg);

unsigned getSize(const DML_BINDING_DESCs_Argument& arg);
void encode(char* dest, unsigned& offset, const DML_BINDING_DESCs_Argument& arg);

unsigned getSize(const DML_BINDING_TABLE_DESC_Argument& arg);
void encode(char* dest, unsigned& offset, const DML_BINDING_TABLE_DESC_Argument& arg);

unsigned getSize(const DML_OPERATOR_DESC_Argument& arg);
void encode(char* dest, unsigned& offset, const DML_OPERATOR_DESC_Argument& arg);

unsigned getSize(const DML_GRAPH_DESC_Argument& arg);
void encode(char* dest, unsigned& offset, const DML_GRAPH_DESC_Argument& arg);

unsigned getSize(const DML_CheckFeatureSupport_BufferArgument& arg);
void encode(char* src, unsigned& offset, const DML_CheckFeatureSupport_BufferArgument& arg);

unsigned getSize(const PointerArgument<INTCExtensionAppInfo>& arg);
void encode(char* dest, unsigned& offset, const PointerArgument<INTCExtensionAppInfo>& arg);

unsigned getSize(const PointerArgument<INTCExtensionAppInfo1>& arg);
void encode(char* dest, unsigned& offset, const PointerArgument<INTCExtensionAppInfo1>& arg);

unsigned getSize(const PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>& arg);
void encode(char* dest,
            unsigned& offset,
            const PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>& arg);

unsigned getSize(const PointerArgument<INTC_D3D12_RESOURCE_DESC_0001>& arg);
void encode(char* dest,
            unsigned& offset,
            const PointerArgument<INTC_D3D12_RESOURCE_DESC_0001>& arg);

unsigned getSize(const PointerArgument<INTC_D3D12_HEAP_DESC>& arg);
void encode(char* dest, unsigned& offset, const PointerArgument<INTC_D3D12_HEAP_DESC>& arg);

unsigned getSize(const xess_d3d12_init_params_t_Argument& arg);
void encode(char* dest, unsigned& offset, const xess_d3d12_init_params_t_Argument& arg);

unsigned getSize(const xess_d3d12_execute_params_t_Argument& arg);
void encode(char* dest, unsigned& offset, const xess_d3d12_execute_params_t_Argument& arg);

unsigned getSize(const ArrayArgument<D3D12_META_COMMAND_DESC>& arg);
void encode(char* dest, unsigned& offset, const ArrayArgument<D3D12_META_COMMAND_DESC>& arg);

unsigned getSize(const DSTORAGE_QUEUE_DESC_Argument& arg);
void encode(char* dest, unsigned& offset, const DSTORAGE_QUEUE_DESC_Argument& arg);

unsigned getSize(const DSTORAGE_REQUEST_Argument& arg);
void encode(char* dest, unsigned& offset, const DSTORAGE_REQUEST_Argument& arg);

unsigned getSize(
    const PointerArgument<NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS>& arg);
void encode(char* dest,
            unsigned& offset,
            const PointerArgument<NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS>& arg);

unsigned getSize(const PointerArgument<NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS>& arg);
void encode(char* dest,
            unsigned& offset,
            const PointerArgument<NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS>& arg);

unsigned getSize(
    const PointerArgument<NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS>& arg);
void encode(
    char* dest,
    unsigned& offset,
    const PointerArgument<NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS>& arg);

} // namespace DirectX
} // namespace gits

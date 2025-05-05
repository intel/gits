// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "command.h"
#include "intelExtensions.h"

namespace gits {
namespace DirectX {

template <typename T>
bool decodeNullPtr(char* src, unsigned& offset, T& arg) {
  memcpy(&arg.value, src + offset, sizeof(void*));
  offset += sizeof(void*);
  if (!arg.value) {
    return true;
  }
  return false;
}

template <typename T>
void decode(char* src, unsigned& offset, T& var) {
  unsigned size = sizeof(var);
  memcpy(&var, src + offset, size);
  offset += size;
}

template <typename T>
void decode(char* src, unsigned& offset, Argument<T>& arg) {
  unsigned size = sizeof(arg.value);
  memcpy(&arg.value, src + offset, size);
  offset += size;
}

template <typename T>
void decode(char* src, unsigned& offset, PointerArgument<T>& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }
  arg.value = reinterpret_cast<T*>(const_cast<char*>(src + offset));
  offset += sizeof(T);
}

template <typename T>
void decode(char* src, unsigned& offset, ArrayArgument<T>& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  memcpy(&arg.size, src + offset, sizeof(arg.size));
  offset += sizeof(arg.size);

  arg.value = reinterpret_cast<T*>(const_cast<char*>(src + offset));
  offset += sizeof(T) * arg.size;
}

void decode(char* src, unsigned& offset, BufferArgument& arg);

void decode(char* src, unsigned& offset, OutputBufferArgument& arg);

template <typename T>
void decode(char* src, unsigned& offset, InterfaceArgument<T>& arg) {
  unsigned size = sizeof(arg.key);
  memcpy(&arg.key, src + offset, size);
  offset += size;
}

template <typename T>
void decode(char* src, unsigned& offset, InterfaceArrayArgument<T>& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  memcpy(&arg.size, src + offset, sizeof(arg.size));
  offset += sizeof(arg.size);
  arg.data.resize(arg.size);
  arg.value = arg.data.data();

  arg.keys.resize(arg.size);
  memcpy(arg.keys.data(), src + offset, arg.size * sizeof(unsigned));
  offset += arg.size * sizeof(unsigned);
}

template <typename T>
void decode(char* src, unsigned& offset, InterfaceOutputArgument<T>& arg) {
  unsigned size = sizeof(arg.key);
  memcpy(&arg.key, src + offset, size);
  offset += size;
  arg.value = &arg.data;
}

template <typename T>
void decode(char* src, unsigned& offset, DescriptorHandleArgument<T>& arg) {
  memcpy(&arg.value, src + offset, sizeof(arg.value));
  offset += sizeof(arg.value);
  memcpy(&arg.interfaceKey, src + offset, sizeof(arg.interfaceKey));
  offset += sizeof(arg.interfaceKey);
  memcpy(&arg.index, src + offset, sizeof(arg.index));
  offset += sizeof(arg.index);
}

template <typename T>
void decode(char* src, unsigned& offset, DescriptorHandleArrayArgument<T>& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  memcpy(&arg.size, src + offset, sizeof(arg.size));
  offset += sizeof(arg.size);
  arg.data.resize(arg.size);
  arg.value = arg.data.data();

  arg.interfaceKeys.resize(arg.size);
  memcpy(arg.interfaceKeys.data(), src + offset, arg.size * sizeof(unsigned));
  offset += arg.size * sizeof(unsigned);

  arg.indexes.resize(arg.size);
  memcpy(arg.indexes.data(), src + offset, arg.size * sizeof(unsigned));
  offset += arg.size * sizeof(unsigned);
}

template <typename T>
void decode(char* src, unsigned& offset, ContextArgument<T>& arg) {
  memcpy(&arg.key, src + offset, sizeof(arg.key));
  offset += sizeof(arg.key);
}

template <typename T>
void decode(char* src, unsigned& offset, ContextOutputArgument<T>& arg) {
  memcpy(&arg.key, src + offset, sizeof(arg.key));
  offset += sizeof(arg.key);
  arg.value = &arg.data;
}

void decode(char* src, unsigned& offset, LPCWSTR_Argument& arg);
void decode(char* src, unsigned& offset, LPCSTR_Argument& arg);
void decode(char* src, unsigned& offset, D3D12_GPU_VIRTUAL_ADDRESS_Argument& arg);
void decode(char* src, unsigned& offset, D3D12_GPU_VIRTUAL_ADDRESSs_Argument& arg);
void decode(char* src, unsigned& offset, ShaderIdentifierArgument& arg);
void decode(char* src, unsigned& offset, D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument& arg);
void decode(char* src, unsigned& offset, D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument& arg);
void decode(char* src, unsigned& offset, D3D12_TEXTURE_COPY_LOCATION_Argument& arg);
void decode(char* src, unsigned& offset, D3D12_RESOURCE_BARRIERs_Argument& arg);
void decode(char* src, unsigned& offset, PointerArgument<D3D12_ROOT_SIGNATURE_DESC>& arg);
void decode(char* src, unsigned& offset, PointerArgument<D3D12_VERSIONED_ROOT_SIGNATURE_DESC>& arg);
void decode(char* src, unsigned& offset, PointerArgument<D3D12_COMMAND_SIGNATURE_DESC>& arg);
void decode(char* src, unsigned& offset, D3D12_INDEX_BUFFER_VIEW_Argument& arg);
void decode(char* src, unsigned& offset, D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument& arg);
void decode(char* src, unsigned& offset, D3D12_VERTEX_BUFFER_VIEWs_Argument& arg);
void decode(char* src, unsigned& offset, D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument& arg);
void decode(char* src, unsigned& offset, D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument& arg);
void decode(char* src, unsigned& offset, D3D12_STATE_OBJECT_DESC_Argument& arg);
void decode(char* src, unsigned& offset, D3D12_PIPELINE_STATE_STREAM_DESC_Argument& arg);
void decode(char* src, unsigned& offset, DML_BINDING_DESC_Argument& arg);
void decode(char* src, unsigned& offset, DML_BINDING_DESCs_Argument& arg);
void decode(char* src, unsigned& offset, DML_BINDING_TABLE_DESC_Argument& arg);
void decode(char* src, unsigned& offset, DML_OPERATOR_DESC_Argument& arg);
void decode(char* src, unsigned& offset, DML_GRAPH_DESC_Argument& arg);
void decode(char* src, unsigned& offset, DML_CheckFeatureSupport_BufferArgument& arg);

void decode(char* src,
            unsigned& offset,
            PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC>& arg);
void decode(char* src, unsigned& offset, PointerArgument<D3D12_DISPATCH_RAYS_DESC>& arg);
void decode(char* src,
            unsigned& offset,
            ArrayArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg);
void decode(char* src, unsigned& offset, PointerArgument<INTCExtensionInfo>& arg);
void decode(char* src, unsigned& offset, PointerArgument<INTCExtensionAppInfo>& arg);
void decode(char* src, unsigned& offset, PointerArgument<INTCExtensionAppInfo1>& arg);
void decode(char* src,
            unsigned& offset,
            PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>& arg);
void decode(char* src, unsigned& offset, PointerArgument<INTC_D3D12_HEAP_DESC>& arg);
void decode(char* src, unsigned& offset, D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument& arg);
void decode(char* src, unsigned& offset, D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument& arg);
void decode(char* src, unsigned& offset, D3D12_SHADER_RESOURCE_VIEW_DESC_Argument& arg);
void decode(char* src, unsigned& offset, PointerArgument<INTC_D3D12_RESOURCE_DESC_0001>& arg);
void decode(char* src, unsigned& offset, xess_d3d12_init_params_t_Argument& arg);
void decode(char* src, unsigned& offset, xess_d3d12_execute_params_t_Argument& arg);
void decode(char* src, unsigned& offset, DSTORAGE_QUEUE_DESC_Argument& arg);
void decode(char* src, unsigned& offset, DSTORAGE_REQUEST_Argument& arg);

} // namespace DirectX
} // namespace gits

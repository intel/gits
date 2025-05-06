// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "directx.h"

#include <vector>
#include <map>

struct INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC;
struct INTCExtensionContext;
struct INTCExtensionAppInfo;
struct INTCExtensionAppInfo1;

namespace gits {
namespace DirectX {

#pragma region Generic

template <typename T>
struct Argument {
  T value;
};

template <typename T>
struct ContextArgument {
  T value{};
  unsigned key{};
};

template <typename T>
struct ContextOutputArgument {
  T* value{};
  T data{};
  unsigned key{};
};

template <typename T, int N>
struct StaticArrayArgument {
  StaticArrayArgument(T* value_) {
    for (int i = 0; i < N; ++i) {
      value[i] = value_[i];
    }
  }
  StaticArrayArgument() {}
  T value[N];
};

template <typename T>
struct PointerArgument {
  PointerArgument(const T* value_) : value(const_cast<T*>(value_)) {}
  PointerArgument() {}
  T* value{};
};

template <typename T>
struct ArrayArgument {
  ArrayArgument(const T* value_, size_t size_) : value(const_cast<T*>(value_)), size(size_) {}
  ArrayArgument() {}
  T* value{};
  size_t size{};
};

struct BufferArgument {
  BufferArgument(const void* value_, size_t size_)
      : value(const_cast<void*>(value_)), size(size_) {}
  BufferArgument() {}
  void* value{};
  size_t size{};
};

struct OutputBufferArgument {
  void** value{};
  void* captureValue{};
  void* data{};
};

template <typename T>
struct InterfaceArgument {
  T* value{};
  unsigned key{};
};

template <typename T>
struct InterfaceArrayArgument {
  InterfaceArrayArgument(T** value_, size_t size_) : value(value_), size(size_) {
    keys.resize(size);
  }
  InterfaceArrayArgument() {}
  T** value{};
  size_t size{};
  std::vector<unsigned> keys{};
  std::vector<T*> data;
};

template <typename T>
struct InterfaceOutputArgument {
  T** value{};
  unsigned key{};
  T* data{};
};

struct LPCWSTR_Argument {
  LPCWSTR_Argument(LPCWSTR value_) : value(const_cast<LPWSTR>(value_)) {}
  LPCWSTR_Argument() {}
  LPWSTR value{};
};

struct LPCSTR_Argument {
  LPCSTR_Argument(LPCSTR value_) : value(const_cast<LPSTR>(value_)) {}
  LPCSTR_Argument() {}
  LPSTR value{};
};

using PCWSTR_Argument = LPCWSTR_Argument;
using WCHAR_Argument = LPCWSTR_Argument;
using PCSTR_Argument = LPCSTR_Argument;

// Opaque handles
using INTCExtensionContextArgument = ContextArgument<INTCExtensionContext*>;
using INTCExtensionContextOutputArgument = ContextOutputArgument<INTCExtensionContext*>;
using XESSContextArgument = ContextArgument<xess_context_handle_t>;
using XESSContextOutputArgument = ContextOutputArgument<xess_context_handle_t>;

#pragma endregion

#pragma region D3D12

template <typename T>
struct DescriptorHandleArgument {
  T value{};
  unsigned interfaceKey{};
  unsigned index{};
};

template <typename T>
struct DescriptorHandleArrayArgument {
  DescriptorHandleArrayArgument(const T* value_, size_t size_)
      : value(const_cast<T*>(value_)), size(size_) {
    interfaceKeys.resize(size);
    indexes.resize(size);
  }
  DescriptorHandleArrayArgument() {}
  T* value{};
  size_t size{};
  std::vector<unsigned> interfaceKeys{};
  std::vector<unsigned> indexes{};
  std::vector<T> data;
};

struct D3D12_GPU_VIRTUAL_ADDRESS_Argument {
  D3D12_GPU_VIRTUAL_ADDRESS value{};
  unsigned interfaceKey{};
  unsigned offset{};
};

struct D3D12_GPU_VIRTUAL_ADDRESSs_Argument {
  D3D12_GPU_VIRTUAL_ADDRESSs_Argument(const D3D12_GPU_VIRTUAL_ADDRESS* value_, unsigned size_)
      : value(const_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(value_)), size(size_) {
    interfaceKeys.resize(size_);
    offsets.resize(size_);
  }
  D3D12_GPU_VIRTUAL_ADDRESSs_Argument() {}
  D3D12_GPU_VIRTUAL_ADDRESS* value{};
  size_t size{};
  std::vector<unsigned> interfaceKeys{};
  std::vector<unsigned> offsets{};
};

struct ShaderIdentifierArgument {
  ShaderIdentifierArgument(const void* value_) : value(const_cast<void*>(value_)) {}
  ShaderIdentifierArgument() {}
  void* value{};
  std::vector<uint8_t> data;
};

struct D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument {
  D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument(const D3D12_GRAPHICS_PIPELINE_STATE_DESC* value_)
      : value(const_cast<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>(value_)) {}
  D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument() {}
  D3D12_GRAPHICS_PIPELINE_STATE_DESC* value{};
  unsigned rootSignatureKey{};
};

struct D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument {
  D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument(const D3D12_COMPUTE_PIPELINE_STATE_DESC* value_)
      : value(const_cast<D3D12_COMPUTE_PIPELINE_STATE_DESC*>(value_)) {}
  D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument() {}
  D3D12_COMPUTE_PIPELINE_STATE_DESC* value{};
  unsigned rootSignatureKey{};
};

struct D3D12_TEXTURE_COPY_LOCATION_Argument {
  D3D12_TEXTURE_COPY_LOCATION_Argument(const D3D12_TEXTURE_COPY_LOCATION* value_)
      : value(const_cast<D3D12_TEXTURE_COPY_LOCATION*>(value_)) {}
  D3D12_TEXTURE_COPY_LOCATION_Argument() {}
  D3D12_TEXTURE_COPY_LOCATION* value{};
  unsigned resourceKey{};
};

struct D3D12_RESOURCE_BARRIERs_Argument {
  D3D12_RESOURCE_BARRIERs_Argument(const D3D12_RESOURCE_BARRIER* value_, unsigned size_)
      : value(const_cast<D3D12_RESOURCE_BARRIER*>(value_)), size(size_) {
    resourceKeys.resize(size_);
    resourceAfterKeys.resize(size_);
  }
  D3D12_RESOURCE_BARRIERs_Argument() {}
  D3D12_RESOURCE_BARRIER* value{};
  size_t size{};
  std::vector<unsigned> resourceKeys{};
  std::vector<unsigned> resourceAfterKeys{};
};

struct D3D12_INDEX_BUFFER_VIEW_Argument {
  D3D12_INDEX_BUFFER_VIEW_Argument(const D3D12_INDEX_BUFFER_VIEW* value_)
      : value(const_cast<D3D12_INDEX_BUFFER_VIEW*>(value_)) {}
  D3D12_INDEX_BUFFER_VIEW_Argument() {}
  D3D12_INDEX_BUFFER_VIEW* value{};
  unsigned bufferLocationKey{};
  unsigned bufferLocationOffset{};
};

struct D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument {
  D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument(const D3D12_CONSTANT_BUFFER_VIEW_DESC* value_)
      : value(const_cast<D3D12_CONSTANT_BUFFER_VIEW_DESC*>(value_)) {}
  D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument() {}
  D3D12_CONSTANT_BUFFER_VIEW_DESC* value{};
  unsigned bufferLocationKey{};
  unsigned bufferLocationOffset{};
};

struct D3D12_VERTEX_BUFFER_VIEWs_Argument {
  D3D12_VERTEX_BUFFER_VIEWs_Argument(const D3D12_VERTEX_BUFFER_VIEW* value_, unsigned size_)
      : value(const_cast<D3D12_VERTEX_BUFFER_VIEW*>(value_)), size(size_) {
    bufferLocationKeys.resize(size_);
    bufferLocationOffsets.resize(size_);
  }
  D3D12_VERTEX_BUFFER_VIEWs_Argument() {}
  D3D12_VERTEX_BUFFER_VIEW* value{};
  size_t size{};
  std::vector<unsigned> bufferLocationKeys{};
  std::vector<unsigned> bufferLocationOffsets{};
};

struct D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument {
  D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument(const D3D12_STREAM_OUTPUT_BUFFER_VIEW* value_,
                                            unsigned size_)
      : value(const_cast<D3D12_STREAM_OUTPUT_BUFFER_VIEW*>(value_)), size(size_) {
    bufferLocationKeys.resize(size_);
    bufferLocationOffsets.resize(size_);
    bufferFilledSizeLocationKeys.resize(size_);
    bufferFilledSizeLocationOffsets.resize(size_);
  }
  D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument() {}
  D3D12_STREAM_OUTPUT_BUFFER_VIEW* value{};
  size_t size{};
  std::vector<unsigned> bufferLocationKeys{};
  std::vector<unsigned> bufferLocationOffsets{};
  std::vector<unsigned> bufferFilledSizeLocationKeys{};
  std::vector<unsigned> bufferFilledSizeLocationOffsets{};
};

struct D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument {
  D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument(const D3D12_WRITEBUFFERIMMEDIATE_PARAMETER* value_,
                                                 unsigned size_)
      : value(const_cast<D3D12_WRITEBUFFERIMMEDIATE_PARAMETER*>(value_)), size(size_) {
    destKeys.resize(size_);
    destOffsets.resize(size_);
  }
  D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument() {}
  D3D12_WRITEBUFFERIMMEDIATE_PARAMETER* value{};
  size_t size{};
  std::vector<unsigned> destKeys{};
  std::vector<unsigned> destOffsets{};
};

struct D3D12_PIPELINE_STATE_STREAM_DESC_Argument {
  D3D12_PIPELINE_STATE_STREAM_DESC_Argument(const D3D12_PIPELINE_STATE_STREAM_DESC* value_)
      : value(const_cast<D3D12_PIPELINE_STATE_STREAM_DESC*>(value_)) {}
  D3D12_PIPELINE_STATE_STREAM_DESC_Argument() {}
  D3D12_PIPELINE_STATE_STREAM_DESC* value{};
  unsigned rootSignatureKey{};
};

struct D3D12_STATE_OBJECT_DESC_Argument {
  D3D12_STATE_OBJECT_DESC_Argument(const D3D12_STATE_OBJECT_DESC* value_)
      : value(const_cast<D3D12_STATE_OBJECT_DESC*>(value_)) {}
  D3D12_STATE_OBJECT_DESC_Argument() {}
  D3D12_STATE_OBJECT_DESC* value{};
  std::map<unsigned, unsigned> interfaceKeysBySubobject;
};

template <>
struct PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC> {
  PointerArgument(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* value_)
      : value(const_cast<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC*>(value_)) {}
  PointerArgument() {}
  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* value{};
  unsigned destAccelerationStructureKey{};
  unsigned destAccelerationStructureOffset{};
  unsigned sourceAccelerationStructureKey{};
  unsigned sourceAccelerationStructureOffset{};
  unsigned scratchAccelerationStructureKey{};
  unsigned scratchAccelerationStructureOffset{};
  std::vector<unsigned> inputKeys{};
  std::vector<unsigned> inputOffsets{};
};

template <>
struct ArrayArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC> {
  ArrayArgument(const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC* value_,
                size_t size_)
      : value(const_cast<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC*>(value_)),
        size(size_) {}
  ArrayArgument() {}
  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC* value{};
  size_t size{};
  std::vector<unsigned> destBufferKeys{};
  std::vector<unsigned> destBufferOffsets{};
};

template <>
struct PointerArgument<D3D12_DISPATCH_RAYS_DESC> {
  PointerArgument(const D3D12_DISPATCH_RAYS_DESC* value_)
      : value(const_cast<D3D12_DISPATCH_RAYS_DESC*>(value_)) {}
  PointerArgument() {}
  D3D12_DISPATCH_RAYS_DESC* value{};
  unsigned rayGenerationShaderRecordKey{};
  unsigned rayGenerationShaderRecordOffset{};
  unsigned missShaderTableKey{};
  unsigned missShaderTableOffset{};
  unsigned hitGroupTableKey{};
  unsigned hitGroupTableOffset{};
  unsigned callableShaderTableKey{};
  unsigned callableShaderTableOffset{};
};

template <>
struct PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC> {
  PointerArgument(const INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC* value_)
      : value(const_cast<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC*>(value_)) {}
  PointerArgument() {}
  INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC* value{};
  const void* cs{};
  const void* compileOptions{};
  const void* internalOptions{};
  unsigned rootSignatureKey{};
};

template <>
struct PointerArgument<INTCExtensionAppInfo> {
  PointerArgument(const INTCExtensionAppInfo* value_)
      : value(const_cast<INTCExtensionAppInfo*>(value_)) {}
  PointerArgument() {}
  INTCExtensionAppInfo* value{};
  const wchar_t* pApplicationName{};
  const wchar_t* pEngineName{};
};

template <>
struct PointerArgument<INTCExtensionAppInfo1> {
  PointerArgument(const INTCExtensionAppInfo1* value_)
      : value(const_cast<INTCExtensionAppInfo1*>(value_)) {}
  PointerArgument() {}
  INTCExtensionAppInfo1* value{};
  const wchar_t* pApplicationName{};
  const wchar_t* pEngineName{};
};

struct D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument {
  D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument(const D3D12_RENDER_PASS_RENDER_TARGET_DESC* value_,
                                                 size_t size_)
      : value(const_cast<D3D12_RENDER_PASS_RENDER_TARGET_DESC*>(value_)), size(size_) {}
  D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument() {}
  D3D12_RENDER_PASS_RENDER_TARGET_DESC* value{};
  size_t size{};
  std::vector<unsigned> descriptorKeys{};
  std::vector<unsigned> descriptorIndexes{};
  std::vector<unsigned> resolveSrcResourceKeys{};
  std::vector<unsigned> resolveDstResourceKeys{};
};

struct D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument {
  D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument(const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC* value_)
      : value(const_cast<D3D12_RENDER_PASS_DEPTH_STENCIL_DESC*>(value_)) {}
  D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument() {}
  D3D12_RENDER_PASS_DEPTH_STENCIL_DESC* value{};
  unsigned descriptorKey{};
  unsigned descriptorIndex{};
  unsigned resolveSrcDepthKey{};
  unsigned resolveDstDepthKey{};
  unsigned resolveSrcStencilKey{};
  unsigned resolveDstStencilKey{};
};

struct D3D12_SHADER_RESOURCE_VIEW_DESC_Argument {
  D3D12_SHADER_RESOURCE_VIEW_DESC_Argument(const D3D12_SHADER_RESOURCE_VIEW_DESC* value_)
      : value(const_cast<D3D12_SHADER_RESOURCE_VIEW_DESC*>(value_)) {}
  D3D12_SHADER_RESOURCE_VIEW_DESC_Argument() {}
  D3D12_SHADER_RESOURCE_VIEW_DESC* value{};
  unsigned raytracingLocationKey{};
  unsigned raytracingLocationOffset{};
};

#pragma endregion

#pragma region DML

struct DML_BINDING_TABLE_DESC_Argument {
  DML_BINDING_TABLE_DESC_Argument(const DML_BINDING_TABLE_DESC* value_)
      : value(const_cast<DML_BINDING_TABLE_DESC*>(value_)) {}
  DML_BINDING_TABLE_DESC_Argument() {}
  DML_BINDING_TABLE_DESC* value{};
  struct Data {
    unsigned dispatchableKey{};
    unsigned cpuDescHandleKey{};
    unsigned cpuDescHandleIndex{};
    unsigned gpuDescHandleKey{};
    unsigned gpuDescHandleIndex{};
  } data;
};

struct DML_BINDING_DESC_Argument {
  DML_BINDING_DESC_Argument(const DML_BINDING_DESC* value_)
      : value(const_cast<DML_BINDING_DESC*>(value_)) {}
  DML_BINDING_DESC_Argument() {}
  DML_BINDING_DESC* value{};
  size_t resourceKeysSize{};
  std::vector<unsigned> resourceKeys{};
};

struct DML_BINDING_DESCs_Argument {
  DML_BINDING_DESCs_Argument(const DML_BINDING_DESC* value_, unsigned size_)
      : value(const_cast<DML_BINDING_DESC*>(value_)), size(size_) {}
  DML_BINDING_DESCs_Argument() {}
  DML_BINDING_DESC* value{};
  size_t size{};
  size_t resourceKeysSize{};
  std::vector<unsigned> resourceKeys{};
};

struct DML_GRAPH_DESC_Argument {
  DML_GRAPH_DESC_Argument(const DML_GRAPH_DESC* value_)
      : value(const_cast<DML_GRAPH_DESC*>(value_)) {}
  DML_GRAPH_DESC_Argument() {}
  DML_GRAPH_DESC* value{};
  size_t operatorKeysSize{};
  std::vector<unsigned> operatorKeys{};
};

struct DML_OPERATOR_DESC_Argument {
  DML_OPERATOR_DESC_Argument(const DML_OPERATOR_DESC* value_)
      : value(const_cast<DML_OPERATOR_DESC*>(value_)) {}
  DML_OPERATOR_DESC_Argument() {}
  DML_OPERATOR_DESC* value{};
};

struct DML_CheckFeatureSupport_BufferArgument {
  DML_CheckFeatureSupport_BufferArgument(const void* value_, size_t size_, DML_FEATURE feat)
      : value{const_cast<void*>(value_)}, size{size_}, feature{feat} {}

  DML_CheckFeatureSupport_BufferArgument() {}
  void* value{};
  size_t size{};
  DML_FEATURE feature{};
};

#pragma endregion

#pragma region DStorage

struct DSTORAGE_QUEUE_DESC_Argument {
  DSTORAGE_QUEUE_DESC_Argument(const DSTORAGE_QUEUE_DESC* value)
      : value(const_cast<DSTORAGE_QUEUE_DESC*>(value)) {}

  DSTORAGE_QUEUE_DESC_Argument() {}

  DSTORAGE_QUEUE_DESC* value{};
  size_t deviceKey{};
  const char* name{};
};

struct DSTORAGE_REQUEST_Argument {
  DSTORAGE_REQUEST_Argument(const DSTORAGE_REQUEST* value)
      : value(const_cast<DSTORAGE_REQUEST*>(value)) {}

  DSTORAGE_REQUEST_Argument() {}

  DSTORAGE_REQUEST* value{};
  unsigned fileKey{};
  unsigned resourceKey{};
  UINT64 newOffset{};
  const char* name{};
};

#pragma endregion

#pragma region XESS

struct xess_d3d12_init_params_t_Argument {
  xess_d3d12_init_params_t_Argument(const xess_d3d12_init_params_t* value_)
      : value(const_cast<xess_d3d12_init_params_t*>(value_)) {}
  xess_d3d12_init_params_t_Argument() {}

  xess_d3d12_init_params_t* value{};
  unsigned key{}; // Used for subcapture restore order
  unsigned tempBufferHeapKey{};
  unsigned tempTextureHeapKey{};
  unsigned pipelineLibraryKey{};
};

struct xess_d3d12_execute_params_t_Argument {
  xess_d3d12_execute_params_t_Argument(const xess_d3d12_execute_params_t* value_)
      : value(const_cast<xess_d3d12_execute_params_t*>(value_)) {}
  xess_d3d12_execute_params_t_Argument() {}

  xess_d3d12_execute_params_t* value{};

  unsigned colorTextureKey{};
  unsigned velocityTextureKey{};
  unsigned depthTextureKey{};
  unsigned exposureScaleTextureKey{};
  unsigned responsivePixelMaskTextureKey{};
  unsigned outputTextureKey{};
  unsigned descriptorHeapKey{};
};

#pragma endregion

} // namespace DirectX
} // namespace gits

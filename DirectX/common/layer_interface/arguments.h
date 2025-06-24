// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "directx.h"

struct INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC;
struct INTCExtensionContext;
struct INTCExtensionAppInfo;
struct INTCExtensionAppInfo1;
struct INTC_D3D12_HEAP_DESC;
struct INTC_D3D12_RESOURCE_DESC_0001;
struct INTC_D3D12_FEATURE;
struct INTC_D3D12_RESOURCE_DESC;
struct INTC_D3D12_COMMAND_QUEUE_DESC_0001;
struct INTCExtensionInfo;
struct INTCExtensionVersion;

#include <vector>
#include <map>

namespace gits {
namespace DirectX {

#pragma region Generic

template <typename T>
struct Argument {
  T value;
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
  PointerArgument(const PointerArgument<T>& arg) {
    if (arg.value) {
      value = new T();
      *value = *arg.value;
    }
    copy = true;
  }
  PointerArgument& operator=(const PointerArgument<T>&) = default;
  ~PointerArgument() {
    if (copy) {
      delete static_cast<T*>(value);
    }
  }
  T* value{};
  bool copy{};
};

template <typename T>
struct ArrayArgument {
  ArrayArgument(const T* value_, size_t size_) : value(const_cast<T*>(value_)), size(size_) {}
  ArrayArgument() {}
  ArrayArgument(const ArrayArgument<T>& arg) {
    size = arg.size;
    if (arg.value) {
      value = new T[size];
      memcpy(value, arg.value, size * sizeof(T));
    }
    copy = true;
  }
  ~ArrayArgument() {
    if (copy) {
      delete[] value;
    }
  }
  T* value{};
  size_t size{};
  bool copy{};
};

struct BufferArgument {
  BufferArgument(const void* value_, size_t size_)
      : value(const_cast<void*>(value_)), size(size_) {}
  BufferArgument() {}
  BufferArgument(const BufferArgument& arg);
  BufferArgument& operator=(const BufferArgument&) = default;
  ~BufferArgument();
  void* value{};
  size_t size{};
  bool copy{};
};

struct OutputBufferArgument {
  OutputBufferArgument(void** value_) : value(value_) {}
  OutputBufferArgument() {}
  OutputBufferArgument(const OutputBufferArgument& arg);
  OutputBufferArgument& operator=(const OutputBufferArgument&) = default;
  ~OutputBufferArgument() = default;
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
  InterfaceArrayArgument(const InterfaceArrayArgument& arg) {
    size = arg.size;
    keys = arg.keys;
    data.resize(size);
    memcpy(data.data(), arg.value, size * sizeof(T*));
    value = data.data();
  }
  InterfaceArrayArgument& operator=(const InterfaceArrayArgument&) = default;
  ~InterfaceArrayArgument() = default;
  T** value{};
  size_t size{};
  std::vector<unsigned> keys{};
  std::vector<T*> data;
};

template <typename T>
struct InterfaceOutputArgument {
  InterfaceOutputArgument(T** value_) : value(value_) {}
  InterfaceOutputArgument() {}
  InterfaceOutputArgument(const InterfaceOutputArgument& arg) {
    key = arg.key;
    data = *arg.value;
    value = &data;
  }
  InterfaceOutputArgument& operator=(const InterfaceOutputArgument&) = default;
  ~InterfaceOutputArgument() = default;
  T** value{};
  unsigned key{};
  T* data{};
};

struct LPCWSTR_Argument {
  LPCWSTR_Argument(LPCWSTR value_) : value(const_cast<LPWSTR>(value_)) {}
  LPCWSTR_Argument() {}
  LPCWSTR_Argument(const LPCWSTR_Argument& arg);
  LPCWSTR_Argument& operator=(const LPCWSTR_Argument&) = default;
  ~LPCWSTR_Argument();
  LPWSTR value{};
  bool copy{};
};

struct LPCSTR_Argument {
  LPCSTR_Argument(LPCSTR value_) : value(const_cast<LPSTR>(value_)) {}
  LPCSTR_Argument() {}
  LPCSTR_Argument(const LPCSTR_Argument& arg);
  LPCSTR_Argument& operator=(const LPCSTR_Argument&) = default;
  ~LPCSTR_Argument();
  LPSTR value{};
  bool copy{};
};

using PCWSTR_Argument = LPCWSTR_Argument;
using WCHAR_Argument = LPCWSTR_Argument;
using PCSTR_Argument = LPCSTR_Argument;

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
  DescriptorHandleArrayArgument(const DescriptorHandleArrayArgument& arg) {
    size = arg.size;
    interfaceKeys = arg.interfaceKeys;
    indexes = arg.indexes;
    data.resize(size);
    memcpy(data.data(), arg.value, size * sizeof(T));
    value = data.data();
  }
  DescriptorHandleArrayArgument& operator=(DescriptorHandleArrayArgument const&) = default;
  ~DescriptorHandleArrayArgument() = default;
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
  D3D12_GPU_VIRTUAL_ADDRESSs_Argument(const D3D12_GPU_VIRTUAL_ADDRESSs_Argument& arg);
  D3D12_GPU_VIRTUAL_ADDRESSs_Argument& operator=(const D3D12_GPU_VIRTUAL_ADDRESSs_Argument&) =
      delete;
  ~D3D12_GPU_VIRTUAL_ADDRESSs_Argument();
  D3D12_GPU_VIRTUAL_ADDRESS* value{};
  size_t size{};
  std::vector<unsigned> interfaceKeys{};
  std::vector<unsigned> offsets{};
  bool copy{};
};

struct ShaderIdentifierArgument {
  ShaderIdentifierArgument(const void* value_) : value(const_cast<void*>(value_)) {}
  ShaderIdentifierArgument() {}
  ShaderIdentifierArgument(const ShaderIdentifierArgument& arg);
  ShaderIdentifierArgument& operator=(ShaderIdentifierArgument const&) = default;
  ~ShaderIdentifierArgument() = default;
  void* value{};
  std::vector<uint8_t> data;
};

template <>
struct PointerArgument<D3D12_ROOT_SIGNATURE_DESC> {
  PointerArgument(const D3D12_ROOT_SIGNATURE_DESC* value_)
      : value(const_cast<D3D12_ROOT_SIGNATURE_DESC*>(value_)) {}
  PointerArgument() {}
  PointerArgument(const PointerArgument<D3D12_ROOT_SIGNATURE_DESC>& arg);
  PointerArgument& operator=(const PointerArgument<D3D12_ROOT_SIGNATURE_DESC>&) = default;
  ~PointerArgument();
  D3D12_ROOT_SIGNATURE_DESC* value{};
  bool copy{};
};

template <>
struct PointerArgument<D3D12_VERSIONED_ROOT_SIGNATURE_DESC> {
  PointerArgument(const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* value_)
      : value(const_cast<D3D12_VERSIONED_ROOT_SIGNATURE_DESC*>(value_)) {}
  PointerArgument() {}
  PointerArgument(const PointerArgument<D3D12_VERSIONED_ROOT_SIGNATURE_DESC>& arg);
  PointerArgument& operator=(const PointerArgument<D3D12_VERSIONED_ROOT_SIGNATURE_DESC>&) = default;
  ~PointerArgument();
  D3D12_VERSIONED_ROOT_SIGNATURE_DESC* value{};
  bool copy{};
};

struct D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument {
  D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument(const D3D12_GRAPHICS_PIPELINE_STATE_DESC* value_)
      : value(const_cast<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>(value_)) {}
  D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument() {}
  D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument(
      const D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument& arg);
  D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument& operator=(
      const D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument&) = default;
  ~D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument();
  D3D12_GRAPHICS_PIPELINE_STATE_DESC* value{};
  unsigned rootSignatureKey{};
  bool copy{};
};

struct D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument {
  D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument(const D3D12_COMPUTE_PIPELINE_STATE_DESC* value_)
      : value(const_cast<D3D12_COMPUTE_PIPELINE_STATE_DESC*>(value_)) {}
  D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument() {}
  D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument(const D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument& arg);
  D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument& operator=(
      const D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument&) = default;
  ~D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument();
  D3D12_COMPUTE_PIPELINE_STATE_DESC* value{};
  unsigned rootSignatureKey{};
  bool copy{};
};

struct D3D12_TEXTURE_COPY_LOCATION_Argument {
  D3D12_TEXTURE_COPY_LOCATION_Argument(const D3D12_TEXTURE_COPY_LOCATION* value_)
      : value(const_cast<D3D12_TEXTURE_COPY_LOCATION*>(value_)) {}
  D3D12_TEXTURE_COPY_LOCATION_Argument() {}
  D3D12_TEXTURE_COPY_LOCATION_Argument(const D3D12_TEXTURE_COPY_LOCATION_Argument& arg);
  D3D12_TEXTURE_COPY_LOCATION_Argument& operator=(const D3D12_TEXTURE_COPY_LOCATION_Argument&) =
      default;
  ~D3D12_TEXTURE_COPY_LOCATION_Argument();
  D3D12_TEXTURE_COPY_LOCATION* value{};
  unsigned resourceKey{};
  bool copy{};
};

struct D3D12_RESOURCE_BARRIERs_Argument {
  D3D12_RESOURCE_BARRIERs_Argument(const D3D12_RESOURCE_BARRIER* value_, unsigned size_)
      : value(const_cast<D3D12_RESOURCE_BARRIER*>(value_)), size(size_) {
    resourceKeys.resize(size_);
    resourceAfterKeys.resize(size_);
  }
  D3D12_RESOURCE_BARRIERs_Argument() {}
  D3D12_RESOURCE_BARRIERs_Argument(const D3D12_RESOURCE_BARRIERs_Argument& arg);
  D3D12_RESOURCE_BARRIERs_Argument& operator=(const D3D12_RESOURCE_BARRIERs_Argument&) = default;
  ~D3D12_RESOURCE_BARRIERs_Argument();
  D3D12_RESOURCE_BARRIER* value{};
  size_t size{};
  std::vector<unsigned> resourceKeys{};
  std::vector<unsigned> resourceAfterKeys{};
  bool copy{};
};

struct D3D12_SHADER_RESOURCE_VIEW_DESC_Argument {
  D3D12_SHADER_RESOURCE_VIEW_DESC_Argument(const D3D12_SHADER_RESOURCE_VIEW_DESC* value_)
      : value(const_cast<D3D12_SHADER_RESOURCE_VIEW_DESC*>(value_)) {}
  D3D12_SHADER_RESOURCE_VIEW_DESC_Argument() {}
  D3D12_SHADER_RESOURCE_VIEW_DESC_Argument(const D3D12_SHADER_RESOURCE_VIEW_DESC_Argument& arg);
  D3D12_SHADER_RESOURCE_VIEW_DESC_Argument& operator=(
      const D3D12_SHADER_RESOURCE_VIEW_DESC_Argument&) = default;
  ~D3D12_SHADER_RESOURCE_VIEW_DESC_Argument();
  D3D12_SHADER_RESOURCE_VIEW_DESC* value{};
  unsigned raytracingLocationKey{};
  unsigned raytracingLocationOffset{};
  bool copy{};
};

struct D3D12_INDEX_BUFFER_VIEW_Argument {
  D3D12_INDEX_BUFFER_VIEW_Argument(const D3D12_INDEX_BUFFER_VIEW* value_)
      : value(const_cast<D3D12_INDEX_BUFFER_VIEW*>(value_)) {}
  D3D12_INDEX_BUFFER_VIEW_Argument() {}
  D3D12_INDEX_BUFFER_VIEW_Argument(const D3D12_INDEX_BUFFER_VIEW_Argument& arg);
  D3D12_INDEX_BUFFER_VIEW_Argument& operator=(const D3D12_INDEX_BUFFER_VIEW_Argument&) = default;
  ~D3D12_INDEX_BUFFER_VIEW_Argument();
  D3D12_INDEX_BUFFER_VIEW* value{};
  unsigned bufferLocationKey{};
  unsigned bufferLocationOffset{};
  bool copy{};
};

struct D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument {
  D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument(const D3D12_CONSTANT_BUFFER_VIEW_DESC* value_)
      : value(const_cast<D3D12_CONSTANT_BUFFER_VIEW_DESC*>(value_)) {}
  D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument() {}
  D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument(const D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument& arg);
  D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument& operator=(
      const D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument&) = default;
  ~D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument();
  D3D12_CONSTANT_BUFFER_VIEW_DESC* value{};
  unsigned bufferLocationKey{};
  unsigned bufferLocationOffset{};
  bool copy{};
};

struct D3D12_VERTEX_BUFFER_VIEWs_Argument {
  D3D12_VERTEX_BUFFER_VIEWs_Argument(const D3D12_VERTEX_BUFFER_VIEW* value_, unsigned size_)
      : value(const_cast<D3D12_VERTEX_BUFFER_VIEW*>(value_)), size(size_) {
    bufferLocationKeys.resize(size_);
    bufferLocationOffsets.resize(size_);
  }
  D3D12_VERTEX_BUFFER_VIEWs_Argument() {}
  D3D12_VERTEX_BUFFER_VIEWs_Argument(const D3D12_VERTEX_BUFFER_VIEWs_Argument& arg);
  ~D3D12_VERTEX_BUFFER_VIEWs_Argument();
  D3D12_VERTEX_BUFFER_VIEW* value{};
  size_t size{};
  std::vector<unsigned> bufferLocationKeys{};
  std::vector<unsigned> bufferLocationOffsets{};
  bool copy{};
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
  D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument(const D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument& arg);
  D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument& operator=(
      const D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument&) = default;
  ~D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument();
  D3D12_STREAM_OUTPUT_BUFFER_VIEW* value{};
  size_t size{};
  std::vector<unsigned> bufferLocationKeys{};
  std::vector<unsigned> bufferLocationOffsets{};
  std::vector<unsigned> bufferFilledSizeLocationKeys{};
  std::vector<unsigned> bufferFilledSizeLocationOffsets{};
  bool copy{};
};

struct D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument {
  D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument(const D3D12_WRITEBUFFERIMMEDIATE_PARAMETER* value_,
                                                 unsigned size_)
      : value(const_cast<D3D12_WRITEBUFFERIMMEDIATE_PARAMETER*>(value_)), size(size_) {
    destKeys.resize(size_);
    destOffsets.resize(size_);
  }
  D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument() {}
  D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument(
      const D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument& arg);
  D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument& operator=(
      const D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument&) = default;
  ~D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument();
  D3D12_WRITEBUFFERIMMEDIATE_PARAMETER* value{};
  size_t size{};
  std::vector<unsigned> destKeys{};
  std::vector<unsigned> destOffsets{};
  bool copy{};
};

struct D3D12_PIPELINE_STATE_STREAM_DESC_Argument {
  D3D12_PIPELINE_STATE_STREAM_DESC_Argument(const D3D12_PIPELINE_STATE_STREAM_DESC* value_)
      : value(const_cast<D3D12_PIPELINE_STATE_STREAM_DESC*>(value_)) {}
  D3D12_PIPELINE_STATE_STREAM_DESC_Argument() {}
  D3D12_PIPELINE_STATE_STREAM_DESC_Argument(const D3D12_PIPELINE_STATE_STREAM_DESC_Argument& arg);
  ~D3D12_PIPELINE_STATE_STREAM_DESC_Argument();
  D3D12_PIPELINE_STATE_STREAM_DESC* value{};
  unsigned rootSignatureKey{};
  bool copy{};
};

struct D3D12_STATE_OBJECT_DESC_Argument {
  D3D12_STATE_OBJECT_DESC_Argument(const D3D12_STATE_OBJECT_DESC* value_)
      : value(const_cast<D3D12_STATE_OBJECT_DESC*>(value_)) {}
  D3D12_STATE_OBJECT_DESC_Argument() {}
  D3D12_STATE_OBJECT_DESC_Argument(const D3D12_STATE_OBJECT_DESC_Argument& arg);
  D3D12_STATE_OBJECT_DESC_Argument& operator=(const D3D12_STATE_OBJECT_DESC_Argument&) = default;
  ~D3D12_STATE_OBJECT_DESC_Argument();
  D3D12_STATE_OBJECT_DESC* value{};
  std::map<unsigned, unsigned> interfaceKeysBySubobject;
  bool copy{};
};

template <>
struct PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS> {
  PointerArgument(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS* value_)
      : value(const_cast<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS*>(value_)) {}
  PointerArgument() {}
  PointerArgument(const PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS>& arg);
  PointerArgument& operator=(
      const PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS>&) = default;
  ~PointerArgument();
  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS* value{};
  std::vector<unsigned> inputKeys{};
  std::vector<unsigned> inputOffsets{};
  bool copy{};
};

template <>
struct PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC> {
  PointerArgument(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* value_)
      : value(const_cast<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC*>(value_)) {}
  PointerArgument() {}
  PointerArgument(const PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC>& arg);
  PointerArgument& operator=(
      const PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC>&) = default;
  ~PointerArgument();
  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* value{};
  unsigned destAccelerationStructureKey{};
  unsigned destAccelerationStructureOffset{};
  unsigned sourceAccelerationStructureKey{};
  unsigned sourceAccelerationStructureOffset{};
  unsigned scratchAccelerationStructureKey{};
  unsigned scratchAccelerationStructureOffset{};
  std::vector<unsigned> inputKeys{};
  std::vector<unsigned> inputOffsets{};
  bool copy{};
};

template <>
struct ArrayArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC> {
  ArrayArgument(const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC* value_,
                size_t size_)
      : value(const_cast<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC*>(value_)),
        size(size_) {}
  ArrayArgument() {}
  ArrayArgument(
      const ArrayArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg);
  ArrayArgument& operator=(
      const ArrayArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>&) = default;
  ~ArrayArgument();
  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC* value{};
  size_t size{};
  std::vector<unsigned> destBufferKeys{};
  std::vector<unsigned> destBufferOffsets{};
  bool copy{};
};

template <>
struct PointerArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC> {
  PointerArgument(const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC* value_)
      : value(const_cast<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC*>(value_)) {}
  PointerArgument() {}
  PointerArgument(
      const PointerArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg);
  PointerArgument& operator=(
      const PointerArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>&) =
      default;
  ~PointerArgument();
  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC* value{};
  unsigned destBufferKey{};
  unsigned destBufferOffset{};
  bool copy{};
};

template <>
struct PointerArgument<D3D12_DISPATCH_RAYS_DESC> {
  PointerArgument(const D3D12_DISPATCH_RAYS_DESC* value_)
      : value(const_cast<D3D12_DISPATCH_RAYS_DESC*>(value_)) {}
  PointerArgument() {}
  PointerArgument(const PointerArgument<D3D12_DISPATCH_RAYS_DESC>& arg);
  PointerArgument& operator=(const PointerArgument<D3D12_DISPATCH_RAYS_DESC>&) = default;
  ~PointerArgument();
  D3D12_DISPATCH_RAYS_DESC* value{};
  unsigned rayGenerationShaderRecordKey{};
  unsigned rayGenerationShaderRecordOffset{};
  unsigned missShaderTableKey{};
  unsigned missShaderTableOffset{};
  unsigned hitGroupTableKey{};
  unsigned hitGroupTableOffset{};
  unsigned callableShaderTableKey{};
  unsigned callableShaderTableOffset{};
  bool copy{};
};

struct D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument {
  D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument(const D3D12_RENDER_PASS_RENDER_TARGET_DESC* value_,
                                                 size_t size_)
      : value(const_cast<D3D12_RENDER_PASS_RENDER_TARGET_DESC*>(value_)), size(size_) {}
  D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument() {}
  D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument(
      const D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument& arg);
  D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument& operator=(
      const D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument&) = default;
  ~D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument();
  D3D12_RENDER_PASS_RENDER_TARGET_DESC* value{};
  size_t size{};
  std::vector<unsigned> descriptorKeys{};
  std::vector<unsigned> descriptorIndexes{};
  std::vector<unsigned> resolveSrcResourceKeys{};
  std::vector<unsigned> resolveDstResourceKeys{};
  bool copy{};
};

struct D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument {
  D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument(const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC* value_)
      : value(const_cast<D3D12_RENDER_PASS_DEPTH_STENCIL_DESC*>(value_)) {}
  D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument() {}
  D3D12_RENDER_PASS_DEPTH_STENCIL_DESC* value{};
  D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument(
      const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument& arg);
  D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument& operator=(
      const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument&) = default;
  ~D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument();
  unsigned descriptorKey{};
  unsigned descriptorIndex{};
  unsigned resolveSrcDepthKey{};
  unsigned resolveDstDepthKey{};
  unsigned resolveSrcStencilKey{};
  unsigned resolveDstStencilKey{};
  bool copy{};
};

template <>
struct PointerArgument<D3D12_COMMAND_SIGNATURE_DESC> {
  PointerArgument(const D3D12_COMMAND_SIGNATURE_DESC* value_)
      : value(const_cast<D3D12_COMMAND_SIGNATURE_DESC*>(value_)) {}
  PointerArgument() {}
  PointerArgument(const PointerArgument<D3D12_COMMAND_SIGNATURE_DESC>& arg);
  PointerArgument& operator=(const PointerArgument<D3D12_COMMAND_SIGNATURE_DESC>&) = default;
  ~PointerArgument();
  D3D12_COMMAND_SIGNATURE_DESC* value{};
  bool copy{};
};

template <>
struct ArrayArgument<D3D12_META_COMMAND_DESC> {
  ArrayArgument(const D3D12_META_COMMAND_DESC* value_, size_t size_)
      : value(const_cast<D3D12_META_COMMAND_DESC*>(value_)), size(size_) {}
  ArrayArgument() {}
  ArrayArgument(const ArrayArgument<D3D12_META_COMMAND_DESC>& arg);
  ArrayArgument& operator=(const ArrayArgument<D3D12_META_COMMAND_DESC>&) = default;
  ~ArrayArgument();
  D3D12_META_COMMAND_DESC* value{};
  size_t size{};
  bool copy{};
};

#pragma endregion

#pragma region INTC

template <typename T>
struct ContextArgument {
  T value{};
  unsigned key{};
};

template <typename T>
struct ContextOutputArgument {
  ContextOutputArgument(T* value_) : value(value_) {}
  ContextOutputArgument() {}
  ContextOutputArgument(const ContextOutputArgument& arg) {
    key = arg.key;
    data = *arg.value;
    value = &data;
  }
  ContextOutputArgument& operator=(const ContextOutputArgument&) = default;
  ~ContextOutputArgument() = default;
  T* value{};
  T data{};
  unsigned key{};
};

using INTCExtensionContextArgument = ContextArgument<INTCExtensionContext*>;
using INTCExtensionContextOutputArgument = ContextOutputArgument<INTCExtensionContext*>;
using XESSContextArgument = ContextArgument<xess_context_handle_t>;
using XESSContextOutputArgument = ContextOutputArgument<xess_context_handle_t>;

template <>
struct PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC> {
  PointerArgument(const INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC* value_)
      : value(const_cast<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC*>(value_)) {}
  PointerArgument() {}
  PointerArgument(const PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>& arg);
  PointerArgument& operator=(const PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>&) =
      default;
  ~PointerArgument();
  INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC* value{};
  const void* cs{};
  const void* compileOptions{};
  const void* internalOptions{};
  unsigned rootSignatureKey{};
  bool copy{};
};

template <>
struct PointerArgument<INTCExtensionAppInfo> {
  PointerArgument(const INTCExtensionAppInfo* value_)
      : value(const_cast<INTCExtensionAppInfo*>(value_)) {}
  PointerArgument() {}
  PointerArgument(const PointerArgument<INTCExtensionAppInfo>& arg);
  ~PointerArgument();
  INTCExtensionAppInfo* value{};
  const wchar_t* pApplicationName{};
  const wchar_t* pEngineName{};
  bool copy{};
};

template <>
struct PointerArgument<INTCExtensionAppInfo1> {
  PointerArgument(const INTCExtensionAppInfo1* value_)
      : value(const_cast<INTCExtensionAppInfo1*>(value_)) {}
  PointerArgument() {}
  PointerArgument(const PointerArgument<INTCExtensionAppInfo1>& arg);
  PointerArgument& operator=(const PointerArgument<INTCExtensionAppInfo1>&) = default;
  ~PointerArgument();
  INTCExtensionAppInfo1* value{};
  const wchar_t* pApplicationName{};
  const wchar_t* pEngineName{};
  bool copy{};
};

template <>
struct PointerArgument<INTC_D3D12_HEAP_DESC> {
  PointerArgument(const INTC_D3D12_HEAP_DESC* value_)
      : value(const_cast<INTC_D3D12_HEAP_DESC*>(value_)) {}
  PointerArgument() {}
  PointerArgument(const PointerArgument<INTC_D3D12_HEAP_DESC>& arg);
  PointerArgument& operator=(const PointerArgument<INTC_D3D12_HEAP_DESC>&) = default;
  ~PointerArgument();
  INTC_D3D12_HEAP_DESC* value{};
  bool copy{};
};

template <>
struct PointerArgument<INTC_D3D12_RESOURCE_DESC_0001> {
  PointerArgument(const INTC_D3D12_RESOURCE_DESC_0001* value_)
      : value(const_cast<INTC_D3D12_RESOURCE_DESC_0001*>(value_)) {}
  PointerArgument() {}
  PointerArgument(const PointerArgument<INTC_D3D12_RESOURCE_DESC_0001>& arg);
  PointerArgument& operator=(const PointerArgument<INTC_D3D12_RESOURCE_DESC_0001>&) = default;
  ~PointerArgument();
  INTC_D3D12_RESOURCE_DESC_0001* value{};
  bool copy{};
};

template <>
struct PointerArgument<INTC_D3D12_FEATURE> {
  PointerArgument(const INTC_D3D12_FEATURE* value_)
      : value(const_cast<INTC_D3D12_FEATURE*>(value_)) {}
  PointerArgument() {}
  PointerArgument(const PointerArgument<INTC_D3D12_FEATURE>& arg);
  PointerArgument& operator=(const PointerArgument<INTC_D3D12_FEATURE>&) = default;
  ~PointerArgument();
  INTC_D3D12_FEATURE* value{};
  bool copy{};
};

template <>
struct PointerArgument<INTC_D3D12_RESOURCE_DESC> {
  PointerArgument(const INTC_D3D12_RESOURCE_DESC* value_)
      : value(const_cast<INTC_D3D12_RESOURCE_DESC*>(value_)) {}
  PointerArgument() {}
  PointerArgument(const PointerArgument<INTC_D3D12_RESOURCE_DESC>& arg);
  PointerArgument& operator=(const PointerArgument<INTC_D3D12_RESOURCE_DESC>&) = default;
  ~PointerArgument();
  INTC_D3D12_RESOURCE_DESC* value{};
  bool copy{};
};

template <>
struct PointerArgument<INTC_D3D12_COMMAND_QUEUE_DESC_0001> {
  PointerArgument(const INTC_D3D12_COMMAND_QUEUE_DESC_0001* value_)
      : value(const_cast<INTC_D3D12_COMMAND_QUEUE_DESC_0001*>(value_)) {}
  PointerArgument() {}
  PointerArgument(const PointerArgument<INTC_D3D12_COMMAND_QUEUE_DESC_0001>& arg);
  PointerArgument& operator=(const PointerArgument<INTC_D3D12_COMMAND_QUEUE_DESC_0001>&) = default;
  ~PointerArgument();
  INTC_D3D12_COMMAND_QUEUE_DESC_0001* value{};
  bool copy{};
};

template <>
struct PointerArgument<INTCExtensionInfo> {
  PointerArgument(const INTCExtensionInfo* value_)
      : value(const_cast<INTCExtensionInfo*>(value_)) {}
  PointerArgument() {}
  PointerArgument(const PointerArgument<INTCExtensionInfo>& arg);
  PointerArgument& operator=(const PointerArgument<INTCExtensionInfo>&) = default;
  ~PointerArgument();
  INTCExtensionInfo* value{};
  bool copy{};
  bool copyDeviceDriverDesc{};
  bool copyDeviceDriverVersion{};
};

template <>
struct ArrayArgument<INTCExtensionVersion> {
  ArrayArgument(const INTCExtensionVersion* value_, size_t size_)
      : value(const_cast<INTCExtensionVersion*>(value_)), size(size_) {}
  ArrayArgument() {}
  ArrayArgument(const ArrayArgument<INTCExtensionVersion>& arg);
  ArrayArgument& operator=(const ArrayArgument<INTCExtensionVersion>&) = default;
  ~ArrayArgument();
  INTCExtensionVersion* value{};
  size_t size{};
  bool copy{};
};

#pragma endregion

#pragma region DML

struct DML_BINDING_TABLE_DESC_Argument {
  DML_BINDING_TABLE_DESC_Argument(const DML_BINDING_TABLE_DESC* value_)
      : value(const_cast<DML_BINDING_TABLE_DESC*>(value_)) {}
  DML_BINDING_TABLE_DESC_Argument() {}
  DML_BINDING_TABLE_DESC_Argument(const DML_BINDING_TABLE_DESC_Argument& arg);
  DML_BINDING_TABLE_DESC_Argument& operator=(const DML_BINDING_TABLE_DESC_Argument&) = default;
  ~DML_BINDING_TABLE_DESC_Argument();
  DML_BINDING_TABLE_DESC* value{};
  struct Data {
    unsigned dispatchableKey{};
    unsigned cpuDescHandleKey{};
    unsigned cpuDescHandleIndex{};
    unsigned gpuDescHandleKey{};
    unsigned gpuDescHandleIndex{};
  } data;
  bool copy{};
};

struct DML_BINDING_DESC_Argument {
  DML_BINDING_DESC_Argument(const DML_BINDING_DESC* value_)
      : value(const_cast<DML_BINDING_DESC*>(value_)) {}
  DML_BINDING_DESC_Argument() {}
  DML_BINDING_DESC_Argument(const DML_BINDING_DESC_Argument& arg);
  DML_BINDING_DESC_Argument& operator=(const DML_BINDING_DESC_Argument&) = default;
  ~DML_BINDING_DESC_Argument();
  DML_BINDING_DESC* value{};
  size_t resourceKeysSize{};
  std::vector<unsigned> resourceKeys{};
  bool copy{};
};

struct DML_BINDING_DESCs_Argument {
  DML_BINDING_DESCs_Argument(const DML_BINDING_DESC* value_, unsigned size_)
      : value(const_cast<DML_BINDING_DESC*>(value_)), size(size_) {}
  DML_BINDING_DESCs_Argument() {}
  DML_BINDING_DESCs_Argument(const DML_BINDING_DESCs_Argument& arg);
  DML_BINDING_DESCs_Argument& operator=(const DML_BINDING_DESCs_Argument&) = default;
  ~DML_BINDING_DESCs_Argument();
  DML_BINDING_DESC* value{};
  size_t size{};
  size_t resourceKeysSize{};
  std::vector<unsigned> resourceKeys{};
  bool copy{};
};

struct DML_GRAPH_DESC_Argument {
  DML_GRAPH_DESC_Argument(const DML_GRAPH_DESC* value_)
      : value(const_cast<DML_GRAPH_DESC*>(value_)) {}
  DML_GRAPH_DESC_Argument() {}
  DML_GRAPH_DESC_Argument(const DML_GRAPH_DESC_Argument& arg);
  DML_GRAPH_DESC_Argument& operator=(const DML_GRAPH_DESC_Argument&) = default;
  ~DML_GRAPH_DESC_Argument();
  DML_GRAPH_DESC* value{};
  size_t operatorKeysSize{};
  std::vector<unsigned> operatorKeys{};
  bool copy{};
};

struct DML_OPERATOR_DESC_Argument {
  DML_OPERATOR_DESC_Argument(const DML_OPERATOR_DESC* value_)
      : value(const_cast<DML_OPERATOR_DESC*>(value_)) {}
  DML_OPERATOR_DESC_Argument() {}
  DML_OPERATOR_DESC_Argument(const DML_OPERATOR_DESC_Argument& arg);
  DML_OPERATOR_DESC_Argument& operator=(const DML_OPERATOR_DESC_Argument&) = default;
  ~DML_OPERATOR_DESC_Argument();
  DML_OPERATOR_DESC* value{};
  bool copy{};
};

struct DML_CheckFeatureSupport_BufferArgument {
  DML_CheckFeatureSupport_BufferArgument(const void* value_, size_t size_, DML_FEATURE feat)
      : value{const_cast<void*>(value_)}, size{size_}, feature{feat} {}
  DML_CheckFeatureSupport_BufferArgument() {}
  DML_CheckFeatureSupport_BufferArgument(const DML_CheckFeatureSupport_BufferArgument& arg);
  DML_CheckFeatureSupport_BufferArgument& operator=(const DML_CheckFeatureSupport_BufferArgument&) =
      default;
  ~DML_CheckFeatureSupport_BufferArgument();
  void* value{};
  size_t size{};
  DML_FEATURE feature{};
  bool copy{};
};

#pragma endregion

#pragma region DStorage

struct DSTORAGE_QUEUE_DESC_Argument {
  DSTORAGE_QUEUE_DESC_Argument(const DSTORAGE_QUEUE_DESC* value)
      : value(const_cast<DSTORAGE_QUEUE_DESC*>(value)) {}
  DSTORAGE_QUEUE_DESC_Argument() {}
  DSTORAGE_QUEUE_DESC_Argument(const DSTORAGE_QUEUE_DESC_Argument& arg);
  DSTORAGE_QUEUE_DESC_Argument& operator=(const DSTORAGE_QUEUE_DESC_Argument&) = default;
  ~DSTORAGE_QUEUE_DESC_Argument();
  DSTORAGE_QUEUE_DESC* value{};
  size_t deviceKey{};
  bool copy{};
};

struct DSTORAGE_REQUEST_Argument {
  DSTORAGE_REQUEST_Argument(const DSTORAGE_REQUEST* value)
      : value(const_cast<DSTORAGE_REQUEST*>(value)) {}
  DSTORAGE_REQUEST_Argument() {}
  DSTORAGE_REQUEST_Argument(const DSTORAGE_REQUEST_Argument& arg);
  DSTORAGE_REQUEST_Argument& operator=(const DSTORAGE_REQUEST_Argument&) = default;
  ~DSTORAGE_REQUEST_Argument();
  DSTORAGE_REQUEST* value{};
  unsigned fileKey{};
  unsigned resourceKey{};
  UINT64 newOffset{};
  bool copy{};
};

#pragma endregion

#pragma region XESS

struct xess_d3d12_init_params_t_Argument {
  xess_d3d12_init_params_t_Argument(const xess_d3d12_init_params_t* value_)
      : value(const_cast<xess_d3d12_init_params_t*>(value_)) {}
  xess_d3d12_init_params_t_Argument() {}
  xess_d3d12_init_params_t_Argument(const xess_d3d12_init_params_t_Argument& arg);
  xess_d3d12_init_params_t_Argument& operator=(const xess_d3d12_init_params_t_Argument&) = default;
  ~xess_d3d12_init_params_t_Argument();
  xess_d3d12_init_params_t* value{};
  unsigned key{}; // Used for subcapture restore order
  unsigned tempBufferHeapKey{};
  unsigned tempTextureHeapKey{};
  unsigned pipelineLibraryKey{};
  bool copy{};
};

struct xess_d3d12_execute_params_t_Argument {
  xess_d3d12_execute_params_t_Argument(const xess_d3d12_execute_params_t* value_)
      : value(const_cast<xess_d3d12_execute_params_t*>(value_)) {}
  xess_d3d12_execute_params_t_Argument() {}
  xess_d3d12_execute_params_t_Argument(const xess_d3d12_execute_params_t_Argument& arg);
  xess_d3d12_execute_params_t_Argument& operator=(const xess_d3d12_execute_params_t_Argument&) =
      default;
  ~xess_d3d12_execute_params_t_Argument();
  xess_d3d12_execute_params_t* value{};
  unsigned colorTextureKey{};
  unsigned velocityTextureKey{};
  unsigned depthTextureKey{};
  unsigned exposureScaleTextureKey{};
  unsigned responsivePixelMaskTextureKey{};
  unsigned outputTextureKey{};
  unsigned descriptorHeapKey{};
  bool copy{};
};

#pragma endregion

} // namespace DirectX
} // namespace gits

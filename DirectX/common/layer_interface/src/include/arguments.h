// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "directx.h"
#include "nvapi.h"

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
  T Value{};
};

template <typename T, int N>
struct StaticArrayArgument {
  StaticArrayArgument(T* value_) {
    for (int i = 0; i < N; ++i) {
      Value[i] = value_[i];
    }
  }
  StaticArrayArgument() {}
  T Value[N];
};

template <typename T>
struct PointerArgument {
  PointerArgument(const T* value_) : Value(const_cast<T*>(value_)) {}
  PointerArgument() {}
  PointerArgument(const PointerArgument<T>& arg) {
    if (arg.Value) {
      Value = new T();
      *Value = *arg.Value;
    }
    Copy = true;
  }
  PointerArgument& operator=(const PointerArgument<T>&) = delete;
  ~PointerArgument() {
    if (Copy) {
      delete static_cast<T*>(Value);
    }
  }
  T* Value{};
  bool Copy{};
};

template <typename T>
struct ArrayArgument {
  ArrayArgument(const T* value_, size_t size_) : Value(const_cast<T*>(value_)), Size(size_) {}
  ArrayArgument() {}
  ArrayArgument(const ArrayArgument<T>& arg) {
    Size = arg.Size;
    if (arg.Value) {
      Value = new T[Size];
      memcpy(Value, arg.Value, Size * sizeof(T));
    }
    Copy = true;
  }
  ArrayArgument& operator=(const ArrayArgument<T>&) = delete;
  ~ArrayArgument() {
    if (Copy) {
      delete[] Value;
    }
  }
  T* Value{};
  size_t Size{};
  bool Copy{};
};

struct BufferArgument {
  BufferArgument(const void* value_, size_t size_)
      : Value(const_cast<void*>(value_)), Size(size_) {}
  BufferArgument() {}
  BufferArgument(const BufferArgument& arg);
  BufferArgument& operator=(const BufferArgument&) = delete;
  ~BufferArgument();
  void* Value{};
  size_t Size{};
  bool Copy{};
};

struct OutputBufferArgument {
  OutputBufferArgument(void** value_) : Value(value_) {}
  OutputBufferArgument() {}
  OutputBufferArgument(const OutputBufferArgument& arg);
  OutputBufferArgument& operator=(const OutputBufferArgument&) = delete;
  ~OutputBufferArgument() {}
  void** Value{};
  void* CaptureValue{};
  void* Data{};
};

template <typename T>
struct InterfaceArgument {
  T* Value{};
  unsigned Key{};
};

template <typename T>
struct InterfaceArrayArgument {
  InterfaceArrayArgument(T** value_, size_t size_) : Value(value_), Size(size_) {
    Keys.resize(Size);
  }
  InterfaceArrayArgument() {}
  InterfaceArrayArgument(const InterfaceArrayArgument& arg) {
    Size = arg.Size;
    Keys = arg.Keys;
    Data.resize(Size);
    memcpy(Data.data(), arg.Value, Size * sizeof(T*));
    Value = Data.data();
  }
  InterfaceArrayArgument& operator=(const InterfaceArrayArgument&) = delete;
  ~InterfaceArrayArgument() {}
  T** Value{};
  size_t Size{};
  std::vector<unsigned> Keys{};
  std::vector<T*> Data;
};

template <typename T>
struct InterfaceOutputArgument {
  InterfaceOutputArgument(T** value_) : Value(value_) {}
  InterfaceOutputArgument() {}
  InterfaceOutputArgument(const InterfaceOutputArgument& arg) {
    Key = arg.Key;
    Data = *arg.Value;
    Value = &Data;
  }
  InterfaceOutputArgument& operator=(const InterfaceOutputArgument&) = delete;
  ~InterfaceOutputArgument() {}
  T** Value{};
  unsigned Key{};
  T* Data{};
};

struct LPCWSTR_Argument {
  LPCWSTR_Argument(LPCWSTR value_) : Value(const_cast<LPWSTR>(value_)) {}
  LPCWSTR_Argument() {}
  LPCWSTR_Argument(const LPCWSTR_Argument& arg);
  LPCWSTR_Argument& operator=(const LPCWSTR_Argument&) = delete;
  ~LPCWSTR_Argument();
  LPWSTR Value{};
  bool Copy{};
};

struct LPCSTR_Argument {
  LPCSTR_Argument(LPCSTR value_) : Value(const_cast<LPSTR>(value_)) {}
  LPCSTR_Argument() {}
  LPCSTR_Argument(const LPCSTR_Argument& arg);
  LPCSTR_Argument& operator=(const LPCSTR_Argument&) = delete;
  ~LPCSTR_Argument();
  LPSTR Value{};
  bool Copy{};
};

using PCWSTR_Argument = LPCWSTR_Argument;
using WCHAR_Argument = LPCWSTR_Argument;
using PCSTR_Argument = LPCSTR_Argument;

#pragma endregion

#pragma region D3D12

template <typename T>
struct DescriptorHandleArgument {
  T Value{};
  unsigned InterfaceKey{};
  unsigned Index{};
};

template <typename T>
struct DescriptorHandleArrayArgument {
  DescriptorHandleArrayArgument(const T* value_, size_t size_)
      : Value(const_cast<T*>(value_)), Size(size_) {
    InterfaceKeys.resize(Size);
    Indexes.resize(Size);
  }
  DescriptorHandleArrayArgument() {}
  DescriptorHandleArrayArgument(const DescriptorHandleArrayArgument& arg) {
    Size = arg.Size;
    InterfaceKeys = arg.InterfaceKeys;
    Indexes = arg.Indexes;
    Data.resize(Size);
    memcpy(Data.data(), arg.Value, Size * sizeof(T));
    Value = Data.data();
  }
  DescriptorHandleArrayArgument& operator=(DescriptorHandleArrayArgument const&) = delete;
  ~DescriptorHandleArrayArgument() {}
  T* Value{};
  size_t Size{};
  std::vector<unsigned> InterfaceKeys{};
  std::vector<unsigned> Indexes{};
  std::vector<T> Data;
};

struct D3D12_GPU_VIRTUAL_ADDRESS_Argument {
  D3D12_GPU_VIRTUAL_ADDRESS Value{};
  unsigned InterfaceKey{};
  unsigned Offset{};
};

struct D3D12_GPU_VIRTUAL_ADDRESSs_Argument {
  D3D12_GPU_VIRTUAL_ADDRESSs_Argument(const D3D12_GPU_VIRTUAL_ADDRESS* value_, unsigned size_)
      : Value(const_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(value_)), Size(size_) {
    InterfaceKeys.resize(size_);
    Offsets.resize(size_);
  }
  D3D12_GPU_VIRTUAL_ADDRESSs_Argument() {}
  D3D12_GPU_VIRTUAL_ADDRESSs_Argument(const D3D12_GPU_VIRTUAL_ADDRESSs_Argument& arg);
  D3D12_GPU_VIRTUAL_ADDRESSs_Argument& operator=(const D3D12_GPU_VIRTUAL_ADDRESSs_Argument&) =
      delete;
  ~D3D12_GPU_VIRTUAL_ADDRESSs_Argument();
  D3D12_GPU_VIRTUAL_ADDRESS* Value{};
  size_t Size{};
  std::vector<unsigned> InterfaceKeys{};
  std::vector<unsigned> Offsets{};
  bool Copy{};
};

struct ShaderIdentifierArgument {
  ShaderIdentifierArgument(const void* value_) : Value(const_cast<void*>(value_)) {}
  ShaderIdentifierArgument() {}
  ShaderIdentifierArgument(const ShaderIdentifierArgument& arg);
  ShaderIdentifierArgument& operator=(ShaderIdentifierArgument const&) = delete;
  ~ShaderIdentifierArgument() {}
  void* Value{};
  std::vector<uint8_t> Data;
};

template <>
struct PointerArgument<D3D12_ROOT_SIGNATURE_DESC> {
  PointerArgument(const D3D12_ROOT_SIGNATURE_DESC* value_)
      : Value(const_cast<D3D12_ROOT_SIGNATURE_DESC*>(value_)) {}
  PointerArgument() {}
  PointerArgument(const PointerArgument<D3D12_ROOT_SIGNATURE_DESC>& arg);
  PointerArgument& operator=(const PointerArgument<D3D12_ROOT_SIGNATURE_DESC>&) = delete;
  ~PointerArgument();
  D3D12_ROOT_SIGNATURE_DESC* Value{};
  bool Copy{};
};

template <>
struct PointerArgument<D3D12_VERSIONED_ROOT_SIGNATURE_DESC> {
  PointerArgument(const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* value_)
      : Value(const_cast<D3D12_VERSIONED_ROOT_SIGNATURE_DESC*>(value_)) {}
  PointerArgument() {}
  PointerArgument(const PointerArgument<D3D12_VERSIONED_ROOT_SIGNATURE_DESC>& arg);
  PointerArgument& operator=(const PointerArgument<D3D12_VERSIONED_ROOT_SIGNATURE_DESC>&) = delete;
  ~PointerArgument();
  D3D12_VERSIONED_ROOT_SIGNATURE_DESC* Value{};
  bool Copy{};
};

struct D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument {
  D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument(const D3D12_GRAPHICS_PIPELINE_STATE_DESC* value_)
      : Value(const_cast<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>(value_)) {}
  D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument() {}
  D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument(
      const D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument& arg);
  D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument& operator=(
      const D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument&) = delete;
  ~D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument();
  D3D12_GRAPHICS_PIPELINE_STATE_DESC* Value{};
  unsigned RootSignatureKey{};
  bool Copy{};
};

struct D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument {
  D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument(const D3D12_COMPUTE_PIPELINE_STATE_DESC* value_)
      : Value(const_cast<D3D12_COMPUTE_PIPELINE_STATE_DESC*>(value_)) {}
  D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument() {}
  D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument(const D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument& arg);
  D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument& operator=(
      const D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument&) = delete;
  ~D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument();
  D3D12_COMPUTE_PIPELINE_STATE_DESC* Value{};
  unsigned RootSignatureKey{};
  bool Copy{};
};

struct D3D12_TEXTURE_COPY_LOCATION_Argument {
  D3D12_TEXTURE_COPY_LOCATION_Argument(const D3D12_TEXTURE_COPY_LOCATION* value_)
      : Value(const_cast<D3D12_TEXTURE_COPY_LOCATION*>(value_)) {}
  D3D12_TEXTURE_COPY_LOCATION_Argument() {}
  D3D12_TEXTURE_COPY_LOCATION_Argument(const D3D12_TEXTURE_COPY_LOCATION_Argument& arg);
  D3D12_TEXTURE_COPY_LOCATION_Argument& operator=(const D3D12_TEXTURE_COPY_LOCATION_Argument&) =
      delete;
  ~D3D12_TEXTURE_COPY_LOCATION_Argument();
  D3D12_TEXTURE_COPY_LOCATION* Value{};
  unsigned ResourceKey{};
  bool Copy{};
};

struct D3D12_RESOURCE_BARRIERs_Argument {
  D3D12_RESOURCE_BARRIERs_Argument(const D3D12_RESOURCE_BARRIER* value_, unsigned size_)
      : Value(const_cast<D3D12_RESOURCE_BARRIER*>(value_)), Size(size_) {
    ResourceKeys.resize(size_);
    ResourceAfterKeys.resize(size_);
  }
  D3D12_RESOURCE_BARRIERs_Argument() {}
  D3D12_RESOURCE_BARRIERs_Argument(const D3D12_RESOURCE_BARRIERs_Argument& arg);
  D3D12_RESOURCE_BARRIERs_Argument& operator=(const D3D12_RESOURCE_BARRIERs_Argument&) = delete;
  ~D3D12_RESOURCE_BARRIERs_Argument();
  D3D12_RESOURCE_BARRIER* Value{};
  size_t Size{};
  std::vector<unsigned> ResourceKeys{};
  std::vector<unsigned> ResourceAfterKeys{};
  bool Copy{};
};

struct D3D12_SHADER_RESOURCE_VIEW_DESC_Argument {
  D3D12_SHADER_RESOURCE_VIEW_DESC_Argument(const D3D12_SHADER_RESOURCE_VIEW_DESC* value_)
      : Value(const_cast<D3D12_SHADER_RESOURCE_VIEW_DESC*>(value_)) {}
  D3D12_SHADER_RESOURCE_VIEW_DESC_Argument() {}
  D3D12_SHADER_RESOURCE_VIEW_DESC_Argument(const D3D12_SHADER_RESOURCE_VIEW_DESC_Argument& arg);
  D3D12_SHADER_RESOURCE_VIEW_DESC_Argument& operator=(
      const D3D12_SHADER_RESOURCE_VIEW_DESC_Argument&) = delete;
  ~D3D12_SHADER_RESOURCE_VIEW_DESC_Argument();
  D3D12_SHADER_RESOURCE_VIEW_DESC* Value{};
  unsigned RaytracingLocationKey{};
  unsigned RaytracingLocationOffset{};
  bool Copy{};
};

struct D3D12_INDEX_BUFFER_VIEW_Argument {
  D3D12_INDEX_BUFFER_VIEW_Argument(const D3D12_INDEX_BUFFER_VIEW* value_)
      : Value(const_cast<D3D12_INDEX_BUFFER_VIEW*>(value_)) {}
  D3D12_INDEX_BUFFER_VIEW_Argument() {}
  D3D12_INDEX_BUFFER_VIEW_Argument(const D3D12_INDEX_BUFFER_VIEW_Argument& arg);
  D3D12_INDEX_BUFFER_VIEW_Argument& operator=(const D3D12_INDEX_BUFFER_VIEW_Argument&) = delete;
  ~D3D12_INDEX_BUFFER_VIEW_Argument();
  D3D12_INDEX_BUFFER_VIEW* Value{};
  unsigned BufferLocationKey{};
  unsigned BufferLocationOffset{};
  bool Copy{};
};

struct D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument {
  D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument(const D3D12_CONSTANT_BUFFER_VIEW_DESC* value_)
      : Value(const_cast<D3D12_CONSTANT_BUFFER_VIEW_DESC*>(value_)) {}
  D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument() {}
  D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument(const D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument& arg);
  D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument& operator=(
      const D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument&) = delete;
  ~D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument();
  D3D12_CONSTANT_BUFFER_VIEW_DESC* Value{};
  unsigned BufferLocationKey{};
  unsigned BufferLocationOffset{};
  bool Copy{};
};

struct D3D12_VERTEX_BUFFER_VIEWs_Argument {
  D3D12_VERTEX_BUFFER_VIEWs_Argument(const D3D12_VERTEX_BUFFER_VIEW* value_, unsigned size_)
      : Value(const_cast<D3D12_VERTEX_BUFFER_VIEW*>(value_)), Size(size_) {
    BufferLocationKeys.resize(size_);
    BufferLocationOffsets.resize(size_);
  }
  D3D12_VERTEX_BUFFER_VIEWs_Argument() {}
  D3D12_VERTEX_BUFFER_VIEWs_Argument(const D3D12_VERTEX_BUFFER_VIEWs_Argument& arg);
  D3D12_VERTEX_BUFFER_VIEWs_Argument& operator=(const D3D12_VERTEX_BUFFER_VIEWs_Argument&) = delete;
  ~D3D12_VERTEX_BUFFER_VIEWs_Argument();
  D3D12_VERTEX_BUFFER_VIEW* Value{};
  size_t Size{};
  std::vector<unsigned> BufferLocationKeys{};
  std::vector<unsigned> BufferLocationOffsets{};
  bool Copy{};
};

struct D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument {
  D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument(const D3D12_STREAM_OUTPUT_BUFFER_VIEW* value_,
                                            unsigned size_)
      : Value(const_cast<D3D12_STREAM_OUTPUT_BUFFER_VIEW*>(value_)), Size(size_) {
    BufferLocationKeys.resize(size_);
    BufferLocationOffsets.resize(size_);
    BufferFilledSizeLocationKeys.resize(size_);
    BufferFilledSizeLocationOffsets.resize(size_);
  }
  D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument() {}
  D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument(const D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument& arg);
  D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument& operator=(
      const D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument&) = delete;
  ~D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument();
  D3D12_STREAM_OUTPUT_BUFFER_VIEW* Value{};
  size_t Size{};
  std::vector<unsigned> BufferLocationKeys{};
  std::vector<unsigned> BufferLocationOffsets{};
  std::vector<unsigned> BufferFilledSizeLocationKeys{};
  std::vector<unsigned> BufferFilledSizeLocationOffsets{};
  bool Copy{};
};

struct D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument {
  D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument(const D3D12_WRITEBUFFERIMMEDIATE_PARAMETER* value_,
                                                 unsigned size_)
      : Value(const_cast<D3D12_WRITEBUFFERIMMEDIATE_PARAMETER*>(value_)), Size(size_) {
    DestKeys.resize(size_);
    DestOffsets.resize(size_);
  }
  D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument() {}
  D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument(
      const D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument& arg);
  D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument& operator=(
      const D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument&) = delete;
  ~D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument();
  D3D12_WRITEBUFFERIMMEDIATE_PARAMETER* Value{};
  size_t Size{};
  std::vector<unsigned> DestKeys{};
  std::vector<unsigned> DestOffsets{};
  bool Copy{};
};

struct D3D12_PIPELINE_STATE_STREAM_DESC_Argument {
  D3D12_PIPELINE_STATE_STREAM_DESC_Argument(const D3D12_PIPELINE_STATE_STREAM_DESC* value_)
      : Value(const_cast<D3D12_PIPELINE_STATE_STREAM_DESC*>(value_)) {}
  D3D12_PIPELINE_STATE_STREAM_DESC_Argument() {}
  D3D12_PIPELINE_STATE_STREAM_DESC_Argument(const D3D12_PIPELINE_STATE_STREAM_DESC_Argument& arg);
  D3D12_PIPELINE_STATE_STREAM_DESC_Argument& operator=(
      const D3D12_PIPELINE_STATE_STREAM_DESC_Argument&) = delete;
  ~D3D12_PIPELINE_STATE_STREAM_DESC_Argument();
  D3D12_PIPELINE_STATE_STREAM_DESC* Value{};
  unsigned RootSignatureKey{};
  bool Copy{};
};

struct D3D12_STATE_OBJECT_DESC_Argument {
  D3D12_STATE_OBJECT_DESC_Argument(const D3D12_STATE_OBJECT_DESC* value_)
      : Value(const_cast<D3D12_STATE_OBJECT_DESC*>(value_)) {}
  D3D12_STATE_OBJECT_DESC_Argument() {}
  D3D12_STATE_OBJECT_DESC_Argument(const D3D12_STATE_OBJECT_DESC_Argument& arg);
  D3D12_STATE_OBJECT_DESC_Argument& operator=(const D3D12_STATE_OBJECT_DESC_Argument&) = delete;
  ~D3D12_STATE_OBJECT_DESC_Argument();
  D3D12_STATE_OBJECT_DESC* Value{};
  std::map<unsigned, unsigned> InterfaceKeysBySubobject;
  bool Copy{};
};

struct D3D12_EXTENSION_ARGUMENTS_Argument {
  D3D12_EXTENSION_ARGUMENTS_Argument(const D3D12_EXTENSION_ARGUMENTS* value_)
      : Value(const_cast<D3D12_EXTENSION_ARGUMENTS*>(value_)) {}
  D3D12_EXTENSION_ARGUMENTS_Argument() {}
  D3D12_EXTENSION_ARGUMENTS_Argument(const D3D12_EXTENSION_ARGUMENTS_Argument& arg);
  D3D12_EXTENSION_ARGUMENTS_Argument& operator=(const D3D12_EXTENSION_ARGUMENTS_Argument&) = delete;
  ~D3D12_EXTENSION_ARGUMENTS_Argument();
  D3D12_EXTENSION_ARGUMENTS* Value{};
  std::vector<unsigned> ObjectKeys{};
  bool Copy{};
};

struct D3D12_EXTENDED_OPERATION_DATA_Argument {
  D3D12_EXTENDED_OPERATION_DATA_Argument(const D3D12_EXTENDED_OPERATION_DATA* value_)
      : Value(const_cast<D3D12_EXTENDED_OPERATION_DATA*>(value_)) {}
  D3D12_EXTENDED_OPERATION_DATA_Argument() {}
  D3D12_EXTENDED_OPERATION_DATA_Argument(const D3D12_EXTENDED_OPERATION_DATA_Argument& arg);
  D3D12_EXTENDED_OPERATION_DATA_Argument& operator=(const D3D12_EXTENDED_OPERATION_DATA_Argument&) =
      delete;
  ~D3D12_EXTENDED_OPERATION_DATA_Argument();
  D3D12_EXTENDED_OPERATION_DATA* Value{};
  std::vector<unsigned> ObjectKeys{};
  bool Copy{};
};

template <>
struct PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS> {
  PointerArgument(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS* value_)
      : Value(const_cast<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS*>(value_)) {}
  PointerArgument() {}
  PointerArgument(const PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS>& arg);
  PointerArgument& operator=(
      const PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS>&) = delete;
  ~PointerArgument();
  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS* Value{};
  std::vector<unsigned> InputKeys{};
  std::vector<unsigned> InputOffsets{};
  bool Copy{};
};

template <>
struct PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC> {
  PointerArgument(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* value_)
      : Value(const_cast<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC*>(value_)) {}
  PointerArgument() {}
  PointerArgument(const PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC>& arg);
  PointerArgument& operator=(
      const PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC>&) = delete;
  ~PointerArgument();
  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* Value{};
  unsigned DestAccelerationStructureKey{};
  unsigned DestAccelerationStructureOffset{};
  unsigned SourceAccelerationStructureKey{};
  unsigned SourceAccelerationStructureOffset{};
  unsigned ScratchAccelerationStructureKey{};
  unsigned ScratchAccelerationStructureOffset{};
  std::vector<unsigned> InputKeys{};
  std::vector<unsigned> InputOffsets{};
  bool Copy{};
};

template <>
struct ArrayArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC> {
  ArrayArgument(const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC* value_,
                size_t size_)
      : Value(const_cast<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC*>(value_)),
        Size(size_) {}
  ArrayArgument() {}
  ArrayArgument(
      const ArrayArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg);
  ArrayArgument& operator=(
      const ArrayArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>&) = delete;
  ~ArrayArgument();
  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC* Value{};
  size_t Size{};
  std::vector<unsigned> DestBufferKeys{};
  std::vector<unsigned> DestBufferOffsets{};
  bool Copy{};
};

template <>
struct PointerArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC> {
  PointerArgument(const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC* value_)
      : Value(const_cast<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC*>(value_)) {}
  PointerArgument() {}
  PointerArgument(
      const PointerArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg);
  PointerArgument& operator=(
      const PointerArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>&) = delete;
  ~PointerArgument();
  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC* Value{};
  unsigned destBufferKey{};
  unsigned destBufferOffset{};
  bool Copy{};
};

template <>
struct PointerArgument<D3D12_DISPATCH_RAYS_DESC> {
  PointerArgument(const D3D12_DISPATCH_RAYS_DESC* value_)
      : Value(const_cast<D3D12_DISPATCH_RAYS_DESC*>(value_)) {}
  PointerArgument() {}
  PointerArgument(const PointerArgument<D3D12_DISPATCH_RAYS_DESC>& arg);
  PointerArgument& operator=(const PointerArgument<D3D12_DISPATCH_RAYS_DESC>&) = delete;
  ~PointerArgument();
  D3D12_DISPATCH_RAYS_DESC* Value{};
  unsigned RayGenerationShaderRecordKey{};
  unsigned RayGenerationShaderRecordOffset{};
  unsigned MissShaderTableKey{};
  unsigned MissShaderTableOffset{};
  unsigned HitGroupTableKey{};
  unsigned HitGroupTableOffset{};
  unsigned CallableShaderTableKey{};
  unsigned CallableShaderTableOffset{};
  bool Copy{};
};

struct D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument {
  D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument(const D3D12_RENDER_PASS_RENDER_TARGET_DESC* value_,
                                                 size_t size_)
      : Value(const_cast<D3D12_RENDER_PASS_RENDER_TARGET_DESC*>(value_)), Size(size_) {}
  D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument() {}
  D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument(
      const D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument& arg);
  D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument& operator=(
      const D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument&) = delete;
  ~D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument();
  D3D12_RENDER_PASS_RENDER_TARGET_DESC* Value{};
  size_t Size{};
  std::vector<unsigned> DescriptorKeys{};
  std::vector<unsigned> DescriptorIndexes{};
  std::vector<unsigned> ResolveSrcResourceKeys{};
  std::vector<unsigned> ResolveDstResourceKeys{};
  bool Copy{};
};

struct D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument {
  D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument(const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC* value_)
      : Value(const_cast<D3D12_RENDER_PASS_DEPTH_STENCIL_DESC*>(value_)) {}
  D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument() {}
  D3D12_RENDER_PASS_DEPTH_STENCIL_DESC* Value{};
  D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument(
      const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument& arg);
  D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument& operator=(
      const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument&) = delete;
  ~D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument();
  unsigned DescriptorKey{};
  unsigned DescriptorIndex{};
  unsigned ResolveSrcDepthKey{};
  unsigned ResolveDstDepthKey{};
  unsigned ResolveSrcStencilKey{};
  unsigned ResolveDstStencilKey{};
  bool Copy{};
};

template <>
struct PointerArgument<D3D12_COMMAND_SIGNATURE_DESC> {
  PointerArgument(const D3D12_COMMAND_SIGNATURE_DESC* value_)
      : Value(const_cast<D3D12_COMMAND_SIGNATURE_DESC*>(value_)) {}
  PointerArgument() {}
  PointerArgument(const PointerArgument<D3D12_COMMAND_SIGNATURE_DESC>& arg);
  PointerArgument& operator=(const PointerArgument<D3D12_COMMAND_SIGNATURE_DESC>&) = delete;
  ~PointerArgument();
  D3D12_COMMAND_SIGNATURE_DESC* Value{};
  bool Copy{};
};

template <>
struct ArrayArgument<D3D12_META_COMMAND_DESC> {
  ArrayArgument(const D3D12_META_COMMAND_DESC* value_, size_t size_)
      : Value(const_cast<D3D12_META_COMMAND_DESC*>(value_)), Size(size_) {}
  ArrayArgument() {}
  ArrayArgument(const ArrayArgument<D3D12_META_COMMAND_DESC>& arg);
  ArrayArgument& operator=(const ArrayArgument<D3D12_META_COMMAND_DESC>&) = delete;
  ~ArrayArgument();
  D3D12_META_COMMAND_DESC* Value{};
  size_t Size{};
  bool Copy{};
};

struct D3D12_BARRIER_GROUPs_Argument {
  D3D12_BARRIER_GROUPs_Argument(const D3D12_BARRIER_GROUP* value_, unsigned size_)
      : Value(const_cast<D3D12_BARRIER_GROUP*>(value_)), Size(size_) {
    unsigned numResourceKeys{};
    for (unsigned i = 0; i < Size; ++i) {
      if (Value[i].Type == D3D12_BARRIER_TYPE_GLOBAL) {
        continue;
      }
      numResourceKeys += Value[i].NumBarriers;
    }
    ResourceKeys.resize(numResourceKeys);
  }
  D3D12_BARRIER_GROUPs_Argument() {}
  D3D12_BARRIER_GROUPs_Argument(const D3D12_BARRIER_GROUPs_Argument& arg);
  D3D12_BARRIER_GROUPs_Argument& operator=(const D3D12_BARRIER_GROUPs_Argument&) = delete;
  ~D3D12_BARRIER_GROUPs_Argument();
  D3D12_BARRIER_GROUP* Value{};
  size_t Size{};
  std::vector<unsigned> ResourceKeys{};
  bool Copy{};
};

template <>
struct ArrayArgument<D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_INFO> {
  ArrayArgument(const D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_INFO* value_, size_t size_)
      : Value(const_cast<D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_INFO*>(value_)), Size(size_) {}
  ArrayArgument() {}
  ArrayArgument(const ArrayArgument<D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_INFO>& arg);
  ArrayArgument& operator=(const ArrayArgument<D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_INFO>&) =
      delete;
  ~ArrayArgument();
  D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_INFO* Value{};
  size_t Size{};
  std::vector<unsigned> DestKey{};
  std::vector<unsigned> DestOffset{};
  std::vector<unsigned> SourceKey{};
  std::vector<unsigned> SourceOffset{};
  bool Copy{};
};

#pragma endregion

#pragma region INTC

template <typename T>
struct ContextArgument {
  ContextArgument(T value_) : Value(value_) {}
  ContextArgument(void* value_) : Value(reinterpret_cast<T>(value_)) {}
  ContextArgument() {}
  T Value{};
  unsigned Key{};
};

template <typename T>
struct ContextOutputArgument {
  ContextOutputArgument(T* value_) : Value(value_) {}
  ContextOutputArgument() {}
  ContextOutputArgument(const ContextOutputArgument& arg) {
    Key = arg.Key;
    Data = *arg.Value;
    Value = &Data;
  }
  ContextOutputArgument& operator=(const ContextOutputArgument&) = delete;
  ~ContextOutputArgument() {}
  T* Value{};
  T Data{};
  unsigned Key{};
};

using INTCExtensionContextArgument = ContextArgument<INTCExtensionContext*>;
using INTCExtensionContextOutputArgument = ContextOutputArgument<INTCExtensionContext*>;
using XESSContextArgument = ContextArgument<xess_context_handle_t>;
using XESSContextOutputArgument = ContextOutputArgument<xess_context_handle_t>;
using XELLContextArgument = ContextArgument<xell_context_handle_t>;
using XELLContextOutputArgument = ContextOutputArgument<xell_context_handle_t>;
using XEFGContextArgument = ContextArgument<xefg_swapchain_handle_t>;
using XEFGContextOutputArgument = ContextOutputArgument<xefg_swapchain_handle_t>;

template <>
struct PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC> {
  PointerArgument(const INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC* value_)
      : Value(const_cast<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC*>(value_)) {}
  PointerArgument() {}
  PointerArgument(const PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>& arg);
  PointerArgument& operator=(const PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>&) =
      delete;
  ~PointerArgument();
  INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC* Value{};
  const void* Cs{};
  const void* CompileOptions{};
  const void* InternalOptions{};
  unsigned RootSignatureKey{};
  bool Copy{};
};

template <>
struct PointerArgument<INTCExtensionAppInfo> {
  PointerArgument(const INTCExtensionAppInfo* value_)
      : Value(const_cast<INTCExtensionAppInfo*>(value_)) {}
  PointerArgument() {}
  PointerArgument(const PointerArgument<INTCExtensionAppInfo>& arg);
  PointerArgument& operator=(const PointerArgument<INTCExtensionAppInfo>&) = delete;
  ~PointerArgument();
  INTCExtensionAppInfo* Value{};
  const wchar_t* ApplicationName{};
  const wchar_t* EngineName{};
  bool Copy{};
};

template <>
struct PointerArgument<INTCExtensionAppInfo1> {
  PointerArgument(const INTCExtensionAppInfo1* value_)
      : Value(const_cast<INTCExtensionAppInfo1*>(value_)) {}
  PointerArgument() {}
  PointerArgument(const PointerArgument<INTCExtensionAppInfo1>& arg);
  PointerArgument& operator=(const PointerArgument<INTCExtensionAppInfo1>&) = delete;
  ~PointerArgument();
  INTCExtensionAppInfo1* Value{};
  const wchar_t* ApplicationName{};
  const wchar_t* EngineName{};
  bool Copy{};
};

template <>
struct PointerArgument<INTC_D3D12_HEAP_DESC> {
  PointerArgument(const INTC_D3D12_HEAP_DESC* value_)
      : Value(const_cast<INTC_D3D12_HEAP_DESC*>(value_)) {}
  PointerArgument() {}
  PointerArgument(const PointerArgument<INTC_D3D12_HEAP_DESC>& arg);
  PointerArgument& operator=(const PointerArgument<INTC_D3D12_HEAP_DESC>&) = delete;
  ~PointerArgument();
  INTC_D3D12_HEAP_DESC* Value{};
  bool Copy{};
};

template <>
struct PointerArgument<INTC_D3D12_RESOURCE_DESC_0001> {
  PointerArgument(const INTC_D3D12_RESOURCE_DESC_0001* value_)
      : Value(const_cast<INTC_D3D12_RESOURCE_DESC_0001*>(value_)) {}
  PointerArgument() {}
  PointerArgument(const PointerArgument<INTC_D3D12_RESOURCE_DESC_0001>& arg);
  PointerArgument& operator=(const PointerArgument<INTC_D3D12_RESOURCE_DESC_0001>&) = delete;
  ~PointerArgument();
  INTC_D3D12_RESOURCE_DESC_0001* Value{};
  bool Copy{};
};

template <>
struct PointerArgument<INTC_D3D12_FEATURE> {
  PointerArgument(const INTC_D3D12_FEATURE* value_)
      : Value(const_cast<INTC_D3D12_FEATURE*>(value_)) {}
  PointerArgument() {}
  PointerArgument(const PointerArgument<INTC_D3D12_FEATURE>& arg);
  PointerArgument& operator=(const PointerArgument<INTC_D3D12_FEATURE>&) = delete;
  ~PointerArgument();
  INTC_D3D12_FEATURE* Value{};
  bool Copy{};
};

template <>
struct PointerArgument<INTC_D3D12_RESOURCE_DESC> {
  PointerArgument(const INTC_D3D12_RESOURCE_DESC* value_)
      : Value(const_cast<INTC_D3D12_RESOURCE_DESC*>(value_)) {}
  PointerArgument() {}
  PointerArgument(const PointerArgument<INTC_D3D12_RESOURCE_DESC>& arg);
  PointerArgument& operator=(const PointerArgument<INTC_D3D12_RESOURCE_DESC>&) = delete;
  ~PointerArgument();
  INTC_D3D12_RESOURCE_DESC* Value{};
  bool Copy{};
};

template <>
struct PointerArgument<INTC_D3D12_COMMAND_QUEUE_DESC_0001> {
  PointerArgument(const INTC_D3D12_COMMAND_QUEUE_DESC_0001* value_)
      : Value(const_cast<INTC_D3D12_COMMAND_QUEUE_DESC_0001*>(value_)) {}
  PointerArgument() {}
  PointerArgument(const PointerArgument<INTC_D3D12_COMMAND_QUEUE_DESC_0001>& arg);
  PointerArgument& operator=(const PointerArgument<INTC_D3D12_COMMAND_QUEUE_DESC_0001>&) = delete;
  ~PointerArgument();
  INTC_D3D12_COMMAND_QUEUE_DESC_0001* Value{};
  bool Copy{};
};

template <>
struct PointerArgument<INTCExtensionInfo> {
  PointerArgument(const INTCExtensionInfo* value_)
      : Value(const_cast<INTCExtensionInfo*>(value_)) {}
  PointerArgument() {}
  PointerArgument(const PointerArgument<INTCExtensionInfo>& arg);
  PointerArgument& operator=(const PointerArgument<INTCExtensionInfo>&) = delete;
  ~PointerArgument();
  INTCExtensionInfo* Value{};
  bool Copy{};
  bool CopyDeviceDriverDesc{};
  bool CopyDeviceDriverVersion{};
};

template <>
struct ArrayArgument<INTCExtensionVersion> {
  ArrayArgument(const INTCExtensionVersion* value_, size_t size_)
      : Value(const_cast<INTCExtensionVersion*>(value_)), Size(size_) {}
  ArrayArgument() {}
  ArrayArgument(const ArrayArgument<INTCExtensionVersion>& arg);
  ArrayArgument& operator=(const ArrayArgument<INTCExtensionVersion>&) = delete;
  ~ArrayArgument();
  INTCExtensionVersion* Value{};
  size_t Size{};
  bool Copy{};
};

#pragma endregion

#pragma region DML

struct DML_BINDING_TABLE_DESC_Argument {
  DML_BINDING_TABLE_DESC_Argument(const DML_BINDING_TABLE_DESC* value_)
      : Value(const_cast<DML_BINDING_TABLE_DESC*>(value_)) {}
  DML_BINDING_TABLE_DESC_Argument() {}
  DML_BINDING_TABLE_DESC_Argument(const DML_BINDING_TABLE_DESC_Argument& arg);
  DML_BINDING_TABLE_DESC_Argument& operator=(const DML_BINDING_TABLE_DESC_Argument&) = delete;
  ~DML_BINDING_TABLE_DESC_Argument();
  DML_BINDING_TABLE_DESC* Value{};
  struct BindingTableFields {
    unsigned DispatchableKey{};
    unsigned CpuDescHandleKey{};
    unsigned CpuDescHandleIndex{};
    unsigned GpuDescHandleKey{};
    unsigned GpuDescHandleIndex{};
  } TableFields;
  bool Copy{};
};

struct DML_BINDING_DESC_Argument {
  DML_BINDING_DESC_Argument(const DML_BINDING_DESC* value_)
      : Value(const_cast<DML_BINDING_DESC*>(value_)) {}
  DML_BINDING_DESC_Argument() {}
  DML_BINDING_DESC_Argument(const DML_BINDING_DESC_Argument& arg);
  DML_BINDING_DESC_Argument& operator=(const DML_BINDING_DESC_Argument&) = delete;
  ~DML_BINDING_DESC_Argument();
  DML_BINDING_DESC* Value{};
  size_t ResourceKeysSize{};
  std::vector<unsigned> ResourceKeys{};
  bool Copy{};
};

struct DML_BINDING_DESCs_Argument {
  DML_BINDING_DESCs_Argument(const DML_BINDING_DESC* value_, unsigned size_)
      : Value(const_cast<DML_BINDING_DESC*>(value_)), Size(size_) {}
  DML_BINDING_DESCs_Argument() {}
  DML_BINDING_DESCs_Argument(const DML_BINDING_DESCs_Argument& arg);
  DML_BINDING_DESCs_Argument& operator=(const DML_BINDING_DESCs_Argument&) = delete;
  ~DML_BINDING_DESCs_Argument();
  DML_BINDING_DESC* Value{};
  size_t Size{};
  size_t ResourceKeysSize{};
  std::vector<unsigned> ResourceKeys{};
  bool Copy{};
};

struct DML_GRAPH_DESC_Argument {
  DML_GRAPH_DESC_Argument(const DML_GRAPH_DESC* value_)
      : Value(const_cast<DML_GRAPH_DESC*>(value_)) {}
  DML_GRAPH_DESC_Argument() {}
  DML_GRAPH_DESC_Argument(const DML_GRAPH_DESC_Argument& arg);
  DML_GRAPH_DESC_Argument& operator=(const DML_GRAPH_DESC_Argument&) = delete;
  ~DML_GRAPH_DESC_Argument();
  DML_GRAPH_DESC* Value{};
  size_t OperatorKeysSize{};
  std::vector<unsigned> OperatorKeys{};
  bool Copy{};
};

struct DML_OPERATOR_DESC_Argument {
  DML_OPERATOR_DESC_Argument(const DML_OPERATOR_DESC* value_)
      : Value(const_cast<DML_OPERATOR_DESC*>(value_)) {}
  DML_OPERATOR_DESC_Argument() {}
  DML_OPERATOR_DESC_Argument(const DML_OPERATOR_DESC_Argument& arg);
  DML_OPERATOR_DESC_Argument& operator=(const DML_OPERATOR_DESC_Argument&) = delete;
  ~DML_OPERATOR_DESC_Argument();
  DML_OPERATOR_DESC* Value{};
  bool Copy{};
};

struct DML_CheckFeatureSupport_BufferArgument {
  DML_CheckFeatureSupport_BufferArgument(const void* value_, size_t size_, DML_FEATURE feat)
      : Value{const_cast<void*>(value_)}, Size{size_}, feature{feat} {}
  DML_CheckFeatureSupport_BufferArgument() {}
  DML_CheckFeatureSupport_BufferArgument(const DML_CheckFeatureSupport_BufferArgument& arg);
  DML_CheckFeatureSupport_BufferArgument& operator=(const DML_CheckFeatureSupport_BufferArgument&) =
      delete;
  ~DML_CheckFeatureSupport_BufferArgument();
  void* Value{};
  size_t Size{};
  DML_FEATURE feature{};
  bool Copy{};
};

#pragma endregion

#pragma region DStorage

struct DSTORAGE_QUEUE_DESC_Argument {
  DSTORAGE_QUEUE_DESC_Argument(const DSTORAGE_QUEUE_DESC* value_)
      : Value(const_cast<DSTORAGE_QUEUE_DESC*>(value_)) {}
  DSTORAGE_QUEUE_DESC_Argument() {}
  DSTORAGE_QUEUE_DESC_Argument(const DSTORAGE_QUEUE_DESC_Argument& arg);
  DSTORAGE_QUEUE_DESC_Argument& operator=(const DSTORAGE_QUEUE_DESC_Argument&) = delete;
  ~DSTORAGE_QUEUE_DESC_Argument();
  DSTORAGE_QUEUE_DESC* Value{};
  size_t DeviceKey{};
  bool Copy{};
};

struct DSTORAGE_REQUEST_Argument {
  DSTORAGE_REQUEST_Argument(const DSTORAGE_REQUEST* value_)
      : Value(const_cast<DSTORAGE_REQUEST*>(value_)) {}
  DSTORAGE_REQUEST_Argument() {}
  DSTORAGE_REQUEST_Argument(const DSTORAGE_REQUEST_Argument& arg);
  DSTORAGE_REQUEST_Argument& operator=(const DSTORAGE_REQUEST_Argument&) = delete;
  ~DSTORAGE_REQUEST_Argument();
  DSTORAGE_REQUEST* Value{};
  unsigned FileKey{};
  unsigned ResourceKey{};
  UINT64 NewOffset{};
  bool Copy{};
};

#pragma endregion

#pragma region XESS

struct xess_d3d12_init_params_t_Argument {
  xess_d3d12_init_params_t_Argument(const xess_d3d12_init_params_t* value_)
      : Value(const_cast<xess_d3d12_init_params_t*>(value_)) {}
  xess_d3d12_init_params_t_Argument() {}
  xess_d3d12_init_params_t_Argument(const xess_d3d12_init_params_t_Argument& arg);
  xess_d3d12_init_params_t_Argument& operator=(const xess_d3d12_init_params_t_Argument&) = delete;
  ~xess_d3d12_init_params_t_Argument();
  xess_d3d12_init_params_t* Value{};
  unsigned Key{}; // Used for subcapture restore order
  unsigned TempBufferHeapKey{};
  unsigned TempTextureHeapKey{};
  unsigned PipelineLibraryKey{};
  bool Copy{};
};

struct xess_d3d12_execute_params_t_Argument {
  xess_d3d12_execute_params_t_Argument(const xess_d3d12_execute_params_t* value_)
      : Value(const_cast<xess_d3d12_execute_params_t*>(value_)) {}
  xess_d3d12_execute_params_t_Argument() {}
  xess_d3d12_execute_params_t_Argument(const xess_d3d12_execute_params_t_Argument& arg);
  xess_d3d12_execute_params_t_Argument& operator=(const xess_d3d12_execute_params_t_Argument&) =
      delete;
  ~xess_d3d12_execute_params_t_Argument();
  xess_d3d12_execute_params_t* Value{};
  unsigned ColorTextureKey{};
  unsigned VelocityTextureKey{};
  unsigned DepthTextureKey{};
  unsigned ExposureScaleTextureKey{};
  unsigned ResponsivePixelMaskTextureKey{};
  unsigned OutputTextureKey{};
  unsigned DescriptorHeapKey{};
  bool Copy{};
};

#pragma endregion

#pragma region NVAPI

template <>
struct PointerArgument<NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS> {
  PointerArgument(const NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS* value_)
      : Value(const_cast<NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS*>(value_)) {}
  PointerArgument() {}
  PointerArgument(
      const PointerArgument<NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS>& arg);
  PointerArgument& operator=(
      const PointerArgument<NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS>&) = delete;
  ~PointerArgument();
  NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS* Value{};
  unsigned DestAccelerationStructureKey{};
  unsigned DestAccelerationStructureOffset{};
  unsigned SourceAccelerationStructureKey{};
  unsigned SourceAccelerationStructureOffset{};
  unsigned ScratchAccelerationStructureKey{};
  unsigned ScratchAccelerationStructureOffset{};
  std::vector<unsigned> InputKeys{};
  std::vector<unsigned> InputOffsets{};
  std::vector<unsigned> DestPostBuildBufferKeys{};
  std::vector<unsigned> DestPostBuildBufferOffsets{};
  bool Copy{};
};

template <>
struct PointerArgument<NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS> {
  PointerArgument(const NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS* value_)
      : Value(const_cast<NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS*>(value_)) {}
  PointerArgument() {}
  PointerArgument(const PointerArgument<NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS>& arg);
  PointerArgument& operator=(
      const PointerArgument<NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS>&) = delete;
  ~PointerArgument();
  NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS* Value{};
  unsigned DestOpacityMicromapArrayDataKey{};
  unsigned DestOpacityMicromapArrayDataOffset{};
  unsigned InputBufferKey{};
  unsigned InputBufferOffset{};
  unsigned PerOMMDescsKey{};
  unsigned PerOMMDescsOffset{};
  unsigned ScratchOpacityMicromapArrayDataKey{};
  unsigned ScratchOpacityMicromapArrayDataOffset{};
  std::vector<unsigned> DestPostBuildBufferKeys{};
  std::vector<unsigned> DestPostBuildBufferOffsets{};
  bool Copy{};
};

template <>
struct PointerArgument<NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS> {
  PointerArgument(const NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS* value_)
      : Value(
            const_cast<NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS*>(value_)) {
  }
  PointerArgument() {}
  PointerArgument(
      const PointerArgument<NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS>& arg);
  PointerArgument& operator=(
      const PointerArgument<NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS>&) =
      delete;
  ~PointerArgument();
  NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS* Value{};
  unsigned BatchResultDataKey{};
  unsigned BatchResultDataOffset{};
  unsigned BatchScratchDataKey{};
  unsigned BatchScratchDataOffset{};
  unsigned DestinationAddressArrayKey{};
  unsigned DestinationAddressArrayOffset{};
  unsigned ResultSizeArrayKey{};
  unsigned ResultSizeArrayOffset{};
  unsigned IndirectArgArrayKey{};
  unsigned IndirectArgArrayOffset{};
  unsigned IndirectArgCountKey{};
  unsigned IndirectArgCountOffset{};
  bool Copy{};
};

#pragma endregion

#pragma region XELL

struct xell_frame_report_t_Argument {
  xell_frame_report_t_Argument(xell_frame_report_t* value_) : Value(value_) {}
  xell_frame_report_t_Argument() {}
  xell_frame_report_t_Argument(const xell_frame_report_t_Argument& arg);
  xell_frame_report_t_Argument& operator=(const xell_frame_report_t_Argument&) = delete;
  ~xell_frame_report_t_Argument();
  xell_frame_report_t* Value{};
  const size_t FRAME_REPORTS_COUNT = 64;
  bool Copy{};
};

#pragma endregion

#pragma region XEFG

struct xefg_swapchain_d3d12_init_params_t_Argument {
  xefg_swapchain_d3d12_init_params_t_Argument(const xefg_swapchain_d3d12_init_params_t* value_)
      : Value(const_cast<xefg_swapchain_d3d12_init_params_t*>(value_)) {}
  xefg_swapchain_d3d12_init_params_t_Argument() {}
  xefg_swapchain_d3d12_init_params_t_Argument(
      const xefg_swapchain_d3d12_init_params_t_Argument& arg);
  xefg_swapchain_d3d12_init_params_t_Argument& operator=(
      const xefg_swapchain_d3d12_init_params_t_Argument&) = delete;
  ~xefg_swapchain_d3d12_init_params_t_Argument();
  xefg_swapchain_d3d12_init_params_t* Value{};
  unsigned Key{};
  unsigned ApplicationSwapChainKey{};
  unsigned TempBufferHeapKey{};
  unsigned TempTextureHeapKey{};
  unsigned PipelineLibraryKey{};
  bool Copy{};
};

struct xefg_swapchain_d3d12_resource_data_t_Argument {
  xefg_swapchain_d3d12_resource_data_t_Argument(const xefg_swapchain_d3d12_resource_data_t* value_)
      : Value(const_cast<xefg_swapchain_d3d12_resource_data_t*>(value_)) {}
  xefg_swapchain_d3d12_resource_data_t_Argument() {}
  xefg_swapchain_d3d12_resource_data_t_Argument(
      const xefg_swapchain_d3d12_resource_data_t_Argument& arg);
  xefg_swapchain_d3d12_resource_data_t_Argument& operator=(
      const xefg_swapchain_d3d12_resource_data_t_Argument&) = delete;
  ~xefg_swapchain_d3d12_resource_data_t_Argument();
  xefg_swapchain_d3d12_resource_data_t* Value{};
  unsigned ResourceKey{};
  bool Copy{};
};

#pragma endregion

} // namespace DirectX
} // namespace gits

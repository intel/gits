// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "openclHeader.h"

#include "log.h"
#include "configurationLib.h"
#include "texture_converter.h"

#include <memory>
#include <string>
#include <array>
#include <unordered_map>

namespace gits {
struct CCLMemState;
class CFunction;
class CBinaryResource;
namespace OpenCL {

void MaskAppend(std::string& str, const std::string& maskStr);

typedef unsigned mem_signature_t;
enum class UnifiedMemoryType : unsigned {
  host = 1 << 0,
  device = 1 << 1,
  shared = 1 << 2
};
enum class KernelArgType {
  pointer = 1,
  mem,
  sampler,
  usm,
  svm
};
enum class KernelSetType {
  normal = 1,
  usm,
  svm
};
enum class KernelExecInfoType : unsigned {
  pointers = 1,
  boolean,
  uint
};
enum class ProgramBinaryLink : uint8_t {
  binary = 1,
  program
};

size_t PixelSize(const cl_image_format& imageFormat);
size_t QuerySize(const cl_mem clMem);
cl_image_desc GetImageDescFromQueryImageParams(const cl_mem clMem, cl_image_format& format);
void SaveImage(char* image,
               const cl_image_format& format,
               const cl_image_desc& desc,
               const std::string& name);
void SaveBuffer(const std::string& name, const std::vector<char>& data);
void SaveBuffer(const std::string& name, const CBinaryResource& data);
void D3DWarning();
size_t CountImageSize(const cl_image_format& imageFormat,
                      size_t imageWidth,
                      size_t imageHeight,
                      size_t rowPitch);
size_t CountImageSize(const cl_image_format& imageFormat,
                      size_t imageWidth,
                      size_t imageHeight,
                      size_t imageDepth,
                      size_t rowPitch,
                      size_t slicePitch);
size_t CountImageSize(const cl_image_format& imageFormat,
                      const size_t region[],
                      size_t rowPitch,
                      size_t slicePitch);
size_t CountImageSize(const cl_image_format& imageFormat, const cl_image_desc& imageDesc);
size_t CountBufferRectSize(const size_t region[], size_t rowPitch, size_t slicePitch);
texel_type GetTexelToConvertFromImageFormat(const cl_image_format& format);
std::array<size_t, 3> GetSimplifiedImageSizes(const cl_image_desc& desc);
void GetRegionForWholeImage(const cl_image_desc description, size_t* region);

bool ErrCodeSuccess(cl_int status);
bool ErrCodeSuccess(cl_int* errCodeRet);

bool FlagUseHostPtr(cl_mem_flags flags);

cl_uint GetRefCount(cl_command_queue obj);
cl_uint GetRefCount(cl_context obj);
cl_uint GetRefCount(cl_device_id obj);
cl_uint GetRefCount(cl_event obj);
cl_uint GetRefCount(cl_kernel obj);
cl_uint GetRefCount(cl_mem obj);
cl_uint GetRefCount(cl_program obj);
cl_uint GetRefCount(cl_sampler obj);
cl_uint GetRefCount(void* obj); // for CCLMappedPtr

template <typename T>
void RegisterEvents(cl_event* event, cl_command_queue cmdQueue, T errCodeRet);

template <typename T>
std::vector<T> PropertiesVectorWrapZeroEnded(const T* props);

template <typename T>
std::vector<T> RemoveProperties(const T* props, const std::vector<T> propsToRemove);
std::vector<cl_context_properties> RemoveGLSharingContextProperties(
    const cl_context_properties* props);
std::vector<cl_context_properties> RemoveDXSharingContextProperties(
    const cl_context_properties* props);

bool IsDeviceQuery(cl_uint param_name);
bool IsGLSharingQuery(const cl_uint& param_name);
bool IsDXSharingQuery(const cl_uint& param_name);
bool IsSharingQuery(const cl_uint& param_name);
bool IsGLSharingFunction(const std::string& functionName);
bool IsDXSharingFunction(const std::string& functionName);
bool IsUnsharingEnabled(const Config& cfg);
bool IsGLUnsharingEnabled(const Config& cfg);
bool IsDXUnsharingEnabled(const Config& cfg);
void clGetContextInfo_SetMapping(cl_device_id* old_devices,
                                 size_t num_old_devices,
                                 cl_device_id* devices,
                                 size_t paramValueSize,
                                 size_t* paramValueRetSize);
cl_mem_object_type TextureGLEnumToCLMemType(cl_GLenum textureEnum);
cl_platform_id ExtractPlatform(const cl_context_properties* props);
gits::CFunction* NewTokenPtrCreateCLMem(cl_context context,
                                        cl_mem mem,
                                        cl_mem_flags flags,
                                        cl_mem_object_type type);
gits::CFunction* NewTokenPtrGetDevices(cl_platform_id platform);
void CreateStateFromSharedBuffer(cl_mem return_value,
                                 cl_context context,
                                 cl_mem_flags flags,
                                 cl_int* errcode_ret);
void CreateStateFromSharedImage(cl_mem return_value,
                                cl_context context,
                                cl_mem_flags flags,
                                cl_int* errcode_ret);

template <typename T, typename State>
void ReleaseResourceState(std::unordered_map<T, State>& map, T resource) {
  // This is needed by FFMPEG used by PCMark10.
  // It always calls clReleaseKernel(nullptr).
  if (resource == nullptr) {
    return;
  }

  State& state = map[resource];
  state->Release();
  if (state->GetRefCount() == 0) {
    map.erase(resource);
  }
}

bool IsSharingEventFilteringNeeded(const cl_event& event);
std::vector<cl_event> FilterSharingEvents(const cl_uint num_events_in_wait_list,
                                          const cl_event* event_wait_list);

template <typename T>
T* GetPointerFromVector(std::vector<T>& v) {
  return (v.size() > 0 ? v.data() : nullptr);
}

void InjectKernelArgOperations(cl_kernel kernel,
                               const cl_command_queue command_queue,
                               cl_event* event);
std::vector<char> InjectObjOperations(cl_command_queue cmdQ,
                                      const void* argValue,
                                      cl_event* event_wait_list,
                                      std::string fileName,
                                      KernelArgType type,
                                      KernelSetType setType);

void AddSignature(std::vector<char>& buffer, mem_signature_t signature);
mem_signature_t GenerateSignature();
bool ResourceExists(cl_context resource);

std::string ToStringHelper(const void* handle);

namespace {
template <typename T>
std::string ToStringHelperArithmetic(
    const T handle, typename std::enable_if<std::is_arithmetic<T>::value>::type* = 0) {
  return std::to_string(handle);
}
template <typename T>
std::string ToStringHelperArithmetic(
    const T handle, typename std::enable_if<!std::is_arithmetic<T>::value>::type* = 0) {
  return ToStringHelper((const void*)handle);
}
} // namespace

template <typename T>
std::string ToStringHelper(const T handle) {
  return ToStringHelperArithmetic(handle);
}
template <typename T>
std::string ToStringHelper(const T* handle) {
  return ToStringHelper((const void*)handle);
}
template <>
std::string ToStringHelper(const cl_image_format handle);
template <>
std::string ToStringHelper(const cl_image_desc handle);
template <>
std::string ToStringHelper(const cl_buffer_region handle);
template <>
std::string ToStringHelper(const cl_resource_barrier_descriptor_intel handle);
void UpdateUsmPtrs(cl_kernel kernel);
bool HasUsmPtrsToUpdate(cl_kernel kernel);
void ResetAllUsmUpdateState(const cl_kernel& kernel);
void DetermineUsmToUpdate(const cl_kernel& kernel);
std::pair<void*, uintptr_t> GetSvmPtrFromRegion(void* svmPtr, bool fromKernelArg = false);
std::pair<void*, uintptr_t> GetUsmPtrFromRegion(void* usmPtr, bool fromKernelArg = false);
std::pair<void*, uintptr_t> GetSvmOrUsmFromRegion(void* ptr);
bool CheckIntelPlatform(const cl_platform_id& platform);
bool IsReadOnlyBuffer(const cl_mem_flags& memFlags, const cl_mem_properties_intel* properties);
bool IsReadOnlyBuffer(const cl_svm_mem_flags& memFlags);
bool CheckCfgZeroInitialization(const Config& cfg, const bool& isReadOnlyObj);
bool ZeroInitializeBuffer(const cl_command_queue& commandQueue,
                          const cl_mem& memObj,
                          const size_t& size);
bool ZeroInitializeUsm(const cl_command_queue& commandQueue,
                       void* usmPtr,
                       const size_t& size,
                       const UnifiedMemoryType& type);
bool ZeroInitializeSvm(const cl_command_queue& commandQueue,
                       void* svmPtr,
                       const size_t& size,
                       const bool& isFineGrain);
bool ZeroInitializeImage(const cl_command_queue& commandQueue,
                         const cl_mem& memObj,
                         const size_t& size,
                         const size_t& width,
                         const size_t& height,
                         const size_t& depth,
                         const size_t& input_row_pitch,
                         const size_t& input_slice_pitch);
std::string AppendKernelArgInfoOption(const std::string& options);
std::string AppendStreamPathToIncludePath(const std::string& options, const bool& hasHeaders);
cl_device_id GetGpuDevice();
cl_command_queue GetCommandQueue(const cl_context& context,
                                 const cl_device_id& device,
                                 cl_int* err = nullptr);
std::string RemoveDoubleDotHeaderSyntax(const std::string& src);
void* GetOffsetPointer(void* ptr, const uintptr_t& offset);
uintptr_t GetPointerDifference(void* ptrRegion, void* ptrStart);
void Log_clGitsIndirectAllocationOffsets(void* pAlloc, uint32_t numOffsets, size_t* pOffsets);
std::vector<uint64_t> HashBinaryData(const size_t& n,
                                     const uint8_t** binaries,
                                     const size_t* lengths);
cl_device_type GetDeviceType(const cl_device_id& device);
std::pair<void*, uint32_t> GetOriginalMappedPtrFromRegion(void* originalPtr);
std::string AppendBuildOption(const std::string& options, const std::string& optionToAppend);
} // namespace OpenCL
} // namespace gits

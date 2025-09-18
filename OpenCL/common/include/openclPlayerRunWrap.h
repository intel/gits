// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "openclArgumentsAuto.h"
#include "openclDrivers.h"
#include "openclArguments.h"
#include "openclStateDynamic.h"
#include "openclStateTracking.h"
#include "openclTools.h"

#include "config.h"
#include "log2.h"

namespace gits {
namespace OpenCL {
namespace {
bool CheckCaptureMemObjectReads(const cl_mem& memObj) {
  const auto& cfgOpenCLPlayer = Configurator::Get().opencl.player;
  if (!cfgOpenCLPlayer.captureReads || !cfgOpenCLPlayer.captureKernels.empty()) {
    return false;
  }
  if (cfgOpenCLPlayer.omitReadOnlyObjects) {
    const auto& memState = SD().GetMemState(memObj, EXCEPTION_MESSAGE);
    const auto readOnlyFlag = (GetPropertyVal(memState.intel_mem_properties.data(), CL_MEM_FLAGS) &
                               CL_MEM_READ_ONLY) != 0U;
    const auto isReadOnlyObject = (memState.flags & CL_MEM_READ_ONLY) != 0U || readOnlyFlag;
    return !isReadOnlyObject;
  }
  return true;
}

void PrintBuildLog(const cl_program program,
                   const cl_int numDevices,
                   const std::vector<cl_device_id> deviceVector) {
  cl_int errCode = CL_SUCCESS;
  size_t tmpNumDevices = numDevices;
  std::vector<cl_device_id> tmpDeviceVector = std::move(deviceVector);
  if (tmpNumDevices == 0 || tmpDeviceVector.empty()) {
    errCode |= drvOcl.clGetProgramInfo(program, CL_PROGRAM_NUM_DEVICES, sizeof(size_t),
                                       &tmpNumDevices, nullptr);
    tmpDeviceVector.resize(tmpNumDevices);
    errCode |=
        drvOcl.clGetProgramInfo(program, CL_PROGRAM_DEVICES, tmpNumDevices * sizeof(cl_device_id),
                                tmpDeviceVector.data(), nullptr);
  }
  for (auto& device : tmpDeviceVector) {
    size_t logLength = 0U;
    errCode |=
        drvOcl.clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &logLength);
    std::string buildLog(logLength, 0);
    errCode |= drvOcl.clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, logLength,
                                            &buildLog[0], nullptr);
    if (errCode == CL_SUCCESS) {
      LOG_INFO << "Build log:\n" << buildLog;
    }
  }
  if (errCode != CL_SUCCESS) {
    Log(ERR) << "Getting build log failed";
  }
}

cl_event GetGitsUserEvent(const cl_context& context) {
  static std::unordered_map<cl_context, cl_event> eventMap;
  cl_int err = CL_SUCCESS;
  auto& event = eventMap[context];
  if (event == nullptr) {
    event = drvOcl.clCreateUserEvent(context, &err);
    clCreateUserEvent_SD(event, context, &err);
    drvOcl.clSetUserEventStatus(event, CL_COMPLETE);
  }
  err = drvOcl.clRetainEvent(event);
  clRetainEvent_SD(err, event);
  return event;
}
void TranslatePointerOffsets(void* bufferPtr, const std::map<size_t, bool>& offsetMap) {
  for (const auto& offsetInfo : offsetMap) {
    void* ptrLocation =
        reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(bufferPtr) + offsetInfo.first);
    void* ptrToFetch = nullptr;
    std::memcpy(&ptrToFetch, ptrLocation, sizeof(void*));
    const auto allocInfo = GetOriginalMappedPtrFromRegion(ptrToFetch);
    ptrToFetch = GetOffsetPointer(allocInfo.first, allocInfo.second);
    if (ptrToFetch == nullptr) {
      Log(WARN) << "Couldn't translate pointer inside pAlloc: " << bufferPtr
                << " offset: " << offsetInfo.first;
    }
    std::memcpy(ptrLocation, &ptrToFetch, sizeof(void*));
  }
}
void TranslatePointers() {
  for (auto& allocState : SD()._usmAllocStates) {
    auto& indirectOffsets = allocState.second->indirectPointersOffsets;
    if (indirectOffsets.empty() ||
        indirectOffsets.end() ==
            std::find_if(indirectOffsets.begin(), indirectOffsets.end(),
                         [](const std::pair<size_t, bool>& i) { return !i.second; })) {
      continue;
    }
    if (allocState.second->type == UnifiedMemoryType::device) {
      auto cq = GetCommandQueue(allocState.second->context, allocState.second->device);
      const auto size = allocState.second->size;
      auto* tmpBuffer = new char[size];
      drvOcl.clEnqueueMemcpyINTEL(cq, CL_BLOCKING, tmpBuffer, allocState.first, size, 0, nullptr,
                                  nullptr);
      LOG_TRACEV << "^------------------ injected read to fetch pointers";
      TranslatePointerOffsets(tmpBuffer, indirectOffsets);
      drvOcl.clEnqueueMemcpyINTEL(cq, CL_BLOCKING, allocState.first, tmpBuffer, size, 0, nullptr,
                                  nullptr);
      LOG_TRACEV << "^------------------ injected write to fetch pointers";
      delete[] tmpBuffer;
    } else {
      TranslatePointerOffsets(allocState.first, indirectOffsets);
    }
    for (auto& indirectPair : indirectOffsets) {
      indirectPair.second = true;
    }
  }
  for (auto& svmState : SD()._svmAllocStates) {
    auto& indirectOffsets = svmState.second->indirectPointersOffsets;
    if (indirectOffsets.empty() ||
        indirectOffsets.end() ==
            std::find_if(indirectOffsets.begin(), indirectOffsets.end(),
                         [](const std::pair<size_t, bool>& i) { return !i.second; })) {
      continue;
    }
    if ((svmState.second->flags & CL_MEM_SVM_FINE_GRAIN_BUFFER) != 0U) {
      TranslatePointerOffsets(svmState.first, indirectOffsets);
    } else {
      auto cq = GetCommandQueue(svmState.second->context, GetGpuDevice());
      const auto size = svmState.second->size;
      auto* tmpBuffer = new char[size];
      drvOcl.clEnqueueSVMMemcpy(cq, CL_BLOCKING, tmpBuffer, svmState.first, size, 0, nullptr,
                                nullptr);
      LOG_TRACEV << "^------------------ injected read to fetch pointers";
      TranslatePointerOffsets(tmpBuffer, indirectOffsets);
      drvOcl.clEnqueueSVMMemcpy(cq, CL_BLOCKING, svmState.first, tmpBuffer, size, 0, nullptr,
                                nullptr);
      LOG_TRACEV << "^------------------ injected write to fetch pointers";
      delete[] tmpBuffer;
    }
    for (auto& indirectPair : indirectOffsets) {
      indirectPair.second = true;
    }
  }
}
} // namespace

inline void clBuildProgram_RUNWRAP(CCLResult& _return_value,
                                   Ccl_program& _program,
                                   Ccl_uint& _num_devices,
                                   Ccl_device_id::CSArray& _device_list,
                                   CBuildOptions& _options,
                                   CCallbackProgram& _pfn_notify,
                                   CCLUserData& _user_data) {
  std::string options(_options.ToString());
  options = AppendKernelArgInfoOption(options);
  const auto& hasHeaders = SD().GetProgramState(*_program, EXCEPTION_MESSAGE).HasHeaders();
  options = AppendStreamPathToIncludePath(options, hasHeaders);
  _return_value.Value() = drvOcl.clBuildProgram(*_program, *_num_devices, *_device_list,
                                                options.c_str(), *_pfn_notify, *_user_data);
  if (_return_value.Value() != CL_SUCCESS) {
    LOG_INFO << "clBuildProgram failed - Getting build log";
    PrintBuildLog(*_program, *_num_devices, _device_list._mappedArray);
  }
  clBuildProgram_SD(*_return_value, *_program, *_num_devices, *_device_list, options.c_str(),
                    *_pfn_notify, *_user_data);
}

inline void clCompileProgram_RUNWRAP(CCLResult& _return_value,
                                     Ccl_program& _program,
                                     Ccl_uint& _num_devices,
                                     Ccl_device_id::CSArray& _device_list,
                                     Cchar::CSArray& _options,
                                     Ccl_uint& _num_input_headers,
                                     Ccl_program::CSArray& _input_headers,
                                     CStringArray& _header_include_names,
                                     CCallbackProgram& _pfn_notify,
                                     CCLUserData& _user_data) {
  std::string options(_options.ToString());
  options = AppendKernelArgInfoOption(options);
  const auto& hasHeaders = SD().GetProgramState(*_program, EXCEPTION_MESSAGE).HasHeaders();
  options = AppendStreamPathToIncludePath(options, hasHeaders);
  _return_value.Value() = drvOcl.clCompileProgram(
      *_program, *_num_devices, *_device_list, options.c_str(), *_num_input_headers,
      *_input_headers, *_header_include_names, *_pfn_notify, *_user_data);
  clCompileProgram_SD(*_return_value, *_program, *_num_devices, *_device_list, options.c_str(),
                      *_num_input_headers, *_input_headers, *_header_include_names, *_pfn_notify,
                      *_user_data);
}

inline void clCreateContext_RUNWRAP(Ccl_context& _return_value,
                                    Ccl_context_properties::CSArray& _properties,
                                    Ccl_uint& _num_devices,
                                    Ccl_device_id::CSArray& _devices,
                                    CCallbackContext& _pfn_notify,
                                    CCLUserData& _user_data,
                                    CCLResult::CSArray& _errcode_ret) {
  drvOcl.Initialize();
  auto mapped_props = MapContextProperties(_properties);
  _return_value.Assign(drvOcl.clCreateContext(mapped_props.data(), *_num_devices, *_devices,
                                              *_pfn_notify, *_user_data, *_errcode_ret));
  clCreateContext_SD(*_return_value, *_properties, *_num_devices, *_devices, *_pfn_notify,
                     *_user_data, *_errcode_ret);
}

inline void clCreateContextFromType_RUNWRAP(Ccl_context& _return_value,
                                            Ccl_context_properties::CSArray& _properties,
                                            Ccl_device_type& _device_type,
                                            CCallbackContext& _pfn_notify,
                                            CCLUserData& _user_data,
                                            CCLResult::CSArray& _errcode_ret) {
  drvOcl.Initialize();
  auto mapped_props = MapContextProperties(_properties);
  _return_value.Assign(drvOcl.clCreateContextFromType(mapped_props.data(), *_device_type,
                                                      *_pfn_notify, *_user_data, *_errcode_ret));
  clCreateContextFromType_SD(*_return_value, *_properties, *_device_type, *_pfn_notify, *_user_data,
                             *_errcode_ret);
}

inline void clEnqueueNDRangeKernel_RUNWRAP(CCLResult& _return_value,
                                           Ccl_command_queue& _command_queue,
                                           Ccl_kernel& _kernel,
                                           Ccl_uint& _work_dim,
                                           Csize_t::CSArray& _global_work_offset,
                                           Csize_t::CSArray& _global_work_size,
                                           Csize_t::CSArray& _local_work_size,
                                           Ccl_uint& _num_events_in_wait_list,
                                           Ccl_event::CSArray& _event_wait_list,
                                           Ccl_event::CSMapArray& _event) {
  CGits::Instance().KernelCountUp();
  auto& cfg = Configurator::Get().opencl.player;
  if (cfg.aubSignaturesCL) {
    InjectKernelArgOperations(*_kernel, *_command_queue, *_event);
  }
  cl_event new_event = nullptr;
  TranslatePointers();
  *_return_value = drvOcl.clEnqueueNDRangeKernel(*_command_queue, *_kernel, *_work_dim,
                                                 *_global_work_offset, *_global_work_size,
                                                 *_local_work_size, *_num_events_in_wait_list,
                                                 *_event_wait_list, *_event ? *_event : &new_event);
  clEnqueueNDRangeKernel_SD(*_return_value, *_command_queue, *_kernel, *_work_dim,
                            *_global_work_offset, *_global_work_size, *_local_work_size,
                            *_num_events_in_wait_list, *_event_wait_list,
                            *_event ? *_event : &new_event);
}

inline void clGetCommandQueueInfo_RUNWRAP(CCLResult& _return_value,
                                          Ccl_command_queue& _command_queue,
                                          Ccl_command_queue_info& _param_name,
                                          Csize_t& _param_value_size,
                                          CBinaryData& _param_value,
                                          Csize_t::CSArray& _param_value_size_ret) {
  cl_device_id* old_device = nullptr;
  if (IsDeviceQuery(*_param_name)) {
    old_device = static_cast<cl_device_id*>(*_param_value);
  }
  *_return_value = drvOcl.clGetCommandQueueInfo(*_command_queue, *_param_name, *_param_value_size,
                                                *_param_value, *_param_value_size_ret);
  if (IsDeviceQuery(*_param_name) && ErrCodeSuccess(*_return_value) && *_param_value_size) {
    if (old_device != nullptr && !Ccl_device_id::CheckMapping(*old_device)) {
      Ccl_device_id::AddMapping(*old_device, *static_cast<cl_device_id*>(*_param_value));
    }
  }
}

inline void clGetContextInfo_RUNWRAP(CCLResult& _return_value,
                                     Ccl_context& _context,
                                     Ccl_context_info& _param_name,
                                     Csize_t& _param_value_size,
                                     CBinaryData& _param_value,
                                     Csize_t::CSArray& _param_value_size_ret) {
  std::vector<cl_device_id> old_devices;
  size_t num_old_devices = 0;
  if (IsDeviceQuery(*_param_name)) {
    num_old_devices = _param_value.Length() / sizeof(cl_device_id);
    cl_device_id* dev_ptr = static_cast<cl_device_id*>(*_param_value);
    old_devices.assign(dev_ptr, dev_ptr + num_old_devices);
  }
  *_return_value = drvOcl.clGetContextInfo(*_context, *_param_name, *_param_value_size,
                                           *_param_value, *_param_value_size_ret);
  clGetContextInfo_SD(*_return_value, *_context, *_param_name, *_param_value_size, *_param_value,
                      *_param_value_size_ret);
  if (IsDeviceQuery(*_param_name) && ErrCodeSuccess(*_return_value) && *_param_value_size) {
    clGetContextInfo_SetMapping(old_devices.data(), num_old_devices,
                                static_cast<cl_device_id*>(*_param_value), *_param_value_size,
                                *_param_value_size_ret);
  }
}

inline void clGetEventInfo_RUNWRAP(CCLResult& _return_value,
                                   Ccl_event& _event,
                                   Ccl_event_info& _param_name,
                                   Csize_t& _param_value_size,
                                   CBinaryData& _param_value,
                                   Csize_t::CSArray& _param_value_size_ret) {
  cl_int old_status = -1;
  if (*_param_name == CL_EVENT_COMMAND_EXECUTION_STATUS) {
    old_status = *static_cast<cl_int*>(*_param_value);
  }
  for (;;) {
    *_return_value = drvOcl.clGetEventInfo(*_event, *_param_name, *_param_value_size, *_param_value,
                                           *_param_value_size_ret);
    // CL_COMPLETE is 0, CL_QUEUED is 3, so we wait if the status we got is
    // bigger than the one captured during recording
    if (*_return_value == CL_SUCCESS && old_status != -1 &&
        old_status < *static_cast<cl_int*>(*_param_value)) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    } else {
      break;
    }
  }
}

inline void clGetGLContextInfoKHR_RUNWRAP(CCLResult& _return_value,
                                          Ccl_context_properties::CSArray& _properties,
                                          Ccl_gl_context_info& _param_name,
                                          Csize_t& _param_value_size,
                                          CBinaryData& _param_value,
                                          Csize_t::CSArray& _param_value_size_ret) {
  std::vector<cl_device_id> old_devices;
  size_t num_old_devices = 0;
  if (IsDeviceQuery(*_param_name)) {
    num_old_devices = _param_value.Length() / sizeof(cl_device_id);
    old_devices.resize(num_old_devices, nullptr);
    std::copy_n(static_cast<cl_device_id*>(*_param_value), num_old_devices, old_devices.begin());
  }
  auto mapped_props = MapContextProperties(_properties);
  *_return_value = drvOcl.clGetGLContextInfoKHR(
      mapped_props.data(), *_param_name, *_param_value_size, *_param_value, *_param_value_size_ret);
  if (IsDeviceQuery(*_param_name) && ErrCodeSuccess(*_return_value) && *_param_value_size) {
    clGetContextInfo_SetMapping(old_devices.data(), num_old_devices,
                                static_cast<cl_device_id*>(*_param_value), *_param_value_size,
                                *_param_value_size_ret);
  }
}

inline void clGetKernelSubGroupInfo_RUNWRAP(CCLResult& _return_value,
                                            Ccl_kernel& _in_kernel,
                                            Ccl_device_id& _in_device,
                                            Ccl_kernel_sub_group_info& _param_name,
                                            Csize_t& _input_value_size,
                                            CBinaryData& _input_value,
                                            Csize_t& _param_value_size,
                                            CBinaryData& _param_value,
                                            Csize_t::CSArray& _param_value_size_ret) {
  // *_return_value = drvOcl.clGetKernelSubGroupInfo(*_in_kernel, *_in_device, *_param_name, *_input_value_size, *_input_value, *_param_value_size, *_param_value, *_param_value_size_ret);
}

inline void clGetPlatformIDs_RUNWRAP(CCLResult& _return_value,
                                     Ccl_uint& _num_entries,
                                     Ccl_platform_id::CSMapArray& _platforms,
                                     Ccl_uint::CSArray& _num_platforms) {
  drvOcl.Initialize();
  if (*_num_entries > 0U) { // Skip size getter token
    cl_uint numPlatforms = 0U;
    *_return_value = drvOcl.clGetPlatformIDs(0, nullptr, &numPlatforms);
    std::vector<cl_platform_id> platforms(numPlatforms);
    *_return_value = drvOcl.clGetPlatformIDs(numPlatforms, platforms.data(), nullptr);
    clGetPlatformIDs_SD(*_return_value, numPlatforms, platforms.data(), *_num_platforms);
    if (*_num_entries > numPlatforms) {
      Log(WARN) << "Original application was recorded using more platforms";
    }

    // Obtain devices and devices types for stream compatibility on any
    // platform.
    for (const auto& platform : platforms) {
      CheckIntelPlatform(platform);
      cl_uint numDevices = 0U;
      drvOcl.clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, nullptr, &numDevices);
      std::vector<cl_device_id> devices(numDevices);
      const auto ret =
          drvOcl.clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, numDevices, devices.data(), nullptr);
      clGetDeviceIDs_SD(ret, platform, CL_DEVICE_TYPE_ALL, numDevices, devices.data(), nullptr);
    }

    // First assignment of GPU Platform for every original platform
    const auto* platformOriginalPtr = _platforms.Original();
    if (platformOriginalPtr != nullptr && _platforms.Size() >= *_num_entries) {
      cl_platform_id gpuIntelPlatform = nullptr;
      for (const auto& state : SD()._platformIDStates) {
        if (state.second->GetDeviceType(CL_DEVICE_TYPE_GPU) != nullptr) {
          gpuIntelPlatform = state.first;
          break;
        }
      }
      for (uint32_t i = 0; i < *_num_entries; i++) {
        auto remapPlatform = gpuIntelPlatform;
        if (remapPlatform == nullptr) {
          if (i < platforms.size()) {
            remapPlatform = platforms.at(i);
          } else {
            remapPlatform = platforms.back();
          }
        }
        Ccl_platform_id::AddMapping(platformOriginalPtr[i], remapPlatform);
      }
    }
  }
}

inline void clGetProgramInfo_RUNWRAP(CCLResult& _return_value,
                                     Ccl_program& _program,
                                     Ccl_program_info& _param_name,
                                     Csize_t& _param_value_size,
                                     CBinaryData& _param_value,
                                     Csize_t::CSArray& _param_value_size_ret) {
  const auto paramSize = *_param_value_size;
  if (*_param_name == CL_PROGRAM_BINARIES && paramSize != 0U) {
    auto& program = SD().GetProgramState(*_program, EXCEPTION_MESSAGE);
    auto nDevices = program.DevicesCount();
    cl_int errcode = CL_SUCCESS;
    if (nDevices == 0U) {
      errcode = drvOcl.clGetProgramInfo(*_program, CL_PROGRAM_NUM_DEVICES, sizeof(nDevices),
                                        &nDevices, nullptr);
      program.GetProgramInfoNumDevices(nDevices);
    }
    const size_t binariesCount = program.BinariesCount();
    if (ErrCodeSuccess(errcode) && binariesCount == 0U) {
      std::vector<size_t> binarySizes(nDevices);
      const auto size = sizeof(size_t) * nDevices;
      errcode = drvOcl.clGetProgramInfo(*_program, CL_PROGRAM_BINARY_SIZES, size,
                                        binarySizes.data(), nullptr);
      program.GetProgramInfoBinarySizes(size, binarySizes.data());
    }
    const size_t size = binariesCount * sizeof(unsigned char*);
    if (ErrCodeSuccess(errcode)) {
      program.ReallocBinaries();
      auto* binaries = const_cast<unsigned char**>(program.Binaries());
      *_return_value = drvOcl.clGetProgramInfo(*_program, *_param_name, size, binaries, nullptr);
      clGetProgramInfo_SD(*_return_value, *_program, *_param_name, size, binaries, nullptr);
    }
  } else {
    cl_device_id* old_devices = nullptr;
    if (IsDeviceQuery(*_param_name)) {
      old_devices = static_cast<cl_device_id*>(*_param_value);
    }
    *_return_value = drvOcl.clGetProgramInfo(*_program, *_param_name, paramSize, *_param_value,
                                             *_param_value_size_ret);
    clGetProgramInfo_SD(*_return_value, *_program, *_param_name, paramSize, *_param_value,
                        *_param_value_size_ret);
    if (IsDeviceQuery(*_param_name) && ErrCodeSuccess(*_return_value) && paramSize != 0U) {
      const auto* devices = static_cast<cl_device_id*>(*_param_value);
      const size_t numDevices = paramSize / sizeof(cl_device_id);
      for (auto i = 0U; i < numDevices; i++) {
        if (devices[i] == nullptr) {
          break;
        }
        if (!Ccl_device_id::CheckMapping(old_devices[i])) {
          Ccl_device_id::AddMapping(old_devices[i], devices[i]);
        }
      }
    }
  }
}

inline void clCreateBuffer_RUNWRAP(Ccl_mem& _return_value,
                                   Ccl_context& _context,
                                   Ccl_mem_flags& _flags,
                                   Csize_t& _size,
                                   CAsyncBinaryData& _host_ptr,
                                   CCLResult::CSArray& _errcode_ret) {
  const auto& cfg = Configurator::Get();
  const auto signature = GenerateSignature();
  size_t size = *_size;
  if (cfg.opencl.player.aubSignaturesCL) {
    size += sizeof(mem_signature_t);
  }
  CBinaryResource::PointerProxy ptr = *_host_ptr;
  char* buffer_ptr;
  if ((const void*)ptr != nullptr) {
    SD()._buffers.emplace_back((const char*)ptr, (const char*)ptr + *_size);
    if (cfg.opencl.player.aubSignaturesCL) {
      AddSignature(SD()._buffers.back(), signature);
    }
    if (*_size == sizeof(cl_ulong) &&
        CCLMappedPtr::CheckMapping(*reinterpret_cast<char**>(SD()._buffers.back().data()))) {
      buffer_ptr = static_cast<char*>(
          CCLMappedPtr::GetMapping(*reinterpret_cast<char**>(SD()._buffers.back().data())));
      std::memcpy(SD()._buffers.back().data(), &buffer_ptr, sizeof(cl_ulong));
    }
    buffer_ptr = SD()._buffers.back().data();
  } else {
    buffer_ptr = nullptr;
  }
  _return_value.Assign(drvOcl.clCreateBuffer(*_context, *_flags, size, buffer_ptr, *_errcode_ret));
  clCreateBuffer_SD(*_return_value, *_context, *_flags, *_size, buffer_ptr, *_errcode_ret);
  if (buffer_ptr && !FlagUseHostPtr(*_flags)) {
    DeallocateVector(SD()._buffers.back());
  }
  const auto isUsingHostPtr = *_flags & (CL_MEM_COPY_HOST_PTR | CL_MEM_USE_HOST_PTR);
  if (ErrCodeSuccess(*_errcode_ret) && !isUsingHostPtr &&
      CheckCfgZeroInitialization(cfg, IsReadOnlyBuffer(*_flags, nullptr))) {
    const auto device = GetGpuDevice();
    if (device != nullptr) {
      const auto commandQueue = GetCommandQueue(*_context, device);
      ZeroInitializeBuffer(commandQueue, *_return_value, size);
    }
  }
}

inline void clCreateImage_RUNWRAP(Ccl_mem& _return_value,
                                  Ccl_context& _context,
                                  Ccl_mem_flags& _flags,
                                  Ccl_image_format::CSArray& _image_format,
                                  Ccl_image_desc::CSArray& _image_desc,
                                  CAsyncBinaryData& _host_ptr,
                                  CCLResult::CSArray& _errcode_ret) {
  cl_image_desc image_desc = **_image_desc;
  if ((image_desc.image_type == CL_MEM_OBJECT_IMAGE1D_BUFFER ||
       image_desc.image_type == CL_MEM_OBJECT_IMAGE2D) &&
      image_desc.mem_object != nullptr) {
    image_desc.mem_object = Ccl_mem::GetMapping(image_desc.mem_object);
  }
  CBinaryResource::PointerProxy ptr = *_host_ptr;
  const auto size = CountImageSize(**_image_format, image_desc);
  char* buffer_ptr;
  if ((const void*)ptr != nullptr) {
    SD()._buffers.emplace_back((const char*)ptr, (const char*)ptr + size);
    buffer_ptr = SD()._buffers.back().data();
  } else {
    buffer_ptr = nullptr;
  }
  _return_value.Assign(drvOcl.clCreateImage(*_context, *_flags, *_image_format, &image_desc,
                                            buffer_ptr, *_errcode_ret));
  clCreateImage_SD(*_return_value, *_context, *_flags, *_image_format, &image_desc, buffer_ptr,
                   *_errcode_ret);
  if (buffer_ptr != nullptr && !FlagUseHostPtr(*_flags)) {
    DeallocateVector(SD()._buffers.back());
  }
  const auto isUsingHostPtr = (*_flags & (CL_MEM_COPY_HOST_PTR | CL_MEM_USE_HOST_PTR));
  if (ErrCodeSuccess(*_errcode_ret) && !isUsingHostPtr &&
      CheckCfgZeroInitialization(Configurator::Get(), IsReadOnlyBuffer(*_flags, nullptr))) {
    const auto device = GetGpuDevice();
    const auto commandQueue = GetCommandQueue(*_context, device);
    ZeroInitializeImage(commandQueue, *_return_value, size, image_desc.image_width,
                        image_desc.image_height, image_desc.image_depth, image_desc.image_row_pitch,
                        image_desc.image_slice_pitch);
  }
}

inline void clCreateImage2D_RUNWRAP(Ccl_mem& _return_value,
                                    Ccl_context& _context,
                                    Ccl_mem_flags& _flags,
                                    Ccl_image_format::CSArray& _image_format,
                                    Csize_t& _image_width,
                                    Csize_t& _image_height,
                                    Csize_t& _image_row_pitch,
                                    CAsyncBinaryData& _host_ptr,
                                    CCLResult::CSArray& _errcode_ret) {
  CBinaryResource::PointerProxy ptr = *_host_ptr;
  const auto size =
      CountImageSize(**_image_format, *_image_width, *_image_height, *_image_row_pitch);
  char* buffer_ptr;
  if ((const void*)ptr != nullptr) {
    SD()._buffers.emplace_back((const char*)ptr, (const char*)ptr + size);
    buffer_ptr = SD()._buffers.back().data();
  } else {
    buffer_ptr = nullptr;
  }
  _return_value.Assign(drvOcl.clCreateImage2D(*_context, *_flags, *_image_format, *_image_width,
                                              *_image_height, *_image_row_pitch, buffer_ptr,
                                              *_errcode_ret));
  clCreateImage2D_SD(*_return_value, *_context, *_flags, *_image_format, *_image_width,
                     *_image_height, *_image_row_pitch, buffer_ptr, *_errcode_ret);
  if (buffer_ptr != nullptr && !FlagUseHostPtr(*_flags)) {
    DeallocateVector(SD()._buffers.back());
  }
  const auto isUsingHostPtr = (*_flags & (CL_MEM_COPY_HOST_PTR | CL_MEM_USE_HOST_PTR));
  if (ErrCodeSuccess(*_errcode_ret) && !isUsingHostPtr &&
      CheckCfgZeroInitialization(Configurator::Get(), IsReadOnlyBuffer(*_flags, nullptr))) {
    const auto device = GetGpuDevice();
    const auto commandQueue = GetCommandQueue(*_context, device);
    ZeroInitializeImage(commandQueue, *_return_value, size, *_image_width, *_image_height, 1,
                        *_image_row_pitch, 0);
  }
}

inline void clCreateImage3D_RUNWRAP(Ccl_mem& _return_value,
                                    Ccl_context& _context,
                                    Ccl_mem_flags& _flags,
                                    Ccl_image_format::CSArray& _image_format,
                                    Csize_t& _image_width,
                                    Csize_t& _image_height,
                                    Csize_t& _image_depth,
                                    Csize_t& _image_row_pitch,
                                    Csize_t& _image_slice_pitch,
                                    CAsyncBinaryData& _host_ptr,
                                    CCLResult::CSArray& _errcode_ret) {
  CBinaryResource::PointerProxy ptr = *_host_ptr;
  const auto size = CountImageSize(**_image_format, *_image_width, *_image_height, *_image_depth,
                                   *_image_row_pitch, *_image_slice_pitch);
  char* buffer_ptr;
  if ((const void*)ptr != nullptr) {
    SD()._buffers.emplace_back((const char*)ptr, (const char*)ptr + size);
    buffer_ptr = SD()._buffers.back().data();
  } else {
    buffer_ptr = nullptr;
  }
  _return_value.Assign(drvOcl.clCreateImage3D(*_context, *_flags, *_image_format, *_image_width,
                                              *_image_height, *_image_depth, *_image_row_pitch,
                                              *_image_slice_pitch, buffer_ptr, *_errcode_ret));
  clCreateImage3D_SD(*_return_value, *_context, *_flags, *_image_format, *_image_width,
                     *_image_height, *_image_depth, *_image_row_pitch, *_image_slice_pitch,
                     buffer_ptr, *_errcode_ret);
  if (buffer_ptr != nullptr && !FlagUseHostPtr(*_flags)) {
    DeallocateVector(SD()._buffers.back());
  }
  const auto isUsingHostPtr = (*_flags & (CL_MEM_COPY_HOST_PTR | CL_MEM_USE_HOST_PTR));
  if (ErrCodeSuccess(*_errcode_ret) && !isUsingHostPtr &&
      CheckCfgZeroInitialization(Configurator::Get(), IsReadOnlyBuffer(*_flags, nullptr))) {
    const auto device = GetGpuDevice();
    const auto commandQueue = GetCommandQueue(*_context, device);
    ZeroInitializeImage(commandQueue, *_return_value, size, *_image_width, *_image_height,
                        *_image_depth, *_image_row_pitch, *_image_slice_pitch);
  }
}

inline void clEnqueueUnmapMemObject_RUNWRAP(CFunction* _token,
                                            CCLResult& _return_value,
                                            Ccl_command_queue& _command_queue,
                                            Ccl_mem& _memobj,
                                            CCLMappedPtr& _mapped_ptr,
                                            Ccl_uint& _num_events_in_wait_list,
                                            Ccl_event::CSArray& _event_wait_list,
                                            Ccl_event::CSMapArray& _event) {
  _mapped_ptr.SyncBuffer();
  if (_mapped_ptr.HasData() && CheckCaptureMemObjectReads(*_memobj)) {
    static unsigned num = 1;
    std::string filename = "UnmapMemObject_" + std::to_string(num++);
    SaveBuffer(filename, _mapped_ptr.Data());
  }
  _return_value.Value() =
      drvOcl.clEnqueueUnmapMemObject(*_command_queue, *_memobj, *_mapped_ptr,
                                     *_num_events_in_wait_list, *_event_wait_list, *_event);
  clEnqueueUnmapMemObject_SD(_token, *_return_value, *_command_queue, *_memobj, *_mapped_ptr,
                             *_num_events_in_wait_list, *_event_wait_list, *_event);
}

inline void clEnqueueReadBuffer_RUNWRAP(CFunction* _token,
                                        CCLResult& _return_value,
                                        Ccl_command_queue& _command_queue,
                                        Ccl_mem& _buffer,
                                        Ccl_bool& _blocking_read,
                                        Csize_t& _offset,
                                        Csize_t& _cb,
                                        CAsyncBinaryData& _ptr,
                                        Ccl_uint& _num_events_in_wait_list,
                                        Ccl_event::CSArray& _event_wait_list,
                                        Ccl_event::CSMapArray& _event) {
  const auto captureReads = CheckCaptureMemObjectReads(*_buffer);
  const auto& cfg = Configurator::Get().opencl.player;
  auto& sd = SD();
  if (captureReads || cfg.captureKernels.empty()) {
    size_t size = *_cb;
    if (cfg.aubSignaturesCL && *_offset == 0) {
      size += sizeof(mem_signature_t);
    }
    auto& cqBuffers = sd._enqueueBuffers[*_command_queue];
    cqBuffers.emplace_back(size, static_cast<char>(0));
    const auto blockingRead = captureReads ? CL_BLOCKING : *_blocking_read;
    _return_value.Value() = drvOcl.clEnqueueReadBuffer(
        *_command_queue, *_buffer, blockingRead, *_offset, size, cqBuffers.back().data(),
        *_num_events_in_wait_list, *_event_wait_list, *_event);
    if (captureReads) {
      static unsigned num = 1;
      SaveBuffer("ReadBuffer_" + std::to_string(num++), cqBuffers.back());
    }
    clEnqueueReadBuffer_SD(_token, *_return_value, *_command_queue, *_buffer, blockingRead,
                           *_offset, *_cb, cqBuffers.back().data(), *_num_events_in_wait_list,
                           *_event_wait_list, *_event);
    if (blockingRead == CL_TRUE) {
      DeallocateVector(cqBuffers.back());
    }
  } else if (_event.Size() > 0) {
    const auto context = sd.GetCommandQueueState(*_command_queue, EXCEPTION_MESSAGE).context;
    const auto event = GetGitsUserEvent(context);
    const auto& originalEvent = _event.Original();
    if (originalEvent != nullptr) {
      Ccl_event::AddMapping(originalEvent[0], event);
    }
  }
  sd.deallocationHandler.AddToResourcesInExecution(*_command_queue, _ptr, *_blocking_read);
}

inline void clEnqueueReadBufferRect_RUNWRAP(CFunction* _token,
                                            CCLResult& _return_value,
                                            Ccl_command_queue& _command_queue,
                                            Ccl_mem& _buffer,
                                            Ccl_bool& _blocking_read,
                                            Csize_t::CSArray& _buffer_offset,
                                            Csize_t::CSArray& _host_offset,
                                            Csize_t::CSArray& _region,
                                            Csize_t& _buffer_row_pitch,
                                            Csize_t& _buffer_slice_pitch,
                                            Csize_t& _host_row_pitch,
                                            Csize_t& _host_slice_pitch,
                                            CAsyncBinaryData& _ptr,
                                            Ccl_uint& _num_events_in_wait_list,
                                            Ccl_event::CSArray& _event_wait_list,
                                            Ccl_event::CSMapArray& _event) {
  const auto& cfg = Configurator::Get().opencl.player;
  auto& sd = SD();
  const auto captureReads = CheckCaptureMemObjectReads(*_buffer);
  if (captureReads || cfg.captureKernels.empty()) {
    const size_t size = CountBufferRectSize(*_region, *_buffer_row_pitch, *_buffer_slice_pitch);
    auto& cqBuffers = sd._enqueueBuffers[*_command_queue];
    cqBuffers.emplace_back(size, static_cast<char>(0));
    const auto blockingRead = captureReads ? CL_BLOCKING : *_blocking_read;
    _return_value.Value() = drvOcl.clEnqueueReadBufferRect(
        *_command_queue, *_buffer, blockingRead, *_buffer_offset, *_host_offset, *_region,
        *_buffer_row_pitch, *_buffer_slice_pitch, *_host_row_pitch, *_host_slice_pitch,
        cqBuffers.back().data(), *_num_events_in_wait_list, *_event_wait_list, *_event);
    if (captureReads) {
      static unsigned num = 1;
      SaveBuffer("ReadBufferRect_" + std::to_string(num++), cqBuffers.back());
    }
    clEnqueueReadBufferRect_SD(_token, *_return_value, *_command_queue, *_buffer, blockingRead,
                               *_buffer_offset, *_host_offset, *_region, *_buffer_row_pitch,
                               *_buffer_slice_pitch, *_host_row_pitch, *_host_slice_pitch,
                               cqBuffers.back().data(), *_num_events_in_wait_list,
                               *_event_wait_list, *_event);
    if (blockingRead == CL_TRUE) {
      DeallocateVector(cqBuffers.back());
    }
  } else if (_event.Size() > 0) {
    const auto context = sd.GetCommandQueueState(*_command_queue, EXCEPTION_MESSAGE).context;
    const auto event = GetGitsUserEvent(context);
    const auto& originalEvent = _event.Original();
    if (originalEvent != nullptr) {
      Ccl_event::AddMapping(originalEvent[0], event);
    }
  }
  sd.deallocationHandler.AddToResourcesInExecution(*_command_queue, _ptr, *_blocking_read);
}

inline void clEnqueueReadImage_RUNWRAP(CFunction* _token,
                                       CCLResult& _return_value,
                                       Ccl_command_queue& _command_queue,
                                       Ccl_mem& _image,
                                       Ccl_bool& _blocking_read,
                                       Csize_t::CSArray& _origin,
                                       Csize_t::CSArray& _region,
                                       Csize_t& _row_pitch,
                                       Csize_t& _slice_pitch,
                                       CAsyncBinaryData& _ptr,
                                       Ccl_uint& _num_events_in_wait_list,
                                       Ccl_event::CSArray& _event_wait_list,
                                       Ccl_event::CSMapArray& _event) {
  const auto& cfg = Configurator::Get().opencl.player;
  auto& sd = SD();
  const auto format = sd.GetMemState(*_image, EXCEPTION_MESSAGE).image_format;
  const auto region = *_region;
  const auto size =
      (region == nullptr ? 0 : CountImageSize(format, region, *_row_pitch, *_slice_pitch));
  auto& cqBuffers = sd._enqueueBuffers[*_command_queue];
  cqBuffers.emplace_back(size, static_cast<char>(0));
  cl_bool blockingRead = *_blocking_read;
  if (cfg.captureReads || cfg.captureImages) {
    blockingRead = CL_BLOCKING;
    _return_value.Value() = drvOcl.clEnqueueReadImage(
        *_command_queue, *_image, blockingRead, *_origin, *_region, *_row_pitch, *_slice_pitch,
        cqBuffers.back().data(), *_num_events_in_wait_list, *_event_wait_list, *_event);
    static unsigned num = 1;
    std::string fileName = "ReadImage_" + std::to_string(num);
    if (CheckCaptureMemObjectReads(*_image)) {
      SaveBuffer(fileName, cqBuffers.back());
    }
    if (cfg.captureImages && cfg.captureKernels.empty()) {
      const auto& memState = sd.GetMemState(*_image, EXCEPTION_MESSAGE);
      SaveImage(cqBuffers.back().data(), memState.image_format, memState.image_desc, fileName);
    }
    num++;
  } else if (cfg.captureKernels.empty()) {
    _return_value.Value() = drvOcl.clEnqueueReadImage(
        *_command_queue, *_image, blockingRead, *_origin, *_region, *_row_pitch, *_slice_pitch,
        cqBuffers.back().data(), *_num_events_in_wait_list, *_event_wait_list, *_event);
  } else if (_event.Size() > 0) {
    const auto context = sd.GetCommandQueueState(*_command_queue, EXCEPTION_MESSAGE).context;
    const auto event = GetGitsUserEvent(context);
    Ccl_event::AddMapping(_event.Original()[0], event);
    return;
  }
  clEnqueueReadImage_SD(_token, *_return_value, *_command_queue, *_image, blockingRead, *_origin,
                        *_region, *_row_pitch, *_slice_pitch, cqBuffers.back().data(),
                        *_num_events_in_wait_list, *_event_wait_list, *_event);
  if (blockingRead == CL_TRUE) {
    DeallocateVector(cqBuffers.back());
  }
  sd.deallocationHandler.AddToResourcesInExecution(*_command_queue, _ptr, *_blocking_read);
}

inline void clEnqueueWriteBuffer_RUNWRAP(CFunction* _token,
                                         CCLResult& _return_value,
                                         Ccl_command_queue& _command_queue,
                                         Ccl_mem& _buffer,
                                         Ccl_bool& _blocking_write,
                                         Csize_t& _offset,
                                         Csize_t& _cb,
                                         CAsyncBinaryData& _ptr,
                                         Ccl_uint& _num_events_in_wait_list,
                                         Ccl_event::CSArray& _event_wait_list,
                                         Ccl_event::CSMapArray& _event) {
  CBinaryResource::PointerProxy ptr = *_ptr;
  auto& sd = SD();
  auto& cqBuffers = sd._enqueueBuffers[*_command_queue];
  cqBuffers.emplace_back((const char*)ptr, (const char*)ptr + *_cb);
  _return_value.Value() = drvOcl.clEnqueueWriteBuffer(
      *_command_queue, *_buffer, *_blocking_write, *_offset, *_cb, cqBuffers.back().data(),
      *_num_events_in_wait_list, *_event_wait_list, *_event);
  clEnqueueWriteBuffer_SD(_token, *_return_value, *_command_queue, *_buffer, *_blocking_write,
                          *_offset, *_cb, cqBuffers.back().data(), *_num_events_in_wait_list,
                          *_event_wait_list, *_event);

  if (*_blocking_write == CL_TRUE) {
    DeallocateVector(cqBuffers.back());
  }
  sd.deallocationHandler.AddToResourcesInExecution(*_command_queue, _ptr, *_blocking_write);
}

inline void clEnqueueWriteBufferRect_RUNWRAP(CFunction* _token,
                                             CCLResult& _return_value,
                                             Ccl_command_queue& _command_queue,
                                             Ccl_mem& _buffer,
                                             Ccl_bool& _blocking_write,
                                             Csize_t::CSArray& _buffer_offset,
                                             Csize_t::CSArray& _host_offset,
                                             Csize_t::CSArray& _region,
                                             Csize_t& _buffer_row_pitch,
                                             Csize_t& _buffer_slice_pitch,
                                             Csize_t& _host_row_pitch,
                                             Csize_t& _host_slice_pitch,
                                             CAsyncBinaryData& _ptr,
                                             Ccl_uint& _num_events_in_wait_list,
                                             Ccl_event::CSArray& _event_wait_list,
                                             Ccl_event::CSMapArray& _event) {
  CBinaryResource::PointerProxy ptr = *_ptr;
  size_t size = CountBufferRectSize(*_region, *_buffer_row_pitch, *_buffer_slice_pitch);
  auto& sd = SD();
  auto& cqBuffers = SD()._enqueueBuffers[*_command_queue];
  cqBuffers.emplace_back((const char*)ptr, (const char*)ptr + size);
  _return_value.Value() = drvOcl.clEnqueueWriteBufferRect(
      *_command_queue, *_buffer, *_blocking_write, *_buffer_offset, *_host_offset, *_region,
      *_buffer_row_pitch, *_buffer_slice_pitch, *_host_row_pitch, *_host_slice_pitch,
      cqBuffers.back().data(), *_num_events_in_wait_list, *_event_wait_list, *_event);
  clEnqueueWriteBufferRect_SD(_token, *_return_value, *_command_queue, *_buffer, *_blocking_write,
                              *_buffer_offset, *_host_offset, *_region, *_buffer_row_pitch,
                              *_buffer_slice_pitch, *_host_row_pitch, *_host_slice_pitch,
                              cqBuffers.back().data(), *_num_events_in_wait_list, *_event_wait_list,
                              *_event);

  if (*_blocking_write == CL_TRUE) {
    DeallocateVector(cqBuffers.back());
  }
  sd.deallocationHandler.AddToResourcesInExecution(*_command_queue, _ptr, *_blocking_write);
}

inline void clEnqueueWriteImage_RUNWRAP(CFunction* _token,
                                        CCLResult& _return_value,
                                        Ccl_command_queue& _command_queue,
                                        Ccl_mem& _image,
                                        Ccl_bool& _blocking_write,
                                        Csize_t::CSArray& _origin,
                                        Csize_t::CSArray& _region,
                                        Csize_t& _input_row_pitch,
                                        Csize_t& _input_slice_pitch,
                                        CAsyncBinaryData& _ptr,
                                        Ccl_uint& _num_events_in_wait_list,
                                        Ccl_event::CSArray& _event_wait_list,
                                        Ccl_event::CSMapArray& _event) {
  CBinaryResource::PointerProxy ptr = *_ptr;
  auto& sd = SD();
  cl_image_format format = sd.GetMemState(*_image, EXCEPTION_MESSAGE).image_format;
  const auto region = *_region;
  size_t size =
      (region == nullptr ? 0
                         : CountImageSize(format, region, *_input_row_pitch, *_input_slice_pitch));
  auto& cqBuffers = sd._enqueueBuffers[*_command_queue];
  cqBuffers.emplace_back((const char*)ptr, (const char*)ptr + size);
  _return_value.Value() =
      drvOcl.clEnqueueWriteImage(*_command_queue, *_image, *_blocking_write, *_origin, *_region,
                                 *_input_row_pitch, *_input_slice_pitch, cqBuffers.back().data(),
                                 *_num_events_in_wait_list, *_event_wait_list, *_event);
  clEnqueueWriteImage_SD(_token, *_return_value, *_command_queue, *_image, *_blocking_write,
                         *_origin, *_region, *_input_row_pitch, *_input_slice_pitch,
                         cqBuffers.back().data(), *_num_events_in_wait_list, *_event_wait_list,
                         *_event);

  if (*_blocking_write == CL_TRUE) {
    DeallocateVector(cqBuffers.back());
  }
  sd.deallocationHandler.AddToResourcesInExecution(*_command_queue, _ptr, *_blocking_write);
}

inline void clReleaseMemObject_RUNWRAP(CCLResult& _return_value, Ccl_mem& _memobj) {
  _return_value.Value() = drvOcl.clReleaseMemObject(*_memobj);
  if (*_memobj != nullptr) {
    auto& memState = SD().GetMemState(*_memobj, EXCEPTION_MESSAGE);
    if ((memState.buffer || memState.image) && FlagUseHostPtr(memState.flags) &&
        GetRefCount(*_memobj) == 0) {
      DeallocateVector(SD()._buffers[memState.buffer_number]);
    }
  }
  clReleaseMemObject_SD(*_return_value, *_memobj);
}

inline void clFinish_RUNWRAP(CCLResult& _return_value, Ccl_command_queue& _command_queue) {
  auto& sd = SD();
  _return_value.Value() = drvOcl.clFinish(*_command_queue);
  sd.deallocationHandler.DeallocateExecutedResources(*_command_queue);
  auto& cqBuffers = sd._enqueueBuffers[*_command_queue];
  cqBuffers.clear();
}

inline void clReleaseCommandQueue_RUNWRAP(CCLResult& _return_value,
                                          Ccl_command_queue& _command_queue) {
  _return_value.Value() = drvOcl.clReleaseCommandQueue(*_command_queue);
  clReleaseCommandQueue_SD(*_return_value, *_command_queue);
  if (!GetRefCount(*_command_queue)) {
    auto& sd = SD();
    sd._enqueueBuffers.erase(*_command_queue);
    sd.deallocationHandler.DeallocateExecutedResources(*_command_queue);
  }
}

inline void clCreateBufferWithPropertiesINTEL_RUNWRAP(
    CFunction* _token,
    Ccl_mem& _return_value,
    Ccl_context& _context,
    Ccl_mem_properties_intel::CSArray& _properties,
    Ccl_mem_flags& _flags,
    Csize_t& _size,
    CAsyncBinaryData& _host_ptr,
    CCLResult::CSArray& _errcode_ret) {
  const auto& cfg = Configurator::Get().opencl.player;
  const auto signature = GenerateSignature();
  auto size = *_size;
  if (cfg.aubSignaturesCL) {
    size += sizeof(mem_signature_t);
  }
  auto mappedProps = MapMemPropertiesIntel(_properties);
  const cl_mem_flags propFlags = GetPropertyVal(mappedProps.data(), CL_MEM_FLAGS);
  CBinaryResource::PointerProxy ptr = *_host_ptr;
  char* buffer_ptr;
  if ((const void*)ptr != nullptr) {
    SD()._buffers.emplace_back((const char*)ptr, (const char*)ptr + *_size);
    if (cfg.aubSignaturesCL) {
      AddSignature(SD()._buffers.back(), signature);
    }
    buffer_ptr = SD()._buffers.back().data();
  } else {
    buffer_ptr = nullptr;
  }
  _return_value.Assign(drvOcl.clCreateBufferWithPropertiesINTEL(
      *_context, mappedProps.data(), *_flags, size, buffer_ptr, *_errcode_ret));
  clCreateBufferWithPropertiesINTEL_SD(*_return_value, *_context, mappedProps.data(), *_flags,
                                       *_size, buffer_ptr, *_errcode_ret);
  if (buffer_ptr && (!FlagUseHostPtr(*_flags) || !FlagUseHostPtr(propFlags))) {
    DeallocateVector(SD()._buffers.back());
  }
  const auto isUsingHostPtr =
      (*_properties != nullptr && (propFlags & (CL_MEM_COPY_HOST_PTR | CL_MEM_USE_HOST_PTR))) ||
      (*_flags & (CL_MEM_COPY_HOST_PTR | CL_MEM_USE_HOST_PTR));
  if (ErrCodeSuccess(*_errcode_ret) && !isUsingHostPtr &&
      CheckCfgZeroInitialization(Configurator::Get(), IsReadOnlyBuffer(*_flags, *_properties))) {
    const auto device = GetGpuDevice();
    const auto commandQueue = GetCommandQueue(*_context, device);
    ZeroInitializeBuffer(commandQueue, *_return_value, size);
  }
}

inline void clCreateProgramWithSource_RUNWRAP(CFunction* _token,
                                              Ccl_program& _return_value,
                                              Ccl_context& _context,
                                              Ccl_uint& _count,
                                              CProgramSource& _strings,
                                              Csize_t::CSArray& _lengths,
                                              CCLResult::CSArray& _errcode_ret) {
  auto lengths = Configurator::Get().opencl.player.removeSourceLengths ? nullptr : *_lengths;
  _return_value.Assign(
      drvOcl.clCreateProgramWithSource(*_context, *_count, *_strings, lengths, *_errcode_ret));
  clCreateProgramWithSource_SD(_token, *_return_value, *_context, *_count, *_strings, lengths,
                               *_errcode_ret);
}

inline void clEnqueueNDCountKernelINTEL_RUNWRAP(CCLResult& _return_value,
                                                Ccl_command_queue& _command_queue,
                                                Ccl_kernel& _kernel,
                                                Ccl_uint& _workDim,
                                                Csize_t::CSArray& _globalWorkOffset,
                                                Csize_t::CSArray& _workGroupCount,
                                                Csize_t::CSArray& _localWorkSize,
                                                Ccl_uint& _numEventsInWaitList,
                                                Ccl_event::CSArray& _eventWaitList,
                                                Ccl_event::CSMapArray& _event) {
  CGits::Instance().KernelCountUp();
  auto& cfg = Configurator::Get().opencl.player;
  if (cfg.aubSignaturesCL) {
    InjectKernelArgOperations(*_kernel, *_command_queue, *_event);
  }
  cl_event new_event = nullptr;
  TranslatePointers();
  _return_value.Value() = drvOcl.clEnqueueNDCountKernelINTEL(
      *_command_queue, *_kernel, *_workDim, *_globalWorkOffset, *_workGroupCount, *_localWorkSize,
      *_numEventsInWaitList, *_eventWaitList, *_event ? *_event : &new_event);
  clEnqueueNDCountKernelINTEL_SD(
      *_return_value, *_command_queue, *_kernel, *_workDim, *_globalWorkOffset, *_workGroupCount,
      *_localWorkSize, *_numEventsInWaitList, *_eventWaitList, *_event ? *_event : &new_event);
}

inline void clCreateImageWithPropertiesINTEL_RUNWRAP(Ccl_mem& _return_value,
                                                     Ccl_context& _context,
                                                     Ccl_mem_properties_intel::CSArray& _properties,
                                                     Ccl_mem_flags& _flags,
                                                     Ccl_image_format::CSArray& _image_format,
                                                     Ccl_image_desc::CSArray& _image_desc,
                                                     CAsyncBinaryData& _host_ptr,
                                                     CCLResult::CSArray& _errcode_ret) {
  cl_image_desc image_desc = **_image_desc;
  if ((image_desc.image_type == CL_MEM_OBJECT_IMAGE1D_BUFFER ||
       image_desc.image_type == CL_MEM_OBJECT_IMAGE2D) &&
      image_desc.mem_object) {
    image_desc.mem_object = Ccl_mem::GetMapping(image_desc.mem_object);
  }
  const auto size = CountImageSize(**_image_format, image_desc);
  auto mappedProps = MapMemPropertiesIntel(_properties);
  CBinaryResource::PointerProxy ptr = *_host_ptr;
  char* buffer_ptr;
  if ((const void*)ptr) {
    SD()._buffers.emplace_back((const char*)ptr, (const char*)ptr + size);
    buffer_ptr = SD()._buffers.back().data();
  } else {
    buffer_ptr = nullptr;
  }
  _return_value.Assign(drvOcl.clCreateImageWithPropertiesINTEL(*_context, mappedProps.data(),
                                                               *_flags, *_image_format, &image_desc,
                                                               buffer_ptr, *_errcode_ret));
  clCreateImageWithPropertiesINTEL_SD(*_return_value, *_context, mappedProps.data(), *_flags,
                                      *_image_format, &image_desc, buffer_ptr, *_errcode_ret);
  if (buffer_ptr && (!FlagUseHostPtr(*_flags) ||
                     !FlagUseHostPtr(GetPropertyVal(mappedProps.data(), CL_MEM_FLAGS)))) {
    DeallocateVector(SD()._buffers.back());
  }
  const auto isUsingHostPtr =
      (*_properties != nullptr && (GetPropertyVal(mappedProps.data(), CL_MEM_FLAGS) &
                                   (CL_MEM_COPY_HOST_PTR | CL_MEM_USE_HOST_PTR))) ||
      (*_flags & (CL_MEM_COPY_HOST_PTR | CL_MEM_USE_HOST_PTR));
  if (ErrCodeSuccess(*_errcode_ret) && !isUsingHostPtr &&
      CheckCfgZeroInitialization(Configurator::Get(),
                                 IsReadOnlyBuffer(*_flags, mappedProps.data()))) {
    const auto device = GetGpuDevice();
    const auto commandQueue = GetCommandQueue(*_context, device);
    ZeroInitializeImage(commandQueue, *_return_value, size, image_desc.image_width,
                        image_desc.image_height, image_desc.image_depth, image_desc.image_row_pitch,
                        image_desc.image_slice_pitch);
  }
}

inline void clGetDeviceFunctionPointerINTEL_RUNWRAP(CCLResult& _return_value,
                                                    Ccl_device_id& _device,
                                                    Ccl_program& _program,
                                                    Cchar::CSArray& _function_name,
                                                    CCLMappedPtr& _function_pointer_ret) {
  cl_ulong funcPointer = 0;
  _return_value.Value() =
      drvOcl.clGetDeviceFunctionPointerINTEL(*_device, *_program, *_function_name, &funcPointer);
  CCLMappedPtr::AddMapping(_function_pointer_ret.Original(), reinterpret_cast<void*>(funcPointer));
}

inline void clDeviceMemAllocINTEL_RUNWRAP(CCLMappedPtr& _return_value,
                                          Ccl_context& _context,
                                          Ccl_device_id& _device,
                                          Ccl_mem_properties_intel::CSArray& _properties,
                                          Csize_t& _size,
                                          Ccl_uint& _alignment,
                                          CCLResult::CSArray& _errcode_ret) {
  auto modProps = MapMemPropertiesIntel(_properties);
  _return_value.Assign(drvOcl.clDeviceMemAllocINTEL(*_context, *_device, modProps.data(), *_size,
                                                    *_alignment, *_errcode_ret));
  if (ErrCodeSuccess(*_errcode_ret) &&
      CheckCfgZeroInitialization(Configurator::Get(), IsReadOnlyBuffer(0, modProps.data()))) {
    const auto device = GetGpuDevice();
    const auto commandQueue = GetCommandQueue(*_context, device);
    ZeroInitializeUsm(commandQueue, *_return_value, *_size, UnifiedMemoryType::device);
  }
  clDeviceMemAllocINTEL_SD(*_return_value, *_context, *_device, modProps.data(), *_size,
                           *_alignment, *_errcode_ret);
}

inline void clHostMemAllocINTEL_RUNWRAP(CCLMappedPtr& _return_value,
                                        Ccl_context& _context,
                                        Ccl_mem_properties_intel::CSArray& _properties,
                                        Csize_t& _size,
                                        Ccl_uint& _alignment,
                                        CCLResult::CSArray& _errcode_ret) {
  auto modProps = MapMemPropertiesIntel(_properties);
  _return_value.Assign(
      drvOcl.clHostMemAllocINTEL(*_context, modProps.data(), *_size, *_alignment, *_errcode_ret));
  if (ErrCodeSuccess(*_errcode_ret) &&
      CheckCfgZeroInitialization(Configurator::Get(), IsReadOnlyBuffer(0, modProps.data()))) {
    ZeroInitializeUsm(nullptr, *_return_value, *_size, UnifiedMemoryType::host);
  }
  clHostMemAllocINTEL_SD(*_return_value, *_context, modProps.data(), *_size, *_alignment,
                         *_errcode_ret);
}

inline void clSharedMemAllocINTEL_RUNWRAP(CCLMappedPtr& _return_value,
                                          Ccl_context& _context,
                                          Ccl_device_id& _device,
                                          Ccl_mem_properties_intel::CSArray& _properties,
                                          Csize_t& _size,
                                          Ccl_uint& _alignment,
                                          CCLResult::CSArray& _errcode_ret) {
  auto modProps = MapMemPropertiesIntel(_properties);
  _return_value.Assign(drvOcl.clSharedMemAllocINTEL(*_context, *_device, modProps.data(), *_size,
                                                    *_alignment, *_errcode_ret));
  if (ErrCodeSuccess(*_errcode_ret) &&
      CheckCfgZeroInitialization(Configurator::Get(), IsReadOnlyBuffer(0, modProps.data()))) {
    ZeroInitializeUsm(nullptr, *_return_value, *_size, UnifiedMemoryType::shared);
  }
  clSharedMemAllocINTEL_SD(*_return_value, *_context, *_device, modProps.data(), *_size,
                           *_alignment, *_errcode_ret);
}

inline void clEnqueueSVMMigrateMem_RUNWRAP(CCLResult& _return_value,
                                           Ccl_command_queue& _command_queue,
                                           Ccl_uint& _num_svm_pointers,
                                           CCLMappedPtr::CSArray& _svm_pointers,
                                           Csize_t::CSArray& _sizes,
                                           Ccl_mem_migration_flags& _flags,
                                           Ccl_uint& _num_events_in_wait_list,
                                           Ccl_event::CSArray& _event_wait_list,
                                           Ccl_event::CSMapArray& _event) {
  std::vector<const void*> svmPtrs;
  for (auto i = 0u; i < *_num_svm_pointers; i++) {
    const auto svmPointersOriginal = _svm_pointers.Original();
    if (svmPointersOriginal != nullptr) {
      const auto allocInfo = GetOriginalMappedPtrFromRegion(svmPointersOriginal[i]);
      const auto ptrValue = GetOffsetPointer(allocInfo.first, allocInfo.second);
      svmPtrs.push_back(ptrValue);
    } else {
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
  }
  _return_value.Value() =
      drvOcl.clEnqueueSVMMigrateMem(*_command_queue, *_num_svm_pointers, svmPtrs.data(), *_sizes,
                                    *_flags, *_num_events_in_wait_list, *_event_wait_list, *_event);
  clEnqueueSVMMigrateMem_SD(*_return_value, *_command_queue, *_num_svm_pointers, svmPtrs.data(),
                            *_sizes, *_flags, *_num_events_in_wait_list, *_event_wait_list,
                            *_event);
}

inline void clGetDeviceIDs_RUNWRAP(CCLResult& _return_value,
                                   Ccl_platform_id& _platform,
                                   Ccl_device_type& _device_type,
                                   Ccl_uint& _num_entries,
                                   Ccl_device_id::CSMapArray& _devices,
                                   Ccl_uint::CSArray& _num_devices) {
  drvOcl.Initialize();
  const auto* originalPtr = _devices.Original();
  if (*_num_entries > 0U && originalPtr != nullptr && _devices.Size() >= *_num_entries) {
    // saving original platform to device behavior.
    for (auto i = 0U; i < *_num_entries; i++) {
      SD().originalPlaybackPlatforms[_platform.Original()].insert(originalPtr[i]);
    }
  }
  const auto originalSuccess = ErrCodeSuccess(*_return_value);
  _return_value.Value() =
      drvOcl.clGetDeviceIDs(*_platform, *_device_type, *_num_entries, *_devices, *_num_devices);
  const auto isExplicitDeviceGetter =
      *_platform != nullptr && *_num_entries > 0U && *_device_type != CL_DEVICE_TYPE_ALL;
  const auto isReturnValueDifferent = originalSuccess && !ErrCodeSuccess(*_return_value);
  if (isExplicitDeviceGetter && isReturnValueDifferent) {
    bool isRemappingPossible = false;
    for (const auto& state : SD()._platformIDStates) {
      const auto device = state.second->GetDeviceType(*_device_type);
      if (device != nullptr) {
        Ccl_platform_id::AddMapping(_platform.Original(), state.first);
        isRemappingPossible = true;
        break;
      }
    }
    if (isRemappingPossible) {
      LOG_TRACEV << "Original application had successfuly asked for device, "
                    "remapping platform to the original application behavior.";
      _return_value.Value() =
          drvOcl.clGetDeviceIDs(*_platform, *_device_type, *_num_entries, *_devices, *_num_devices);
    }
  }
  clGetDeviceIDs_SD(*_return_value, *_platform, *_device_type, *_num_entries, *_devices,
                    *_num_devices);
}

inline void clCreateSubDevices_RUNWRAP(CCLResult& _return_value,
                                       Ccl_device_id& _in_device,
                                       Ccl_device_partition_property::CSArray& _properties,
                                       Ccl_uint& _num_entries,
                                       Ccl_device_id::CSMapArray& _out_devices,
                                       Ccl_uint::CSArray& _num_devices) {
  cl_uint num_devices_ret = 0;
  _return_value.Value() = drvOcl.clCreateSubDevices(*_in_device, *_properties, *_num_entries,
                                                    *_out_devices, &num_devices_ret);
  const auto* numDevices = *_num_devices;
  if (numDevices != nullptr && *numDevices > num_devices_ret) {
    Log(WARN) << "Stream was recorded on the device that can be "
                 "partitioned into more devices than this one: "
              << ToStringHelper(num_devices_ret);
  }
  if (!ErrCodeSuccess(*_return_value) && *_num_entries > 0 && *_in_device != nullptr &&
      *_out_devices != nullptr) {
    Log(WARN) << "Adding a mapping of all out-devices to the primary in-device";
    for (uint32_t i = 0U; i < static_cast<uint32_t>(*_num_entries); i++) {
      Ccl_device_id::AddMapping(_out_devices._array[i], *_in_device);
    }
  }
  clCreateSubDevices_SD(*_return_value, *_in_device, *_properties, *_num_entries, *_out_devices,
                        &num_devices_ret);
}

inline void clSetKernelArgSVMPointer_RUNWRAP(CCLResult& _return_value,
                                             Ccl_kernel& _kernel,
                                             Ccl_uint& _arg_index,
                                             CCLMappedPtr& _arg_value) {
  const auto allocInfo = GetOriginalMappedPtrFromRegion(_arg_value.Original());
  void* svmPtr = GetOffsetPointer(allocInfo.first, allocInfo.second);
  if (svmPtr == nullptr) {
    Log(WARN) << "Couldn't find mapping for SVM/USM pointer, original pointer: "
              << _arg_value.Original();
  }
  _return_value.Value() = drvOcl.clSetKernelArgSVMPointer(*_kernel, *_arg_index, svmPtr);
  clSetKernelArgSVMPointer_SD(*_return_value, *_kernel, *_arg_index, svmPtr);
}

inline void clEnqueueSVMMap_RUNWRAP(CCLResult& _return_value,
                                    Ccl_command_queue& _command_queue,
                                    Ccl_bool& _blocking_map,
                                    Ccl_map_flags& _flags,
                                    CCLMappedPtr& _svm_ptr,
                                    Csize_t& _size,
                                    Ccl_uint& _num_events_in_wait_list,
                                    Ccl_event::CSArray& _event_wait_list,
                                    Ccl_event::CSMapArray& _event) {
  const auto allocInfo = GetOriginalMappedPtrFromRegion(_svm_ptr.Original());
  void* svmPtr = GetOffsetPointer(allocInfo.first, allocInfo.second);
  _return_value.Value() =
      drvOcl.clEnqueueSVMMap(*_command_queue, *_blocking_map, *_flags, svmPtr, *_size,
                             *_num_events_in_wait_list, *_event_wait_list, *_event);
  clEnqueueSVMMap_SD(*_return_value, *_command_queue, *_blocking_map, *_flags, svmPtr, *_size,
                     *_num_events_in_wait_list, *_event_wait_list, *_event);
}

inline void clEnqueueSVMUnmap_RUNWRAP(CCLResult& _return_value,
                                      Ccl_command_queue& _command_queue,
                                      CCLMappedPtr& _svm_ptr,
                                      Ccl_uint& _num_events_in_wait_list,
                                      Ccl_event::CSArray& _event_wait_list,
                                      Ccl_event::CSMapArray& _event) {
  const auto allocInfo = GetOriginalMappedPtrFromRegion(_svm_ptr.Original());
  void* svmPtr = GetOffsetPointer(allocInfo.first, allocInfo.second);
  _return_value.Value() = drvOcl.clEnqueueSVMUnmap(
      *_command_queue, svmPtr, *_num_events_in_wait_list, *_event_wait_list, *_event);
  clEnqueueSVMUnmap_SD(*_return_value, *_command_queue, svmPtr, *_num_events_in_wait_list,
                       *_event_wait_list, *_event);
}

inline void clSVMAlloc_RUNWRAP(CCLMappedPtr& _return_value,
                               Ccl_context& _context,
                               Ccl_svm_mem_flags& _flags,
                               Csize_t& _size,
                               Ccl_uint& _alignment) {
  _return_value.Assign(drvOcl.clSVMAlloc(*_context, *_flags, *_size, *_alignment));
  if (*_return_value != nullptr &&
      CheckCfgZeroInitialization(Configurator::Get(), IsReadOnlyBuffer(*_flags))) {
    const auto device = GetGpuDevice();
    const auto commandQueue = GetCommandQueue(*_context, device);
    ZeroInitializeSvm(commandQueue, *_return_value, *_size, *_flags & CL_MEM_SVM_FINE_GRAIN_BUFFER);
  }
  clSVMAlloc_SD(*_return_value, *_context, *_flags, *_size, *_alignment);
}

inline void clGitsIndirectAllocationOffsets_RUNWRAP(CCLMappedPtr& _pAlloc,
                                                    Cuint32_t& _numOffsets,
                                                    Csize_t::CSArray& _pOffsets) {
  Log_clGitsIndirectAllocationOffsets(*_pAlloc, *_numOffsets, *_pOffsets);
  clGitsIndirectAllocationOffsets_SD(*_pAlloc, *_numOffsets, *_pOffsets);
}

inline void clCreateProgramWithBinary_V1_RUNWRAP(CFunction* token,
                                                 Ccl_program& _return_value,
                                                 Ccl_context& _context,
                                                 Ccl_uint& _num_devices,
                                                 Ccl_device_id::CSArray& _device_list,
                                                 Csize_t::CSArray& _lengths,
                                                 CBinariesArray_V1& _binaries,
                                                 CCLResult::CSArray& _binary_status,
                                                 CCLResult::CSArray& _errcode_ret) {
  const auto usingProgramLink = _binaries.GetProgramBinaryLink() == ProgramBinaryLink::program;
  auto* program =
      usingProgramLink ? Ccl_program::GetMapping(_binaries.GetProgramOriginal()) : nullptr;
  const auto* lengths = *_lengths;
  if (usingProgramLink) {
    lengths = SD().GetProgramState(program, EXCEPTION_MESSAGE).BinarySizes();
    LOG_TRACEV << "Detected available program connection. Linking to the original: "
               << ToStringHelper(_binaries.GetProgramOriginal())
               << " with mapping: " << ToStringHelper(program);
  }
  _return_value.Assign(drvOcl.clCreateProgramWithBinary(*_context, *_num_devices, *_device_list,
                                                        lengths, *_binaries, *_binary_status,
                                                        *_errcode_ret));
  clCreateProgramWithBinary_SD(token, *_return_value, *_context, *_num_devices, *_device_list,
                               lengths, *_binaries, *_binary_status, *_errcode_ret);
  if (program != nullptr) {
    SD().GetProgramState(*_return_value, EXCEPTION_MESSAGE).SetBinaryLinkedProgram(program);
  }
}

inline void clGetDeviceInfo_RUNWRAP(CCLResult& _return_value,
                                    Ccl_device_id& _device,
                                    Ccl_device_info& _param_name,
                                    Csize_t& _param_value_size,
                                    CBinaryData& _param_value,
                                    Csize_t::CSArray& _param_value_size_ret) {
  cl_device_type originalDeviceType = CL_DEVICE_TYPE_ALL;
  if (*_param_name == CL_DEVICE_TYPE && *_param_value_size > 0U) {
    originalDeviceType = *reinterpret_cast<cl_device_type*>(*_param_value);
  }
  const auto originalRetVal = *_return_value;
  _return_value.Value() = drvOcl.clGetDeviceInfo(*_device, *_param_name, *_param_value_size,
                                                 *_param_value, *_param_value_size_ret);
  clGetDeviceInfo_SD(*_return_value, *_device, *_param_name, *_param_value_size, *_param_value,
                     *_param_value_size_ret);
  const auto isDeviceTypeQuery = *_param_value_size > 0U && *_param_name == CL_DEVICE_TYPE;
  if (isDeviceTypeQuery) {
    const auto differentParamValue =
        originalDeviceType != *reinterpret_cast<cl_device_type*>(*_param_value);
    const auto differentRetVal = ErrCodeSuccess(originalRetVal) != ErrCodeSuccess(*_return_value);
    if (differentParamValue || differentRetVal) {
      if (differentParamValue) {
        LOG_TRACEV << "Looking for original application device, current: "
                   << cl_device_typeToString(*reinterpret_cast<cl_device_type*>(*_param_value))
                   << ", original: " << cl_device_typeToString(originalDeviceType);
      }
      for (const auto& state : SD()._platformIDStates) {
        const auto device = state.second->GetDeviceType(originalDeviceType);
        if (device != nullptr) {
          for (const auto& originalMap : SD().originalPlaybackPlatforms) {
            for (const auto& deviceOriginal : originalMap.second) {
              if (_device.Original() == deviceOriginal) {
                LOG_TRACEV << "Original application received different "
                              "device type info, "
                              "changing device and platform mapping";
                Ccl_platform_id::AddMapping(originalMap.first, state.first);
                Ccl_device_id::AddMapping(_device.Original(), device);
                break;
              }
            }
          }
        }
      }
    }
  }
}

inline void clSetKernelArg_V1_RUNWRAP(CCLResult& _return_value,
                                      Ccl_kernel& _kernel,
                                      Ccl_uint& _arg_index,
                                      Csize_t& _arg_size,
                                      CKernelArgValue_V1& _arg_value) {
  size_t size = *_arg_size;
  if (_arg_value.Value() != *_arg_value && size == 0U) {
    size = sizeof(void*);
  }
  _return_value.Value() = drvOcl.clSetKernelArg(*_kernel, *_arg_index, size, *_arg_value);
  clSetKernelArg_SD(*_return_value, *_kernel, *_arg_index, size, *_arg_value);
}

inline void clEnqueueMemcpyINTEL_RUNWRAP(CCLResult& _return_value,
                                         Ccl_command_queue& _command_queue,
                                         Ccl_bool& _blocking,
                                         CUSMPtr& _dst_ptr,
                                         CUSMPtr& _src_ptr,
                                         Csize_t& _size,
                                         Ccl_uint& _num_events_in_wait_list,
                                         Ccl_event::CSArray& _event_wait_list,
                                         Ccl_event::CSMapArray& _event) {
  auto& sd = SD();
  _return_value.Value() =
      drvOcl.clEnqueueMemcpyINTEL(*_command_queue, *_blocking, *_dst_ptr, *_src_ptr, *_size,
                                  *_num_events_in_wait_list, *_event_wait_list, *_event);

  clEnqueueMemcpyINTEL_SD(*_return_value, *_command_queue, *_blocking, *_dst_ptr, *_src_ptr, *_size,
                          *_num_events_in_wait_list, *_event_wait_list, *_event);
  sd.deallocationHandler.AddToResourcesInExecution(*_command_queue, _dst_ptr, *_blocking);
  sd.deallocationHandler.AddToResourcesInExecution(*_command_queue, _src_ptr, *_blocking);
}

inline void clEnqueueSVMMemcpy_V1_RUNWRAP(CCLResult& _return_value,
                                          Ccl_command_queue& _command_queue,
                                          Ccl_bool& _blocking_copy,
                                          CSVMPtr_V1& _dst_ptr,
                                          CSVMPtr_V1& _src_ptr,
                                          Csize_t& _size,
                                          Ccl_uint& _num_events_in_wait_list,
                                          Ccl_event::CSArray& _event_wait_list,
                                          Ccl_event::CSMapArray& _event) {
  auto& sd = SD();
  _return_value.Value() =
      drvOcl.clEnqueueSVMMemcpy(*_command_queue, *_blocking_copy, *_dst_ptr, *_src_ptr, *_size,
                                *_num_events_in_wait_list, *_event_wait_list, *_event);
  clEnqueueSVMMemcpy_SD(*_return_value, *_command_queue, *_blocking_copy, *_dst_ptr, *_src_ptr,
                        *_size, *_num_events_in_wait_list, *_event_wait_list, *_event);
  sd.deallocationHandler.AddToResourcesInExecution(*_command_queue, _dst_ptr, *_blocking_copy);
  sd.deallocationHandler.AddToResourcesInExecution(*_command_queue, _src_ptr, *_blocking_copy);
}

} // namespace OpenCL
} // namespace gits

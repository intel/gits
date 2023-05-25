// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   openclRecorderSubWrappers.h
*
* @brief API RUNWRAP implementations.
*/

#pragma once

#include "openclFunctionsAuto.h"
#include "openclStateTracking.h"
#include "openclTools.h"
#include "openclHelperFunctions.h"

#include "recorder.h"

namespace gits {
namespace OpenCL {
namespace {
void ScheduleMemoryUpdate(CRecorder& recorder, cl_kernel kernel) {
  if (!recorder.Running()) {
    return;
  }
  for (const auto& usmState : SD()._usmAllocStates) {
    if (usmState.second->toUpdate[kernel] &&
        (usmState.second->sniffedRegionHandle &&
         !(**usmState.second->sniffedRegionHandle).GetTouchedPages().empty())) {
      recorder.Schedule(new CGitsClMemoryUpdate(usmState.first));
    }
  }
  for (const auto& svmState : SD()._svmAllocStates) {
    if (svmState.second->toUpdate[kernel] &&
        (svmState.second->sniffedRegionHandle &&
         !(**svmState.second->sniffedRegionHandle).GetTouchedPages().empty())) {
      recorder.Schedule(new CGitsClMemoryUpdate(svmState.first));
    }
  }
}
bool CheckWhetherUpdateUSM(const void* ptr) {
  bool update = false;
  void* usmPtr = GetSvmOrUsmFromRegion(const_cast<void*>(ptr)).first;
  if (SD().CheckIfUSMAllocExists(usmPtr)) {
    const auto& allocState = SD().GetUSMAllocState(usmPtr, EXCEPTION_MESSAGE);
    update = (allocState.type != UnifiedMemoryType::device) &&
             !(**allocState.sniffedRegionHandle).GetTouchedPages().empty();
  } else if (SD().CheckIfSVMAllocExists(usmPtr)) {
    const auto& allocState = SD().GetSVMAllocState(usmPtr, EXCEPTION_MESSAGE);
    update = (allocState.sniffedRegionHandle &&
              !(**allocState.sniffedRegionHandle).GetTouchedPages().empty());
  }
  return update;
}
cl_command_queue GetCommandQueueRec(const cl_context& context, CRecorder* recorder) {
  const auto device = GetGpuDevice();
  cl_int err = CL_INVALID_COMMAND_QUEUE;
  const auto commandQueue = GetCommandQueue(context, device, &err);
  if (ErrCodeSuccess(err)) {
    recorder->Schedule(new CclCreateCommandQueue(commandQueue, context, device, 0, &err));
  }
  return commandQueue;
}
} // namespace
inline void clCreateContext_RECWRAP(
    CRecorder& recorder,
    cl_context return_value,
    const cl_context_properties* properties,
    cl_uint num_devices,
    const cl_device_id* devices,
    void(CL_CALLBACK* pfn_notify)(const char*, const void*, size_t, void*),
    void* user_data,
    cl_int* errcode_ret) {
  std::vector<cl_context_properties> unsharingPropsVec;
  const cl_context_properties* props = properties;
  if (recorder.Running()) {
    if (props != nullptr) {
      const auto& cfg = Config::Get();
      if (IsGLUnsharingEnabled(cfg)) {
        unsharingPropsVec = RemoveGLSharingContextProperties(props);
        props = unsharingPropsVec.data();
      }
      if (IsDXUnsharingEnabled(cfg)) {
        unsharingPropsVec = RemoveDXSharingContextProperties(props);
        props = unsharingPropsVec.data();
      }
    }
    recorder.Schedule(new CclCreateContext(return_value, props, num_devices, devices, pfn_notify,
                                           user_data, errcode_ret));
  }
  clCreateContext_SD(return_value, props, num_devices, devices, pfn_notify, user_data, errcode_ret);
}

inline void clCreateContextFromType_RECWRAP(
    CRecorder& recorder,
    cl_context return_value,
    const cl_context_properties* properties,
    cl_device_type device_type,
    void(CL_CALLBACK* pfn_notify)(const char*, const void*, size_t, void*),
    void* user_data,
    cl_int* errcode_ret) {
  std::vector<cl_context_properties> unsharingPropsVec;
  const cl_context_properties* props = properties;
  if (recorder.Running()) {
    if (props != nullptr) {
      const auto opPlatform = ExtractPlatform(props);
      const auto& oclIFace = gits::CGits::Instance().apis.IfaceCompute();
      if (opPlatform.is_initialized() && opPlatform.get() == nullptr &&
          !oclIFace.MemorySnifferInstall()) {
        Log(WARN) << "Memory Sniffer installation failed";
      }
      const auto& cfg = Config::Get();
      if (IsGLUnsharingEnabled(cfg)) {
        unsharingPropsVec = RemoveGLSharingContextProperties(props);
        props = unsharingPropsVec.data();
      }
      if (IsDXUnsharingEnabled(cfg)) {
        unsharingPropsVec = RemoveDXSharingContextProperties(props);
        props = unsharingPropsVec.data();
      }
    }
    recorder.Schedule(new CclCreateContextFromType(return_value, props, device_type, pfn_notify,
                                                   user_data, errcode_ret));
  }
  clCreateContextFromType_SD(return_value, props, device_type, pfn_notify, user_data, errcode_ret);
}

inline void clCreateFromGLBuffer_RECWRAP(CRecorder& recorder,
                                         cl_mem return_value,
                                         cl_context context,
                                         cl_mem_flags flags,
                                         cl_GLuint bufobj,
                                         cl_int* errcode_ret) {
  if (recorder.Running()) {
    if (IsGLUnsharingEnabled(Config::Get())) {
      recorder.Schedule(NewTokenPtrCreateCLMem(context, return_value, flags, CL_MEM_OBJECT_BUFFER));
    } else {
      recorder.Schedule(
          new CclCreateFromGLBuffer(return_value, context, flags, bufobj, errcode_ret));
    }
  }
  clCreateFromGLBuffer_SD(return_value, context, flags, bufobj, errcode_ret);
}

inline void clCreateFromGLRenderbuffer_RECWRAP(CRecorder& recorder,
                                               cl_mem return_value,
                                               cl_context context,
                                               cl_mem_flags flags,
                                               cl_GLuint bufobj,
                                               cl_int* errcode_ret) {
  if (recorder.Running()) {
    if (IsGLUnsharingEnabled(Config::Get())) {
      recorder.Schedule(
          NewTokenPtrCreateCLMem(context, return_value, flags, CL_MEM_OBJECT_IMAGE2D));
    } else {
      recorder.Schedule(
          new CclCreateFromGLRenderbuffer(return_value, context, flags, bufobj, errcode_ret));
    }
  }
  clCreateFromGLRenderbuffer_SD(return_value, context, flags, bufobj, errcode_ret);
}

inline void clCreateFromGLTexture_RECWRAP(CRecorder& recorder,
                                          cl_mem return_value,
                                          cl_context context,
                                          cl_mem_flags flags,
                                          cl_GLenum target,
                                          cl_GLint miplevel,
                                          cl_GLuint texture,
                                          cl_int* errcode_ret) {
  if (recorder.Running()) {
    if (IsGLUnsharingEnabled(Config::Get())) {
      auto type = TextureGLEnumToCLMemType(target);
      recorder.Schedule(NewTokenPtrCreateCLMem(context, return_value, flags, type));
    } else {
      recorder.Schedule(new CclCreateFromGLTexture(return_value, context, flags, target, miplevel,
                                                   texture, errcode_ret));
    }
  }
  clCreateFromGLTexture_SD(return_value, context, flags, target, miplevel, texture, errcode_ret);
}

inline void clCreateFromGLTexture2D_RECWRAP(CRecorder& recorder,
                                            cl_mem return_value,
                                            cl_context context,
                                            cl_mem_flags flags,
                                            cl_GLenum target,
                                            cl_GLint miplevel,
                                            cl_GLuint texture,
                                            cl_int* errcode_ret) {
  if (recorder.Running()) {
    if (IsGLUnsharingEnabled(Config::Get())) {
      recorder.Schedule(
          NewTokenPtrCreateCLMem(context, return_value, flags, CL_MEM_OBJECT_IMAGE2D));
    } else {
      recorder.Schedule(new CclCreateFromGLTexture2D(return_value, context, flags, target, miplevel,
                                                     texture, errcode_ret));
    }
  }
  clCreateFromGLTexture2D_SD(return_value, context, flags, target, miplevel, texture, errcode_ret);
}

inline void clCreateFromGLTexture3D_RECWRAP(CRecorder& recorder,
                                            cl_mem return_value,
                                            cl_context context,
                                            cl_mem_flags flags,
                                            cl_GLenum target,
                                            cl_GLint miplevel,
                                            cl_GLuint texture,
                                            cl_int* errcode_ret) {
  if (recorder.Running()) {
    if (IsGLUnsharingEnabled(Config::Get())) {
      recorder.Schedule(
          NewTokenPtrCreateCLMem(context, return_value, flags, CL_MEM_OBJECT_IMAGE3D));
    } else {
      recorder.Schedule(new CclCreateFromGLTexture3D(return_value, context, flags, target, miplevel,
                                                     texture, errcode_ret));
    }
  }
  clCreateFromGLTexture3D_SD(return_value, context, flags, target, miplevel, texture, errcode_ret);
}

inline void clEnqueueAcquireGLObjects_RECWRAP(CRecorder& recorder,
                                              cl_int return_value,
                                              cl_command_queue command_queue,
                                              cl_uint num_objects,
                                              const cl_mem* mem_objects,
                                              cl_uint num_events_in_wait_list,
                                              const cl_event* event_wait_list,
                                              cl_event* event) {
  if (recorder.Running()) {
    if (IsGLUnsharingEnabled(Config::Get())) {
      //TODO: Inject verification calls?
    } else {
      recorder.Schedule(new CclEnqueueAcquireGLObjects(return_value, command_queue, num_objects,
                                                       mem_objects, num_events_in_wait_list,
                                                       event_wait_list, event));
    }
  }
  clEnqueueAcquireGLObjects_SD(return_value, command_queue, num_objects, mem_objects,
                               num_events_in_wait_list, event_wait_list, event);
}

inline void clEnqueueReleaseGLObjects_RECWRAP(CRecorder& recorder,
                                              cl_int return_value,
                                              cl_command_queue command_queue,
                                              cl_uint num_objects,
                                              const cl_mem* mem_objects,
                                              cl_uint num_events_in_wait_list,
                                              const cl_event* event_wait_list,
                                              cl_event* event) {
  if (recorder.Running()) {
    if (IsGLUnsharingEnabled(Config::Get())) {
      //TODO: Inject verification calls?
    } else {
      recorder.Schedule(new CclEnqueueReleaseGLObjects(return_value, command_queue, num_objects,
                                                       mem_objects, num_events_in_wait_list,
                                                       event_wait_list, event));
    }
  }
  clEnqueueReleaseGLObjects_SD(return_value, command_queue, num_objects, mem_objects,
                               num_events_in_wait_list, event_wait_list, event);
}

inline void clGetExtensionFunctionAddress_RECWRAP(CRecorder& recorder,
                                                  void* return_value,
                                                  const char* function_name) {
  if (recorder.Running()) {
    const auto& cfg = Config::Get();
    if ((IsGLUnsharingEnabled(cfg) && IsGLSharingFunction(function_name)) ||
        (IsDXUnsharingEnabled(cfg) && IsDXSharingFunction(function_name))) {
      return;
    }
    recorder.Schedule(new CclGetExtensionFunctionAddress(return_value, function_name));
  }
}

inline void clGetExtensionFunctionAddressForPlatform_RECWRAP(CRecorder& recorder,
                                                             void* return_value,
                                                             cl_platform_id platform,
                                                             const char* function_name) {
  if (recorder.Running()) {
    const auto& cfg = Config::Get();
    if ((IsGLUnsharingEnabled(cfg) && IsGLSharingFunction(function_name)) ||
        (IsDXUnsharingEnabled(cfg) && IsDXSharingFunction(function_name))) {
      return;
    }
    recorder.Schedule(
        new CclGetExtensionFunctionAddressForPlatform(return_value, platform, function_name));
    clGetExtensionFunctionAddressForPlatform_SD(return_value, platform, function_name);
  }
}

inline void clGetGLObjectInfo_RECWRAP(CRecorder& recorder,
                                      cl_int return_value,
                                      cl_mem memobj,
                                      cl_gl_object_type* gl_object_type,
                                      cl_GLuint* gl_object_name) {
  if (recorder.Running()) {
    if (IsGLUnsharingEnabled(Config::Get())) {
      return;
    }
    recorder.Schedule(new CclGetGLObjectInfo(return_value, memobj, gl_object_type, gl_object_name));
  }
}

inline void clGetGLTextureInfo_RECWRAP(CRecorder& recorder,
                                       cl_int return_value,
                                       cl_mem memobj,
                                       cl_gl_texture_info param_name,
                                       size_t param_value_size,
                                       void* param_value,
                                       size_t* param_value_size_ret) {
  if (recorder.Running()) {
    if (IsGLUnsharingEnabled(Config::Get())) {
      return;
    }
    recorder.Schedule(new CclGetGLTextureInfo(return_value, memobj, param_name, param_value_size,
                                              param_value, param_value_size_ret));
  }
}

inline void clReleaseEvent_RECWRAP(CRecorder& recorder, cl_int return_value, cl_event event) {
  if (recorder.Running()) {
    const auto& eventState = SD()._eventStates[event];
    const auto& cfg = Config::Get();
    if ((IsGLUnsharingEnabled(cfg) && eventState->isGLSharingEvent) ||
        (IsDXUnsharingEnabled(cfg) && eventState->isDXSharingEvent)) {
      return;
    } else {
      recorder.Schedule(new CclReleaseEvent(return_value, event));
    }
  }
  clReleaseEvent_SD(return_value, event);
}

inline void clWaitForEvents_RECWRAP(CRecorder& recorder,
                                    cl_int return_value,
                                    cl_uint num_events,
                                    const cl_event* event_list) {
  if (recorder.Running()) {
    if (IsUnsharingEnabled(Config::Get())) {
      std::vector<cl_event> filteredEvents(FilterSharingEvents(num_events, event_list));
      recorder.Schedule(new CclWaitForEvents(
          return_value, static_cast<cl_uint>(filteredEvents.size()), filteredEvents.data()));
    } else {
      recorder.Schedule(new CclWaitForEvents(return_value, num_events, event_list));
    }
  }
}

inline void clGetGLContextInfoKHR_RECWRAP(CRecorder& recorder,
                                          cl_int return_value,
                                          const cl_context_properties* properties,
                                          cl_gl_context_info param_name,
                                          size_t param_value_size,
                                          void* param_value,
                                          size_t* param_value_size_ret) {
  if (recorder.Running()) {
    if (IsGLUnsharingEnabled(Config::Get())) {
      //Unsharing enabled - need to replace GL device query by standard device query for the same platform.
      if (return_value == CL_SUCCESS && (param_name == CL_DEVICES_FOR_GL_CONTEXT_KHR ||
                                         param_name == CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR)) {
        if (auto optionalPlatform = ExtractPlatform(properties)) {
          auto platform = *optionalPlatform;
          recorder.Schedule(NewTokenPtrGetDevices(platform));
        }
      }
    } else {
      recorder.Schedule(new CclGetGLContextInfoKHR(return_value, properties, param_name,
                                                   param_value_size, param_value,
                                                   param_value_size_ret));
    }
  }
}

inline void clEnqueueNDRangeKernel_RECWRAP_PRE(CRecorder& recorder,
                                               cl_int return_value,
                                               cl_command_queue command_queue,
                                               cl_kernel kernel,
                                               cl_uint work_dim,
                                               const size_t* global_work_offset,
                                               const size_t* global_work_size,
                                               const size_t* local_work_size,
                                               cl_uint num_events_in_wait_list,
                                               const cl_event* event_wait_list,
                                               cl_event* event) {
  gits::CGits::Instance().KernelCountUp();
  ScheduleMemoryUpdate(recorder, kernel);
  recorder.KernelBegin();
}

inline void clEnqueueNDRangeKernel_RECWRAP(CRecorder& recorder,
                                           cl_int return_value,
                                           cl_command_queue command_queue,
                                           cl_kernel kernel,
                                           cl_uint work_dim,
                                           const size_t* global_work_offset,
                                           const size_t* global_work_size,
                                           const size_t* local_work_size,
                                           cl_uint num_events_in_wait_list,
                                           const cl_event* event_wait_list,
                                           cl_event* event) {
  CFunction* _token = nullptr;
  if (recorder.Running()) {
    const auto& computeIface = CGits::Instance().apis.IfaceCompute();
    if (computeIface.CfgRec_IsSingleKernelMode()) {
      _token = new CclEnqueueNDRangeKernel(return_value, command_queue, kernel, work_dim,
                                           global_work_offset, global_work_size, local_work_size, 0,
                                           nullptr, nullptr);
    } else {
      if (IsUnsharingEnabled(Config::Get())) {
        std::vector<cl_event> filteredEvents(
            FilterSharingEvents(num_events_in_wait_list, event_wait_list));
        cl_uint num_filtered_events = static_cast<cl_uint>(filteredEvents.size());
        _token = new CclEnqueueNDRangeKernel(
            return_value, command_queue, kernel, work_dim, global_work_offset, global_work_size,
            local_work_size, num_filtered_events, GetPointerFromVector(filteredEvents), event);
      } else {
        _token = new CclEnqueueNDRangeKernel(return_value, command_queue, kernel, work_dim,
                                             global_work_offset, global_work_size, local_work_size,
                                             num_events_in_wait_list, event_wait_list, event);
      }
    }
    recorder.Schedule(_token);
  }
  clEnqueueNDRangeKernel_SD(return_value, command_queue, kernel, work_dim, global_work_offset,
                            global_work_size, local_work_size, num_events_in_wait_list,
                            event_wait_list, event);
  recorder.KernelEnd();
}

#ifdef GITS_PLATFORM_WINDOWS

inline void clCreateFromD3D10BufferKHR_RECWRAP(CRecorder& recorder,
                                               cl_mem return_value,
                                               cl_context context,
                                               cl_mem_flags flags,
                                               ID3D10Buffer* resource,
                                               cl_int* errcode_ret) {
  if (recorder.Running()) {
    if (IsDXUnsharingEnabled(Config::Get())) {
      recorder.Schedule(NewTokenPtrCreateCLMem(context, return_value, flags, CL_MEM_OBJECT_BUFFER));
    } else {
      recorder.Schedule(
          new CclCreateFromD3D10BufferKHR(return_value, context, flags, resource, errcode_ret));
    }
  }
  clCreateFromD3D10BufferKHR_SD(return_value, context, flags, resource, errcode_ret);
}

inline void clCreateFromD3D10Texture2DKHR_RECWRAP(CRecorder& recorder,
                                                  cl_mem return_value,
                                                  cl_context context,
                                                  cl_mem_flags flags,
                                                  ID3D10Texture2D* resource,
                                                  UINT subresource,
                                                  cl_int* errcode_ret) {
  if (recorder.Running()) {
    if (IsDXUnsharingEnabled(Config::Get())) {
      recorder.Schedule(
          NewTokenPtrCreateCLMem(context, return_value, flags, CL_MEM_OBJECT_IMAGE2D));
    } else {
      recorder.Schedule(new CclCreateFromD3D10Texture2DKHR(return_value, context, flags, resource,
                                                           subresource, errcode_ret));
    }
  }
  clCreateFromD3D10Texture2DKHR_SD(return_value, context, flags, resource, subresource,
                                   errcode_ret);
}

inline void clCreateFromD3D10Texture3DKHR_RECWRAP(CRecorder& recorder,
                                                  cl_mem return_value,
                                                  cl_context context,
                                                  cl_mem_flags flags,
                                                  ID3D10Texture3D* resource,
                                                  UINT subresource,
                                                  cl_int* errcode_ret) {
  if (recorder.Running()) {
    if (IsDXUnsharingEnabled(Config::Get())) {
      recorder.Schedule(
          NewTokenPtrCreateCLMem(context, return_value, flags, CL_MEM_OBJECT_IMAGE3D));
    } else {
      recorder.Schedule(new CclCreateFromD3D10Texture3DKHR(return_value, context, flags, resource,
                                                           subresource, errcode_ret));
    }
  }
  clCreateFromD3D10Texture3DKHR_SD(return_value, context, flags, resource, subresource,
                                   errcode_ret);
}

inline void clCreateFromD3D11BufferKHR_RECWRAP(CRecorder& recorder,
                                               cl_mem return_value,
                                               cl_context context,
                                               cl_mem_flags flags,
                                               ID3D11Buffer* resource,
                                               cl_int* errcode_ret) {
  if (recorder.Running()) {
    if (IsDXUnsharingEnabled(Config::Get())) {
      recorder.Schedule(NewTokenPtrCreateCLMem(context, return_value, flags, CL_MEM_OBJECT_BUFFER));
    } else {
      recorder.Schedule(
          new CclCreateFromD3D11BufferKHR(return_value, context, flags, resource, errcode_ret));
    }
  }
  clCreateFromD3D11BufferKHR_SD(return_value, context, flags, resource, errcode_ret);
}

inline void clCreateFromD3D11Texture2DKHR_RECWRAP(CRecorder& recorder,
                                                  cl_mem return_value,
                                                  cl_context context,
                                                  cl_mem_flags flags,
                                                  ID3D11Texture2D* resource,
                                                  UINT subresource,
                                                  cl_int* errcode_ret) {
  if (recorder.Running()) {
    if (IsDXUnsharingEnabled(Config::Get())) {
      recorder.Schedule(
          NewTokenPtrCreateCLMem(context, return_value, flags, CL_MEM_OBJECT_IMAGE2D));
    } else {
      recorder.Schedule(new CclCreateFromD3D11Texture2DKHR(return_value, context, flags, resource,
                                                           subresource, errcode_ret));
    }
  }
  clCreateFromD3D11Texture2DKHR_SD(return_value, context, flags, resource, subresource,
                                   errcode_ret);
}

inline void clCreateFromD3D11Texture3DKHR_RECWRAP(CRecorder& recorder,
                                                  cl_mem return_value,
                                                  cl_context context,
                                                  cl_mem_flags flags,
                                                  ID3D11Texture3D* resource,
                                                  UINT subresource,
                                                  cl_int* errcode_ret) {
  if (recorder.Running()) {
    if (IsDXUnsharingEnabled(Config::Get())) {
      recorder.Schedule(
          NewTokenPtrCreateCLMem(context, return_value, flags, CL_MEM_OBJECT_IMAGE3D));
    } else {
      recorder.Schedule(new CclCreateFromD3D11Texture3DKHR(return_value, context, flags, resource,
                                                           subresource, errcode_ret));
    }
  }
  clCreateFromD3D11Texture3DKHR_SD(return_value, context, flags, resource, subresource,
                                   errcode_ret);
}

inline void clCreateFromDX9MediaSurfaceINTEL_RECWRAP(CRecorder& recorder,
                                                     cl_mem return_value,
                                                     cl_context context,
                                                     cl_mem_flags flags,
                                                     IDirect3DSurface9* resource,
                                                     void* shared_handle,
                                                     UINT plane,
                                                     cl_int* errcode_ret) {
  if (recorder.Running()) {
    if (IsDXUnsharingEnabled(Config::Get())) {
      recorder.Schedule(NewTokenPtrCreateCLMem(context, return_value, flags, CL_MEM_OBJECT_BUFFER));
    } else {
      recorder.Schedule(new CclCreateFromDX9MediaSurfaceINTEL(
          return_value, context, flags, resource, shared_handle, plane, errcode_ret));
    }
  }
  clCreateFromDX9MediaSurfaceINTEL_SD(return_value, context, flags, resource, shared_handle, plane,
                                      errcode_ret);
}

inline void clEnqueueAcquireD3D10ObjectsKHR_RECWRAP(CRecorder& recorder,
                                                    cl_int return_value,
                                                    cl_command_queue command_queue,
                                                    cl_uint num_objects,
                                                    const cl_mem* mem_objects,
                                                    cl_uint num_events_in_wait_list,
                                                    const cl_event* event_wait_list,
                                                    cl_event* event) {
  if (recorder.Running()) {
    if (IsDXUnsharingEnabled(Config::Get())) {
    } else {
      recorder.Schedule(new CclEnqueueAcquireD3D10ObjectsKHR(
          return_value, command_queue, num_objects, mem_objects, num_events_in_wait_list,
          event_wait_list, event));
    }
  }
  clEnqueueAcquireD3D10ObjectsKHR_SD(return_value, command_queue, num_objects, mem_objects,
                                     num_events_in_wait_list, event_wait_list, event);
}

inline void clEnqueueAcquireD3D11ObjectsKHR_RECWRAP(CRecorder& recorder,
                                                    cl_int return_value,
                                                    cl_command_queue command_queue,
                                                    cl_uint num_objects,
                                                    const cl_mem* mem_objects,
                                                    cl_uint num_events_in_wait_list,
                                                    const cl_event* event_wait_list,
                                                    cl_event* event) {
  if (recorder.Running()) {
    if (IsDXUnsharingEnabled(Config::Get())) {
    } else {
      recorder.Schedule(new CclEnqueueAcquireD3D11ObjectsKHR(
          return_value, command_queue, num_objects, mem_objects, num_events_in_wait_list,
          event_wait_list, event));
    }
  }
  clEnqueueAcquireD3D10ObjectsKHR_SD(return_value, command_queue, num_objects, mem_objects,
                                     num_events_in_wait_list, event_wait_list, event);
}

inline void clEnqueueAcquireDX9ObjectsINTEL_RECWRAP(CRecorder& recorder,
                                                    cl_int return_value,
                                                    cl_command_queue command_queue,
                                                    cl_uint num_objects,
                                                    const cl_mem* mem_objects,
                                                    cl_uint num_events_in_wait_list,
                                                    const cl_event* event_wait_list,
                                                    cl_event* event) {
  if (recorder.Running()) {
    if (IsDXUnsharingEnabled(Config::Get())) {
    } else {
      recorder.Schedule(new CclEnqueueAcquireDX9ObjectsINTEL(
          return_value, command_queue, num_objects, mem_objects, num_events_in_wait_list,
          event_wait_list, event));
    }
  }
  clEnqueueAcquireD3D10ObjectsKHR_SD(return_value, command_queue, num_objects, mem_objects,
                                     num_events_in_wait_list, event_wait_list, event);
}

inline void clEnqueueReleaseD3D10ObjectsKHR_RECWRAP(CRecorder& recorder,
                                                    cl_int return_value,
                                                    cl_command_queue command_queue,
                                                    cl_uint num_objects,
                                                    const cl_mem* mem_objects,
                                                    cl_uint num_events_in_wait_list,
                                                    const cl_event* event_wait_list,
                                                    cl_event* event) {
  if (recorder.Running()) {
    if (IsDXUnsharingEnabled(Config::Get())) {
    } else {
      recorder.Schedule(new CclEnqueueReleaseD3D10ObjectsKHR(
          return_value, command_queue, num_objects, mem_objects, num_events_in_wait_list,
          event_wait_list, event));
    }
  }
  clEnqueueReleaseD3D10ObjectsKHR_SD(return_value, command_queue, num_objects, mem_objects,
                                     num_events_in_wait_list, event_wait_list, event);
}

inline void clEnqueueReleaseD3D11ObjectsKHR_RECWRAP(CRecorder& recorder,
                                                    cl_int return_value,
                                                    cl_command_queue command_queue,
                                                    cl_uint num_objects,
                                                    const cl_mem* mem_objects,
                                                    cl_uint num_events_in_wait_list,
                                                    const cl_event* event_wait_list,
                                                    cl_event* event) {
  if (recorder.Running()) {
    if (IsDXUnsharingEnabled(Config::Get())) {
    } else {
      recorder.Schedule(new CclEnqueueReleaseD3D11ObjectsKHR(
          return_value, command_queue, num_objects, mem_objects, num_events_in_wait_list,
          event_wait_list, event));
    }
  }
  clEnqueueReleaseD3D10ObjectsKHR_SD(return_value, command_queue, num_objects, mem_objects,
                                     num_events_in_wait_list, event_wait_list, event);
}

inline void clEnqueueReleaseDX9ObjectsINTEL_RECWRAP(CRecorder& recorder,
                                                    cl_int return_value,
                                                    cl_command_queue command_queue,
                                                    cl_uint num_objects,
                                                    const cl_mem* mem_objects,
                                                    cl_uint num_events_in_wait_list,
                                                    const cl_event* event_wait_list,
                                                    cl_event* event) {
  if (recorder.Running()) {
    if (IsDXUnsharingEnabled(Config::Get())) {
    } else {
      recorder.Schedule(new CclEnqueueReleaseDX9ObjectsINTEL(
          return_value, command_queue, num_objects, mem_objects, num_events_in_wait_list,
          event_wait_list, event));
    }
  }
  clEnqueueReleaseD3D10ObjectsKHR_SD(return_value, command_queue, num_objects, mem_objects,
                                     num_events_in_wait_list, event_wait_list, event);
}

inline void clGetDeviceIDsFromD3D10KHR_RECWRAP(CRecorder& recorder,
                                               cl_int return_value,
                                               cl_platform_id platform,
                                               cl_d3d10_device_source_khr d3d_device_source,
                                               void* d3d_object,
                                               cl_d3d10_device_set_khr d3d_device_set,
                                               cl_uint num_entries,
                                               cl_device_id* devices,
                                               cl_uint* num_devices) {
  if (recorder.Running()) {
    if (IsDXUnsharingEnabled(Config::Get())) {
      if (return_value == CL_SUCCESS) {
        recorder.Schedule(NewTokenPtrGetDevices(platform));
      }
    } else {
      recorder.Schedule(new CclGetDeviceIDsFromD3D10KHR(return_value, platform, d3d_device_source,
                                                        d3d_object, d3d_device_set, num_entries,
                                                        devices, num_devices));
    }
  }
  clGetDeviceIDsFromD3D10KHR_SD(return_value, platform, d3d_device_source, d3d_object,
                                d3d_device_set, num_entries, devices, num_devices);
}

inline void clGetDeviceIDsFromD3D11KHR_RECWRAP(CRecorder& recorder,
                                               cl_int return_value,
                                               cl_platform_id platform,
                                               cl_d3d11_device_source_khr d3d_device_source,
                                               void* d3d_object,
                                               cl_d3d11_device_set_khr d3d_device_set,
                                               cl_uint num_entries,
                                               cl_device_id* devices,
                                               cl_uint* num_devices) {
  if (recorder.Running()) {
    if (IsDXUnsharingEnabled(Config::Get())) {
      if (return_value == CL_SUCCESS) {
        recorder.Schedule(NewTokenPtrGetDevices(platform));
      }
    } else {
      recorder.Schedule(new CclGetDeviceIDsFromD3D11KHR(return_value, platform, d3d_device_source,
                                                        d3d_object, d3d_device_set, num_entries,
                                                        devices, num_devices));
    }
    clGetDeviceIDsFromD3D11KHR_SD(return_value, platform, d3d_device_source, d3d_object,
                                  d3d_device_set, num_entries, devices, num_devices);
  }
}

inline void clGetDeviceIDsFromDX9INTEL_RECWRAP(CRecorder& recorder,
                                               cl_int return_value,
                                               cl_platform_id platform,
                                               cl_dx9_device_source_intel dx9_device_source,
                                               void* dx9_object,
                                               cl_dx9_device_set_intel dx9_device_set,
                                               cl_uint num_entries,
                                               cl_device_id* devices,
                                               cl_uint* num_devices) {
  if (recorder.Running()) {
    if (IsDXUnsharingEnabled(Config::Get())) {
      if (return_value == CL_SUCCESS) {
        recorder.Schedule(NewTokenPtrGetDevices(platform));
      }
    } else {
      recorder.Schedule(new CclGetDeviceIDsFromDX9INTEL(return_value, platform, dx9_device_source,
                                                        dx9_object, dx9_device_set, num_entries,
                                                        devices, num_devices));
    }
  }
  clGetDeviceIDsFromDX9INTEL_SD(return_value, platform, dx9_device_source, dx9_object,
                                dx9_device_set, num_entries, devices, num_devices);
}
#endif

inline void clGetContextInfo_RECWRAP(CRecorder& recorder,
                                     cl_int return_value,
                                     cl_context context,
                                     cl_context_info param_name,
                                     size_t param_value_size,
                                     void* param_value,
                                     size_t* param_value_size_ret) {
  if (recorder.Running()) {
    if (IsUnsharingEnabled(Config::Get()) && IsSharingQuery(param_name)) {
      return;
    } else {
      recorder.Schedule(new CclGetContextInfo(return_value, context, param_name, param_value_size,
                                              param_value, param_value_size_ret));
    }
  }
  clGetContextInfo_SD(return_value, context, param_name, param_value_size, param_value,
                      param_value_size_ret);
}

#ifdef GITS_PLATFORM_WINDOWS
inline void clCreateFromDX9MediaSurfaceKHR_RECWRAP(CRecorder& recorder,
                                                   cl_mem return_value,
                                                   cl_context context,
                                                   cl_mem_flags flags,
                                                   cl_dx9_media_adapter_type_khr adapter_type,
                                                   void* surface_info,
                                                   cl_uint plane,
                                                   cl_int* errcode_ret) {
  if (recorder.Running()) {
    if (IsDXUnsharingEnabled(Config::Get())) {
      recorder.Schedule(
          NewTokenPtrCreateCLMem(context, return_value, flags, CL_MEM_OBJECT_IMAGE2D));
    } else {
      recorder.Schedule(new CclCreateFromDX9MediaSurfaceKHR(
          return_value, context, flags, adapter_type, surface_info, plane, errcode_ret));
    }
  }
  clCreateFromDX9MediaSurfaceKHR_SD(return_value, context, flags, adapter_type, surface_info, plane,
                                    errcode_ret);
}

inline void clGetDeviceIDsFromDX9MediaAdapterKHR_RECWRAP(
    CRecorder& recorder,
    cl_int return_value,
    cl_platform_id platform,
    cl_uint num_media_adapters,
    cl_dx9_media_adapter_type_khr* media_adapters_type,
    void* media_adapters,
    cl_dx9_media_adapter_set_khr media_adapter_set,
    cl_uint num_entries,
    cl_device_id* devices,
    cl_int* num_devices) {
  if (recorder.Running()) {
    if (IsDXUnsharingEnabled(Config::Get())) {
    } else {
      recorder.Schedule(new CclGetDeviceIDsFromDX9MediaAdapterKHR(
          return_value, platform, num_media_adapters, media_adapters_type, media_adapters,
          media_adapter_set, num_entries, devices, num_devices));
    }
  }
}
#endif

inline void clGetEventInfo_RECWRAP(CRecorder& recorder,
                                   cl_int return_value,
                                   cl_event event,
                                   cl_event_info param_name,
                                   size_t param_value_size,
                                   void* param_value,
                                   size_t* param_value_size_ret) {
  if (recorder.Running()) {
    const auto& eventState = SD()._eventStates[event];
    const auto& cfg = Config::Get();
    if ((IsGLUnsharingEnabled(cfg) &&
         (IsGLSharingQuery(param_name) || eventState->isGLSharingEvent)) ||
        (IsDXUnsharingEnabled(cfg) &&
         (IsDXSharingQuery(param_name) || eventState->isDXSharingEvent))) {
      return;
    } else {
      recorder.Schedule(new CclGetEventInfo(return_value, event, param_name, param_value_size,
                                            param_value, param_value_size_ret));
    }
  }
}

inline void clGetImageInfo_RECWRAP(CRecorder& recorder,
                                   cl_int return_value,
                                   cl_mem image,
                                   cl_image_info param_name,
                                   size_t param_value_size,
                                   void* param_value,
                                   size_t* param_value_size_ret) {
  if (recorder.Running()) {
    if (IsUnsharingEnabled(Config::Get()) && IsSharingQuery(param_name)) {
      return;
    } else {
      recorder.Schedule(new CclGetImageInfo(return_value, image, param_name, param_value_size,
                                            param_value, param_value_size_ret));
    }
  }
}

inline void clGetMemObjectInfo_RECWRAP(CRecorder& recorder,
                                       cl_int return_value,
                                       cl_mem memobj,
                                       cl_mem_info param_name,
                                       size_t param_value_size,
                                       void* param_value,
                                       size_t* param_value_size_ret) {
  if (recorder.Running()) {
    if (IsUnsharingEnabled(Config::Get()) && IsSharingQuery(param_name)) {
      return;
    } else {
      recorder.Schedule(new CclGetMemObjectInfo(return_value, memobj, param_name, param_value_size,
                                                param_value, param_value_size_ret));
    }
  }
}

inline void clRetainEvent_RECWRAP(CRecorder& recorder, cl_int return_value, cl_event event) {
  if (recorder.Running()) {
    const auto& eventState = SD()._eventStates[event];
    const auto& cfg = Config::Get();
    if ((IsGLUnsharingEnabled(cfg) && eventState->isGLSharingEvent) ||
        (IsDXUnsharingEnabled(cfg) && eventState->isDXSharingEvent)) {
      return;
    } else {
      recorder.Schedule(new CclRetainEvent(return_value, event));
    }
  }
  clRetainEvent_SD(return_value, event);
}

inline void clEnqueueBarrierWithWaitList_RECWRAP(CRecorder& recorder,
                                                 cl_int return_value,
                                                 cl_command_queue command_queue,
                                                 cl_uint num_events_in_wait_list,
                                                 const cl_event* event_wait_list,
                                                 cl_event* event) {
  CFunction* _token = nullptr;
  if (recorder.Running()) {
    if (IsUnsharingEnabled(Config::Get())) {
      std::vector<cl_event> filteredEvents(
          FilterSharingEvents(num_events_in_wait_list, event_wait_list));
      cl_uint num_filtered_events = static_cast<cl_uint>(filteredEvents.size());
      _token = new CclEnqueueBarrierWithWaitList(return_value, command_queue, num_filtered_events,
                                                 GetPointerFromVector(filteredEvents), event);
    } else {
      _token = new CclEnqueueBarrierWithWaitList(return_value, command_queue,
                                                 num_events_in_wait_list, event_wait_list, event);
    }
    recorder.Schedule(_token);
  }
  clEnqueueBarrierWithWaitList_SD(return_value, command_queue, num_events_in_wait_list,
                                  event_wait_list, event);
}

inline void clEnqueueCopyBuffer_RECWRAP(CRecorder& recorder,
                                        cl_int return_value,
                                        cl_command_queue command_queue,
                                        cl_mem src_buffer,
                                        cl_mem dst_buffer,
                                        size_t src_offset,
                                        size_t dst_offset,
                                        size_t cb,
                                        cl_uint num_events_in_wait_list,
                                        const cl_event* event_wait_list,
                                        cl_event* event) {
  CFunction* _token = nullptr;
  if (recorder.Running()) {
    if (IsUnsharingEnabled(Config::Get())) {
      std::vector<cl_event> filteredEvents(
          FilterSharingEvents(num_events_in_wait_list, event_wait_list));
      cl_uint num_filtered_events = static_cast<cl_uint>(filteredEvents.size());
      _token = new CclEnqueueCopyBuffer(return_value, command_queue, src_buffer, dst_buffer,
                                        src_offset, dst_offset, cb, num_filtered_events,
                                        GetPointerFromVector(filteredEvents), event);
    } else {
      _token =
          new CclEnqueueCopyBuffer(return_value, command_queue, src_buffer, dst_buffer, src_offset,
                                   dst_offset, cb, num_events_in_wait_list, event_wait_list, event);
    }
    recorder.Schedule(_token);
  }
  clEnqueueCopyBuffer_SD(_token, return_value, command_queue, src_buffer, dst_buffer, src_offset,
                         dst_offset, cb, num_events_in_wait_list, event_wait_list, event);
}

inline void clEnqueueCopyBufferRect_RECWRAP(CRecorder& recorder,
                                            cl_int return_value,
                                            cl_command_queue command_queue,
                                            cl_mem src_buffer,
                                            cl_mem dst_buffer,
                                            const size_t* src_origin,
                                            const size_t* dst_origin,
                                            const size_t* region,
                                            size_t src_row_pitch,
                                            size_t src_slice_pitch,
                                            size_t dst_row_pitch,
                                            size_t dst_slice_pitch,
                                            cl_uint num_events_in_wait_list,
                                            const cl_event* event_wait_list,
                                            cl_event* event) {
  CFunction* _token = nullptr;
  if (recorder.Running()) {
    if (IsUnsharingEnabled(Config::Get())) {
      std::vector<cl_event> filteredEvents(
          FilterSharingEvents(num_events_in_wait_list, event_wait_list));
      cl_uint num_filtered_events = static_cast<cl_uint>(filteredEvents.size());
      _token = new CclEnqueueCopyBufferRect(
          return_value, command_queue, src_buffer, dst_buffer, src_origin, dst_origin, region,
          src_row_pitch, src_slice_pitch, dst_row_pitch, dst_slice_pitch, num_filtered_events,
          GetPointerFromVector(filteredEvents), event);
    } else {
      _token = new CclEnqueueCopyBufferRect(return_value, command_queue, src_buffer, dst_buffer,
                                            src_origin, dst_origin, region, src_row_pitch,
                                            src_slice_pitch, dst_row_pitch, dst_slice_pitch,
                                            num_events_in_wait_list, event_wait_list, event);
    }
    recorder.Schedule(_token);
  }
  clEnqueueCopyBufferRect_SD(_token, return_value, command_queue, src_buffer, dst_buffer,
                             src_origin, dst_origin, region, src_row_pitch, src_slice_pitch,
                             dst_row_pitch, dst_slice_pitch, num_events_in_wait_list,
                             event_wait_list, event);
}

inline void clEnqueueCopyBufferToImage_RECWRAP(CRecorder& recorder,
                                               cl_int return_value,
                                               cl_command_queue command_queue,
                                               cl_mem src_buffer,
                                               cl_mem dst_image,
                                               size_t src_offset,
                                               const size_t* dst_origin,
                                               const size_t* region,
                                               cl_uint num_events_in_wait_list,
                                               const cl_event* event_wait_list,
                                               cl_event* event) {
  CFunction* _token = nullptr;
  if (recorder.Running()) {
    if (IsUnsharingEnabled(Config::Get())) {
      std::vector<cl_event> filteredEvents(
          FilterSharingEvents(num_events_in_wait_list, event_wait_list));
      cl_uint num_filtered_events = static_cast<cl_uint>(filteredEvents.size());
      _token = new CclEnqueueCopyBufferToImage(return_value, command_queue, src_buffer, dst_image,
                                               src_offset, dst_origin, region, num_filtered_events,
                                               GetPointerFromVector(filteredEvents), event);
    } else {
      _token = new CclEnqueueCopyBufferToImage(return_value, command_queue, src_buffer, dst_image,
                                               src_offset, dst_origin, region,
                                               num_events_in_wait_list, event_wait_list, event);
    }
    recorder.Schedule(_token);
  }
  clEnqueueCopyBufferToImage_SD(_token, return_value, command_queue, src_buffer, dst_image,
                                src_offset, dst_origin, region, num_events_in_wait_list,
                                event_wait_list, event);
}

inline void clEnqueueCopyImage_RECWRAP(CRecorder& recorder,
                                       cl_int return_value,
                                       cl_command_queue command_queue,
                                       cl_mem src_image,
                                       cl_mem dst_image,
                                       const size_t* src_origin,
                                       const size_t* dst_origin,
                                       const size_t* region,
                                       cl_uint num_events_in_wait_list,
                                       const cl_event* event_wait_list,
                                       cl_event* event) {
  CFunction* _token = nullptr;
  if (recorder.Running()) {
    if (IsUnsharingEnabled(Config::Get())) {
      std::vector<cl_event> filteredEvents(
          FilterSharingEvents(num_events_in_wait_list, event_wait_list));
      cl_uint num_filtered_events = static_cast<cl_uint>(filteredEvents.size());
      _token = new CclEnqueueCopyImage(return_value, command_queue, src_image, dst_image,
                                       src_origin, dst_origin, region, num_filtered_events,
                                       GetPointerFromVector(filteredEvents), event);
    } else {
      _token = new CclEnqueueCopyImage(return_value, command_queue, src_image, dst_image,
                                       src_origin, dst_origin, region, num_events_in_wait_list,
                                       event_wait_list, event);
    }
    recorder.Schedule(_token);
  }
  clEnqueueCopyImage_SD(_token, return_value, command_queue, src_image, dst_image, src_origin,
                        dst_origin, region, num_events_in_wait_list, event_wait_list, event);
}

inline void clEnqueueCopyImageToBuffer_RECWRAP(CRecorder& recorder,
                                               cl_int return_value,
                                               cl_command_queue command_queue,
                                               cl_mem src_image,
                                               cl_mem dst_buffer,
                                               const size_t* src_origin,
                                               const size_t* region,
                                               size_t dst_offset,
                                               cl_uint num_events_in_wait_list,
                                               const cl_event* event_wait_list,
                                               cl_event* event) {
  CFunction* _token = nullptr;
  if (recorder.Running()) {
    if (IsUnsharingEnabled(Config::Get())) {
      std::vector<cl_event> filteredEvents(
          FilterSharingEvents(num_events_in_wait_list, event_wait_list));
      cl_uint num_filtered_events = static_cast<cl_uint>(filteredEvents.size());
      _token = new CclEnqueueCopyImageToBuffer(return_value, command_queue, src_image, dst_buffer,
                                               src_origin, region, dst_offset, num_filtered_events,
                                               GetPointerFromVector(filteredEvents), event);
    } else {
      _token = new CclEnqueueCopyImageToBuffer(return_value, command_queue, src_image, dst_buffer,
                                               src_origin, region, dst_offset,
                                               num_events_in_wait_list, event_wait_list, event);
    }
    recorder.Schedule(_token);
  }
  clEnqueueCopyImageToBuffer_SD(_token, return_value, command_queue, src_image, dst_buffer,
                                src_origin, region, dst_offset, num_events_in_wait_list,
                                event_wait_list, event);
}

inline void clEnqueueFillBuffer_RECWRAP(CRecorder& recorder,
                                        cl_int return_value,
                                        cl_command_queue command_queue,
                                        cl_mem buffer,
                                        const void* pattern,
                                        size_t pattern_size,
                                        size_t offset,
                                        size_t cb,
                                        cl_uint num_events_in_wait_list,
                                        const cl_event* event_wait_list,
                                        cl_event* event) {
  CFunction* _token = nullptr;
  if (recorder.Running()) {
    if (IsUnsharingEnabled(Config::Get())) {
      std::vector<cl_event> filteredEvents(
          FilterSharingEvents(num_events_in_wait_list, event_wait_list));
      cl_uint num_filtered_events = static_cast<cl_uint>(filteredEvents.size());
      _token = new CclEnqueueFillBuffer(return_value, command_queue, buffer, pattern, pattern_size,
                                        offset, cb, num_filtered_events,
                                        GetPointerFromVector(filteredEvents), event);
    } else {
      _token =
          new CclEnqueueFillBuffer(return_value, command_queue, buffer, pattern, pattern_size,
                                   offset, cb, num_events_in_wait_list, event_wait_list, event);
    }
    recorder.Schedule(_token);
  }
  clEnqueueFillBuffer_SD(_token, return_value, command_queue, buffer, pattern, pattern_size, offset,
                         cb, num_events_in_wait_list, event_wait_list, event);
}

inline void clEnqueueFillImage_RECWRAP(CRecorder& recorder,
                                       cl_int return_value,
                                       cl_command_queue command_queue,
                                       cl_mem image,
                                       const void* fill_color,
                                       const size_t* origin,
                                       const size_t* region,
                                       cl_uint num_events_in_wait_list,
                                       const cl_event* event_wait_list,
                                       cl_event* event) {
  CFunction* _token = nullptr;
  if (recorder.Running()) {
    if (IsUnsharingEnabled(Config::Get())) {
      std::vector<cl_event> filteredEvents(
          FilterSharingEvents(num_events_in_wait_list, event_wait_list));
      cl_uint num_filtered_events = static_cast<cl_uint>(filteredEvents.size());
      _token =
          new CclEnqueueFillImage(return_value, command_queue, image, fill_color, origin, region,
                                  num_filtered_events, GetPointerFromVector(filteredEvents), event);
    } else {
      _token = new CclEnqueueFillImage(return_value, command_queue, image, fill_color, origin,
                                       region, num_events_in_wait_list, event_wait_list, event);
    }
    recorder.Schedule(_token);
  }
  clEnqueueFillImage_SD(_token, return_value, command_queue, image, fill_color, origin, region,
                        num_events_in_wait_list, event_wait_list, event);
}

inline void clEnqueueMapBuffer_RECWRAP(CRecorder& recorder,
                                       void* return_value,
                                       cl_command_queue command_queue,
                                       cl_mem buffer,
                                       cl_bool blocking_map,
                                       cl_map_flags map_flags,
                                       size_t offset,
                                       size_t cb,
                                       cl_uint num_events_in_wait_list,
                                       const cl_event* event_wait_list,
                                       cl_event* event,
                                       cl_int* errcode_ret) {
  CFunction* _token = nullptr;
  if (recorder.Running()) {
    if (IsUnsharingEnabled(Config::Get())) {
      std::vector<cl_event> filteredEvents(
          FilterSharingEvents(num_events_in_wait_list, event_wait_list));
      cl_uint num_filtered_events = static_cast<cl_uint>(filteredEvents.size());
      _token = new CclEnqueueMapBuffer(return_value, command_queue, buffer, blocking_map, map_flags,
                                       offset, cb, num_filtered_events,
                                       GetPointerFromVector(filteredEvents), event, errcode_ret);
    } else {
      _token = new CclEnqueueMapBuffer(return_value, command_queue, buffer, blocking_map, map_flags,
                                       offset, cb, num_events_in_wait_list, event_wait_list, event,
                                       errcode_ret);
    }
    recorder.Schedule(_token);
  }
  clEnqueueMapBuffer_SD(_token, return_value, command_queue, buffer, blocking_map, map_flags,
                        offset, cb, num_events_in_wait_list, event_wait_list, event, errcode_ret);
  // has to be here instead of _SD because it's not needed during playback,
  // but still allocates significant amount of memory
  SD()._mappedBufferStates[return_value].emplace_back(cb, buffer, command_queue, map_flags);
}

inline void clEnqueueMapImage_RECWRAP(CRecorder& recorder,
                                      void* return_value,
                                      cl_command_queue command_queue,
                                      cl_mem image,
                                      cl_bool blocking_map,
                                      cl_map_flags map_flags,
                                      const size_t* origin,
                                      const size_t* region,
                                      size_t* image_row_pitch,
                                      size_t* image_slice_pitch,
                                      cl_uint num_events_in_wait_list,
                                      const cl_event* event_wait_list,
                                      cl_event* event,
                                      cl_int* errcode_ret) {
  CFunction* _token = nullptr;
  if (recorder.Running()) {
    if (IsUnsharingEnabled(Config::Get())) {
      std::vector<cl_event> filteredEvents(
          FilterSharingEvents(num_events_in_wait_list, event_wait_list));
      cl_uint num_filtered_events = static_cast<cl_uint>(filteredEvents.size());
      _token = new CclEnqueueMapImage(return_value, command_queue, image, blocking_map, map_flags,
                                      origin, region, image_row_pitch, image_slice_pitch,
                                      num_filtered_events, GetPointerFromVector(filteredEvents),
                                      event, errcode_ret);
    } else {
      _token = new CclEnqueueMapImage(return_value, command_queue, image, blocking_map, map_flags,
                                      origin, region, image_row_pitch, image_slice_pitch,
                                      num_events_in_wait_list, event_wait_list, event, errcode_ret);
    }
    recorder.Schedule(_token);
  }
  clEnqueueMapImage_SD(_token, return_value, command_queue, image, blocking_map, map_flags, origin,
                       region, image_row_pitch, image_slice_pitch, num_events_in_wait_list,
                       event_wait_list, event, errcode_ret);
  // has to be here instead of _SD because it's not needed during playback,
  // but still allocates significant amount of memory
  size_t imageSize =
      CountImageSize(SD()._memStates[image]->image_format, SD()._memStates[image]->image_desc);
  SD()._mappedBufferStates[return_value].emplace_back(imageSize, image, command_queue, map_flags);
}

inline void clEnqueueMarkerWithWaitList_RECWRAP(CRecorder& recorder,
                                                cl_int return_value,
                                                cl_command_queue command_queue,
                                                cl_uint num_events_in_wait_list,
                                                const cl_event* event_wait_list,
                                                cl_event* event) {
  CFunction* _token = nullptr;
  if (recorder.Running()) {
    if (IsUnsharingEnabled(Config::Get())) {
      std::vector<cl_event> filteredEvents(
          FilterSharingEvents(num_events_in_wait_list, event_wait_list));
      cl_uint num_filtered_events = static_cast<cl_uint>(filteredEvents.size());
      _token = new CclEnqueueMarkerWithWaitList(return_value, command_queue, num_filtered_events,
                                                GetPointerFromVector(filteredEvents), event);
    } else {
      _token = new CclEnqueueMarkerWithWaitList(return_value, command_queue,
                                                num_events_in_wait_list, event_wait_list, event);
    }
    recorder.Schedule(_token);
  }
  clEnqueueMarkerWithWaitList_SD(return_value, command_queue, num_events_in_wait_list,
                                 event_wait_list, event);
}

inline void clEnqueueMigrateMemObjects_RECWRAP(CRecorder& recorder,
                                               cl_int return_value,
                                               cl_command_queue command_queue,
                                               cl_uint num_mem_objects,
                                               const cl_mem* mem_objects,
                                               cl_mem_migration_flags flags,
                                               cl_uint num_events_in_wait_list,
                                               const cl_event* event_wait_list,
                                               cl_event* event) {
  CFunction* _token = nullptr;
  if (recorder.Running()) {
    if (IsUnsharingEnabled(Config::Get())) {
      std::vector<cl_event> filteredEvents(
          FilterSharingEvents(num_events_in_wait_list, event_wait_list));
      cl_uint num_filtered_events = static_cast<cl_uint>(filteredEvents.size());
      _token = new CclEnqueueMigrateMemObjects(return_value, command_queue, num_mem_objects,
                                               mem_objects, flags, num_filtered_events,
                                               GetPointerFromVector(filteredEvents), event);
    } else {
      _token =
          new CclEnqueueMigrateMemObjects(return_value, command_queue, num_mem_objects, mem_objects,
                                          flags, num_events_in_wait_list, event_wait_list, event);
    }
    recorder.Schedule(_token);
  }
  clEnqueueMigrateMemObjects_SD(return_value, command_queue, num_mem_objects, mem_objects, flags,
                                num_events_in_wait_list, event_wait_list, event);
}

inline void clEnqueueReadBuffer_RECWRAP(CRecorder& recorder,
                                        cl_int return_value,
                                        cl_command_queue command_queue,
                                        cl_mem buffer,
                                        cl_bool blocking_read,
                                        size_t offset,
                                        size_t cb,
                                        void* ptr,
                                        cl_uint num_events_in_wait_list,
                                        const cl_event* event_wait_list,
                                        cl_event* event) {
  CFunction* _token = nullptr;
  if (recorder.Running()) {
    if (IsUnsharingEnabled(Config::Get())) {
      std::vector<cl_event> filteredEvents(
          FilterSharingEvents(num_events_in_wait_list, event_wait_list));
      cl_uint num_filtered_events = static_cast<cl_uint>(filteredEvents.size());
      _token = new CclEnqueueReadBuffer(return_value, command_queue, buffer, blocking_read, offset,
                                        cb, ptr, num_filtered_events,
                                        GetPointerFromVector(filteredEvents), event);
    } else {
      _token = new CclEnqueueReadBuffer(return_value, command_queue, buffer, blocking_read, offset,
                                        cb, ptr, num_events_in_wait_list, event_wait_list, event);
    }
    recorder.Schedule(_token);
  }
  clEnqueueReadBuffer_SD(_token, return_value, command_queue, buffer, blocking_read, offset, cb,
                         ptr, num_events_in_wait_list, event_wait_list, event);
}

inline void clEnqueueReadBufferRect_RECWRAP(CRecorder& recorder,
                                            cl_int return_value,
                                            cl_command_queue command_queue,
                                            cl_mem buffer,
                                            cl_bool blocking_read,
                                            const size_t* buffer_offset,
                                            const size_t* host_offset,
                                            const size_t* region,
                                            size_t buffer_row_pitch,
                                            size_t buffer_slice_pitch,
                                            size_t host_row_pitch,
                                            size_t host_slice_pitch,
                                            void* ptr,
                                            cl_uint num_events_in_wait_list,
                                            const cl_event* event_wait_list,
                                            cl_event* event) {
  CFunction* _token = nullptr;
  if (recorder.Running()) {
    if (IsUnsharingEnabled(Config::Get())) {
      std::vector<cl_event> filteredEvents(
          FilterSharingEvents(num_events_in_wait_list, event_wait_list));
      cl_uint num_filtered_events = static_cast<cl_uint>(filteredEvents.size());
      _token = new CclEnqueueReadBufferRect(
          return_value, command_queue, buffer, blocking_read, buffer_offset, host_offset, region,
          buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch, ptr,
          num_filtered_events, GetPointerFromVector(filteredEvents), event);
    } else {
      _token = new CclEnqueueReadBufferRect(return_value, command_queue, buffer, blocking_read,
                                            buffer_offset, host_offset, region, buffer_row_pitch,
                                            buffer_slice_pitch, host_row_pitch, host_slice_pitch,
                                            ptr, num_events_in_wait_list, event_wait_list, event);
    }
    recorder.Schedule(_token);
  }
  clEnqueueReadBufferRect_SD(_token, return_value, command_queue, buffer, blocking_read,
                             buffer_offset, host_offset, region, buffer_row_pitch,
                             buffer_slice_pitch, host_row_pitch, host_slice_pitch, ptr,
                             num_events_in_wait_list, event_wait_list, event);
}

inline void clEnqueueReadImage_RECWRAP(CRecorder& recorder,
                                       cl_int return_value,
                                       cl_command_queue command_queue,
                                       cl_mem image,
                                       cl_bool blocking_read,
                                       const size_t* origin,
                                       const size_t* region,
                                       size_t row_pitch,
                                       size_t slice_pitch,
                                       void* ptr,
                                       cl_uint num_events_in_wait_list,
                                       const cl_event* event_wait_list,
                                       cl_event* event) {
  CFunction* _token = nullptr;
  if (recorder.Running()) {
    if (IsUnsharingEnabled(Config::Get())) {
      std::vector<cl_event> filteredEvents(
          FilterSharingEvents(num_events_in_wait_list, event_wait_list));
      cl_uint num_filtered_events = static_cast<cl_uint>(filteredEvents.size());
      _token = new CclEnqueueReadImage(return_value, command_queue, image, blocking_read, origin,
                                       region, row_pitch, slice_pitch, ptr, num_filtered_events,
                                       GetPointerFromVector(filteredEvents), event);
    } else {
      _token = new CclEnqueueReadImage(return_value, command_queue, image, blocking_read, origin,
                                       region, row_pitch, slice_pitch, ptr, num_events_in_wait_list,
                                       event_wait_list, event);
    }
    recorder.Schedule(_token);
  }
  clEnqueueReadImage_SD(_token, return_value, command_queue, image, blocking_read, origin, region,
                        row_pitch, slice_pitch, ptr, num_events_in_wait_list, event_wait_list,
                        event);
}

inline void clEnqueueSVMFree_RECWRAP(
    CRecorder& recorder,
    cl_int return_value,
    cl_command_queue command_queue,
    cl_uint num_svm_pointers,
    void** svm_pointers,
    void(CL_CALLBACK* pfn_free_func)(cl_command_queue, cl_uint, void**, void*),
    void* user_data,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event) {
  CFunction* _token = nullptr;
  if (recorder.Running()) {
    if (IsUnsharingEnabled(Config::Get())) {
      std::vector<cl_event> filteredEvents(
          FilterSharingEvents(num_events_in_wait_list, event_wait_list));
      cl_uint num_filtered_events = static_cast<cl_uint>(filteredEvents.size());
      _token = new CclEnqueueSVMFree(return_value, command_queue, num_svm_pointers, svm_pointers,
                                     pfn_free_func, user_data, num_filtered_events,
                                     GetPointerFromVector(filteredEvents), event);
    } else {
      _token = new CclEnqueueSVMFree(return_value, command_queue, num_svm_pointers, svm_pointers,
                                     pfn_free_func, user_data, num_events_in_wait_list,
                                     event_wait_list, event);
    }
    recorder.Schedule(_token);
  }
  clEnqueueSVMFree_SD(return_value, command_queue, num_svm_pointers, svm_pointers, pfn_free_func,
                      user_data, num_events_in_wait_list, event_wait_list, event);
}

inline void clEnqueueSVMMap_RECWRAP(CRecorder& recorder,
                                    cl_int return_value,
                                    cl_command_queue command_queue,
                                    cl_bool blocking_map,
                                    cl_map_flags flags,
                                    void* svm_ptr,
                                    size_t size,
                                    cl_uint num_events_in_wait_list,
                                    const cl_event* event_wait_list,
                                    cl_event* event) {
  CFunction* _token = nullptr;
  if (recorder.Running()) {
    if (IsUnsharingEnabled(Config::Get())) {
      std::vector<cl_event> filteredEvents(
          FilterSharingEvents(num_events_in_wait_list, event_wait_list));
      cl_uint num_filtered_events = static_cast<cl_uint>(filteredEvents.size());
      _token =
          new CclEnqueueSVMMap(return_value, command_queue, blocking_map, flags, svm_ptr, size,
                               num_filtered_events, GetPointerFromVector(filteredEvents), event);
    } else {
      _token = new CclEnqueueSVMMap(return_value, command_queue, blocking_map, flags, svm_ptr, size,
                                    num_events_in_wait_list, event_wait_list, event);
    }
    recorder.Schedule(_token);
    SD()._enqueueSvmMapSize[svm_ptr] = flags & CL_MAP_WRITE ? size : 0UL;
  }
  clEnqueueSVMMap_SD(return_value, command_queue, blocking_map, flags, svm_ptr, size,
                     num_events_in_wait_list, event_wait_list, event);
}

inline void clEnqueueSVMUnmap_RECWRAP_PRE(CRecorder& recorder,
                                          cl_int return_value,
                                          cl_command_queue command_queue,
                                          void* svm_ptr,
                                          cl_uint num_events_in_wait_list,
                                          const cl_event* event_wait_list,
                                          cl_event* event) {
  if (recorder.Running()) {
    const auto ptr = GetSvmOrUsmFromRegion(svm_ptr).first;
    const auto map_size = SD()._enqueueSvmMapSize.at(svm_ptr);
    if (ptr != nullptr && map_size != 0UL) {
      const auto offset = reinterpret_cast<uintptr_t>(svm_ptr) - reinterpret_cast<uintptr_t>(ptr);
      recorder.Schedule(new CGitsClMemoryRegionRestore(ptr, static_cast<uint64_t>(offset),
                                                       static_cast<uint64_t>(map_size)));
    }
    SD()._enqueueSvmMapSize.erase(svm_ptr);
  }
}

inline void clEnqueueSVMUnmap_RECWRAP(CRecorder& recorder,
                                      cl_int return_value,
                                      cl_command_queue command_queue,
                                      void* svm_ptr,
                                      cl_uint num_events_in_wait_list,
                                      const cl_event* event_wait_list,
                                      cl_event* event) {
  CFunction* _token = nullptr;
  if (recorder.Running()) {
    if (IsUnsharingEnabled(Config::Get())) {
      std::vector<cl_event> filteredEvents(
          FilterSharingEvents(num_events_in_wait_list, event_wait_list));
      cl_uint num_filtered_events = static_cast<cl_uint>(filteredEvents.size());
      _token = new CclEnqueueSVMUnmap(return_value, command_queue, svm_ptr, num_filtered_events,
                                      GetPointerFromVector(filteredEvents), event);
    } else {
      _token = new CclEnqueueSVMUnmap(return_value, command_queue, svm_ptr, num_events_in_wait_list,
                                      event_wait_list, event);
    }
    recorder.Schedule(_token);
  }
  clEnqueueSVMUnmap_SD(return_value, command_queue, svm_ptr, num_events_in_wait_list,
                       event_wait_list, event);
}

inline void clEnqueueTask_RECWRAP(CRecorder& recorder,
                                  cl_int return_value,
                                  cl_command_queue command_queue,
                                  cl_kernel kernel,
                                  cl_uint num_events_in_wait_list,
                                  const cl_event* event_wait_list,
                                  cl_event* event) {
  CFunction* _token = nullptr;
  if (recorder.Running()) {
    if (IsUnsharingEnabled(Config::Get())) {
      std::vector<cl_event> filteredEvents(
          FilterSharingEvents(num_events_in_wait_list, event_wait_list));
      cl_uint num_filtered_events = static_cast<cl_uint>(filteredEvents.size());
      _token = new CclEnqueueTask(return_value, command_queue, kernel, num_filtered_events,
                                  GetPointerFromVector(filteredEvents), event);
    } else {
      _token = new CclEnqueueTask(return_value, command_queue, kernel, num_events_in_wait_list,
                                  event_wait_list, event);
    }
    recorder.Schedule(_token);
  }
  clEnqueueTask_SD(return_value, command_queue, kernel, num_events_in_wait_list, event_wait_list,
                   event);
}

inline void clEnqueueUnmapMemObject_RECWRAP(CRecorder& recorder,
                                            cl_int return_value,
                                            cl_command_queue command_queue,
                                            cl_mem memobj,
                                            void* mapped_ptr,
                                            cl_uint num_events_in_wait_list,
                                            const cl_event* event_wait_list,
                                            cl_event* event) {
  CFunction* _token = nullptr;
  if (recorder.Running()) {
    if (IsUnsharingEnabled(Config::Get())) {
      std::vector<cl_event> filteredEvents(
          FilterSharingEvents(num_events_in_wait_list, event_wait_list));
      cl_uint num_filtered_events = static_cast<cl_uint>(filteredEvents.size());
      _token = new CclEnqueueUnmapMemObject(return_value, command_queue, memobj, mapped_ptr,
                                            num_filtered_events,
                                            GetPointerFromVector(filteredEvents), event);
    } else {
      _token = new CclEnqueueUnmapMemObject(return_value, command_queue, memobj, mapped_ptr,
                                            num_events_in_wait_list, event_wait_list, event);
    }
    recorder.Schedule(_token);
  } else {
    auto& mappedBufferStates = SD()._mappedBufferStates;
    mappedBufferStates[mapped_ptr].pop_back();
    if (mappedBufferStates[mapped_ptr].empty()) {
      mappedBufferStates.erase(mapped_ptr);
    }
  }
  clEnqueueUnmapMemObject_SD(_token, return_value, command_queue, memobj, mapped_ptr,
                             num_events_in_wait_list, event_wait_list, event);
}

inline void clEnqueueWaitForEvents_RECWRAP(CRecorder& recorder,
                                           cl_int return_value,
                                           cl_command_queue command_queue,
                                           cl_uint num_events,
                                           const cl_event* event_list) {
  CFunction* _token = nullptr;
  if (recorder.Running()) {
    if (IsUnsharingEnabled(Config::Get())) {
      std::vector<cl_event> filteredEvents(FilterSharingEvents(num_events, event_list));
      cl_uint num_filtered_events = static_cast<cl_uint>(filteredEvents.size());
      _token = new CclEnqueueWaitForEvents(return_value, command_queue, num_filtered_events,
                                           GetPointerFromVector(filteredEvents));
    } else {
      _token = new CclEnqueueWaitForEvents(return_value, command_queue, num_events, event_list);
    }
    recorder.Schedule(_token);
  }
}

inline void clEnqueueWriteBuffer_RECWRAP(CRecorder& recorder,
                                         cl_int return_value,
                                         cl_command_queue command_queue,
                                         cl_mem buffer,
                                         cl_bool blocking_write,
                                         size_t offset,
                                         size_t cb,
                                         const void* ptr,
                                         cl_uint num_events_in_wait_list,
                                         const cl_event* event_wait_list,
                                         cl_event* event) {
  CFunction* _token = nullptr;
  if (recorder.Running()) {
    if (IsUnsharingEnabled(Config::Get())) {
      std::vector<cl_event> filteredEvents(
          FilterSharingEvents(num_events_in_wait_list, event_wait_list));
      cl_uint num_filtered_events = static_cast<cl_uint>(filteredEvents.size());
      _token = new CclEnqueueWriteBuffer(return_value, command_queue, buffer, blocking_write,
                                         offset, cb, ptr, num_filtered_events,
                                         GetPointerFromVector(filteredEvents), event);
    } else {
      _token =
          new CclEnqueueWriteBuffer(return_value, command_queue, buffer, blocking_write, offset, cb,
                                    ptr, num_events_in_wait_list, event_wait_list, event);
    }
    recorder.Schedule(_token);
  }
  clEnqueueWriteBuffer_SD(_token, return_value, command_queue, buffer, blocking_write, offset, cb,
                          ptr, num_events_in_wait_list, event_wait_list, event);
}

inline void clEnqueueWriteBufferRect_RECWRAP(CRecorder& recorder,
                                             cl_int return_value,
                                             cl_command_queue command_queue,
                                             cl_mem buffer,
                                             cl_bool blocking_write,
                                             const size_t* buffer_offset,
                                             const size_t* host_offset,
                                             const size_t* region,
                                             size_t buffer_row_pitch,
                                             size_t buffer_slice_pitch,
                                             size_t host_row_pitch,
                                             size_t host_slice_pitch,
                                             const void* ptr,
                                             cl_uint num_events_in_wait_list,
                                             const cl_event* event_wait_list,
                                             cl_event* event) {
  CFunction* _token = nullptr;
  if (recorder.Running()) {
    if (IsUnsharingEnabled(Config::Get())) {
      std::vector<cl_event> filteredEvents(
          FilterSharingEvents(num_events_in_wait_list, event_wait_list));
      cl_uint num_filtered_events = static_cast<cl_uint>(filteredEvents.size());
      _token = new CclEnqueueWriteBufferRect(
          return_value, command_queue, buffer, blocking_write, buffer_offset, host_offset, region,
          buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch, ptr,
          num_filtered_events, GetPointerFromVector(filteredEvents), event);
    } else {
      _token = new CclEnqueueWriteBufferRect(return_value, command_queue, buffer, blocking_write,
                                             buffer_offset, host_offset, region, buffer_row_pitch,
                                             buffer_slice_pitch, host_row_pitch, host_slice_pitch,
                                             ptr, num_events_in_wait_list, event_wait_list, event);
    }
    recorder.Schedule(_token);
  }
  clEnqueueWriteBufferRect_SD(_token, return_value, command_queue, buffer, blocking_write,
                              buffer_offset, host_offset, region, buffer_row_pitch,
                              buffer_slice_pitch, host_row_pitch, host_slice_pitch, ptr,
                              num_events_in_wait_list, event_wait_list, event);
}

inline void clEnqueueWriteImage_RECWRAP(CRecorder& recorder,
                                        cl_int return_value,
                                        cl_command_queue command_queue,
                                        cl_mem image,
                                        cl_bool blocking_write,
                                        const size_t* origin,
                                        const size_t* region,
                                        size_t input_row_pitch,
                                        size_t input_slice_pitch,
                                        const void* ptr,
                                        cl_uint num_events_in_wait_list,
                                        const cl_event* event_wait_list,
                                        cl_event* event) {
  CFunction* _token = nullptr;
  if (recorder.Running()) {
    if (IsUnsharingEnabled(Config::Get())) {
      std::vector<cl_event> filteredEvents(
          FilterSharingEvents(num_events_in_wait_list, event_wait_list));
      cl_uint num_filtered_events = static_cast<cl_uint>(filteredEvents.size());
      _token = new CclEnqueueWriteImage(
          return_value, command_queue, image, blocking_write, origin, region, input_row_pitch,
          input_slice_pitch, ptr, num_filtered_events, GetPointerFromVector(filteredEvents), event);
    } else {
      _token = new CclEnqueueWriteImage(return_value, command_queue, image, blocking_write, origin,
                                        region, input_row_pitch, input_slice_pitch, ptr,
                                        num_events_in_wait_list, event_wait_list, event);
    }
    recorder.Schedule(_token);
  }
  clEnqueueWriteImage_SD(_token, return_value, command_queue, image, blocking_write, origin, region,
                         input_row_pitch, input_slice_pitch, ptr, num_events_in_wait_list,
                         event_wait_list, event);
}

inline void clEnqueueNativeKernel_RECWRAP(CRecorder& recorder,
                                          cl_int return_value,
                                          cl_command_queue command_queue,
                                          void(CL_CALLBACK* user_func)(void*),
                                          void* args,
                                          size_t cb_args,
                                          cl_uint num_mem_objects,
                                          const cl_mem* mem_list,
                                          const void** args_mem_loc,
                                          cl_uint num_events_in_wait_list,
                                          const cl_event* event_wait_list,
                                          cl_event* event) {}
#ifdef GITS_PLATFORM_WINDOWS

inline void clEnqueueAcquireDX9MediaSurfacesKHR_RECWRAP(CRecorder& recorder,
                                                        cl_int return_value,
                                                        cl_command_queue command_queue,
                                                        cl_uint num_objects,
                                                        const cl_mem* mem_objects,
                                                        cl_uint num_events_in_wait_list,
                                                        const cl_event* event_wait_list,
                                                        cl_event* event) {
  if (recorder.Running()) {
    if (IsDXUnsharingEnabled(Config::Get())) {
    } else {
      recorder.Schedule(new CclEnqueueAcquireDX9MediaSurfacesKHR(
          return_value, command_queue, num_objects, mem_objects, num_events_in_wait_list,
          event_wait_list, event));
    }
  }
  clEnqueueReleaseD3D10ObjectsKHR_SD(return_value, command_queue, num_objects, mem_objects,
                                     num_events_in_wait_list, event_wait_list, event);
}
#endif
#ifdef GITS_PLATFORM_WINDOWS

inline void clEnqueueReleaseDX9MediaSurfacesKHR_RECWRAP(CRecorder& recorder,
                                                        cl_int return_value,
                                                        cl_command_queue command_queue,
                                                        cl_uint num_objects,
                                                        const cl_mem* mem_objects,
                                                        cl_uint num_events_in_wait_list,
                                                        const cl_event* event_wait_list,
                                                        cl_event* event) {
  if (recorder.Running()) {
    if (IsDXUnsharingEnabled(Config::Get())) {
    } else {
      recorder.Schedule(new CclEnqueueReleaseDX9MediaSurfacesKHR(
          return_value, command_queue, num_objects, mem_objects, num_events_in_wait_list,
          event_wait_list, event));
    }
  }
  clEnqueueReleaseD3D10ObjectsKHR_SD(return_value, command_queue, num_objects, mem_objects,
                                     num_events_in_wait_list, event_wait_list, event);
}
#endif

inline void clEnqueueMemcpyINTEL_RECWRAP(CRecorder& recorder,
                                         cl_int return_value,
                                         cl_command_queue command_queue,
                                         cl_bool blocking,
                                         void* dst_ptr,
                                         const void* src_ptr,
                                         size_t size,
                                         cl_uint num_events_in_wait_list,
                                         const cl_event* event_wait_list,
                                         cl_event* event) {
  CFunction* _token = nullptr;
  if (recorder.Running()) {
    if (CheckWhetherUpdateUSM(src_ptr)) {
      _token = new CGitsClMemoryUpdate(const_cast<void*>(src_ptr));
      recorder.Schedule(_token);
    }
    if (IsUnsharingEnabled(Config::Get())) {
      std::vector<cl_event> filteredEvents(
          FilterSharingEvents(num_events_in_wait_list, event_wait_list));
      cl_uint num_filtered_events = static_cast<cl_uint>(filteredEvents.size());
      _token = new CclEnqueueMemcpyINTEL(return_value, command_queue, blocking, dst_ptr, src_ptr,
                                         size, num_filtered_events,
                                         GetPointerFromVector(filteredEvents), event);
    } else {
      _token = new CclEnqueueMemcpyINTEL(return_value, command_queue, blocking, dst_ptr, src_ptr,
                                         size, num_events_in_wait_list, event_wait_list, event);
    }
    recorder.Schedule(_token);
  }
  clEnqueueMemcpyINTEL_SD(return_value, command_queue, blocking, dst_ptr, src_ptr, size,
                          num_events_in_wait_list, event_wait_list, event);
}

inline void clEnqueueMemAdviseINTEL_RECWRAP(CRecorder& recorder,
                                            cl_int return_value,
                                            cl_command_queue command_queue,
                                            const void* ptr,
                                            size_t size,
                                            cl_mem_advice_intel advice,
                                            cl_uint num_events_in_wait_list,
                                            const cl_event* event_wait_list,
                                            cl_event* event) {
  CFunction* _token = nullptr;
  if (recorder.Running()) {
    if (IsUnsharingEnabled(Config::Get())) {
      std::vector<cl_event> filteredEvents(
          FilterSharingEvents(num_events_in_wait_list, event_wait_list));
      cl_uint num_filtered_events = static_cast<cl_uint>(filteredEvents.size());
      _token = new CclEnqueueMemAdviseINTEL(return_value, command_queue, ptr, size, advice,
                                            num_filtered_events,
                                            GetPointerFromVector(filteredEvents), event);
    } else {
      _token = new CclEnqueueMemAdviseINTEL(return_value, command_queue, ptr, size, advice,
                                            num_events_in_wait_list, event_wait_list, event);
    }
    recorder.Schedule(_token);
  }
  clEnqueueMemAdviseINTEL_SD(return_value, command_queue, ptr, size, advice,
                             num_events_in_wait_list, event_wait_list, event);
}

inline void clEnqueueMigrateMemINTEL_RECWRAP(CRecorder& recorder,
                                             cl_int return_value,
                                             cl_command_queue command_queue,
                                             const void* ptr,
                                             size_t size,
                                             cl_mem_migration_flags_intel flags,
                                             cl_uint num_events_in_wait_list,
                                             const cl_event* event_wait_list,
                                             cl_event* event) {
  CFunction* _token = nullptr;
  if (recorder.Running()) {
    if (IsUnsharingEnabled(Config::Get())) {
      std::vector<cl_event> filteredEvents(
          FilterSharingEvents(num_events_in_wait_list, event_wait_list));
      cl_uint num_filtered_events = static_cast<cl_uint>(filteredEvents.size());
      _token = new CclEnqueueMigrateMemINTEL(return_value, command_queue, ptr, size, flags,
                                             num_filtered_events,
                                             GetPointerFromVector(filteredEvents), event);
    } else {
      _token = new CclEnqueueMigrateMemINTEL(return_value, command_queue, ptr, size, flags,
                                             num_events_in_wait_list, event_wait_list, event);
    }
    recorder.Schedule(_token);
  }
  clEnqueueMigrateMemINTEL_SD(return_value, command_queue, ptr, size, flags,
                              num_events_in_wait_list, event_wait_list, event);
}

inline void clEnqueueNDCountKernelINTEL_RECWRAP(CRecorder& recorder,
                                                cl_int return_value,
                                                cl_command_queue command_queue,
                                                cl_kernel kernel,
                                                cl_uint workDim,
                                                const size_t* globalWorkOffset,
                                                const size_t* workGroupCount,
                                                const size_t* localWorkSize,
                                                cl_uint numEventsInWaitList,
                                                const cl_event* eventWaitList,
                                                cl_event* event) {
  CFunction* _token = nullptr;
  if (recorder.Running()) {
    const auto& computeIface = CGits::Instance().apis.IfaceCompute();
    if (computeIface.CfgRec_IsSingleKernelMode()) {
      _token = new CclEnqueueNDCountKernelINTEL(return_value, command_queue, kernel, workDim,
                                                globalWorkOffset, workGroupCount, localWorkSize, 0,
                                                nullptr, nullptr);
    } else {
      if (IsUnsharingEnabled(Config::Get())) {
        std::vector<cl_event> filteredEvents(
            FilterSharingEvents(numEventsInWaitList, eventWaitList));
        cl_uint num_filtered_events = static_cast<cl_uint>(filteredEvents.size());
        _token = new CclEnqueueNDCountKernelINTEL(
            return_value, command_queue, kernel, workDim, globalWorkOffset, workGroupCount,
            localWorkSize, num_filtered_events, GetPointerFromVector(filteredEvents), event);
      } else {
        _token = new CclEnqueueNDCountKernelINTEL(return_value, command_queue, kernel, workDim,
                                                  globalWorkOffset, workGroupCount, localWorkSize,
                                                  numEventsInWaitList, eventWaitList, event);
      }
    }
    recorder.Schedule(_token);
  }
  clEnqueueNDCountKernelINTEL_SD(return_value, command_queue, kernel, workDim, globalWorkOffset,
                                 workGroupCount, localWorkSize, numEventsInWaitList, eventWaitList,
                                 event);
  recorder.KernelEnd();
}

inline void clEnqueueMemFillINTEL_RECWRAP(CRecorder& recorder,
                                          cl_int return_value,
                                          cl_command_queue command_queue,
                                          void* dst_ptr,
                                          const void* pattern,
                                          size_t pattern_size,
                                          size_t size,
                                          cl_uint num_events_in_wait_list,
                                          const cl_event* event_wait_list,
                                          cl_event* event) {
  CFunction* _token = nullptr;
  if (recorder.Running()) {
    if (IsUnsharingEnabled(Config::Get())) {
      std::vector<cl_event> filteredEvents(
          FilterSharingEvents(num_events_in_wait_list, event_wait_list));
      cl_uint num_filtered_events = static_cast<cl_uint>(filteredEvents.size());
      _token = new CclEnqueueMemFillINTEL(return_value, command_queue, dst_ptr, pattern,
                                          pattern_size, size, num_filtered_events,
                                          GetPointerFromVector(filteredEvents), event);
    } else {
      _token =
          new CclEnqueueMemFillINTEL(return_value, command_queue, dst_ptr, pattern, pattern_size,
                                     size, num_events_in_wait_list, event_wait_list, event);
    }
    recorder.Schedule(_token);
  }
  clEnqueueMemFillINTEL_SD(return_value, command_queue, dst_ptr, pattern, pattern_size, size,
                           num_events_in_wait_list, event_wait_list, event);
}

inline void clEnqueueSVMMemcpy_V1_RECWRAP(CRecorder& recorder,
                                          cl_int return_value,
                                          cl_command_queue command_queue,
                                          cl_bool blocking_copy,
                                          void* dst_ptr,
                                          const void* src_ptr,
                                          size_t size,
                                          cl_uint num_events_in_wait_list,
                                          const cl_event* event_wait_list,
                                          cl_event* event) {
  CFunction* _token = nullptr;
  if (recorder.Running()) {
    if (IsUnsharingEnabled(Config::Get())) {
      std::vector<cl_event> filteredEvents(
          FilterSharingEvents(num_events_in_wait_list, event_wait_list));
      cl_uint num_filtered_events = static_cast<cl_uint>(filteredEvents.size());
      _token = new CclEnqueueSVMMemcpy_V1(return_value, command_queue, blocking_copy, dst_ptr,
                                          src_ptr, size, num_filtered_events,
                                          GetPointerFromVector(filteredEvents), event);
    } else {
      _token =
          new CclEnqueueSVMMemcpy_V1(return_value, command_queue, blocking_copy, dst_ptr, src_ptr,
                                     size, num_events_in_wait_list, event_wait_list, event);
    }
    recorder.Schedule(_token);
  }
  clEnqueueSVMMemcpy_SD(return_value, command_queue, blocking_copy, dst_ptr, src_ptr, size,
                        num_events_in_wait_list, event_wait_list, event);
}

inline void clEnqueueSVMMemFill_V1_RECWRAP(CRecorder& recorder,
                                           cl_int return_value,
                                           cl_command_queue command_queue,
                                           void* svm_ptr,
                                           const void* pattern,
                                           size_t pattern_size,
                                           size_t size,
                                           cl_uint num_events_in_wait_list,
                                           const cl_event* event_wait_list,
                                           cl_event* event) {
  CFunction* _token = nullptr;
  if (recorder.Running()) {
    if (IsUnsharingEnabled(Config::Get())) {
      std::vector<cl_event> filteredEvents(
          FilterSharingEvents(num_events_in_wait_list, event_wait_list));
      cl_uint num_filtered_events = static_cast<cl_uint>(filteredEvents.size());
      _token = new CclEnqueueSVMMemFill_V1(return_value, command_queue, svm_ptr, pattern,
                                           pattern_size, size, num_filtered_events,
                                           GetPointerFromVector(filteredEvents), event);
    } else {
      _token =
          new CclEnqueueSVMMemFill_V1(return_value, command_queue, svm_ptr, pattern, pattern_size,
                                      size, num_events_in_wait_list, event_wait_list, event);
    }
    recorder.Schedule(_token);
  }
  clEnqueueSVMMemFill_SD(return_value, command_queue, svm_ptr, pattern, pattern_size, size,
                         num_events_in_wait_list, event_wait_list, event);
}

inline void clDeviceMemAllocINTEL_RECWRAP(CRecorder& recorder,
                                          void* return_value,
                                          cl_context context,
                                          cl_device_id device,
                                          cl_mem_properties_intel* properties,
                                          size_t size,
                                          cl_uint alignment,
                                          cl_int* errcode_ret) {
  if (recorder.Running()) {
    recorder.Schedule(new CclDeviceMemAllocINTEL(return_value, context, device, properties, size,
                                                 alignment, errcode_ret));
  }
  clDeviceMemAllocINTEL_SD(return_value, context, device, properties, size, alignment, errcode_ret);
  if (recorder.Running() && ErrCodeSuccess(errcode_ret) &&
      CheckCfgZeroInitialization(Config::Get(), IsReadOnlyBuffer(0, properties))) {
    const auto commandQueue = GetCommandQueueRec(context, &recorder);
    if (commandQueue != nullptr) {
      if (ZeroInitializeUsm(commandQueue, return_value, size, UnifiedMemoryType::device)) {
        const auto zeroBuffer = std::vector<char>(size, 0);
        recorder.Schedule(new CclEnqueueMemcpyINTEL(CL_SUCCESS, commandQueue, CL_BLOCKING,
                                                    return_value, zeroBuffer.data(), size, 0,
                                                    nullptr, nullptr));
      }
    }
  }
}

inline void clHostMemAllocINTEL_RECWRAP(CRecorder& recorder,
                                        void* return_value,
                                        cl_context context,
                                        cl_mem_properties_intel* properties,
                                        size_t size,
                                        cl_uint alignment,
                                        cl_int* errcode_ret) {
  if (recorder.Running()) {
    recorder.Schedule(
        new CclHostMemAllocINTEL(return_value, context, properties, size, alignment, errcode_ret));
  }
  clHostMemAllocINTEL_SD(return_value, context, properties, size, alignment, errcode_ret);
  if (recorder.Running() && ErrCodeSuccess(errcode_ret) &&
      CheckCfgZeroInitialization(Config::Get(), IsReadOnlyBuffer(0, properties))) {
    const auto commandQueue = GetCommandQueueRec(context, &recorder);
    if (commandQueue != nullptr) {
      if (ZeroInitializeUsm(commandQueue, return_value, size, UnifiedMemoryType::host)) {
        recorder.Schedule(new CGitsClMemoryRestore(return_value, size));
      }
    }
  }
  if (recorder.Running() && return_value != nullptr) {
    auto& sniffedRegionHandle =
        SD().GetUSMAllocState(return_value, EXCEPTION_MESSAGE).sniffedRegionHandle;
    const auto& oclIFace = CGits::Instance().apis.IfaceCompute();
    oclIFace.EnableMemorySnifferForPointer(return_value, size, sniffedRegionHandle);
  }
}

inline void clSVMAlloc_RECWRAP(CRecorder& recorder,
                               void* return_value,
                               cl_context context,
                               cl_svm_mem_flags flags,
                               size_t size,
                               cl_uint alignment) {
  if (recorder.Running()) {
    recorder.Schedule(new CclSVMAlloc(return_value, context, flags, size, alignment));
  }
  clSVMAlloc_SD(return_value, context, flags, size, alignment);
  if (recorder.Running() && return_value != nullptr &&
      CheckCfgZeroInitialization(Config::Get(), IsReadOnlyBuffer(flags))) {
    const auto commandQueue = GetCommandQueueRec(context, &recorder);
    if (commandQueue != nullptr) {
      const auto fineGrain = (flags & CL_MEM_SVM_FINE_GRAIN_BUFFER) != 0U;
      if (ZeroInitializeSvm(commandQueue, return_value, size, fineGrain)) {
        const auto zeroBuffer = std::vector<char>(size, 0);
        if (fineGrain) {
          recorder.Schedule(new CGitsClMemoryRestore(return_value, size));
        } else {
          recorder.Schedule(new CclEnqueueSVMMemcpy(CL_SUCCESS, commandQueue, CL_BLOCKING,
                                                    return_value, zeroBuffer.data(), size, 0,
                                                    nullptr, nullptr));
        }
      }
    }
  }
  if (recorder.Running() && return_value != nullptr && (flags & CL_MEM_SVM_FINE_GRAIN_BUFFER)) {
    auto& sniffedRegionHandle =
        SD().GetSVMAllocState(return_value, EXCEPTION_MESSAGE).sniffedRegionHandle;
    const auto& oclIFace = CGits::Instance().apis.IfaceCompute();
    oclIFace.EnableMemorySnifferForPointer(return_value, size, sniffedRegionHandle);
  }
}

inline void clSharedMemAllocINTEL_RECWRAP(CRecorder& recorder,
                                          void* return_value,
                                          cl_context context,
                                          cl_device_id device,
                                          cl_mem_properties_intel* properties,
                                          size_t size,
                                          cl_uint alignment,
                                          cl_int* errcode_ret) {
  if (recorder.Running()) {
    recorder.Schedule(new CclSharedMemAllocINTEL(return_value, context, device, properties, size,
                                                 alignment, errcode_ret));
  }
  clSharedMemAllocINTEL_SD(return_value, context, device, properties, size, alignment, errcode_ret);
  if (recorder.Running() && ErrCodeSuccess(errcode_ret) &&
      CheckCfgZeroInitialization(Config::Get(), IsReadOnlyBuffer(0, properties))) {
    const auto commandQueue = GetCommandQueueRec(context, &recorder);
    if (commandQueue != nullptr) {
      ZeroInitializeUsm(commandQueue, return_value, size, UnifiedMemoryType::shared);
      recorder.Schedule(new CGitsClMemoryRestore(return_value, size));
    }
  }
  if (recorder.Running() && return_value != nullptr) {
    auto& sniffedRegionHandle =
        SD().GetUSMAllocState(return_value, EXCEPTION_MESSAGE).sniffedRegionHandle;
    const auto& oclIFace = CGits::Instance().apis.IfaceCompute();
    oclIFace.EnableMemorySnifferForPointer(return_value, size, sniffedRegionHandle);
  }
}

inline void clCreateBuffer_RECWRAP(CRecorder& recorder,
                                   cl_mem return_value,
                                   cl_context context,
                                   cl_mem_flags flags,
                                   size_t size,
                                   void* host_ptr,
                                   cl_int* errcode_ret) {
  if (recorder.Running()) {
    recorder.Schedule(
        new CclCreateBuffer(return_value, context, flags, size, host_ptr, errcode_ret));
  }
  clCreateBuffer_SD(return_value, context, flags, size, host_ptr, errcode_ret);
  const auto isUsingHostPtr = flags & (CL_MEM_COPY_HOST_PTR | CL_MEM_USE_HOST_PTR);
  if (recorder.Running() && ErrCodeSuccess(errcode_ret) && !isUsingHostPtr &&
      CheckCfgZeroInitialization(Config::Get(), IsReadOnlyBuffer(flags, nullptr))) {
    const auto commandQueue = GetCommandQueueRec(context, &recorder);
    if (commandQueue != nullptr && ZeroInitializeBuffer(commandQueue, return_value, size)) {
      const auto zeroBuffer = std::vector<char>(size, 0);
      recorder.Schedule(new CclEnqueueWriteBuffer(CL_SUCCESS, commandQueue, return_value,
                                                  CL_BLOCKING, 0, zeroBuffer.size(),
                                                  zeroBuffer.data(), 0, nullptr, nullptr));
    }
  }
}

inline void clCreateBufferWithPropertiesINTEL_RECWRAP(CRecorder& recorder,
                                                      cl_mem return_value,
                                                      cl_context context,
                                                      cl_mem_properties_intel* properties,
                                                      cl_mem_flags flags,
                                                      size_t size,
                                                      void* host_ptr,
                                                      cl_int* errcode_ret) {
  if (recorder.Running()) {
    recorder.Schedule(new CclCreateBufferWithPropertiesINTEL(return_value, context, properties,
                                                             flags, size, host_ptr, errcode_ret));
  }
  clCreateBufferWithPropertiesINTEL_SD(return_value, context, properties, flags, size, host_ptr,
                                       errcode_ret);
  bool isUsingHostPtr = false;
  if (properties != nullptr) {
    isUsingHostPtr =
        GetPropertyVal(properties, CL_MEM_FLAGS) & (CL_MEM_COPY_HOST_PTR | CL_MEM_USE_HOST_PTR);
  }
  isUsingHostPtr = isUsingHostPtr || (flags & (CL_MEM_COPY_HOST_PTR | CL_MEM_USE_HOST_PTR));
  if (recorder.Running() && ErrCodeSuccess(errcode_ret) && !isUsingHostPtr &&
      CheckCfgZeroInitialization(Config::Get(), IsReadOnlyBuffer(flags, properties))) {
    const auto commandQueue = GetCommandQueueRec(context, &recorder);
    if (commandQueue != nullptr && ZeroInitializeBuffer(commandQueue, return_value, size)) {
      const auto zeroBuffer = std::vector<char>(size, 0);
      recorder.Schedule(new CclEnqueueWriteBuffer(CL_SUCCESS, commandQueue, return_value,
                                                  CL_BLOCKING, 0, zeroBuffer.size(),
                                                  zeroBuffer.data(), 0, nullptr, nullptr));
    }
  }
}

inline void clCreateImage_RECWRAP(CRecorder& recorder,
                                  cl_mem return_value,
                                  cl_context context,
                                  cl_mem_flags flags,
                                  const cl_image_format* image_format,
                                  const cl_image_desc* image_desc,
                                  void* host_ptr,
                                  cl_int* errcode_ret) {
  if (recorder.Running()) {
    recorder.Schedule(new CclCreateImage(return_value, context, flags, image_format, image_desc,
                                         host_ptr, errcode_ret));
  }
  clCreateImage_SD(return_value, context, flags, image_format, image_desc, host_ptr, errcode_ret);
  const auto isUsingHostPtr = (flags & (CL_MEM_COPY_HOST_PTR | CL_MEM_USE_HOST_PTR));
  if (recorder.Running() && ErrCodeSuccess(errcode_ret) && !isUsingHostPtr &&
      CheckCfgZeroInitialization(Config::Get(), IsReadOnlyBuffer(flags, nullptr))) {
    const auto commandQueue = GetCommandQueueRec(context, &recorder);
    if (commandQueue != nullptr) {
      const auto size = CountImageSize(*image_format, *image_desc);
      if (ZeroInitializeImage(commandQueue, return_value, size, image_desc->image_width,
                              image_desc->image_height, image_desc->image_depth,
                              image_desc->image_row_pitch, image_desc->image_slice_pitch)) {
        const auto zeroBuffer = std::vector<char>(size, 0);
        const std::vector<size_t> origin = {0, 0, 0};
        const std::vector<size_t> region = {image_desc->image_width, image_desc->image_height,
                                            image_desc->image_depth};
        recorder.Schedule(new CclEnqueueWriteImage(
            CL_SUCCESS, commandQueue, return_value, CL_BLOCKING, origin.data(), region.data(),
            image_desc->image_row_pitch, image_desc->image_slice_pitch, zeroBuffer.data(), 0,
            nullptr, nullptr));
      }
    }
  }
}

inline void clCreateImage2D_RECWRAP(CRecorder& recorder,
                                    cl_mem return_value,
                                    cl_context context,
                                    cl_mem_flags flags,
                                    const cl_image_format* image_format,
                                    size_t image_width,
                                    size_t image_height,
                                    size_t image_row_pitch,
                                    void* host_ptr,
                                    cl_int* errcode_ret) {
  if (recorder.Running()) {
    recorder.Schedule(new CclCreateImage2D(return_value, context, flags, image_format, image_width,
                                           image_height, image_row_pitch, host_ptr, errcode_ret));
  }
  clCreateImage2D_SD(return_value, context, flags, image_format, image_width, image_height,
                     image_row_pitch, host_ptr, errcode_ret);
  const auto isUsingHostPtr = (flags & (CL_MEM_COPY_HOST_PTR | CL_MEM_USE_HOST_PTR));
  if (recorder.Running() && ErrCodeSuccess(errcode_ret) && !isUsingHostPtr &&
      CheckCfgZeroInitialization(Config::Get(), IsReadOnlyBuffer(flags, nullptr))) {
    const auto commandQueue = GetCommandQueueRec(context, &recorder);
    if (commandQueue != nullptr) {
      const auto size = CountImageSize(*image_format, image_width, image_height, image_row_pitch);
      if (ZeroInitializeImage(commandQueue, return_value, size, image_width, image_height, 1,
                              image_row_pitch, 0)) {
        const auto zeroBuffer = std::vector<char>(size, 0);
        const std::vector<size_t> origin = {0, 0, 0};
        const std::vector<size_t> region = {image_width, image_height, 1};
        recorder.Schedule(new CclEnqueueWriteImage(
            CL_SUCCESS, commandQueue, return_value, CL_BLOCKING, origin.data(), region.data(),
            image_row_pitch, 0, zeroBuffer.data(), 0, nullptr, nullptr));
      }
    }
  }
}

inline void clCreateImage3D_RECWRAP(CRecorder& recorder,
                                    cl_mem return_value,
                                    cl_context context,
                                    cl_mem_flags flags,
                                    const cl_image_format* image_format,
                                    size_t image_width,
                                    size_t image_height,
                                    size_t image_depth,
                                    size_t image_row_pitch,
                                    size_t image_slice_pitch,
                                    void* host_ptr,
                                    cl_int* errcode_ret) {
  if (recorder.Running()) {
    recorder.Schedule(new CclCreateImage3D(return_value, context, flags, image_format, image_width,
                                           image_height, image_depth, image_row_pitch,
                                           image_slice_pitch, host_ptr, errcode_ret));
  }
  clCreateImage3D_SD(return_value, context, flags, image_format, image_width, image_height,
                     image_depth, image_row_pitch, image_slice_pitch, host_ptr, errcode_ret);
  const auto isUsingHostPtr = (flags & (CL_MEM_COPY_HOST_PTR | CL_MEM_USE_HOST_PTR));
  if (recorder.Running() && ErrCodeSuccess(errcode_ret) && !isUsingHostPtr &&
      CheckCfgZeroInitialization(Config::Get(), IsReadOnlyBuffer(flags, nullptr))) {
    const auto commandQueue = GetCommandQueueRec(context, &recorder);
    if (commandQueue != nullptr) {
      const auto size = CountImageSize(*image_format, image_width, image_height, image_depth,
                                       image_row_pitch, image_slice_pitch);
      if (ZeroInitializeImage(commandQueue, return_value, size, image_width, image_height,
                              image_depth, image_row_pitch, image_slice_pitch)) {
        const auto zeroBuffer = std::vector<char>(size, 0);
        const std::vector<size_t> origin = {0, 0, 0};
        const std::vector<size_t> region = {image_width, image_height, image_width};
        recorder.Schedule(new CclEnqueueWriteImage(
            CL_SUCCESS, commandQueue, return_value, CL_BLOCKING, origin.data(), region.data(),
            image_row_pitch, image_slice_pitch, zeroBuffer.data(), 0, nullptr, nullptr));
      }
    }
  }
}

inline void clCreateImageWithPropertiesINTEL_RECWRAP(CRecorder& recorder,
                                                     cl_mem return_value,
                                                     cl_context context,
                                                     cl_mem_properties_intel* properties,
                                                     cl_mem_flags flags,
                                                     const cl_image_format* image_format,
                                                     const cl_image_desc* image_desc,
                                                     void* host_ptr,
                                                     cl_int* errcode_ret) {
  if (recorder.Running()) {
    recorder.Schedule(new CclCreateImageWithPropertiesINTEL(
        return_value, context, properties, flags, image_format, image_desc, host_ptr, errcode_ret));
  }
  clCreateImageWithPropertiesINTEL_SD(return_value, context, properties, flags, image_format,
                                      image_desc, host_ptr, errcode_ret);
  const auto isUsingHostPtr =
      (properties != nullptr &&
       (GetPropertyVal(properties, CL_MEM_FLAGS) & (CL_MEM_COPY_HOST_PTR | CL_MEM_USE_HOST_PTR))) ||
      (flags & (CL_MEM_COPY_HOST_PTR | CL_MEM_USE_HOST_PTR));
  if (recorder.Running() && ErrCodeSuccess(errcode_ret) && !isUsingHostPtr &&
      CheckCfgZeroInitialization(Config::Get(), IsReadOnlyBuffer(flags, properties))) {
    const auto commandQueue = GetCommandQueueRec(context, &recorder);
    if (commandQueue != nullptr) {
      const auto size = CountImageSize(*image_format, *image_desc);
      if (ZeroInitializeImage(commandQueue, return_value, size, image_desc->image_width,
                              image_desc->image_height, image_desc->image_depth,
                              image_desc->image_row_pitch, image_desc->image_slice_pitch)) {
        const auto zeroBuffer = std::vector<char>(size, 0);
        const std::vector<size_t> origin = {0, 0, 0};
        const std::vector<size_t> region = {image_desc->image_width, image_desc->image_height,
                                            image_desc->image_depth};
        recorder.Schedule(new CclEnqueueWriteImage(
            CL_SUCCESS, commandQueue, return_value, CL_BLOCKING, origin.data(), region.data(),
            image_desc->image_row_pitch, image_desc->image_slice_pitch, zeroBuffer.data(), 0,
            nullptr, nullptr));
      }
    }
  }
}

inline void clGetPlatformIDs_RECWRAP(CRecorder& recorder,
                                     cl_int return_value,
                                     cl_uint num_entries,
                                     cl_platform_id* platforms,
                                     cl_uint* num_platforms) {
  if (recorder.Running()) {
    recorder.Schedule(new CclGetPlatformIDs(return_value, num_entries, platforms, num_platforms));
    const auto& oclIFace = gits::CGits::Instance().apis.IfaceCompute();
    if (!oclIFace.MemorySnifferInstall()) {
      Log(WARN) << "Memory Sniffer installation failed";
    }
  }
  clGetPlatformIDs_SD(return_value, num_entries, platforms, num_platforms);
}

inline void clGetDeviceIDs_RECWRAP(CRecorder& recorder,
                                   cl_int return_value,
                                   cl_platform_id platform,
                                   cl_device_type device_type,
                                   cl_uint num_entries,
                                   cl_device_id* devices,
                                   cl_uint* num_devices) {
  if (recorder.Running()) {
    recorder.Schedule(new CclGetDeviceIDs(return_value, platform, device_type, num_entries, devices,
                                          num_devices));
    const auto& oclIFace = gits::CGits::Instance().apis.IfaceCompute();
    if (platform == nullptr && !oclIFace.MemorySnifferInstall()) {
      Log(WARN) << "Memory Sniffer installation failed";
    }
  }
  clGetDeviceIDs_SD(return_value, platform, device_type, num_entries, devices, num_devices);
}

inline void clEnqueueMemsetINTEL_RECWRAP(CRecorder& recorder,
                                         cl_int return_value,
                                         cl_command_queue command_queue,
                                         void* dst_ptr,
                                         cl_int value,
                                         size_t size,
                                         cl_uint num_events_in_wait_list,
                                         const cl_event* event_wait_list,
                                         cl_event* event) {
  CFunction* _token = nullptr;
  if (recorder.Running()) {
    if (IsUnsharingEnabled(Config::Get())) {
      std::vector<cl_event> filteredEvents(
          FilterSharingEvents(num_events_in_wait_list, event_wait_list));
      cl_uint num_filtered_events = static_cast<cl_uint>(filteredEvents.size());
      _token = new CclEnqueueMemsetINTEL(return_value, command_queue, dst_ptr, value, size,
                                         num_filtered_events, GetPointerFromVector(filteredEvents),
                                         event);
    } else {
      _token = new CclEnqueueMemsetINTEL(return_value, command_queue, dst_ptr, value, size,
                                         num_events_in_wait_list, event_wait_list, event);
    }
    recorder.Schedule(_token);
  }
  clEnqueueMemsetINTEL_SD(return_value, command_queue, dst_ptr, value, size,
                          num_events_in_wait_list, event_wait_list, event);
}

inline void clGitsIndirectAllocationOffsets_RECWRAP(CRecorder& recorder,
                                                    void* pAlloc,
                                                    uint32_t numOffsets,
                                                    size_t* pOffsets) {
  Log_clGitsIndirectAllocationOffsets(pAlloc, numOffsets, pOffsets);
  void* allocPtr = GetSvmOrUsmFromRegion(pAlloc).first;
  std::vector<size_t> offsets(numOffsets);
  if (allocPtr == nullptr) {
    Log(ERR) << "Couldn't correlate pAlloc " << ToStringHelper(pAlloc)
             << " to any device allocation";
    return;
  }
  const auto offset = GetPointerDifference(pAlloc, allocPtr);
  for (uint32_t i = 0U; i < numOffsets; i++) {
    offsets[i] = offset + pOffsets[i];
  }
  if (recorder.Running()) {
    recorder.Schedule(new CclGitsIndirectAllocationOffsets(allocPtr, numOffsets, offsets.data()));
  }
  clGitsIndirectAllocationOffsets_SD(allocPtr, numOffsets, offsets.data());
}

inline void clGetProgramInfo_RECWRAP(CRecorder& recorder,
                                     cl_int return_value,
                                     cl_program program,
                                     cl_program_info param_name,
                                     size_t param_value_size,
                                     void* param_value,
                                     size_t* param_value_size_ret) {
  if (recorder.Running()) {
    recorder.Schedule(new CclGetProgramInfo(
        return_value, program, param_name, param_value_size,
        param_name == CL_PROGRAM_BINARIES ? nullptr : param_value, param_value_size_ret));
  }
  clGetProgramInfo_SD(return_value, program, param_name, param_value_size, param_value,
                      param_value_size_ret);
}

inline void clCreateProgramWithBinary_V1_RECWRAP(CRecorder& recorder,
                                                 cl_program return_value,
                                                 cl_context context,
                                                 cl_uint num_devices,
                                                 const cl_device_id* device_list,
                                                 const size_t* lengths,
                                                 const unsigned char** binaries,
                                                 cl_int* binary_status,
                                                 cl_int* errcode_ret) {
  CFunction* token = nullptr;
  if (recorder.Running()) {
    token = new CclCreateProgramWithBinary_V1(return_value, context, num_devices, device_list,
                                              lengths, binaries, binary_status, errcode_ret);
    recorder.Schedule(token);
  }
  clCreateProgramWithBinary_SD(return_value, context, num_devices, device_list, lengths, binaries,
                               binary_status, errcode_ret);
  if (token != nullptr && ErrCodeSuccess(errcode_ret)) {
    auto& cBinaryArray = dynamic_cast<CBinariesArray_V1&>(token->Argument(4U));
    if (cBinaryArray.GetProgramBinaryLink() == ProgramBinaryLink::program) {
      SD().GetProgramState(return_value, EXCEPTION_MESSAGE)
          .SetBinaryLinkedProgram(cBinaryArray.GetProgramOriginal());
      Log(TRACEV) << "Setting binary linked program: "
                  << ToStringHelper(cBinaryArray.GetProgramOriginal());
    }
  }
}
inline void clSetEventCallback_RECWRAP(CRecorder& recorder,
                                       cl_int return_value,
                                       cl_event event,
                                       cl_int command_exec_callback_type,
                                       void(CL_CALLBACK* pfn_notify)(cl_event, cl_int, void*),
                                       void* user_data) {
  if (recorder.Running()) {
    bool scheduleEventStatus = false;
    const auto sharingEvent =
        IsUnsharingEnabled(Config::Get()) && IsSharingEventFilteringNeeded(event);
    if (sharingEvent) {
      cl_int errCode = CL_SUCCESS;
      auto& eventState = SD().GetEventState(event, EXCEPTION_MESSAGE);
      if (!eventState.isSharingUserEvent) {
        recorder.Schedule(new CclCreateUserEvent(event, eventState.context, &errCode));
        scheduleEventStatus = true;
        eventState.isSharingUserEvent = true;
      }
    }
    recorder.Schedule(new CclSetEventCallback(return_value, event, command_exec_callback_type,
                                              pfn_notify, user_data));
    if (scheduleEventStatus) {
      recorder.Schedule(new CclSetUserEventStatus(CL_SUCCESS, event, CL_COMPLETE));
    }
  }
}

} // namespace OpenCL
} // namespace gits

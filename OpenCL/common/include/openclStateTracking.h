// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "exception.h"
#include "oclFunction.h"
#include "openclArguments.h"
#include "openclDrivers.h"
#include "openclStateDynamic.h"
#include "openclTools.h"
#include "tools.h"

namespace gits {
namespace OpenCL {
namespace {
bool checkWhetherInject() {
  auto& cfg = Configurator::Get();
  const auto& kernels =
      Configurator::IsPlayer() ? cfg.opencl.player.captureKernels : cfg.opencl.recorder.dumpKernels;
  if (!kernels.empty()) {
    return kernels[CGits::Instance().CurrentKernelCount()];
  }
  return false;
}
} // namespace
inline void clBuildProgram_SD(cl_int return_value,
                              cl_program program,
                              cl_uint num_devices,
                              const cl_device_id* device_list,
                              const char* options,
                              void(CL_CALLBACK* pfn_notify)(cl_program, void*),
                              void* user_data) {
  if (ErrCodeSuccess(return_value)) {
    auto& programState = SD()._programStates[program];
    if (programState->HasHeaders()) {
      auto alreadyCreatedHeaders = std::set<std::string>();
      CreateHeaderFiles(programState->HeaderIncludeNames(), GetIncludePaths(options),
                        alreadyCreatedHeaders);
    }
    if (!programState->fileName.empty()) {
      LOG_TRACE << "^------------- Building file " << programState->fileName << std::endl;
    }
    programState->BuildProgram(num_devices, options);
    auto* prog = programState->GetBinaryLinkedProgram();
    if (prog != nullptr) {
      programState->SetBuildOptions(SD().GetProgramState(prog, EXCEPTION_MESSAGE).BuildOptions());
    }
  }
}

inline void clCompileProgram_SD(cl_int return_value,
                                cl_program program,
                                cl_uint num_devices,
                                const cl_device_id* device_list,
                                const char* options,
                                cl_uint num_input_headers,
                                const cl_program* input_headers,
                                const char** header_include_names,
                                void(CL_CALLBACK* pfn_notify)(cl_program, void*),
                                void* user_data) {
  if (ErrCodeSuccess(return_value)) {
    auto& programState = SD()._programStates[program];
    if (programState->HasHeaders()) {
      auto alreadyCreatedHeaders = std::set<std::string>();
      CreateHeaderFiles(programState->HeaderIncludeNames(), GetIncludePaths(options),
                        alreadyCreatedHeaders);
    }
    programState->CompileProgram(num_devices, num_input_headers, input_headers,
                                 header_include_names, options);
  }
}

inline void clRetainCommandQueue_SD(cl_int return_value, cl_command_queue command_queue) {
  if (ErrCodeSuccess(return_value)) {
    SD().GetCommandQueueState(command_queue, EXCEPTION_MESSAGE).Retain();
  }
}

inline void clRetainContext_SD(cl_int return_value, cl_context context) {
  if (ErrCodeSuccess(return_value)) {
    SD().GetContextState(context, EXCEPTION_MESSAGE).Retain();
  }
}

inline void clRetainDevice_SD(cl_int return_value, cl_device_id device) {
  if (ErrCodeSuccess(return_value)) {
    auto& deviceState = SD().GetDeviceIDState(device, EXCEPTION_MESSAGE);
    if (deviceState.parentDevice == nullptr) {
      return;
    }
    deviceState.Retain();
  }
}

inline void clRetainEvent_SD(cl_int return_value, cl_event event) {
  if (ErrCodeSuccess(return_value)) {
    SD().GetEventState(event, EXCEPTION_MESSAGE).Retain();
  }
}

inline void clRetainKernel_SD(cl_int return_value, cl_kernel kernel) {
  if (ErrCodeSuccess(return_value)) {
    SD().GetKernelState(kernel, EXCEPTION_MESSAGE).Retain();
  }
}

inline void clRetainMemObject_SD(cl_int return_value, cl_mem memobj) {
  if (ErrCodeSuccess(return_value)) {
    SD().GetMemState(memobj, EXCEPTION_MESSAGE).Retain();
  }
}

inline void clRetainProgram_SD(cl_int return_value, cl_program program) {
  if (ErrCodeSuccess(return_value)) {
    SD().GetProgramState(program, EXCEPTION_MESSAGE).Retain();
  }
}

inline void clRetainSampler_SD(cl_int return_value, cl_sampler sampler) {
  if (ErrCodeSuccess(return_value)) {
    SD().GetSamplerState(sampler, EXCEPTION_MESSAGE).Retain();
  }
}

inline void clCreateBuffer_SD(cl_mem return_value,
                              cl_context context,
                              cl_mem_flags flags,
                              size_t size,
                              void* host_ptr,
                              cl_int* errcode_ret) {
  if (ErrCodeSuccess(errcode_ret)) {
    auto& memState = SD()._memStates[return_value];
    memState.reset(new CCLMemState(context, flags, size, host_ptr));
    memState->Retain();
    if (host_ptr != nullptr && FlagUseHostPtr(flags)) {
      memState->buffer_number = SD()._buffers.size() - 1;
    }
  }
}

inline void clCreateBufferWithPropertiesINTEL_SD(cl_mem return_value,
                                                 cl_context context,
                                                 cl_mem_properties_intel* properties,
                                                 cl_mem_flags flags,
                                                 size_t size,
                                                 void* host_ptr,
                                                 cl_int* errcode_ret) {
  if (ErrCodeSuccess(errcode_ret)) {
    auto& memState = SD()._memStates[return_value];
    memState.reset(new CCLMemState(context, flags, properties, size, host_ptr));
    memState->Retain();
    if (host_ptr &&
        ((properties != nullptr && FlagUseHostPtr(GetPropertyVal(properties, CL_MEM_FLAGS))) ||
         FlagUseHostPtr(flags))) {
      memState->buffer_number = SD()._buffers.size() - 1;
    }
  }
}

inline void clCreateCommandQueue_SD(cl_command_queue return_value,
                                    cl_context context,
                                    cl_device_id device,
                                    cl_command_queue_properties properties,
                                    cl_int* errcode_ret) {
  if (ErrCodeSuccess(errcode_ret)) {
    auto& cqState = SD()._commandQueueStates[return_value];
    auto& contextState = SD()._contextStates[context];
    cqState.reset(new CCLCommandQueueState(context, device, properties));
    contextState->commandQueueArray.push_back(return_value);
    cqState->Retain();
  }
}

inline void clCreateContext_SD(
    cl_context return_value,
    const cl_context_properties* properties,
    cl_uint num_devices,
    const cl_device_id* devices,
    void(CL_CALLBACK* pfn_notify)(const char*, const void*, size_t, void*),
    void* user_data,
    cl_int* errcode_ret) {
  if (ErrCodeSuccess(errcode_ret)) {
    auto& contextState = SD()._contextStates[return_value];
    contextState.reset(new CCLContextState(false, properties));
    contextState->Retain();
    if (ErrCodeSuccess(errcode_ret) && num_devices > 0 && devices != nullptr) {
      SD()._contextStates[return_value]->devices.assign(devices, devices + num_devices);
    }
  }
}

inline void clCreateContextFromType_SD(
    cl_context return_value,
    const cl_context_properties* properties,
    cl_device_type device_type,
    void(CL_CALLBACK* pfn_notify)(const char*, const void*, size_t, void*),
    void* user_data,
    cl_int* errcode_ret) {
  if (ErrCodeSuccess(errcode_ret)) {
    auto& contextState = SD()._contextStates[return_value];
    contextState.reset(new CCLContextState(true, properties, device_type));
    contextState->Retain();
  }
}

#ifdef GITS_PLATFORM_WINDOWS
inline void clCreateFromDX9MediaSurfaceINTEL_SD(cl_mem return_value,
                                                cl_context context,
                                                cl_mem_flags flags,
                                                IDirect3DSurface9* resource,
                                                void* shared_handle,
                                                UINT plane,
                                                cl_int* errcode_ret) {
  CreateStateFromSharedImage(return_value, context, flags, errcode_ret);
}
#endif

inline void clCreateFromGLBuffer_SD(cl_mem return_value,
                                    cl_context context,
                                    cl_mem_flags flags,
                                    cl_GLuint bufobj,
                                    cl_int* errcode_ret) {
  CreateStateFromSharedBuffer(return_value, context, flags, errcode_ret);
}

inline void clCreateFromGLRenderbuffer_SD(cl_mem return_value,
                                          cl_context context,
                                          cl_mem_flags flags,
                                          cl_GLuint bufobj,
                                          cl_int* errcode_ret) {
  CreateStateFromSharedImage(return_value, context, flags, errcode_ret);
}

inline void clCreateFromGLTexture_SD(cl_mem return_value,
                                     cl_context context,
                                     cl_mem_flags flags,
                                     cl_GLenum target,
                                     cl_GLint miplevel,
                                     cl_GLuint texture,
                                     cl_int* errcode_ret) {
  CreateStateFromSharedImage(return_value, context, flags, errcode_ret);
}

inline void clCreateFromGLTexture2D_SD(cl_mem return_value,
                                       cl_context context,
                                       cl_mem_flags flags,
                                       cl_GLenum target,
                                       cl_GLint miplevel,
                                       cl_GLuint texture,
                                       cl_int* errcode_ret) {
  CreateStateFromSharedImage(return_value, context, flags, errcode_ret);
}

inline void clCreateFromGLTexture3D_SD(cl_mem return_value,
                                       cl_context context,
                                       cl_mem_flags flags,
                                       cl_GLenum target,
                                       cl_GLint miplevel,
                                       cl_GLuint texture,
                                       cl_int* errcode_ret) {
  CreateStateFromSharedImage(return_value, context, flags, errcode_ret);
}

inline void clCreateImage_SD(cl_mem return_value,
                             cl_context context,
                             cl_mem_flags flags,
                             const cl_image_format* image_format,
                             const cl_image_desc* image_desc,
                             void* host_ptr,
                             cl_int* errcode_ret) {
  if (ErrCodeSuccess(errcode_ret)) {
    auto& sd = SD();
    auto& memState = sd._memStates[return_value];
    memState.reset(new CCLMemState(context, flags, CountImageSize(*image_format, *image_desc),
                                   image_format, image_desc, host_ptr));
    memState->image_format = *image_format;
    memState->image_desc = *image_desc;
    memState->Retain();
    if (host_ptr != nullptr) {
      memState->buffer_number = sd._buffers.size() - 1;
    }
    if (image_desc->mem_object != nullptr) {
      clRetainMemObject_SD(CL_SUCCESS, image_desc->mem_object);
    }
  }
}

inline void clCreateImage2D_SD(cl_mem return_value,
                               cl_context context,
                               cl_mem_flags flags,
                               const cl_image_format* image_format,
                               size_t image_width,
                               size_t image_height,
                               size_t image_row_pitch,
                               void* host_ptr,
                               cl_int* errcode_ret) {
  if (ErrCodeSuccess(errcode_ret)) {
    cl_image_desc desc = {CL_MEM_OBJECT_IMAGE2D,
                          image_width,
                          image_height,
                          1,
                          0,
                          image_row_pitch,
                          0,
                          0,
                          0,
                          {nullptr}};
    auto& memState = SD()._memStates[return_value];
    memState.reset(new CCLMemState(context, flags, CountImageSize(*image_format, desc),
                                   image_format, &desc, host_ptr));
    memState->image_format = *image_format;
    memState->image_desc = desc;
    memState->Retain();
    if (host_ptr != nullptr) {
      memState->buffer_number = SD()._buffers.size() - 1;
    }
  }
}

inline void clCreateImage3D_SD(cl_mem return_value,
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
  if (ErrCodeSuccess(errcode_ret)) {
    cl_image_desc desc = {CL_MEM_OBJECT_IMAGE3D,
                          image_width,
                          image_height,
                          image_depth,
                          0,
                          image_row_pitch,
                          image_slice_pitch,
                          0,
                          0,
                          {nullptr}};
    auto& memState = SD()._memStates[return_value];
    memState.reset(new CCLMemState(context, flags, CountImageSize(*image_format, desc),
                                   image_format, &desc, host_ptr));
    memState->image_format = *image_format;
    memState->image_desc = desc;
    memState->Retain();
    if (host_ptr != nullptr) {
      memState->buffer_number = SD()._buffers.size() - 1;
    }
  }
}

inline void clCreateKernel_SD(cl_kernel return_value,
                              cl_program program,
                              const char* kernel_name,
                              cl_int* errcode_ret) {
  if (ErrCodeSuccess(errcode_ret)) {
    auto& kernelState = SD()._kernelStates[return_value];
    kernelState.reset(new CCLKernelState(program, kernel_name));
    kernelState->Retain();
  }
}

inline void clCreateKernelsInProgram_SD(cl_int return_value,
                                        cl_program program,
                                        cl_uint num_kernels,
                                        cl_kernel* kernels,
                                        cl_uint* num_kernels_ret) {
  if (ErrCodeSuccess(return_value) && kernels != nullptr) {
    auto validKernelsNum = num_kernels; // kernels exist so num_kernels > 0
    if (num_kernels_ret != nullptr) {
      // valid kernels count could be smaller than num_kernels
      validKernelsNum = std::min(num_kernels, *num_kernels_ret);
    }
    for (cl_uint i = 0; i < validKernelsNum; i++) {
      auto& kernelState = SD()._kernelStates[kernels[i]];
      kernelState.reset(new CCLKernelState(program, ""));
      kernelState->Retain();
    }
  }
}

inline void clCreateProgramWithBinary_SD(CFunction* token,
                                         cl_program return_value,
                                         cl_context context,
                                         cl_uint num_devices,
                                         const cl_device_id* device_list,
                                         const size_t* lengths,
                                         const unsigned char** binaries,
                                         cl_int* binary_status,
                                         cl_int* errcode_ret) {
  if (ErrCodeSuccess(errcode_ret)) {
    auto& programState = SD()._programStates[return_value];
    programState.reset(new CCLProgramState(context, num_devices, device_list, binaries, lengths));
    programState->Retain();
    if (token != nullptr) {
      auto& cBinaryArray = dynamic_cast<CBinariesArray_V1&>(token->Argument(4U));
      if (cBinaryArray.FileNames().empty()) {
        throw EOperationFailed(EXCEPTION_MESSAGE);
      }
      programState->fileName = cBinaryArray.FileNames()[0];
    }
  }
}

inline void clCreateProgramWithBuiltInKernels_SD(cl_program return_value,
                                                 cl_context context,
                                                 cl_uint num_devices,
                                                 const cl_device_id* device_list,
                                                 const char* kernel_names,
                                                 cl_int* errcode_ret) {
  if (ErrCodeSuccess(errcode_ret)) {
    auto& programState = SD()._programStates[return_value];
    programState.reset(new CCLProgramState(context, num_devices, device_list, kernel_names));
    programState->Retain();
  }
}

inline void clCreateProgramWithSource_SD(CFunction* token,
                                         cl_program return_value,
                                         cl_context context,
                                         cl_uint count,
                                         const char** strings,
                                         const size_t* lengths,
                                         cl_int* errcode_ret) {
  if (ErrCodeSuccess(errcode_ret)) {
    auto& programState = SD()._programStates[return_value];
    std::string filename;
    if (token != nullptr) {
      filename = static_cast<CProgramSource&>(token->Argument(2)).FileName();
    }
    programState.reset(new CCLProgramState(context, count, strings, lengths, std::move(filename)));
    programState->Retain();
    programState->isKernelArgInfoAvailable = true;
  }
}

inline void clCreateProgramWithIL_SD(CFunction* token,
                                     cl_program return_value,
                                     cl_context context,
                                     const void* il,
                                     size_t length,
                                     cl_int* errcode_ret) {
  if (ErrCodeSuccess(errcode_ret)) {
    auto& programState = SD()._programStates[return_value];
    programState.reset(new CCLProgramState(context, il, length));
    if (token != nullptr) {
      programState->fileName = static_cast<CProgramSource&>(token->Argument(1U)).FileName();
    }
    programState->Retain();
  }
}

inline void clCreateSampler_SD(cl_sampler return_value,
                               cl_context context,
                               cl_bool normalized_coords,
                               cl_addressing_mode addressing_mode,
                               cl_filter_mode filter_mode,
                               cl_int* errcode_ret) {
  if (ErrCodeSuccess(errcode_ret)) {
    auto& samplerState = SD()._samplerStates[return_value];
    samplerState.reset(
        new CCLSamplerState(context, normalized_coords, addressing_mode, filter_mode));
    samplerState->Retain();
  }
}

inline void clCreateSubBuffer_SD(cl_mem return_value,
                                 cl_mem buffer,
                                 cl_mem_flags flags,
                                 cl_buffer_create_type buffer_create_type,
                                 const void* buffer_create_info,
                                 cl_int* errcode_ret) {
  if (ErrCodeSuccess(errcode_ret) && buffer_create_info != nullptr) {
    auto& memState = SD()._memStates[return_value];
    memState.reset(new CCLMemState(buffer, flags, buffer_create_type, buffer_create_info));
    memState->Retain();
    auto& parentMemState = SD().GetMemState(buffer, EXCEPTION_MESSAGE);
    parentMemState.Retain();
  }
}

#ifdef GITS_PLATFORM_WINDOWS
inline void clEnqueueAcquireD3D10ObjectsKHR_SD(cl_int return_value,
                                               cl_command_queue command_queue,
                                               cl_uint num_objects,
                                               const cl_mem* mem_objects,
                                               cl_uint num_events_in_wait_list,
                                               const cl_event* event_wait_list,
                                               cl_event* event) {
  RegisterEvents(event, command_queue, return_value);
  if (event && return_value == CL_SUCCESS) {
    SD()._eventStates[*event]->isDXSharingEvent = true;
  }
}
#endif

inline void clEnqueueAcquireGLObjects_SD(cl_int return_value,
                                         cl_command_queue command_queue,
                                         cl_uint num_objects,
                                         const cl_mem* mem_objects,
                                         cl_uint num_events_in_wait_list,
                                         const cl_event* event_wait_list,
                                         cl_event* event) {
  RegisterEvents(event, command_queue, return_value);
  if (event && return_value == CL_SUCCESS) {
    SD()._eventStates[*event]->isGLSharingEvent = true;
  }
}

inline void clEnqueueBarrierWithWaitList_SD(cl_int return_value,
                                            cl_command_queue command_queue,
                                            cl_uint num_events_in_wait_list,
                                            const cl_event* event_wait_list,
                                            cl_event* event) {
  RegisterEvents(event, command_queue, return_value);
}

inline void clEnqueueCopyBuffer_SD(CFunction* token,
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
  RegisterEvents(event, command_queue, return_value);
}

inline void clEnqueueCopyBufferRect_SD(CFunction* token,
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
  RegisterEvents(event, command_queue, return_value);
}

inline void clEnqueueCopyBufferToImage_SD(CFunction* token,
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
  RegisterEvents(event, command_queue, return_value);
}

inline void clEnqueueCopyImage_SD(CFunction* token,
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
  RegisterEvents(event, command_queue, return_value);
}

inline void clEnqueueCopyImageToBuffer_SD(CFunction* token,
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
  RegisterEvents(event, command_queue, return_value);
}

inline void clEnqueueFillBuffer_SD(CFunction* token,
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
  RegisterEvents(event, command_queue, return_value);
}

inline void clEnqueueFillImage_SD(CFunction* token,
                                  cl_int return_value,
                                  cl_command_queue command_queue,
                                  cl_mem image,
                                  const void* fill_color,
                                  const size_t* origin,
                                  const size_t* region,
                                  cl_uint num_events_in_wait_list,
                                  const cl_event* event_wait_list,
                                  cl_event* event) {
  RegisterEvents(event, command_queue, return_value);
}

inline void clEnqueueMapBuffer_SD(CFunction* token,
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
  RegisterEvents(event, command_queue, errcode_ret);
  auto& sd = SD();
  if (token != nullptr && return_value != nullptr &&
      sd._memStates.find(buffer) != sd._memStates.end()) {
    auto& memState = sd.GetMemState(buffer, EXCEPTION_MESSAGE);
    void* originalPtr = Configurator::IsPlayer()
                            ? static_cast<CCLMappedPtr&>(token->Argument(0U)).Original()
                            : return_value;
    memState.originalMappedPtrs.push_back(originalPtr);
  }
}

inline void clEnqueueMapImage_SD(CFunction* token,
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
  RegisterEvents(event, command_queue, errcode_ret);
  auto& sd = SD();
  if (token != nullptr && return_value != nullptr &&
      sd._memStates.find(image) != sd._memStates.end()) {
    auto& memState = sd.GetMemState(image, EXCEPTION_MESSAGE);
    void* originalPtr = Configurator::IsPlayer()
                            ? static_cast<CCLMappedPtr&>(token->Argument(0U)).Original()
                            : return_value;
    memState.originalMappedPtrs.push_back(originalPtr);
  }
}

inline void clEnqueueMarker_SD(cl_int return_value,
                               cl_command_queue command_queue,
                               cl_event* event) {
  RegisterEvents(event, command_queue, return_value);
}

inline void clEnqueueMarkerWithWaitList_SD(cl_int return_value,
                                           cl_command_queue command_queue,
                                           cl_uint num_events_in_wait_list,
                                           const cl_event* event_wait_list,
                                           cl_event* event) {
  RegisterEvents(event, command_queue, return_value);
}

inline void clEnqueueMigrateMemObjects_SD(cl_int return_value,
                                          cl_command_queue command_queue,
                                          cl_uint num_mem_objects,
                                          const cl_mem* mem_objects,
                                          cl_mem_migration_flags flags,
                                          cl_uint num_events_in_wait_list,
                                          const cl_event* event_wait_list,
                                          cl_event* event) {
  RegisterEvents(event, command_queue, return_value);
}

inline void clEnqueueNDRangeKernel_SD(cl_int return_value,
                                      cl_command_queue command_queue,
                                      cl_kernel kernel,
                                      cl_uint work_dim,
                                      const size_t* global_work_offset,
                                      const size_t* global_work_size,
                                      const size_t* local_work_size,
                                      cl_uint num_events_in_wait_list,
                                      const cl_event* event_wait_list,
                                      cl_event* event) {
  if (SD()._kernelStates[kernel]->name.empty()) {
    size_t size = 0;
    cl_int error = drvOcl.clGetKernelInfo(kernel, CL_KERNEL_FUNCTION_NAME, 0, nullptr, &size);
    // size_t is unsigned, casting prevents overflow issue when size==0
    std::string kernelName(std::max(static_cast<int>(size - 1), 0), 0);
    error |= drvOcl.clGetKernelInfo(kernel, CL_KERNEL_FUNCTION_NAME, size, &kernelName[0], nullptr);
    if (error) {
      kernelName = "UNKNOWN_" + std::to_string(CGits::Instance().CurrentKernelCount());
    }
    SD()._kernelStates[kernel]->name = std::move(kernelName);
  }
  LOG_TRACE << "--- kernel call #" << CGits::Instance().CurrentKernelCount() << ", kernel name \""
            << SD()._kernelStates[kernel]->name << "\" ---" << std::endl;
  RegisterEvents(event, command_queue, return_value);
  if (Configurator::IsRecorder() && HasUsmPtrsToUpdate(kernel)) {
    // have to wait for kernel execution before protecting memory
    drvOcl.clFinish(command_queue);
    UpdateUsmPtrs(kernel);
  }
  if (checkWhetherInject() || Configurator::Get().opencl.player.aubSignaturesCL) {
    if (num_events_in_wait_list != 0U) {
      for (auto i = 0U; i < num_events_in_wait_list; i++) {
        const auto& e = event_wait_list[i];
        if (SD()._eventStates.find(e) != SD()._eventStates.end() &&
            SD().GetEventState(e, EXCEPTION_MESSAGE).isUserEvent) {
          drvOcl.clSetUserEventStatus(e, CL_COMPLETE);
          LOG_INFO << "Setting user event status to CL_COMPLETE, event: " << ToStringHelper(e);
        }
      }
    }
    InjectKernelArgOperations(kernel, command_queue, event);
  }
}

inline void clEnqueueReadBuffer_SD(CFunction* token,
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
  RegisterEvents(event, command_queue, return_value);
}

inline void clEnqueueReadBufferRect_SD(CFunction* token,
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
  RegisterEvents(event, command_queue, return_value);
}

inline void clEnqueueReadImage_SD(CFunction* token,
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
  RegisterEvents(event, command_queue, return_value);
}

#ifdef GITS_PLATFORM_WINDOWS
inline void clEnqueueReleaseD3D10ObjectsKHR_SD(cl_int return_value,
                                               cl_command_queue command_queue,
                                               cl_uint num_objects,
                                               const cl_mem* mem_objects,
                                               cl_uint num_events_in_wait_list,
                                               const cl_event* event_wait_list,
                                               cl_event* event) {
  RegisterEvents(event, command_queue, return_value);
  if (event && return_value == CL_SUCCESS) {
    SD()._eventStates[*event]->isDXSharingEvent = true;
  }
}
#endif

inline void clEnqueueReleaseGLObjects_SD(cl_int return_value,
                                         cl_command_queue command_queue,
                                         cl_uint num_objects,
                                         const cl_mem* mem_objects,
                                         cl_uint num_events_in_wait_list,
                                         const cl_event* event_wait_list,
                                         cl_event* event) {
  RegisterEvents(event, command_queue, return_value);
  if (event && return_value == CL_SUCCESS) {
    SD()._eventStates[*event]->isGLSharingEvent = true;
  }
}

inline void clEnqueueTask_SD(cl_int return_value,
                             cl_command_queue command_queue,
                             cl_kernel kernel,
                             cl_uint num_events_in_wait_list,
                             const cl_event* event_wait_list,
                             cl_event* event) {
  RegisterEvents(event, command_queue, return_value);
}

inline void clEnqueueUnmapMemObject_SD(CFunction* token,
                                       cl_int return_value,
                                       cl_command_queue command_queue,
                                       cl_mem memobj,
                                       void* mapped_ptr,
                                       cl_uint num_events_in_wait_list,
                                       const cl_event* event_wait_list,
                                       cl_event* event) {
  RegisterEvents(event, command_queue, return_value);
}

inline void clEnqueueWriteBuffer_SD(CFunction* token,
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
  RegisterEvents(event, command_queue, return_value);
}

inline void clEnqueueWriteBufferRect_SD(CFunction* token,
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
  RegisterEvents(event, command_queue, return_value);
}

inline void clEnqueueWriteImage_SD(CFunction* token,
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
  RegisterEvents(event, command_queue, return_value);
}

inline void clGetContextInfo_SD(cl_int return_value,
                                cl_context context,
                                cl_context_info param_name,
                                size_t param_value_size,
                                void* param_value,
                                size_t* param_value_size_ret) {
  if (IsDeviceQuery(param_name) && ErrCodeSuccess(return_value) && param_value_size) {
    cl_device_id* devices = static_cast<cl_device_id*>(param_value);
    size_t devicesCount = param_value_size / sizeof(cl_device_id);
    if (param_value_size_ret) {
      size_t validDevNum = *param_value_size_ret / sizeof(cl_device_id);
      devicesCount = std::min(devicesCount, validDevNum);
    }
    SD()._contextStates[context]->devices.clear();
    for (size_t i = 0; i < devicesCount; i++) {
      if (devices[i]) {
        if (SD()._deviceIDStates.find(devices[i]) == SD()._deviceIDStates.end()) {
          SD()._deviceIDStates[devices[i]].reset(new CCLDeviceIDState());
        }
        SD()._contextStates[context]->devices.push_back(devices[i]);
      }
    }
  }
}

inline void clGetDeviceIDs_SD(cl_int return_value,
                              cl_platform_id platform,
                              cl_device_type device_type,
                              cl_uint num_entries,
                              cl_device_id* devices,
                              cl_uint* num_devices) {
  if (return_value == CL_SUCCESS && devices != nullptr) {
    auto validDevNum = num_entries;
    if (num_devices != nullptr) {
      validDevNum = std::min(num_entries, *num_devices);
    }
    auto& sd = SD();
    const auto updatePlatformState = !sd._platformIDStates.empty();
    auto deviceType = device_type;
    for (auto i = 0U; i < validDevNum; i++) {
      if (deviceType == CL_DEVICE_TYPE_ALL) {
        deviceType = GetDeviceType(devices[i]);
      }
      if (devices[i] != nullptr && updatePlatformState) {
        auto& platformState = platform == nullptr
                                  ? *sd._platformIDStates.begin()->second
                                  : sd.GetPlatformIDState(platform, EXCEPTION_MESSAGE);
        platformState.AddDevice(devices[i], deviceType);
      }
      if (sd._deviceIDStates.find(devices[i]) == sd._deviceIDStates.end()) {
        sd._deviceIDStates[devices[i]].reset(new CCLDeviceIDState(platform, deviceType));
        sd._deviceIDStates[devices[i]]->Retain();
      }
    }
  }
}

inline void clGetKernelInfo_SD(cl_int return_value,
                               cl_kernel kernel,
                               cl_kernel_info param_name,
                               size_t param_value_size,
                               void* param_value,
                               size_t* param_value_size_ret) {
  if (return_value == CL_SUCCESS && param_name == CL_KERNEL_FUNCTION_NAME &&
      param_value != nullptr) {
    SD().GetKernelState(kernel, EXCEPTION_MESSAGE).name = static_cast<char*>(param_value);
  }
}

inline void clGetPlatformIDs_SD(cl_int return_value,
                                cl_uint num_entries,
                                cl_platform_id* platforms,
                                cl_uint* num_platforms) {
  if (ErrCodeSuccess(return_value) && num_entries > 0 && platforms != nullptr) {
    auto& sd = SD();
    if (Configurator::IsPlayer()) {
      sd.originalPlaybackPlatforms.clear();
    }
    sd._platformIDStates.clear();
    for (auto i = 0U; i < num_entries; i++) {
      sd._platformIDStates[platforms[i]].reset(new CCLPlatformIDState());
      sd._platformIDStates[platforms[i]]->Retain();
    }
  }
}

inline void clGetProgramInfo_SD(cl_int return_value,
                                cl_program program,
                                cl_program_info param_name,
                                size_t param_value_size,
                                void* param_value,
                                size_t* param_value_size_ret) {
  if (return_value == CL_SUCCESS && param_value_size > 0U) {
    auto& programState = SD().GetProgramState(program, EXCEPTION_MESSAGE);
    if (param_name == CL_PROGRAM_BINARY_SIZES) {
      programState.GetProgramInfoBinarySizes(param_value_size, param_value);
    } else if (param_name == CL_PROGRAM_NUM_DEVICES) {
      programState.GetProgramInfoNumDevices(*static_cast<cl_uint*>(param_value));
    } else if (param_name == CL_PROGRAM_BINARIES) {
      // if application asked for program binary data it might
      // reuse it in clCreateProgramWithBinary in a form of `online
      // compilation`
      programState.GetProgramInfoBinaries(param_value_size, param_value);
      // Retain program to save cl_program address.
      drvOcl.clRetainProgram(program);
      programState.Retain();
    }
  }
}

inline void clLinkProgram_SD(cl_program return_value,
                             cl_context context,
                             cl_uint num_devices,
                             const cl_device_id* device_list,
                             const char* options,
                             cl_uint num_input_programs,
                             const cl_program* input_programs,
                             void(CL_CALLBACK* pfn_notify)(cl_program, void*),
                             void* user_data,
                             cl_int* errcode_ret) {
  if (ErrCodeSuccess(errcode_ret)) {
    auto& programState = SD()._programStates[return_value];
    programState.reset(new CCLProgramState(context, num_input_programs, input_programs, options));
    programState->Retain();
    auto* prog = programState->GetBinaryLinkedProgram();
    if (prog != nullptr) {
      programState->SetBuildOptions(SD().GetProgramState(prog, EXCEPTION_MESSAGE).BuildOptions());
    }
  }
}

inline void clReleaseCommandQueue_SD(cl_int return_value, cl_command_queue command_queue) {
  ReleaseResourceState(SD()._commandQueueStates, command_queue);
}

inline void clReleaseContext_SD(cl_int return_value, cl_context context) {
  const auto& queue = SD().GetContextState(context, EXCEPTION_MESSAGE).fakeQueue;
  if (queue != nullptr) {
    drvOcl.clReleaseCommandQueue(queue);
  }
  ReleaseResourceState(SD()._contextStates, context);
}

inline void clReleaseDevice_SD(cl_int return_value, cl_device_id device) {
  if (ErrCodeSuccess(return_value) && device != nullptr) {
    auto& deviceState = SD().GetDeviceIDState(device, EXCEPTION_MESSAGE);
    if (deviceState.parentDevice == nullptr) {
      return;
    }
    ReleaseResourceState(SD()._deviceIDStates, device);
  }
}

inline void clReleaseEvent_SD(cl_int return_value, cl_event event) {
  ReleaseResourceState(SD()._eventStates, event);
}

inline void clReleaseMemObject_SD(cl_int return_value, cl_mem memobj) {
  if (memobj != nullptr) {
    const auto& memState = SD().GetMemState(memobj, EXCEPTION_MESSAGE);
    if (memState.GetRefCount() == 1U) {
      if (memState.bufferObj != nullptr) {
        clReleaseMemObject_SD(return_value, memState.bufferObj);
      }
      if (memState.image && memState.image_desc.mem_object != nullptr) {
        clReleaseMemObject_SD(return_value, memState.image_desc.mem_object);
      }
      // workaround for keeping mapped pointer mapping alive for the same addresses
      std::map<void*, bool> mappingsToRemove;
      for (const auto& ptr : memState.originalMappedPtrs) {
        mappingsToRemove[ptr] = true;
      }
      for (const auto& memoryState : SD()._memStates) {
        if (memoryState.first != memobj) {
          for (const auto& ptr : memoryState.second->originalMappedPtrs) {
            for (const auto& mappingPtr : memState.originalMappedPtrs) {
              if (ptr == mappingPtr) {
                mappingsToRemove[ptr] = false;
              }
            }
          }
        }
      }
      for (const auto& mapping : mappingsToRemove) {
        if (mapping.second) {
          CCLMappedPtr::RemoveMapping(mapping.first);
        }
      }
    }
  }
  ReleaseResourceState(SD()._memStates, memobj);
}

inline void clReleaseProgram_SD(cl_int return_value, cl_program program) {
  if (program != nullptr && ErrCodeSuccess(return_value)) {
    auto& programState = SD().GetProgramState(program, EXCEPTION_MESSAGE);
    if (programState.GetRefCount() == 1U) {
      for (auto& ptr : programState.GetGlobalPointers()) {
        ReleaseResourceState(SD()._usmAllocStates, ptr);
      }
    }
  }
  ReleaseResourceState(SD()._programStates, program);
}

inline void clReleaseKernel_SD(cl_int return_value, cl_kernel kernel) {
  auto& kernelStates = SD()._kernelStates;
  if (kernel != nullptr && kernelStates.at(kernel)->GetRefCount() == 1U) {
    const auto& program = kernelStates.at(kernel)->program;
    if (SD()._programStates.find(program) != SD()._programStates.end()) {
      clReleaseProgram_SD(return_value, program);
    }
  }
  ReleaseResourceState(kernelStates, kernel);
}

inline void clReleaseSampler_SD(cl_int return_value, cl_sampler sampler) {
  ReleaseResourceState(SD()._samplerStates, sampler);
}

inline void clSetKernelArg_SD(cl_int return_value,
                              cl_kernel kernel,
                              cl_uint arg_index,
                              size_t arg_size,
                              const void* arg_value) {
  auto& kernelState = SD()._kernelStates[kernel];
  kernelState->SetArgument(arg_index, arg_size, arg_value);
  kernelState->SetArgSetType(arg_index, KernelSetType::normal);
}

inline void clCreateCommandQueueWithProperties_SD(cl_command_queue return_value,
                                                  cl_context context,
                                                  cl_device_id device,
                                                  const cl_queue_properties* properties,
                                                  cl_int* errcode_ret) {
  if (ErrCodeSuccess(errcode_ret)) {
    auto& cqState = SD()._commandQueueStates[return_value];
    auto& contextState = SD()._contextStates[context];
    cqState.reset(new CCLCommandQueueState(context, device, properties));
    contextState->commandQueueArray.push_back(return_value);
    cqState->Retain();
  }
}

inline void clCreatePipe_SD(cl_mem return_value,
                            cl_context context,
                            cl_mem_flags flags,
                            cl_uint pipe_packet_size,
                            cl_uint pipe_max_packets,
                            const cl_pipe_properties* properties,
                            cl_int* errcode_ret) {
  if (ErrCodeSuccess(errcode_ret)) {
    auto& memState = SD()._memStates[return_value];
    memState.reset(new CCLMemState(context, flags, pipe_packet_size, pipe_max_packets, properties));
    memState->Retain();
  }
}

inline void clCreateSamplerWithProperties_SD(cl_sampler return_value,
                                             cl_context context,
                                             const cl_sampler_properties* sampler_properties,
                                             cl_int* errcode_ret) {
  if (ErrCodeSuccess(errcode_ret)) {
    auto& samplerState = SD()._samplerStates[return_value];
    samplerState.reset(new CCLSamplerState(context, sampler_properties));
    samplerState->Retain();
  }
}

inline void clEnqueueSVMFree_SD(cl_int return_value,
                                cl_command_queue command_queue,
                                cl_uint num_svm_pointers,
                                void** svm_pointers,
                                void(CL_CALLBACK* pfn_free_func)(cl_command_queue queue,
                                                                 cl_uint num_svm_pointers,
                                                                 void** svm_pointers,
                                                                 void* user_data),
                                void* user_data,
                                cl_uint num_events_in_wait_list,
                                const cl_event* event_wait_list,
                                cl_event* event) {
  RegisterEvents(event, command_queue, return_value);
  for (cl_uint i = 0; i < num_svm_pointers; ++i) {
    auto& svmAllocState = SD().GetSVMAllocState(svm_pointers[i], EXCEPTION_MESSAGE);
    svmAllocState.Release();
    if (svmAllocState.GetRefCount() == 0) {
      SD()._svmAllocStates.erase(svm_pointers[i]);
    }
  }
}

inline void clEnqueueSVMMap_SD(cl_int return_value,
                               cl_command_queue command_queue,
                               cl_bool blocking_map,
                               cl_map_flags flags,
                               void* svm_ptr,
                               size_t size,
                               cl_uint num_events_in_wait_list,
                               const cl_event* event_wait_list,
                               cl_event* event) {
  RegisterEvents(event, command_queue, return_value);
}

inline void clEnqueueSVMMemFill_SD(cl_int return_value,
                                   cl_command_queue command_queue,
                                   void* svm_ptr,
                                   const void* pattern,
                                   size_t pattern_size,
                                   size_t size,
                                   cl_uint num_events_in_wait_list,
                                   const cl_event* event_wait_list,
                                   cl_event* event) {
  RegisterEvents(event, command_queue, return_value);
}

inline void clEnqueueSVMMemcpy_SD(cl_int return_value,
                                  cl_command_queue command_queue,
                                  cl_bool blocking_copy,
                                  void* dst_ptr,
                                  const void* src_ptr,
                                  size_t size,
                                  cl_uint num_events_in_wait_list,
                                  const cl_event* event_wait_list,
                                  cl_event* event) {
  RegisterEvents(event, command_queue, return_value);
}

inline void clEnqueueSVMUnmap_SD(cl_int return_value,
                                 cl_command_queue command_queue,
                                 void* svm_ptr,
                                 cl_uint num_events_in_wait_list,
                                 const cl_event* event_wait_list,
                                 cl_event* event) {
  RegisterEvents(event, command_queue, return_value);
}

inline void clSVMAlloc_SD(void* return_value,
                          cl_context context,
                          cl_svm_mem_flags flags,
                          size_t size,
                          cl_uint alignment) {
  if (return_value != nullptr) {
    auto& svmAllocState = SD()._svmAllocStates[return_value];
    svmAllocState.reset(new CCLSVMAllocState(context, flags, size, alignment));
    svmAllocState->Retain();
  }
}

inline void clCreateUserEvent_SD(cl_event return_value, cl_context context, cl_int* errcode_ret) {
  if (ErrCodeSuccess(errcode_ret)) {
    auto& eventState = SD()._eventStates[return_value];
    eventState.reset(new CCLEventState());
    eventState->Retain();
    eventState->context = context;
    eventState->isUserEvent = true;
  }
}

inline void clSVMFree_SD(cl_context context, void* svm_pointer) {
  ReleaseResourceState(SD()._svmAllocStates, svm_pointer);
}

#ifdef GITS_PLATFORM_WINDOWS
inline void clCreateFromD3D10BufferKHR_SD(cl_mem return_value,
                                          cl_context context,
                                          cl_mem_flags flags,
                                          ID3D10Buffer* resource,
                                          cl_int* errcode_ret) {
  CreateStateFromSharedBuffer(return_value, context, flags, errcode_ret);
}

inline void clCreateFromD3D10Texture2DKHR_SD(cl_mem return_value,
                                             cl_context context,
                                             cl_mem_flags flags,
                                             ID3D10Texture2D* resource,
                                             UINT subresource,
                                             cl_int* errcode_ret) {
  CreateStateFromSharedImage(return_value, context, flags, errcode_ret);
}

inline void clCreateFromD3D10Texture3DKHR_SD(cl_mem return_value,
                                             cl_context context,
                                             cl_mem_flags flags,
                                             ID3D10Texture3D* resource,
                                             UINT subresource,
                                             cl_int* errcode_ret) {
  CreateStateFromSharedImage(return_value, context, flags, errcode_ret);
}

inline void clCreateFromD3D11BufferKHR_SD(cl_mem return_value,
                                          cl_context context,
                                          cl_mem_flags flags,
                                          ID3D11Buffer* resource,
                                          cl_int* errcode_ret) {
  CreateStateFromSharedBuffer(return_value, context, flags, errcode_ret);
}

inline void clCreateFromD3D11Texture2DKHR_SD(cl_mem return_value,
                                             cl_context context,
                                             cl_mem_flags flags,
                                             ID3D11Texture2D* resource,
                                             UINT subresource,
                                             cl_int* errcode_ret) {
  CreateStateFromSharedImage(return_value, context, flags, errcode_ret);
}

inline void clCreateFromD3D11Texture3DKHR_SD(cl_mem return_value,
                                             cl_context context,
                                             cl_mem_flags flags,
                                             ID3D11Texture3D* resource,
                                             UINT subresource,
                                             cl_int* errcode_ret) {
  CreateStateFromSharedImage(return_value, context, flags, errcode_ret);
}

inline void clCreateFromDX9MediaSurfaceKHR_SD(cl_mem return_value,
                                              cl_context context,
                                              cl_mem_flags flags,
                                              cl_dx9_media_adapter_type_khr adapter_type,
                                              void* surface_info,
                                              cl_uint plane,
                                              cl_int* errcode_ret) {
  CreateStateFromSharedImage(return_value, context, flags, errcode_ret);
}
#endif

inline void clHostMemAllocINTEL_SD(void* return_value,
                                   cl_context context,
                                   cl_mem_properties_intel* properties,
                                   size_t size,
                                   cl_uint alignment,
                                   cl_int* errcode_ret) {
  if (return_value) {
    auto& usmAllocState = SD()._usmAllocStates[return_value];
    usmAllocState.reset(
        new CCLUSMAllocState(context, properties, size, alignment, UnifiedMemoryType::host));
    usmAllocState->Retain();
  }
}

inline void clDeviceMemAllocINTEL_SD(void* return_value,
                                     cl_context context,
                                     cl_device_id device,
                                     cl_mem_properties_intel* properties,
                                     size_t size,
                                     cl_uint alignment,
                                     cl_int* errcode_ret) {
  if (return_value) {
    auto& usmAllocState = SD()._usmAllocStates[return_value];
    usmAllocState.reset(new CCLUSMAllocState(context, device, properties, size, alignment,
                                             UnifiedMemoryType::device));
    usmAllocState->Retain();
  }
}

inline void clSharedMemAllocINTEL_SD(void* return_value,
                                     cl_context context,
                                     cl_device_id device,
                                     cl_mem_properties_intel* properties,
                                     size_t size,
                                     cl_uint alignment,
                                     cl_int* errcode_ret) {
  if (return_value) {
    auto& usmAllocState = SD()._usmAllocStates[return_value];
    usmAllocState.reset(new CCLUSMAllocState(context, device, properties, size, alignment,
                                             UnifiedMemoryType::shared));
    usmAllocState->Retain();
  }
}

inline void clMemFreeINTEL_SD(cl_int return_value, cl_context context, void* usm_pointer) {
  if (usm_pointer) {
    ReleaseResourceState(SD()._usmAllocStates, usm_pointer);
  }
}

inline void clEnqueueMemcpyINTEL_SD(cl_int return_value,
                                    cl_command_queue command_queue,
                                    cl_bool blocking,
                                    void* dst_ptr,
                                    const void* src_ptr,
                                    size_t size,
                                    cl_uint num_events_in_wait_list,
                                    const cl_event* event_wait_list,
                                    cl_event* event) {
  RegisterEvents(event, command_queue, return_value);
  if (Configurator::IsRecorder()) {
    drvOcl.clFinish(command_queue);
  }
}

inline void clEnqueueMemAdviseINTEL_SD(cl_int return_value,
                                       cl_command_queue command_queue,
                                       const void* ptr,
                                       size_t size,
                                       cl_mem_advice_intel advice,
                                       cl_uint num_events_in_wait_list,
                                       const cl_event* event_wait_list,
                                       cl_event* event) {
  RegisterEvents(event, command_queue, return_value);
}

inline void clEnqueueMigrateMemINTEL_SD(cl_int return_value,
                                        cl_command_queue command_queue,
                                        const void* ptr,
                                        size_t size,
                                        cl_mem_migration_flags flags,
                                        cl_uint num_events_in_wait_list,
                                        const cl_event* event_wait_list,
                                        cl_event* event) {
  RegisterEvents(event, command_queue, return_value);
}

inline void clSetKernelArgMemPointerINTEL_SD(cl_int return_value,
                                             cl_kernel kernel,
                                             cl_uint arg_index,
                                             const void* arg_value) {
  auto& kernelState = SD().GetKernelState(kernel, EXCEPTION_MESSAGE);
  kernelState.SetArgument(arg_index, 0, arg_value);
  void* ptr = const_cast<void*>(arg_value);
  if (SD().CheckIfUSMAllocExists(GetUsmPtrFromRegion(ptr).first)) {
    kernelState.SetArgType(arg_index, KernelArgType::usm);
  } else if (SD().CheckIfSVMAllocExists(GetSvmPtrFromRegion(ptr).first)) {
    kernelState.SetArgType(arg_index, KernelArgType::svm);
  } else {
    kernelState.SetArgType(arg_index, KernelArgType::pointer);
  }
  kernelState.SetArgSetType(arg_index, KernelSetType::usm);
}

inline void clSetKernelExecInfo_SD(cl_int return_value,
                                   cl_kernel kernel,
                                   cl_kernel_exec_info param_name,
                                   size_t param_value_size,
                                   const void* param_value) {
  if (return_value == CL_SUCCESS) {
    auto& kernelState = SD().GetKernelState(kernel, EXCEPTION_MESSAGE);
    switch (param_name) {
    case CL_KERNEL_EXEC_INFO_USM_PTRS_INTEL:
    case CL_KERNEL_EXEC_INFO_SVM_PTRS: {
      const size_t ptrsNumber = param_value_size / sizeof(void*);
      kernelState.indirectUsmPointers.clear();
      kernelState.indirectUsmPointers.resize(ptrsNumber);
      for (size_t i = 0; i < ptrsNumber; i++) {
        kernelState.indirectUsmPointers[i] = static_cast<void**>(const_cast<void*>(param_value))[i];
      }
      break;
    }
    case CL_KERNEL_EXEC_INFO_INDIRECT_HOST_ACCESS_INTEL:
      kernelState.indirectUsmTypes =
          *static_cast<const cl_bool*>(param_value) == CL_TRUE
              ? kernelState.indirectUsmTypes | static_cast<unsigned>(UnifiedMemoryType::host)
              : kernelState.indirectUsmTypes & ~static_cast<unsigned>(UnifiedMemoryType::host);
      break;
    case CL_KERNEL_EXEC_INFO_INDIRECT_DEVICE_ACCESS_INTEL:
      kernelState.indirectUsmTypes =
          *static_cast<const cl_bool*>(param_value) == CL_TRUE
              ? kernelState.indirectUsmTypes | static_cast<unsigned>(UnifiedMemoryType::device)
              : kernelState.indirectUsmTypes & ~static_cast<unsigned>(UnifiedMemoryType::device);
      break;
    case CL_KERNEL_EXEC_INFO_INDIRECT_SHARED_ACCESS_INTEL:
      kernelState.indirectUsmTypes =
          *static_cast<const cl_bool*>(param_value) == CL_TRUE
              ? kernelState.indirectUsmTypes | static_cast<unsigned>(UnifiedMemoryType::shared)
              : kernelState.indirectUsmTypes & ~static_cast<unsigned>(UnifiedMemoryType::shared);
      break;
    }
  }
}

inline void clEnqueueNDCountKernelINTEL_SD(cl_int return_value,
                                           cl_command_queue command_queue,
                                           cl_kernel kernel,
                                           cl_uint workDim,
                                           const size_t* globalWorkOffset,
                                           const size_t* workGroupCount,
                                           const size_t* localWorkSize,
                                           cl_uint numEventsInWaitList,
                                           const cl_event* eventWaitList,
                                           cl_event* event) {
  // calculate gws if needed
  clEnqueueNDRangeKernel_SD(return_value, command_queue, kernel, workDim, globalWorkOffset, nullptr,
                            localWorkSize, numEventsInWaitList, eventWaitList, event);
}

inline void clSetKernelArgSVMPointer_SD(cl_int return_value,
                                        cl_kernel kernel,
                                        cl_uint arg_index,
                                        const void* arg_value) {
  auto& kernelState = SD().GetKernelState(kernel, EXCEPTION_MESSAGE);
  kernelState.SetArgument(arg_index, 0, arg_value);
  void* ptr = const_cast<void*>(arg_value);
  if (SD().CheckIfUSMAllocExists(GetUsmPtrFromRegion(ptr).first)) {
    kernelState.SetArgType(arg_index, KernelArgType::usm);
  } else if (SD().CheckIfSVMAllocExists(GetSvmPtrFromRegion(ptr).first)) {
    kernelState.SetArgType(arg_index, KernelArgType::svm);
  } else {
    kernelState.SetArgType(arg_index, KernelArgType::pointer);
  }
  kernelState.SetArgSetType(arg_index, KernelSetType::svm);
}

inline void clEnqueueMemFillINTEL_SD(cl_int return_value,
                                     cl_command_queue command_queue,
                                     void* dst_ptr,
                                     const void* pattern,
                                     size_t pattern_size,
                                     size_t size,
                                     cl_uint num_events_in_wait_list,
                                     const cl_event* event_wait_list,
                                     cl_event* event) {
  RegisterEvents(event, command_queue, return_value);
}

inline void clMemBlockingFreeINTEL_SD(cl_int return_value, cl_context context, void* ptr) {
  if (ptr != nullptr) {
    ReleaseResourceState(SD()._usmAllocStates, ptr);
  }
}

inline void clCreateImageWithPropertiesINTEL_SD(cl_mem return_value,
                                                cl_context context,
                                                cl_mem_properties_intel* properties,
                                                cl_mem_flags flags,
                                                const cl_image_format* imageFormat,
                                                const cl_image_desc* imageDesc,
                                                void* host_ptr,
                                                cl_int* errcode_ret) {
  if (ErrCodeSuccess(errcode_ret)) {
    auto& sd = SD();
    auto& memState = sd._memStates[return_value];
    memState.reset(new CCLMemState(context, properties, CountImageSize(*imageFormat, *imageDesc),
                                   imageFormat, imageDesc, host_ptr));
    memState->image_format = *imageFormat;
    memState->image_desc = *imageDesc;
    memState->flags = flags;
    memState->Retain();
    if (host_ptr) {
      memState->buffer_number = sd._buffers.size() - 1;
    }
    if (imageDesc->mem_object != nullptr) {
      clRetainMemObject_SD(CL_SUCCESS, imageDesc->mem_object);
    }
  }
}

inline void clGetDeviceGlobalVariablePointerINTEL_SD(cl_int return_value,
                                                     cl_device_id device,
                                                     cl_program program,
                                                     const char* global_variable_name,
                                                     size_t* global_variable_size_ret,
                                                     void** global_variable_pointer_ret) {
  if (ErrCodeSuccess(return_value)) {
    auto& usmAllocState = SD()._usmAllocStates[*global_variable_pointer_ret];
    usmAllocState.reset(new CCLUSMAllocState(device, *global_variable_size_ret,
                                             UnifiedMemoryType::device, program,
                                             global_variable_name));
    usmAllocState->Retain();
    auto& programState = SD().GetProgramState(program, EXCEPTION_MESSAGE);
    programState.AppendGlobalPointer(*global_variable_pointer_ret);
  }
}

inline void clEnqueueResourcesBarrierINTEL_SD(
    cl_int return_value,
    cl_command_queue command_queue,
    cl_uint resource_count,
    const cl_resource_barrier_descriptor_intel* barrier_descriptors,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event) {
  RegisterEvents(event, command_queue, return_value);
}

inline void clEnqueueSVMMigrateMem_SD(cl_int return_value,
                                      cl_command_queue command_queue,
                                      cl_uint num_svm_pointers,
                                      const void** svm_pointers,
                                      const size_t* sizes,
                                      cl_mem_migration_flags flags,
                                      cl_uint num_events_in_wait_list,
                                      const cl_event* event_wait_list,
                                      cl_event* event) {
  RegisterEvents(event, command_queue, return_value);
}

inline void clCreateSubDevices_SD(cl_int return_value,
                                  cl_device_id in_device,
                                  const cl_device_partition_property* properties,
                                  cl_uint num_entries,
                                  cl_device_id* out_devices,
                                  cl_uint* num_devices) {
  auto numDevices = num_entries;
  if (num_devices) {
    numDevices = std::min(num_entries, *num_devices);
  }
  if (out_devices && ErrCodeSuccess(return_value)) {
    const auto subcaptureMode =
        (Configurator::IsRecorder() && !CGits::Instance().apis.IfaceCompute().CfgRec_IsAllMode());
    for (auto i = 0u; i < numDevices; i++) {
      auto& deviceState = SD()._deviceIDStates[out_devices[i]];
      deviceState.reset(new CCLDeviceIDState(in_device, properties));
      deviceState->Retain();
      // Internal retain for state restore
      if (subcaptureMode) {
        deviceState->Retain();
      }
    }
    if (subcaptureMode) {
      auto& parentDeviceState = SD()._deviceIDStates[in_device];
      parentDeviceState->Retain();
    }
  }
}

inline void clGitsIndirectAllocationOffsets_SD(void* pAlloc,
                                               uint32_t numOffsets,
                                               size_t* pOffsets) {
  const auto isBufferTranslated = Configurator::IsRecorder();
  if (SD().CheckIfUSMAllocExists(pAlloc)) {
    auto& allocState = SD().GetUSMAllocState(pAlloc, EXCEPTION_MESSAGE);
    for (uint32_t i = 0U; i < numOffsets; i++) {
      allocState.indirectPointersOffsets[pOffsets[i]] = isBufferTranslated;
    }
  } else {
    auto& allocState = SD().GetSVMAllocState(pAlloc, EXCEPTION_MESSAGE);
    for (uint32_t i = 0U; i < numOffsets; i++) {
      allocState.indirectPointersOffsets[pOffsets[i]] = isBufferTranslated;
    }
  }
}

inline void clEnqueueMemsetINTEL_SD(cl_int return_value,
                                    cl_command_queue command_queue,
                                    void* dst_ptr,
                                    cl_int value,
                                    size_t size,
                                    cl_uint num_events_in_wait_list,
                                    const cl_event* event_wait_list,
                                    cl_event* event) {
  RegisterEvents(event, command_queue, return_value);
}
#ifdef GITS_PLATFORM_WINDOWS
inline void clGetDeviceIDsFromD3D10KHR_SD(cl_int return_value,
                                          cl_platform_id platform,
                                          cl_d3d10_device_source_khr d3d_device_source,
                                          void* d3d_object,
                                          cl_d3d10_device_set_khr d3d_device_set,
                                          cl_uint num_entries,
                                          cl_device_id* devices,
                                          cl_uint* num_devices) {
  clGetDeviceIDs_SD(return_value, platform, CL_DEVICE_TYPE_ALL, num_entries, devices, num_devices);
}
#endif

#ifdef GITS_PLATFORM_WINDOWS
inline void clGetDeviceIDsFromD3D11KHR_SD(cl_int return_value,
                                          cl_platform_id platform,
                                          cl_d3d11_device_source_khr d3d_device_source,
                                          void* d3d_object,
                                          cl_d3d11_device_set_khr d3d_device_set,
                                          cl_uint num_entries,
                                          cl_device_id* devices,
                                          cl_uint* num_devices) {
  clGetDeviceIDs_SD(return_value, platform, CL_DEVICE_TYPE_ALL, num_entries, devices, num_devices);
}
#endif

#ifdef GITS_PLATFORM_WINDOWS
inline void clGetDeviceIDsFromD3D11NV_SD(cl_int return_value,
                                         cl_platform_id platform,
                                         cl_d3d11_device_source_nv d3d_device_source,
                                         void* d3d_object,
                                         cl_d3d11_device_set_nv d3d_device_set,
                                         cl_uint num_entries,
                                         cl_device_id* devices,
                                         cl_uint* num_devices) {
  clGetDeviceIDs_SD(return_value, platform, CL_DEVICE_TYPE_ALL, num_entries, devices, num_devices);
}
#endif

#ifdef GITS_PLATFORM_WINDOWS
inline void clGetDeviceIDsFromDX9INTEL_SD(cl_int return_value,
                                          cl_platform_id platform,
                                          cl_dx9_device_source_intel dx9_device_source,
                                          void* dx9_object,
                                          cl_dx9_device_set_intel dx9_device_set,
                                          cl_uint num_entries,
                                          cl_device_id* devices,
                                          cl_uint* num_devices) {
  clGetDeviceIDs_SD(return_value, platform, CL_DEVICE_TYPE_ALL, num_entries, devices, num_devices);
}
#endif

inline void clCloneKernel_SD(cl_kernel return_value, cl_kernel source_kernel, cl_int* errcode_ret) {
  if (ErrCodeSuccess(errcode_ret)) {
    auto& kernelState = SD().GetKernelState(source_kernel, EXCEPTION_MESSAGE);
    auto& cloneKernelState = SD()._kernelStates[return_value];
    cloneKernelState.reset(new CCLKernelState(kernelState.program, kernelState.name.c_str()));
    cloneKernelState->Retain();
    for (const auto& args : cloneKernelState->GetArguments()) {
      cloneKernelState->SetArgument(args.first, args.second.argSize, args.second.argValue);
      cloneKernelState->SetArgType(args.first, args.second.type);
    }
    cloneKernelState->indirectUsmTypes = kernelState.indirectUsmTypes;
    cloneKernelState->indirectUsmPointers = kernelState.indirectUsmPointers;
    cloneKernelState->clonedKernel = source_kernel;
  }
}

inline void clGetExtensionFunctionAddressForPlatform_SD(void* return_value,
                                                        cl_platform_id platform,
                                                        const char* function_name) {
  if (return_value != nullptr && platform != nullptr) {
    auto& platformState = SD().GetPlatformIDState(platform, EXCEPTION_MESSAGE);
    platformState.AddExtensionFunction(function_name);
  }
}

inline void clGetDeviceInfo_SD(cl_int return_value,
                               cl_device_id device,
                               cl_device_info param_name,
                               size_t param_value_size,
                               void* param_value,
                               size_t* param_value_size_ret) {
  if (ErrCodeSuccess(return_value) && param_value_size > 0) {
    if (param_name == CL_DEVICE_TYPE) {
      auto& deviceState = SD().GetDeviceIDState(device, EXCEPTION_MESSAGE);
      deviceState.type = *reinterpret_cast<cl_device_type*>(param_value);
    } else if (param_name == CL_DEVICE_PLATFORM) {
      auto platform = *reinterpret_cast<cl_platform_id*>(param_value);
      auto deviceType = GetDeviceType(device);
      auto& platformState = SD().GetPlatformIDState(platform, EXCEPTION_MESSAGE);
      platformState.AddDevice(device, deviceType);
    }
  }
}

inline void clGetPlatformInfo_SD(cl_int return_value,
                                 cl_platform_id platform,
                                 cl_platform_info param_name,
                                 size_t param_value_size,
                                 void* param_value,
                                 size_t* param_value_size_ret) {
  if (ErrCodeSuccess(return_value) && param_name == CL_PLATFORM_VENDOR && param_value_size > 0) {
    constexpr char intelPlatformVendorName[] = "Intel(R) Corporation";
    constexpr size_t intelPlatformVendorNameSize =
        sizeof(intelPlatformVendorName) / sizeof(intelPlatformVendorName[0]);
    auto& platformState = SD().GetPlatformIDState(platform, EXCEPTION_MESSAGE);
    const auto isIntelPlatform =
        intelPlatformVendorNameSize == param_value_size &&
        strcmp(intelPlatformVendorName, static_cast<const char*>(param_value)) == 0;
    if (isIntelPlatform) {
      platformState.SetIntelPlatform();
    }
  }
}

inline void clCreateProgramWithILKHR_SD(CFunction* token,
                                        cl_program return_value,
                                        cl_context context,
                                        const void* il,
                                        size_t length,
                                        cl_int* errcode_ret) {
  clCreateProgramWithIL_SD(token, return_value, context, il, length, errcode_ret);
  if (ErrCodeSuccess(errcode_ret)) {
    auto& programState = SD().GetProgramState(return_value, EXCEPTION_MESSAGE);
    programState.isKhrApi = true;
  }
}

} // namespace OpenCL
} // namespace gits

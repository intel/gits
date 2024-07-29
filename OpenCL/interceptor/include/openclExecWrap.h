// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "openclDrivers.h"
#include "openclRecorderWrapperIface.h"
#include "gitsPluginOpenCL.h"

#include "exception.h"
#include "gits.h"
#include "log.h"
#include "platform.h"

#include <map>

using namespace gits::OpenCL;

#define OCLATTRIB
namespace {
// Avoid recording API - recursive functions.
uint32_t recursionDepth = 0;
constexpr uint32_t disableDepth = 1000;
} // namespace

void PrePostDisableOpenCL() {
  recursionDepth = disableDepth;
}

#define GITS_WRAPPER_PRE                                                                           \
  --recursionDepth;                                                                                \
  if (CGitsPluginOpenCL::Configuration().common.recorder.enabled && (recursionDepth == 0)) {       \
    try {                                                                                          \
      wrapper.TrackThread();

#define GITS_WRAPPER_POST                                                                          \
  wrapper.CloseRecorderIfRequired();                                                               \
  }                                                                                                \
  catch (...) {                                                                                    \
    topmost_exception_handler(__FUNCTION__);                                                       \
  }                                                                                                \
  }

#define GITS_ENTRY                                                                                 \
  CGitsPluginOpenCL::Initialize();                                                                 \
  IRecorderWrapper& wrapper = CGitsPluginOpenCL::RecorderWrapper();                                \
  std::unique_lock<std::recursive_mutex> lock(wrapper.GetInterceptorMutex());                      \
  ++recursionDepth;                                                                                \
  COclDriver& drv = wrapper.Drivers();                                                             \
  wrapper.InitializeDriver();

#define GITS_ENTRY_OCL GITS_ENTRY

namespace gits {
namespace OpenCL {
void* GetExtensionFunction(const char* function_name);

inline cl_int clEnqueueUnmapMemObject_RECEXECWRAP(cl_command_queue command_queue,
                                                  cl_mem memobj,
                                                  void* mapped_ptr,
                                                  cl_uint num_events_in_wait_list,
                                                  const cl_event* event_wait_list,
                                                  cl_event* event) {
  GITS_ENTRY_OCL
  cl_int return_value = CL_SUCCESS;
  GITS_WRAPPER_PRE
  wrapper.clEnqueueUnmapMemObject_pre(command_queue, memobj, mapped_ptr, num_events_in_wait_list,
                                      event_wait_list, event);
  return_value = drv.clEnqueueUnmapMemObject(command_queue, memobj, mapped_ptr,
                                             num_events_in_wait_list, event_wait_list, event);
  wrapper.clEnqueueUnmapMemObject(return_value, command_queue, memobj, mapped_ptr,
                                  num_events_in_wait_list, event_wait_list, event);
  GITS_WRAPPER_POST
  // else is needed if drv call is made inside WRAPPER_PRE/POST, to send
  // the calls after requested kernel is captured in subcapture mode
  // see gitsPluginOpenCL.cpp:127 - recursionDepth is set to 1000 at the end of the stream
  else {
    return_value = drv.clEnqueueUnmapMemObject(command_queue, memobj, mapped_ptr,
                                               num_events_in_wait_list, event_wait_list, event);
  }
  return return_value;
}

void* clGetExtensionFunctionAddressForPlatform_RECEXECWRAP(cl_platform_id platform,
                                                           const char* function_name) {
  GITS_ENTRY_OCL(void) drv; // unused variable WA
  void* return_value = GetExtensionFunction(function_name);
  GITS_WRAPPER_PRE
  wrapper.clGetExtensionFunctionAddressForPlatform(return_value, platform, function_name);
  GITS_WRAPPER_POST
  return return_value;
}

void* clGetExtensionFunctionAddress_RECEXECWRAP(const char* function_name) {
  GITS_ENTRY_OCL(void) drv; // unused variable WA
  void* return_value = GetExtensionFunction(function_name);
  GITS_WRAPPER_PRE
  wrapper.clGetExtensionFunctionAddress(return_value, function_name);
  GITS_WRAPPER_POST
  return return_value;
}

inline cl_int clEnqueueNDRangeKernel_RECEXECWRAP(cl_command_queue command_queue,
                                                 cl_kernel kernel,
                                                 cl_uint work_dim,
                                                 const size_t* global_work_offset,
                                                 const size_t* global_work_size,
                                                 const size_t* local_work_size,
                                                 cl_uint num_events_in_wait_list,
                                                 const cl_event* event_wait_list,
                                                 cl_event* event) {
  GITS_ENTRY_OCL
  cl_int return_value = CL_SUCCESS;
  GITS_WRAPPER_PRE
  wrapper.UnProtectMemoryPointers();
  wrapper.clEnqueueNDRangeKernel_pre(return_value, command_queue, kernel, work_dim,
                                     global_work_offset, global_work_size, local_work_size,
                                     num_events_in_wait_list, event_wait_list, event);
  return_value = drv.clEnqueueNDRangeKernel(command_queue, kernel, work_dim, global_work_offset,
                                            global_work_size, local_work_size,
                                            num_events_in_wait_list, event_wait_list, event);
  wrapper.clEnqueueNDRangeKernel(return_value, command_queue, kernel, work_dim, global_work_offset,
                                 global_work_size, local_work_size, num_events_in_wait_list,
                                 event_wait_list, event);
  wrapper.ProtectMemoryPointers();
  GITS_WRAPPER_POST
  // else is needed if drv call is made inside WRAPPER_PRE/POST, to send
  // the calls after requested kernel is captured in subcapture mode
  // see gitsPluginOpenCL.cpp:127 - recursionDepth is set to 1000 at the end of the stream
  else {
    return_value = drv.clEnqueueNDRangeKernel(command_queue, kernel, work_dim, global_work_offset,
                                              global_work_size, local_work_size,
                                              num_events_in_wait_list, event_wait_list, event);
  }
  return return_value;
}

inline cl_mem clCreateBuffer_RECEXECWRAP(
    cl_context context, cl_mem_flags flags, size_t size, void* host_ptr, cl_int* errcode_ret) {
  GITS_ENTRY_OCL
  cl_mem return_value = drv.clCreateBuffer(
      context, flags & ~(CL_MEM_HOST_NO_ACCESS | CL_MEM_HOST_WRITE_ONLY | CL_MEM_HOST_READ_ONLY),
      size, host_ptr, errcode_ret);
  GITS_WRAPPER_PRE
  wrapper.clCreateBuffer(return_value, context, flags, size, host_ptr, errcode_ret);
  GITS_WRAPPER_POST
  return return_value;
}

inline cl_mem clCreateImage_RECEXECWRAP(cl_context context,
                                        cl_mem_flags flags,
                                        const cl_image_format* image_format,
                                        const cl_image_desc* image_desc,
                                        void* host_ptr,
                                        cl_int* errcode_ret) {
  GITS_ENTRY_OCL
  cl_mem return_value = drv.clCreateImage(
      context, flags & ~(CL_MEM_HOST_NO_ACCESS | CL_MEM_HOST_WRITE_ONLY | CL_MEM_HOST_READ_ONLY),
      image_format, image_desc, host_ptr, errcode_ret);
  GITS_WRAPPER_PRE
  wrapper.clCreateImage(return_value, context, flags, image_format, image_desc, host_ptr,
                        errcode_ret);
  GITS_WRAPPER_POST
  return return_value;
}

inline cl_mem clCreateImage2D_RECEXECWRAP(cl_context context,
                                          cl_mem_flags flags,
                                          const cl_image_format* image_format,
                                          size_t image_width,
                                          size_t image_height,
                                          size_t image_row_pitch,
                                          void* host_ptr,
                                          cl_int* errcode_ret) {
  GITS_ENTRY_OCL
  cl_mem return_value = drv.clCreateImage2D(
      context, flags & ~(CL_MEM_HOST_NO_ACCESS | CL_MEM_HOST_WRITE_ONLY | CL_MEM_HOST_READ_ONLY),
      image_format, image_width, image_height, image_row_pitch, host_ptr, errcode_ret);
  GITS_WRAPPER_PRE
  wrapper.clCreateImage2D(return_value, context, flags, image_format, image_width, image_height,
                          image_row_pitch, host_ptr, errcode_ret);
  GITS_WRAPPER_POST
  return return_value;
}

inline cl_mem clCreateImage3D_RECEXECWRAP(cl_context context,
                                          cl_mem_flags flags,
                                          const cl_image_format* image_format,
                                          size_t image_width,
                                          size_t image_height,
                                          size_t image_depth,
                                          size_t image_row_pitch,
                                          size_t image_slice_pitch,
                                          void* host_ptr,
                                          cl_int* errcode_ret) {
  GITS_ENTRY_OCL
  cl_mem return_value = drv.clCreateImage3D(
      context, flags & ~(CL_MEM_HOST_NO_ACCESS | CL_MEM_HOST_WRITE_ONLY | CL_MEM_HOST_READ_ONLY),
      image_format, image_width, image_height, image_depth, image_row_pitch, image_slice_pitch,
      host_ptr, errcode_ret);
  GITS_WRAPPER_PRE
  wrapper.clCreateImage3D(return_value, context, flags, image_format, image_width, image_height,
                          image_depth, image_row_pitch, image_slice_pitch, host_ptr, errcode_ret);
  GITS_WRAPPER_POST
  return return_value;
}

inline cl_int clEnqueueNDCountKernelINTEL_RECEXECWRAP(cl_command_queue command_queue,
                                                      cl_kernel kernel,
                                                      cl_uint workDim,
                                                      const size_t* globalWorkOffset,
                                                      const size_t* workGroupCount,
                                                      const size_t* localWorkSize,
                                                      cl_uint numEventsInWaitList,
                                                      const cl_event* eventWaitList,
                                                      cl_event* event) {
  GITS_ENTRY_OCL
  cl_int return_value = CL_SUCCESS;
  GITS_WRAPPER_PRE
  wrapper.UnProtectMemoryPointers();
  //calculate gws if _pre need to use it
  wrapper.clEnqueueNDRangeKernel_pre(return_value, command_queue, kernel, workDim, globalWorkOffset,
                                     nullptr, localWorkSize, numEventsInWaitList, eventWaitList,
                                     event);
  return_value = drv.clEnqueueNDCountKernelINTEL(command_queue, kernel, workDim, globalWorkOffset,
                                                 workGroupCount, localWorkSize, numEventsInWaitList,
                                                 eventWaitList, event);
  wrapper.clEnqueueNDCountKernelINTEL(return_value, command_queue, kernel, workDim,
                                      globalWorkOffset, workGroupCount, localWorkSize,
                                      numEventsInWaitList, eventWaitList, event);
  wrapper.ProtectMemoryPointers();
  GITS_WRAPPER_POST
  else {
    return_value = drv.clEnqueueNDCountKernelINTEL(command_queue, kernel, workDim, globalWorkOffset,
                                                   workGroupCount, localWorkSize,
                                                   numEventsInWaitList, eventWaitList, event);
  }
  return return_value;
}

inline cl_int clEnqueueSVMUnmap_RECEXECWRAP(cl_command_queue command_queue,
                                            void* svm_ptr,
                                            cl_uint num_events_in_wait_list,
                                            const cl_event* event_wait_list,
                                            cl_event* event) {
  GITS_ENTRY_OCL
  auto return_value = CL_SUCCESS;
  GITS_WRAPPER_PRE
  wrapper.clEnqueueSVMUnmap_pre(return_value, command_queue, svm_ptr, num_events_in_wait_list,
                                event_wait_list, event);
  return_value = drv.clEnqueueSVMUnmap(command_queue, svm_ptr, num_events_in_wait_list,
                                       event_wait_list, event);
  wrapper.clEnqueueSVMUnmap(return_value, command_queue, svm_ptr, num_events_in_wait_list,
                            event_wait_list, event);
  GITS_WRAPPER_POST
  else {
    return_value = drv.clEnqueueSVMUnmap(command_queue, svm_ptr, num_events_in_wait_list,
                                         event_wait_list, event);
  }
  return return_value;
}

inline cl_int clMemFreeINTEL_RECEXECWRAP(cl_context context, void* ptr) {
  GITS_ENTRY_OCL
  auto return_value = CL_SUCCESS;
  GITS_WRAPPER_PRE
  wrapper.DestroySniffedRegion(ptr);
  return_value = drv.clMemFreeINTEL(context, ptr);
  wrapper.clMemFreeINTEL(return_value, context, ptr);
  GITS_WRAPPER_POST
  else {
    return_value = drv.clMemFreeINTEL(context, ptr);
  }
  return return_value;
}

inline cl_int clMemBlockingFreeINTEL_RECEXECWRAP(cl_context context, void* ptr) {
  GITS_ENTRY_OCL
  auto return_value = CL_SUCCESS;
  GITS_WRAPPER_PRE
  wrapper.DestroySniffedRegion(ptr);
  return_value = drv.clMemBlockingFreeINTEL(context, ptr);
  wrapper.clMemBlockingFreeINTEL(return_value, context, ptr);
  GITS_WRAPPER_POST
  else {
    return_value = drv.clMemBlockingFreeINTEL(context, ptr);
  }
  return return_value;
}

inline cl_int clEnqueueSVMFree_RECEXECWRAP(
    cl_command_queue command_queue,
    cl_uint num_svm_pointers,
    void** svm_pointers,
    void(CL_CALLBACK* pfn_free_func)(cl_command_queue, cl_uint, void**, void*),
    void* user_data,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event) {
  GITS_ENTRY_OCL
  auto return_value = CL_SUCCESS;
  GITS_WRAPPER_PRE
  for (cl_uint i = 0; i < num_svm_pointers; i++) {
    wrapper.DestroySniffedRegion(svm_pointers[i]);
  }
  return_value = drv.clEnqueueSVMFree(command_queue, num_svm_pointers, svm_pointers, pfn_free_func,
                                      user_data, num_events_in_wait_list, event_wait_list, event);
  wrapper.clEnqueueSVMFree(return_value, command_queue, num_svm_pointers, svm_pointers,
                           pfn_free_func, user_data, num_events_in_wait_list, event_wait_list,
                           event);
  GITS_WRAPPER_POST
  else {
    return_value =
        drv.clEnqueueSVMFree(command_queue, num_svm_pointers, svm_pointers, pfn_free_func,
                             user_data, num_events_in_wait_list, event_wait_list, event);
  }
  return return_value;
}

inline void clSVMFree_RECEXECWRAP(cl_context context, void* svm_pointer) {
  GITS_ENTRY_OCL
  GITS_WRAPPER_PRE
  wrapper.DestroySniffedRegion(svm_pointer);
  drv.clSVMFree(context, svm_pointer);
  wrapper.clSVMFree(context, svm_pointer);
  GITS_WRAPPER_POST
  else {
    drv.clSVMFree(context, svm_pointer);
  }
}

inline cl_int clEnqueueSVMMap_RECEXECWRAP(cl_command_queue command_queue,
                                          cl_bool blocking_map,
                                          cl_map_flags flags,
                                          void* svm_ptr,
                                          size_t size,
                                          cl_uint num_events_in_wait_list,
                                          const cl_event* event_wait_list,
                                          cl_event* event) {
  GITS_ENTRY_OCL
  const cl_map_flags mod_flags = flags == CL_MAP_READ ? flags : CL_MAP_READ | CL_MAP_WRITE;
  const auto return_value =
      drv.clEnqueueSVMMap(command_queue, blocking_map, mod_flags, svm_ptr, size,
                          num_events_in_wait_list, event_wait_list, event);
  GITS_WRAPPER_PRE
  wrapper.clEnqueueSVMMap(return_value, command_queue, blocking_map, mod_flags, svm_ptr, size,
                          num_events_in_wait_list, event_wait_list, event);
  GITS_WRAPPER_POST
  return return_value;
}

inline void clGitsIndirectAllocationOffsets_RECEXECWRAP(void* pAlloc,
                                                        uint32_t numOffsets,
                                                        size_t* pOffsets) {
  GITS_ENTRY_OCL
  GITS_WRAPPER_PRE(void) drv;
  wrapper.clGitsIndirectAllocationOffsets(pAlloc, numOffsets, pOffsets);
  GITS_WRAPPER_POST
}

inline cl_int clEnqueueMemcpyINTEL_RECEXECWRAP(cl_command_queue command_queue,
                                               cl_bool blocking,
                                               void* dst_ptr,
                                               const void* src_ptr,
                                               size_t size,
                                               cl_uint num_events_in_wait_list,
                                               const cl_event* event_wait_list,
                                               cl_event* event) {
  GITS_ENTRY_OCL
  cl_int return_value = CL_INVALID_VALUE;
  GITS_WRAPPER_PRE
  wrapper.UnProtectMemoryPointers();
  return_value = drv.clEnqueueMemcpyINTEL(command_queue, blocking, dst_ptr, src_ptr, size,
                                          num_events_in_wait_list, event_wait_list, event);
  wrapper.clEnqueueMemcpyINTEL(return_value, command_queue, blocking, dst_ptr, src_ptr, size,
                               num_events_in_wait_list, event_wait_list, event);
  wrapper.ProtectMemoryPointers();
  GITS_WRAPPER_POST
  else {
    return_value = drv.clEnqueueMemcpyINTEL(command_queue, blocking, dst_ptr, src_ptr, size,
                                            num_events_in_wait_list, event_wait_list, event);
  }
  return return_value;
}

inline cl_int clEnqueueMemFillINTEL_RECEXECWRAP(cl_command_queue command_queue,
                                                void* dst_ptr,
                                                const void* pattern,
                                                size_t pattern_size,
                                                size_t size,
                                                cl_uint num_events_in_wait_list,
                                                const cl_event* event_wait_list,
                                                cl_event* event) {
  GITS_ENTRY_OCL
  cl_int return_value = CL_INVALID_VALUE;
  GITS_WRAPPER_PRE
  wrapper.UnProtectMemoryPointers();
  return_value = drv.clEnqueueMemFillINTEL(command_queue, dst_ptr, pattern, pattern_size, size,
                                           num_events_in_wait_list, event_wait_list, event);
  wrapper.clEnqueueMemFillINTEL(return_value, command_queue, dst_ptr, pattern, pattern_size, size,
                                num_events_in_wait_list, event_wait_list, event);
  wrapper.ProtectMemoryPointers();
  GITS_WRAPPER_POST
  else {
    return_value = drv.clEnqueueMemFillINTEL(command_queue, dst_ptr, pattern, pattern_size, size,
                                             num_events_in_wait_list, event_wait_list, event);
  }
  return return_value;
}

} // namespace OpenCL
} // namespace gits

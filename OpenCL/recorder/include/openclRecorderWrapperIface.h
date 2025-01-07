// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   openclRecorderWrapperIface.h
*
* @brief Declaration of OpenCL recorder wrapper interface.
*/

#pragma once

#include "recorderIface.h"
#include "tools.h"
#include "openclHeader.h"

namespace gits {
namespace OpenCL {
class COclDriver;
class IRecorderWrapper {
public:
  virtual void StreamFinishedEvent(std::function<void()> e) = 0;
  virtual void CloseRecorderIfRequired() = 0;
  virtual COclDriver& Drivers() const = 0;
  virtual void InitializeDriver() const = 0;
  virtual std::recursive_mutex& GetInterceptorMutex() const = 0;
#include "openclRecorderWrapperIfaceAuto.h"
  virtual void clEnqueueNDRangeKernel_pre(cl_int return_value,
                                          cl_command_queue command_queue,
                                          cl_kernel kernel,
                                          cl_uint work_dim,
                                          const size_t* global_work_offset,
                                          const size_t* global_work_size,
                                          const size_t* local_work_size,
                                          cl_uint num_events_in_wait_list,
                                          const cl_event* event_wait_list,
                                          cl_event* event) const = 0;
  virtual void clEnqueueUnmapMemObject_pre(cl_command_queue command_queue,
                                           cl_mem memobj,
                                           void* mapped_ptr,
                                           cl_uint num_events_in_wait_list,
                                           const cl_event* event_wait_list,
                                           cl_event* event) const = 0;
  virtual void clEnqueueSVMUnmap_pre(cl_int return_value,
                                     cl_command_queue command_queue,
                                     void* svm_ptr,
                                     cl_uint num_events_in_wait_list,
                                     const cl_event* event_wait_list,
                                     cl_event* event) const = 0;
  virtual void DestroySniffedRegion(void* ptr) const = 0;
  virtual void UnProtectMemoryPointers() const = 0;
  virtual void ProtectMemoryPointers() const = 0;
  virtual void MarkRecorderForDeletion() = 0;
  virtual void TrackThread() const = 0;
};
} // namespace OpenCL
} // namespace gits

typedef gits::OpenCL::IRecorderWrapper*(STDCALL* FGITSRecoderOpenCL)();

extern "C" {
gits::OpenCL::IRecorderWrapper* STDCALL GITSRecorderOpenCL() VISIBLE;
}

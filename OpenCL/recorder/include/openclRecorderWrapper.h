// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   openclRecorderWrapper.h
*
* @brief Declaration of OpenCL recorder wrapper.
*/

#pragma once

#include "openclRecorderWrapperIface.h"
#include "openclDrivers.h"
#include "tools_lite.h"
#include <vector>
#include <functional>

namespace gits {
class CRecorder;

class KernelCallWrapperPrePost : private gits::noncopyable {
private:
  CRecorder& _recorder;

public:
  KernelCallWrapperPrePost(gits::CRecorder& rec);
  ~KernelCallWrapperPrePost();
};

namespace OpenCL {
class CRecorderWrapper : public IRecorderWrapper {
  CRecorder& _recorder;
  CRecorderWrapper(const CRecorderWrapper& ref);            // do not allow copy construction
  CRecorderWrapper& operator=(const CRecorderWrapper& ref); // do not allow class assignment

public:
  CRecorderWrapper(CRecorder& recorder);
  ~CRecorderWrapper() = default;
  void StreamFinishedEvent(std::function<void()> e);
  void CloseRecorderIfRequired() override;
  COclDriver& Drivers() const override;
  void InitializeDriver() const override;
#include "openclRecorderWrapperAuto.h"
  void clEnqueueNDRangeKernel_pre(cl_int return_value,
                                  cl_command_queue command_queue,
                                  cl_kernel kernel,
                                  cl_uint work_dim,
                                  const size_t* global_work_offset,
                                  const size_t* global_work_size,
                                  const size_t* local_work_size,
                                  cl_uint num_events_in_wait_list,
                                  const cl_event* event_wait_list,
                                  cl_event* event) const override;
  void clEnqueueUnmapMemObject_pre(cl_command_queue command_queue,
                                   cl_mem memobj,
                                   void* mapped_ptr,
                                   cl_uint num_events_in_wait_list,
                                   const cl_event* event_wait_list,
                                   cl_event* event) const override;
  void clEnqueueSVMUnmap_pre(cl_int return_value,
                             cl_command_queue command_queue,
                             void* svm_ptr,
                             cl_uint num_events_in_wait_list,
                             const cl_event* event_wait_list,
                             cl_event* event) const override;
  void DestroySniffedRegion(void* ptr) const override;
  void UnProtectMemoryPointers() const override;
  void ProtectMemoryPointers() const override;
  void MarkRecorderForDeletion() override;
  void TrackThread() const override;
};

} // namespace OpenCL
} // namespace gits

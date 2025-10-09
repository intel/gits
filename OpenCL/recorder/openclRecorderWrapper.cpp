// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   openclRecorderWrapper.cpp
*
* @brief Definition of OpenCL recorder wrapper.
*/

#include "openclRecorderWrapper.h"

#include "config.h"
#include "exception.h"
#include "gits.h"
#include "log2.h"
#include "platform.h"
#include "recorder.h"
#include "tools.h"
#include "opengl_apis_iface.h"
#include "opencl_apis_iface.h"
#include "openclLibrary.h"
#include "openclStateRestore.h"
#include "openclStateDynamic.h"
#include "openclRecorderSubwrappers.h"

#include <string>

static gits::OpenCL::CRecorderWrapper* wrapper = nullptr;

gits::OpenCL::IRecorderWrapper* STDCALL GITSRecorderOpenCL() {
  if (wrapper == nullptr) {
    try {
      // library not set - perform initialization
      gits::CGits::Instance().apis.UseApi3dIface(
          std::make_shared<gits::OpenGL::OpenGLApi>()); // OpenCL coexists with OpenGL
      gits::CGits::Instance().apis.UseApiComputeIface(std::make_shared<gits::OpenCL::OpenCLApi>());
      gits::CRecorder& recorder = gits::CRecorder::Instance();
      wrapper = new gits::OpenCL::CRecorderWrapper(recorder);
      recorder.Register(
          std::make_shared<gits::OpenCL::CLibrary>([] { return new gits::OpenCL::CState; }));
    } catch (const std::exception& ex) {
      LOG_ERROR << "Cannot initialize recorder: " << ex.what() << std::endl;
      exit(EXIT_FAILURE);
    }
  }
  return wrapper;
}

namespace gits {

namespace OpenCL {
CRecorderWrapper::CRecorderWrapper(CRecorder& recorder) : _recorder(recorder) {}

void CRecorderWrapper::StreamFinishedEvent(std::function<void()> event) {
  _recorder.RegisterDisposeEvent(std::move(event));
}

void CRecorderWrapper::CloseRecorderIfRequired() {
  if (_recorder.IsMarkedForDeletion()) {
    _recorder.Close();
  }
}

std::recursive_mutex& CRecorderWrapper::GetInterceptorMutex() const {
  return _recorder.GetMutex();
}

COclDriver& CRecorderWrapper::Drivers() const {
  return drvOcl;
}
void CRecorderWrapper::InitializeDriver() const {
  drvOcl.Initialize();
}
void CRecorderWrapper::clEnqueueUnmapMemObject_pre(cl_command_queue command_queue,
                                                   cl_mem memobj,
                                                   void* mapped_ptr,
                                                   cl_uint num_events_in_wait_list,
                                                   const cl_event* event_wait_list,
                                                   cl_event* event) const {
  // according to spec null mapped_ptr is illegal, but Rightware Basemark CL
  // does this anyway
  if (mapped_ptr != nullptr) {
    auto& bufferState = SD()._mappedBufferStates[mapped_ptr].back();
    std::copy_n((const char*)mapped_ptr, bufferState.size, bufferState.buffer.begin());
  }
}
void CRecorderWrapper::clEnqueueNDRangeKernel_pre(cl_int return_value,
                                                  cl_command_queue command_queue,
                                                  cl_kernel kernel,
                                                  cl_uint work_dim,
                                                  const size_t* global_work_offset,
                                                  const size_t* global_work_size,
                                                  const size_t* local_work_size,
                                                  cl_uint num_events_in_wait_list,
                                                  const cl_event* event_wait_list,
                                                  cl_event* event) const {
  DetermineUsmToUpdate(kernel);
  clEnqueueNDRangeKernel_RECWRAP_PRE(_recorder, return_value, command_queue, kernel, work_dim,
                                     global_work_offset, global_work_size, local_work_size,
                                     num_events_in_wait_list, event_wait_list, event);
}

void CRecorderWrapper::clEnqueueSVMUnmap_pre(cl_int return_value,
                                             cl_command_queue command_queue,
                                             void* svm_ptr,
                                             cl_uint num_events_in_wait_list,
                                             const cl_event* event_wait_list,
                                             cl_event* event) const {
  clEnqueueSVMUnmap_RECWRAP_PRE(_recorder, return_value, command_queue, svm_ptr,
                                num_events_in_wait_list, event_wait_list, event);
}

void CRecorderWrapper::DestroySniffedRegion(void* ptr) const {
  if (_recorder.InstancePtr() != nullptr) {
    if (SD().CheckIfUSMAllocExists(ptr)) {
      auto& state = SD().GetUSMAllocState(ptr, EXCEPTION_MESSAGE);
      if (state.sniffedRegionHandle) {
        MemorySniffer::Get().RemoveRegion(state.sniffedRegionHandle);
      }
    }
    if (SD().CheckIfSVMAllocExists(ptr)) {
      auto& state = SD().GetSVMAllocState(ptr, EXCEPTION_MESSAGE);
      if (state.sniffedRegionHandle) {
        MemorySniffer::Get().RemoveRegion(state.sniffedRegionHandle);
      }
    }
  }
}
void CRecorderWrapper::UnProtectMemoryPointers() const {
  if (_recorder.InstancePtr() == nullptr ||
      MemorySniffer::Get().IsOriginalSegvSignalInitialized()) {
    return;
  }
  const auto& oclIFace = gits::CGits::Instance().apis.IfaceCompute();
  for (const auto& allocState : SD()._usmAllocStates) {
    auto& handle = allocState.second->sniffedRegionHandle;
    if (handle != nullptr) {
      oclIFace.MemorySnifferUnProtect(handle);
    }
  }
  for (const auto& allocState : SD()._svmAllocStates) {
    auto& handle = allocState.second->sniffedRegionHandle;
    if (handle != nullptr) {
      oclIFace.MemorySnifferUnProtect(handle);
    }
  }
}
void CRecorderWrapper::ProtectMemoryPointers() const {
  if (_recorder.InstancePtr() == nullptr ||
      MemorySniffer::Get().IsOriginalSegvSignalInitialized()) {
    return;
  }
  const auto& oclIFace = gits::CGits::Instance().apis.IfaceCompute();
  for (const auto& allocState : SD()._usmAllocStates) {
    auto& handle = allocState.second->sniffedRegionHandle;
    if (handle != nullptr) {
      oclIFace.MemorySnifferProtect(handle);
    }
  }
  for (const auto& allocState : SD()._svmAllocStates) {
    auto& handle = allocState.second->sniffedRegionHandle;
    if (handle != nullptr) {
      oclIFace.MemorySnifferProtect(handle);
    }
  }
}

void CRecorderWrapper::MarkRecorderForDeletion() {
  if (_recorder.Running() && _recorder.InstancePtr() != nullptr) {
    _recorder.MarkForDeletion();
  }
}
void CRecorderWrapper::TrackThread() const {
  int threadId = _recorder.TrackThread();
  if (threadId >= 0) {
    _recorder.Schedule(new CGitsClTokenMakeCurrentThread(threadId));
  }
}
} // namespace OpenCL
} // namespace gits

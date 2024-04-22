// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "l0RecorderWrapper.h"

#include "gits.h"
#include "l0Header.h"
#include "l0StateDynamic.h"
#include "l0Tools.h"
#include "log.h"
#include "recorder.h"

#include "l0ApisIface.h"
#include "l0Drivers.h"
#include "l0Library.h"
#include "l0Functions.h"
#include "l0RecorderSubWrappers.h"
#include "l0StateRestore.h"

static gits::l0::CRecorderWrapper* wrapper = nullptr;

gits::l0::IRecorderWrapper* STDCALL GITSRecorderL0() {
  if (wrapper == nullptr) {
    try {
      // library not set - perform initialization
      // TODO: figure out how to have two ComputeApis (L0 supports OCL interop)
      gits::CGits::Instance().apis.UseApiComputeIface(std::make_shared<gits::l0::Api>());
      gits::CRecorder& recorder = gits::CRecorder::Instance();
      wrapper = new gits::l0::CRecorderWrapper(recorder);
      recorder.Register(
          std::make_shared<gits::l0::CLibrary>([] { return new gits::l0::CRestoreState; }));
    } catch (const std::exception& ex) {
      Log(ERR) << "Cannot initialize recorder: " << ex.what() << std::endl;
      exit(EXIT_FAILURE);
    }
  }
  return wrapper;
}

namespace gits {

namespace l0 {
CRecorderWrapper::CRecorderWrapper(CRecorder& recorder) : _recorder(recorder) {}

void CRecorderWrapper::StreamFinishedEvent(std::function<void()> event) {
  _recorder.RegisterDisposeEvent(std::move(event));
}

void CRecorderWrapper::CloseRecorderIfRequired() {
  if (_recorder.IsMarkedForDeletion()) {
    _recorder.Close();
  }
}

CDriver& CRecorderWrapper::Drivers() const {
  return drv;
}

void CRecorderWrapper::InitializeDriver() const {
  drv.Initialize();
}

void CRecorderWrapper::zeCommandQueueExecuteCommandLists_pre(
    ze_result_t return_value,
    ze_command_queue_handle_t hCommandQueue,
    uint32_t numCommandLists,
    ze_command_list_handle_t* phCommandLists,
    ze_fence_handle_t hFence) const {
  zeCommandQueueExecuteCommandLists_RECWRAP_PRE(_recorder, return_value, hCommandQueue,
                                                numCommandLists, phCommandLists, hFence);
}

void CRecorderWrapper::zeCommandListAppendLaunchKernel_pre(ze_result_t return_value,
                                                           ze_command_list_handle_t hCommandList,
                                                           ze_kernel_handle_t hKernel,
                                                           const ze_group_count_t* pLaunchFuncArgs,
                                                           ze_event_handle_t hSignalEvent,
                                                           uint32_t numWaitEvents,
                                                           ze_event_handle_t* phWaitEvents) const {
  zeCommandListAppendLaunchKernel_RECWRAP_PRE(_recorder, return_value, hCommandList, hKernel,
                                              pLaunchFuncArgs, hSignalEvent, numWaitEvents,
                                              phWaitEvents);
}

void CRecorderWrapper::zeCommandListAppendLaunchMultipleKernelsIndirect_pre(
    ze_result_t return_value,
    ze_command_list_handle_t hCommandList,
    uint32_t numKernels,
    ze_kernel_handle_t* phKernels,
    const uint32_t* pCountBuffer,
    const ze_group_count_t* pLaunchArgumentsBuffer,
    ze_event_handle_t hSignalEvent,
    uint32_t numWaitEvents,
    ze_event_handle_t* phWaitEvents) const {
  zeCommandListAppendLaunchMultipleKernelsIndirect_RECWRAP_PRE(
      _recorder, return_value, hCommandList, numKernels, phKernels, pCountBuffer,
      pLaunchArgumentsBuffer, hSignalEvent, numWaitEvents, phWaitEvents);
}

void CRecorderWrapper::zeCommandListAppendMemoryFill_pre(ze_result_t return_value,
                                                         ze_command_list_handle_t hCommandList,
                                                         void* ptr,
                                                         const void* pattern,
                                                         size_t pattern_size,
                                                         size_t size,
                                                         ze_event_handle_t hSignalEvent,
                                                         uint32_t numWaitEvents,
                                                         ze_event_handle_t* phWaitEvents) const {
  zeCommandListAppendMemoryFill_RECWRAP_PRE(_recorder, return_value, hCommandList, ptr, pattern,
                                            pattern_size, size, hSignalEvent, numWaitEvents,
                                            phWaitEvents);
}

void CRecorderWrapper::zeContextDestroy_pre(ze_result_t return_value,
                                            ze_context_handle_t hContext) const {
  zeContextDestroy_RECWRAP_PRE(_recorder, return_value, hContext);
}

void CRecorderWrapper::UnProtectMemoryPointers(const ze_command_list_handle_t& hCommandList) const {
  const auto isImmediate =
      (hCommandList == nullptr)
          ? true
          : SD().Get<CCommandListState>(hCommandList, EXCEPTION_MESSAGE).isImmediate;
  if (!isImmediate || _recorder.InstancePtr() == nullptr) {
    return;
  }
  const auto& l0IFace = gits::CGits::Instance().apis.IfaceCompute();
  for (const auto& allocState : SD().Map<CAllocState>()) {
    auto& handle = allocState.second->sniffedRegionHandle;
    if (handle != nullptr) {
      l0IFace.MemorySnifferUnProtect(handle);
    }
  }
  SD().isProtectionWrapper = true;
}

void CRecorderWrapper::ProtectMemoryPointers(const ze_command_list_handle_t& hCommandList) const {
  const auto isImmediate =
      (hCommandList == nullptr)
          ? true
          : SD().Get<CCommandListState>(hCommandList, EXCEPTION_MESSAGE).isImmediate;
  if (!isImmediate || _recorder.InstancePtr() == nullptr) {
    return;
  }
  const auto& l0IFace = gits::CGits::Instance().apis.IfaceCompute();
  for (const auto& allocState : SD().Map<CAllocState>()) {
    auto& handle = allocState.second->sniffedRegionHandle;
    if (handle != nullptr) {
      l0IFace.MemorySnifferProtect(handle);
    }
  }
  SD().isProtectionWrapper = false;
}

void CRecorderWrapper::MarkRecorderForDeletion() {
  if (_recorder.Running() && _recorder.InstancePtr() != nullptr) {
    _recorder.MarkForDeletion();
  }
}

void CRecorderWrapper::DestroySniffedRegion(void* ptr) const {
  if (ptr == nullptr) {
    Log(ERR) << "Memory deallocation is being called on a nullptr.";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  auto& allocState = SD().Get<CAllocState>(ptr, EXCEPTION_MESSAGE);
  if (allocState.sniffedRegionHandle != nullptr) {
    MemorySniffer::Get().RemoveRegion(allocState.sniffedRegionHandle);
    allocState.sniffedRegionHandle = nullptr;
  }
}

bool CRecorderWrapper::DeallocateVirtualMemory(void* ptr) const {
  auto& sd = SD();
  if (!sd.Exists<CAllocState>(ptr)) {
    return false;
  }
  const auto& allocState = sd.Get<CAllocState>(ptr, EXCEPTION_MESSAGE);
  const auto isVirtualMemoryReserved =
      allocState.memType == UnifiedMemoryType::device &&
      IsMemoryTypeAddressTranslationDisabled(Config::Get(), UnifiedMemoryType::device);
  if (isVirtualMemoryReserved) {
    auto errCode = ZE_RESULT_SUCCESS;
    const auto memMaps = allocState.memMaps;
    for (const auto& memMap : memMaps) {
      const auto offsetPtr = GetOffsetPointer(ptr, memMap.first);
      const auto physicalMemHandle = memMap.second->hPhysicalMemory;
      errCode = Drivers().zeVirtualMemUnmap(allocState.hContext, offsetPtr,
                                            memMap.second->virtualMemorySizeFromOffset);
      CRecorderWrapper::zeVirtualMemUnmap(errCode, allocState.hContext, offsetPtr,
                                          memMap.second->virtualMemorySizeFromOffset);
      errCode = Drivers().zePhysicalMemDestroy(allocState.hContext, physicalMemHandle);
      CRecorderWrapper::zePhysicalMemDestroy(errCode, allocState.hContext, physicalMemHandle);
    }
    Drivers().zeVirtualMemFree(allocState.hContext, ptr, allocState.size);
    CRecorderWrapper::zeVirtualMemFree(errCode, allocState.hContext, ptr, allocState.size);
  }
  return isVirtualMemoryReserved;
}

void CRecorderWrapper::UnProtectMemoryRegion(void* ptr) const {
  if (_recorder.InstancePtr() == nullptr) {
    return;
  }
  auto& sd = SD();
  const auto allocInfo = GetAllocFromRegion(ptr, sd);
  if (allocInfo.first != nullptr) {
    auto& allocState = sd.Get<CAllocState>(allocInfo.first, EXCEPTION_MESSAGE);
    if (allocState.sniffedRegionHandle != nullptr) {
      const auto& l0IFace = gits::CGits::Instance().apis.IfaceCompute();
      l0IFace.MemorySnifferUnProtect(allocState.sniffedRegionHandle);
    }
  }
}

void CRecorderWrapper::ProtectMemoryRegion(void* ptr) const {
  if (_recorder.InstancePtr() == nullptr) {
    return;
  }
  auto& sd = SD();
  const auto allocInfo = GetAllocFromRegion(ptr, sd);
  if (allocInfo.first != nullptr) {
    auto& allocState = sd.Get<CAllocState>(allocInfo.first, EXCEPTION_MESSAGE);
    if (allocState.sniffedRegionHandle != nullptr) {
      const auto& l0IFace = gits::CGits::Instance().apis.IfaceCompute();
      l0IFace.MemorySnifferProtect(allocState.sniffedRegionHandle);
    }
  }
}

bool CRecorderWrapper::IsMemorySnifferInstalled() const {
  const auto& l0IFace = gits::CGits::Instance().apis.IfaceCompute();
  return l0IFace.IsMemorySnifferInstalled();
}

void CRecorderWrapper::TrackThread() const {
  static int generatedThreadId = 0;
  static int previousThreadId = 0;
  static thread_local int currentThreadId = -1;
  if (currentThreadId < 0) {
    currentThreadId = generatedThreadId;
    generatedThreadId++;
  }
  if (currentThreadId != previousThreadId) {
    _recorder.Schedule(new CGitsL0TokenMakeCurrentThread(currentThreadId));
    previousThreadId = currentThreadId;
  }
}

bool CRecorderWrapper::IsAddressTranslationModeDisabled(UnifiedMemoryType type) const {
  return IsMemoryTypeAddressTranslationDisabled(Config::Get(), type);
}

void CRecorderWrapper::InjectMemoryReservationFree(ze_context_handle_t hContext) const {
  auto& contextState = SD().Get<CContextState>(hContext, EXCEPTION_MESSAGE);
  Drivers().inject.zeVirtualMemFree(hContext, contextState.virtualMemory,
                                    contextState.virtualMemorySize);
  contextState.virtualMemory = nullptr;
}

} // namespace l0
} // namespace gits

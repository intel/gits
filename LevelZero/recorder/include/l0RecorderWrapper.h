// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "l0RecorderWrapperIface.h"
#include "l0Tools.h"

namespace gits {
class CRecorder;
namespace l0 {
class CDriver;

class CRecorderWrapper : public IRecorderWrapper {
  CRecorder& _recorder;
  CRecorderWrapper(const CRecorderWrapper& ref);            // do not allow copy construction
  CRecorderWrapper& operator=(const CRecorderWrapper& ref); // do not allow class assignment

public:
  CRecorderWrapper(CRecorder& recorder);
  ~CRecorderWrapper() {}
  void StreamFinishedEvent(std::function<void()> e);
  void CloseRecorderIfRequired() override;
  CDriver& Drivers() const override;
  void InitializeDriver() const override;

#include "l0WrapperFunctions.h"
  void zeCommandQueueExecuteCommandLists_pre(ze_result_t return_value,
                                             ze_command_queue_handle_t hCommandQueue,
                                             uint32_t numCommandLists,
                                             ze_command_list_handle_t* phCommandLists,
                                             ze_fence_handle_t hFence) const override;
  void zeCommandListAppendLaunchKernel_pre(ze_result_t return_value,
                                           ze_command_list_handle_t hCommandList,
                                           ze_kernel_handle_t hKernel,
                                           const ze_group_count_t* pLaunchFuncArgs,
                                           ze_event_handle_t hSignalEvent,
                                           uint32_t numWaitEvents,
                                           ze_event_handle_t* phWaitEvents) const override;
  void zeCommandListAppendLaunchMultipleKernelsIndirect_pre(
      ze_result_t return_value,
      ze_command_list_handle_t hCommandList,
      uint32_t numKernels,
      ze_kernel_handle_t* phKernels,
      const uint32_t* pCountBuffer,
      const ze_group_count_t* pLaunchArgumentsBuffer,
      ze_event_handle_t hSignalEvent,
      uint32_t numWaitEvents,
      ze_event_handle_t* phWaitEvents) const override;
  void zeCommandListAppendMemoryFill_pre(ze_result_t return_value,
                                         ze_command_list_handle_t hCommandList,
                                         void* ptr,
                                         const void* pattern,
                                         size_t pattern_size,
                                         size_t size,
                                         ze_event_handle_t hSignalEvent,
                                         uint32_t numWaitEvents,
                                         ze_event_handle_t* phWaitEvents) const override;
  void zeContextDestroy_pre(ze_result_t return_value, ze_context_handle_t hContext) const override;
  void UnProtectMemoryPointers(
      const ze_command_list_handle_t& hCommandList = nullptr) const override;
  void ProtectMemoryPointers(const ze_command_list_handle_t& hCommandList = nullptr) const override;
  void MarkRecorderForDeletion() override;
  void DestroySniffedRegion(void* ptr) const override;
  virtual bool DeallocateVirtualMemory(void* ptr) const override;
  virtual void UnProtectMemoryRegion(void* ptr) const override;
  virtual void ProtectMemoryRegion(void* ptr) const override;
  virtual bool IsMemorySnifferInstalled() const override;
  virtual void TrackThread() const override;
  virtual bool IsAddressTranslationModeDisabled(UnifiedMemoryType type) const override;
  virtual void InjectMemoryReservationFree(ze_context_handle_t hContext) const override;
  virtual void UpdateConditionMemoryProtection() const override;
};
} // namespace l0
} // namespace gits

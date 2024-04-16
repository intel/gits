// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "exception.h"
#include "l0Arguments.h"
#include "l0Header.h"
#include "l0Log.h"
#include "l0StateDynamic.h"
#include "l0Tools.h"
#include "l0Drivers.h"
#include "mockListExecutor.h"
#include <unordered_map>
#include <utility>

namespace gits {
namespace l0 {
namespace {
void SaveGlobalPointerAllocationToMemory(CStateDynamic& sd,
                                         CAllocState& allocState,
                                         const void* globalPtr) {
  ze_command_list_handle_t hCommandListImmediate =
      GetCommandListImmediate(sd, drv, allocState.hContext);
  if (allocState.size == 0U) {
    const auto err = drv.inject.zeModuleGetGlobalPointer(
        allocState.hModule, allocState.name.c_str(), &allocState.size, nullptr);
    if (err != ZE_RESULT_SUCCESS) {
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
  }
  allocState.globalPtrAllocation.resize(allocState.size);
  const auto err = drv.inject.zeCommandListAppendMemoryCopy(
      hCommandListImmediate, allocState.globalPtrAllocation.data(), globalPtr, allocState.size,
      nullptr, 0, nullptr);
  sd.scanningGlobalPointersMode.insert(allocState.hModule);
  if (err != ZE_RESULT_SUCCESS) {
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
}

bool CheckWhetherDumpKernel(uint32_t kernelNumber, uint32_t cmdListNumber) {
  const auto& cfg = Config::Get();
  const auto& kernelList = cfg.IsPlayer() ? cfg.player.l0CaptureKernels
                                          : cfg.recorder.levelZero.utilities.captureKernels;
  auto cmdList = cfg.IsPlayer() ? cfg.player.l0CaptureCommandLists
                                : cfg.recorder.levelZero.utilities.captureCommandLists;
  return !kernelList.empty()
             ? (kernelList[kernelNumber] && (!cmdList.empty() ? cmdList[cmdListNumber] : false))
             : false;
}

bool CheckWhetherQueueCanBeSynced(const Config& cfg,
                                  CStateDynamic& sd,
                                  CDriver& driver,
                                  const ze_command_queue_handle_t& hCommandQueue) {
  std::vector<ze_command_list_handle_t> commandListsToSynchronize;
  std::vector<ze_command_list_handle_t> submittedCommandList;
  if (CaptureKernels(cfg)) {
    const auto& cmdQueueState = sd.Get<CCommandQueueState>(hCommandQueue, EXCEPTION_MESSAGE);
    for (const auto& queueSubmission : cmdQueueState.notSyncedSubmissions) {
      commandListsToSynchronize.push_back(queueSubmission->hCommandList);
    }
    for (const auto& otherCqState : sd.Map<CCommandQueueState>()) {
      if (otherCqState.first != hCommandQueue) {
        const auto& otherCmdQueueState =
            sd.Get<CCommandQueueState>(otherCqState.first, EXCEPTION_MESSAGE);
        for (const auto& queueSubmission : otherCmdQueueState.notSyncedSubmissions) {
          submittedCommandList.push_back(queueSubmission->hCommandList);
        }
      }
    }
    auto mockListExecutor =
        MockListExecutor(sd, driver, commandListsToSynchronize, submittedCommandList);
    const auto isCommandQueueEligibleForSynchronization = mockListExecutor.Run();
    if (!isCommandQueueEligibleForSynchronization) {
      Log(WARN) << "Dumping kernels from command queue: " << ToStringHelper(hCommandQueue)
                << " will be delayed to the original application synchronization due to "
                   "not completable event dependencies";
    }
    return isCommandQueueEligibleForSynchronization;
  }
  return true;
}

void RegisterEvents(CStateDynamic& sd,
                    const ze_command_list_handle_t& hCommandList,
                    const ze_event_handle_t& signalEvent,
                    const uint32_t& numWaitEvents,
                    ze_event_handle_t* phWaitEvents) {
  auto& cmdListState = sd.Get<CCommandListState>(hCommandList, EXCEPTION_MESSAGE);
  cmdListState.AddAction(signalEvent, numWaitEvents, phWaitEvents);
}

bool CheckKernelResidencyPossibilities(const CAllocState& allocState,
                                       const unsigned int indirectTypes,
                                       const ze_context_handle_t hContext) {
  const auto kernelMightModifyAllocation =
      (allocState.memType == UnifiedMemoryType::device &&
       indirectTypes & ZE_KERNEL_INDIRECT_ACCESS_FLAG_DEVICE) ||
      (allocState.memType == UnifiedMemoryType::host &&
       indirectTypes & ZE_KERNEL_INDIRECT_ACCESS_FLAG_HOST) ||
      (allocState.memType == UnifiedMemoryType::shared &&
       indirectTypes & ZE_KERNEL_INDIRECT_ACCESS_FLAG_SHARED);
  const auto isResident =
      allocState.residencyInfo && allocState.residencyInfo->hContext == hContext;
  return kernelMightModifyAllocation && isResident;
}

} // namespace

inline void zeMemAllocShared_SD(ze_result_t return_value,
                                ze_context_handle_t hContext,
                                const ze_device_mem_alloc_desc_t* device_desc,
                                const ze_host_mem_alloc_desc_t* host_desc,
                                size_t size,
                                size_t alignment,
                                ze_device_handle_t hDevice,
                                void** pptr) {
  if (return_value == ZE_RESULT_SUCCESS && pptr != nullptr && host_desc != nullptr &&
      device_desc != nullptr) {
    auto& allocState = SD().Map<CAllocState>()[*pptr];
    allocState =
        std::make_unique<CAllocState>(hContext, *device_desc, *host_desc, size, alignment, hDevice);
  }
}

inline void zeMemAllocDevice_SD(ze_result_t return_value,
                                ze_context_handle_t hContext,
                                const ze_device_mem_alloc_desc_t* device_desc,
                                size_t size,
                                size_t alignment,
                                ze_device_handle_t hDevice,
                                void** pptr) {
  if (return_value == ZE_RESULT_SUCCESS && pptr != nullptr && device_desc != nullptr) {
    auto& allocState = SD().Map<CAllocState>()[*pptr];
    allocState = std::make_unique<CAllocState>(hContext, *device_desc, size, alignment, hDevice);
  }
}

inline void zeMemAllocHost_SD(ze_result_t return_value,
                              ze_context_handle_t hContext,
                              const ze_host_mem_alloc_desc_t* host_desc,
                              size_t size,
                              size_t alignment,
                              void** pptr) {
  if (return_value == ZE_RESULT_SUCCESS && pptr != nullptr && host_desc != nullptr) {
    auto& allocState = SD().Map<CAllocState>()[*pptr];
    allocState = std::make_unique<CAllocState>(hContext, *host_desc, size, alignment);
  }
}

inline void zeKernelCreate_SD(ze_result_t return_value,
                              ze_module_handle_t hModule,
                              const ze_kernel_desc_t* desc,
                              ze_kernel_handle_t* phKernel) {
  if (return_value == ZE_RESULT_SUCCESS && phKernel != nullptr) {
    auto& kernelState = SD().Map<CKernelState>()[*phKernel];
    kernelState = std::make_unique<CKernelState>(hModule, desc);
    kernelState->currentKernelInfo->handle = *phKernel;
    kernelState->currentKernelInfo->hModule = hModule;
    kernelState->currentKernelInfo->pKernelName = std::string(desc->pKernelName);
  }
}

inline void zeCommandListCreate_SD(ze_result_t return_value,
                                   ze_context_handle_t hContext,
                                   ze_device_handle_t hDevice,
                                   const ze_command_list_desc_t* desc,
                                   ze_command_list_handle_t* phCommandList) {
  if (return_value == ZE_RESULT_SUCCESS && phCommandList != nullptr && desc != nullptr) {
    auto& clState = SD().Map<CCommandListState>()[*phCommandList];
    clState = std::make_unique<CCommandListState>(hContext, hDevice, *desc);
    clState->cmdListNumber = CGits::Instance().CurrentCommandListCount();
  }
}

inline void zeCommandListCreateImmediate_SD(ze_result_t return_value,
                                            ze_context_handle_t hContext,
                                            ze_device_handle_t hDevice,
                                            const ze_command_queue_desc_t* altdesc,
                                            ze_command_list_handle_t* phCommandList) {
  if (return_value == ZE_RESULT_SUCCESS && phCommandList != nullptr && altdesc != nullptr) {
    auto& clState = SD().Map<CCommandListState>()[*phCommandList];
    clState = std::make_unique<CCommandListState>(hContext, hDevice, *altdesc);
    clState->cmdListNumber = CGits::Instance().CurrentCommandListCount();
    clState->cmdQueueNumber = CGits::Instance().CurrentCommandQueueExecCount();
  }
}

inline void zeCommandListAppendLaunchKernel_SD([[maybe_unused]] ze_result_t return_value,
                                               ze_command_list_handle_t hCommandList,
                                               ze_kernel_handle_t hKernel,
                                               const ze_group_count_t* pLaunchFuncArgs,
                                               ze_event_handle_t hSignalEvent,
                                               uint32_t numWaitEvents,
                                               ze_event_handle_t* phWaitEvents) {
  auto& sd = SD();
  AppendLaunchKernel(hCommandList, hKernel, pLaunchFuncArgs, hSignalEvent);
  RegisterEvents(sd, hCommandList, hSignalEvent, numWaitEvents, phWaitEvents);
}

inline void zeCommandListAppendLaunchCooperativeKernel_SD([[maybe_unused]] ze_result_t return_value,
                                                          ze_command_list_handle_t hCommandList,
                                                          ze_kernel_handle_t hKernel,
                                                          const ze_group_count_t* pLaunchFuncArgs,
                                                          ze_event_handle_t hSignalEvent,
                                                          uint32_t numWaitEvents,
                                                          ze_event_handle_t* phWaitEvents) {
  auto& sd = SD();
  AppendLaunchKernel(hCommandList, hKernel, pLaunchFuncArgs, hSignalEvent);
  RegisterEvents(sd, hCommandList, hSignalEvent, numWaitEvents, phWaitEvents);
}

inline void zeCommandListAppendLaunchKernelIndirect_SD(
    [[maybe_unused]] ze_result_t return_value,
    ze_command_list_handle_t hCommandList,
    ze_kernel_handle_t hKernel,
    const ze_group_count_t* pLaunchArgumentsBuffer,
    ze_event_handle_t hSignalEvent,
    uint32_t numWaitEvents,
    ze_event_handle_t* phWaitEvents) {
  auto& sd = SD();
  AppendLaunchKernel(hCommandList, hKernel, pLaunchArgumentsBuffer, hSignalEvent);
  RegisterEvents(sd, hCommandList, hSignalEvent, numWaitEvents, phWaitEvents);
}

inline void zeCommandListAppendLaunchMultipleKernelsIndirect_SD(
    [[maybe_unused]] ze_result_t return_value,
    ze_command_list_handle_t hCommandList,
    uint32_t numKernels,
    ze_kernel_handle_t* phKernels,
    [[maybe_unused]] const uint32_t* pCountBuffer,
    const ze_group_count_t* pLaunchArgumentsBuffer,
    ze_event_handle_t hSignalEvent,
    uint32_t numWaitEvents,
    ze_event_handle_t* phWaitEvents) {
  auto& sd = SD();
  bool callOnce = true;
  const auto& cmdListState = sd.Get<CCommandListState>(hCommandList, EXCEPTION_MESSAGE);
  for (auto i = 0u; i < numKernels; i++) {
    CommandListKernelInit(sd, hCommandList, phKernels[i], pLaunchArgumentsBuffer, hSignalEvent);
    const auto& kernelState = sd.Get<CKernelState>((phKernels)[i], EXCEPTION_MESSAGE);

    if (CheckWhetherDumpKernel(kernelState.currentKernelInfo->kernelNumber,
                               cmdListState.cmdListNumber) &&
        (cmdListState.isImmediate || !CaptureAfterSubmit(Config::Get()))) {
      SaveKernelArguments(hSignalEvent, hCommandList, kernelState, cmdListState, false, callOnce);
      callOnce = false;
    }
  }
  RegisterEvents(sd, hCommandList, hSignalEvent, numWaitEvents, phWaitEvents);
}

inline void zeKernelSetArgumentValue_SD([[maybe_unused]] ze_result_t return_value,
                                        ze_kernel_handle_t hKernel,
                                        uint32_t argIndex,
                                        size_t argSize,
                                        const void* pArgValue) {
  auto& kernelState = SD().Get<CKernelState>(hKernel, EXCEPTION_MESSAGE);
  kernelState.currentKernelInfo->SetArgument(argIndex, argSize, pArgValue);
}

inline void zeImageCreate_SD(ze_result_t return_value,
                             ze_context_handle_t hContext,
                             ze_device_handle_t hDevice,
                             const ze_image_desc_t* desc,
                             ze_image_handle_t* phImage) {
  if (return_value == ZE_RESULT_SUCCESS && phImage != nullptr && desc != nullptr) {
    auto& imageState = SD().Map<CImageState>()[*phImage];
    imageState = std::make_unique<CImageState>(hContext, hDevice, *desc);
  }
}

inline void zeCommandQueueCreate_SD(ze_result_t return_value,
                                    ze_context_handle_t hContext,
                                    ze_device_handle_t hDevice,
                                    const ze_command_queue_desc_t* desc,
                                    ze_command_queue_handle_t* phCommandQueue) {
  if (return_value == ZE_RESULT_SUCCESS && phCommandQueue != nullptr && desc != nullptr) {
    auto& cqState = SD().Map<CCommandQueueState>()[*phCommandQueue];
    cqState = std::make_unique<CCommandQueueState>(hContext, hDevice, *desc);
  }
}

inline void zeCommandQueueExecuteCommandLists_SD([[maybe_unused]] ze_result_t return_value,
                                                 ze_command_queue_handle_t hCommandQueue,
                                                 uint32_t numCommandLists,
                                                 ze_command_list_handle_t* phCommandLists,
                                                 ze_fence_handle_t hFence) {
  const auto& cfg = Config::Get();
  auto& sd = SD();
  if (hFence != nullptr) {
    auto& fenceState = sd.Get<CFenceState>(hFence, EXCEPTION_MESSAGE);
    fenceState.canBeSynced = true;
    fenceState.executionIsSynced = false;
  }
  auto& cqState = sd.Get<CCommandQueueState>(hCommandQueue, EXCEPTION_MESSAGE);
  if (cfg.IsPlayer()) {
    cqState.cmdQueueNumber = gits::CGits::Instance().CurrentCommandQueueExecCount();
  }
  bool containsAppendedKernelsToDump = false;
  unsigned int indirectTypes = 0U;
  for (auto i = 0u; i < numCommandLists; i++) {
    const auto& cmdListState = sd.Get<CCommandListState>(phCommandLists[i], EXCEPTION_MESSAGE);
    for (const auto& kernelInfo : cmdListState.appendedKernels) {
      const auto& kernelState = sd.Get<CKernelState>(kernelInfo->handle, EXCEPTION_MESSAGE);
      LogKernelExecution(kernelInfo->kernelNumber, kernelState.desc.pKernelName,
                         cqState.cmdQueueNumber, cmdListState.cmdListNumber);
      if (CheckWhetherDumpQueueSubmit(cfg, cqState.cmdQueueNumber) &&
          CheckWhetherDumpKernel(kernelInfo->kernelNumber, cmdListState.cmdListNumber)) {
        containsAppendedKernelsToDump = true;
        auto& readyArgVector = sd.Map<CKernelArgumentDump>()[phCommandLists[i]];
        cqState.queueSubmissionDumpState.push_back(std::make_unique<QueueSubmissionSnapshot>(
            phCommandLists[i], cmdListState.isImmediate, cmdListState.appendedKernels,
            cmdListState.cmdListNumber, cmdListState.hContext, cqState.cmdQueueNumber,
            &readyArgVector));
        if (CaptureAfterSubmit(cfg)) {
          sd.Release<CKernelArgumentDump>(phCommandLists[i]);
        }
      }
      indirectTypes |= kernelInfo->indirectUsmTypes;
    }
    if (CaptureKernels(cfg)) {
      cqState.notSyncedSubmissions.push_back(std::make_unique<QueueSubmissionSnapshot>(
          phCommandLists[i], cmdListState.isImmediate, cmdListState.appendedKernels,
          cmdListState.cmdListNumber, cmdListState.hContext, cqState.cmdQueueNumber, nullptr));
    }
  }
  if (IsBruteForceScanForIndirectPointersEnabled(cfg)) {
    for (auto& allocState : sd.Map<CAllocState>()) {
      if (CheckKernelResidencyPossibilities(*allocState.second, indirectTypes, cqState.hContext)) {
        allocState.second->modified = true;
      }
    }
  }
  if (containsAppendedKernelsToDump && CheckWhetherQueueCanBeSynced(cfg, sd, drv, hCommandQueue)) {
    drv.inject.zeCommandQueueSynchronize(hCommandQueue, UINT64_MAX);
    DumpQueueSubmit(cfg, sd, hCommandQueue);
    cqState.notSyncedSubmissions.clear();
    cqState.queueSubmissionDumpState.clear();
  }
}

inline void zeMemFree_SD(ze_result_t return_value,
                         [[maybe_unused]] ze_context_handle_t hContext,
                         void* ptr) {
  if (return_value == ZE_RESULT_SUCCESS) {
    SD().Release<CAllocState>(ptr);
  }
}

inline void zeDriverGet_SD(ze_result_t return_value,
                           uint32_t* pCount,
                           ze_driver_handle_t* phDrivers) {
  if (return_value == ZE_RESULT_SUCCESS && phDrivers != nullptr) {
    for (auto i = 0u; i < *pCount; i++) {
      auto& driverState = SD().Map<CDriverState>()[phDrivers[i]];
      driverState = std::make_unique<CDriverState>();
    }
  }
}

inline void zeDeviceGet_SD(ze_result_t return_value,
                           ze_driver_handle_t hDriver,
                           uint32_t* pCount,
                           ze_device_handle_t* phDevices) {
  if (return_value == ZE_RESULT_SUCCESS && phDevices != nullptr) {
    for (auto i = 0u; i < *pCount; i++) {
      auto& driverState = SD().Map<CDeviceState>()[phDevices[i]];
      driverState = std::make_unique<CDeviceState>(hDriver);
    }
  }
}

inline void zeModuleCreate_SD(ze_result_t return_value,
                              ze_context_handle_t hContext,
                              ze_device_handle_t hDevice,
                              const ze_module_desc_t* desc,
                              ze_module_handle_t* phModule,
                              ze_module_build_log_handle_t* phBuildLog) {
  if (return_value == ZE_RESULT_SUCCESS && phModule != nullptr && desc != nullptr) {
    auto& moduleState = SD().Map<CModuleState>()[*phModule];
    moduleState = std::make_unique<CModuleState>(hContext, hDevice, desc,
                                                 phBuildLog != nullptr ? *phBuildLog : nullptr);
  }
}

inline void zeKernelSetGroupSize_SD(ze_result_t return_value,
                                    ze_kernel_handle_t hKernel,
                                    uint32_t groupSizeX,
                                    uint32_t groupSizeY,
                                    uint32_t groupSizeZ) {
  if (return_value == ZE_RESULT_SUCCESS) {
    auto& kernelState = SD().Get<CKernelState>(hKernel, EXCEPTION_MESSAGE);
    kernelState.currentKernelInfo->groupSizeX = groupSizeX;
    kernelState.currentKernelInfo->groupSizeY = groupSizeY;
    kernelState.currentKernelInfo->groupSizeZ = groupSizeZ;
    kernelState.currentKernelInfo->isGroupSizeSet = true;
  }
}

inline void zeCommandListDestroy_SD(ze_result_t return_value,
                                    ze_command_list_handle_t hCommandList) {
  if (return_value == ZE_RESULT_SUCCESS) {
    SD().Release<CCommandListState>(hCommandList);
    if (SD().Exists<CKernelArgumentDump>(hCommandList)) {
      SD().Release<CKernelArgumentDump>(hCommandList);
    }
  }
}

inline void zeCommandQueueDestroy_SD(ze_result_t return_value,
                                     ze_command_queue_handle_t hCommandQueue) {
  if (return_value == ZE_RESULT_SUCCESS) {
    SD().Release<CCommandQueueState>(hCommandQueue);
  }
}

inline void zeImageDestroy_SD(ze_result_t return_value, ze_image_handle_t hImage) {
  if (return_value == ZE_RESULT_SUCCESS) {
    SD().Release<CImageState>(hImage);
  }
}

inline void zeKernelDestroy_SD(ze_result_t return_value, ze_kernel_handle_t hKernel) {
  if (return_value == ZE_RESULT_SUCCESS) {
    SD().Release<CKernelState>(hKernel);
  }
}

inline void zeModuleDestroy_SD(ze_result_t return_value, ze_module_handle_t hModule) {
  if (return_value == ZE_RESULT_SUCCESS) {
    auto& sd = SD();
    sd.scanningGlobalPointersMode.erase(hModule);
    std::vector<void*> globalPtrs;
    for (const auto& allocState : sd.Map<CAllocState>()) {
      if (allocState.second->allocType == AllocStateType::global_pointer &&
          allocState.second->hModule == hModule) {
        globalPtrs.push_back(allocState.first);
      }
    }
    for (const auto& globalPtr : globalPtrs) {
      sd.Release<CAllocState>(globalPtr);
    }
    sd.Release<CModuleState>(hModule);
  }
}

inline void zeKernelSetIndirectAccess_SD(ze_result_t return_value,
                                         ze_kernel_handle_t hKernel,
                                         ze_kernel_indirect_access_flags_t flags) {
  if (return_value == ZE_RESULT_SUCCESS) {
    auto& kernelState = SD().Get<CKernelState>(hKernel, EXCEPTION_MESSAGE);
    kernelState.currentKernelInfo->indirectUsmTypes |= static_cast<unsigned>(flags);
  }
}

inline void zeContextDestroy_SD(ze_result_t return_value, ze_context_handle_t hContext) {
  if (return_value == ZE_RESULT_SUCCESS) {
    auto& sd = SD();
    const auto& list = sd.Get<CContextState>(hContext, EXCEPTION_MESSAGE).gitsImmediateList;
    if (list != nullptr) {
      drv.inject.zeCommandListDestroy(list);
    }
    std::vector<ze_command_list_handle_t> commandListsToRelease;
    for (const auto& state : sd.Map<CCommandListState>()) {
      if (state.second->hContext == hContext) {
        commandListsToRelease.push_back(state.first);
      }
    }
    for (const auto& handle : commandListsToRelease) {
      sd.Release<CCommandListState>(handle);
    }
    sd.Release<CContextState>(hContext);
  }
}

inline void zeContextCreate_SD(ze_result_t return_value,
                               ze_driver_handle_t hDriver,
                               const ze_context_desc_t* desc,
                               ze_context_handle_t* phContext) {
  if (return_value == ZE_RESULT_SUCCESS && desc != nullptr && phContext != nullptr) {
    auto& contextState = SD().Map<CContextState>()[*phContext];
    contextState = std::make_unique<CContextState>(hDriver, *desc);
  }
}

inline void zeCommandListClose_SD(ze_result_t return_value, ze_command_list_handle_t hCommandList) {
  if (return_value == ZE_RESULT_SUCCESS) {
    auto& cmdListState = SD().Get<CCommandListState>(hCommandList, EXCEPTION_MESSAGE);
    cmdListState.isClosed = true;
  }
}

inline void zeCommandListReset_SD([[maybe_unused]] ze_result_t return_value,
                                  ze_command_list_handle_t hCommandList) {
  auto& cmdListState = SD().Get<CCommandListState>(hCommandList, EXCEPTION_MESSAGE);
  cmdListState.RestoreReset();
  cmdListState.appendedKernels.clear();
  cmdListState.mockList.clear();
  cmdListState.isClosed = false;
}

inline void zeEventCreate_SD(ze_result_t return_value,
                             ze_event_pool_handle_t hEventPool,
                             const ze_event_desc_t* desc,
                             ze_event_handle_t* phEvent) {
  if (return_value == ZE_RESULT_SUCCESS) {
    auto& eventState = SD().Map<CEventState>()[*phEvent];
    eventState = std::make_unique<CEventState>(hEventPool, *desc);
  }
}

inline void zeEventPoolDestroy_SD(ze_result_t return_value, ze_event_pool_handle_t hEventPool) {
  if (return_value == ZE_RESULT_SUCCESS) {
    SD().Release<CEventPoolState>(hEventPool);
  }
}

inline void zeEventDestroy_SD(ze_result_t return_value, ze_event_handle_t hEvent) {
  if (return_value == ZE_RESULT_SUCCESS) {
    SD().Release<CEventState>(hEvent);
  }
}

inline void zeEventPoolCreate_SD(ze_result_t return_value,
                                 ze_context_handle_t hContext,
                                 const ze_event_pool_desc_t* desc,
                                 [[maybe_unused]] uint32_t numDevices,
                                 [[maybe_unused]] ze_device_handle_t* phDevices,
                                 ze_event_pool_handle_t* phEventPool) {
  if (return_value == ZE_RESULT_SUCCESS) {
    auto& eventPoolState = SD().Map<CEventPoolState>()[*phEventPool];
    eventPoolState = std::make_unique<CEventPoolState>(hContext, *desc);
  }
}

inline void zeFenceDestroy_SD(ze_result_t return_value, ze_fence_handle_t hFence) {
  if (return_value == ZE_RESULT_SUCCESS) {
    SD().Release<CFenceState>(hFence);
  }
}

inline void zeFenceCreate_SD(ze_result_t return_value,
                             ze_command_queue_handle_t hCommandQueue,
                             const ze_fence_desc_t* desc,
                             ze_fence_handle_t* phFence) {
  if (return_value == ZE_RESULT_SUCCESS) {
    auto& fenceState = SD().Map<CFenceState>()[*phFence];
    fenceState = std::make_unique<CFenceState>(hCommandQueue, *desc);
  }
}

inline void zeGitsIndirectAllocationOffsets_SD(void* pAlloc,
                                               uint32_t numOffsets,
                                               size_t* pOffsets) {
  auto& allocState = SD().Get<CAllocState>(pAlloc, EXCEPTION_MESSAGE);
  for (uint32_t i = 0U; i < numOffsets; i++) {
    if (allocState.indirectPointersOffsets.find(pOffsets[i]) ==
        allocState.indirectPointersOffsets.end()) {
      allocState.indirectPointersOffsets[pOffsets[i]] = false;
    }
  }
}

inline void zeModuleGetFunctionPointer_SD(ze_result_t return_value,
                                          ze_module_handle_t hModule,
                                          const char* pFunctionName,
                                          void** pfnFunction) {
  if (return_value == ZE_RESULT_SUCCESS) {
    auto& sd = SD();
    auto& allocState = sd.Map<CAllocState>()[*pfnFunction];
    allocState = std::make_unique<CAllocState>(hModule, pFunctionName, sizeof(void*),
                                               AllocStateType::function_pointer);
    const auto& moduleState = sd.Get<CModuleState>(hModule, EXCEPTION_MESSAGE);
    allocState->hContext = moduleState.hContext;
    allocState->hDevice = moduleState.hDevice;
  }
}

inline void zeModuleGetGlobalPointer_SD(ze_result_t return_value,
                                        ze_module_handle_t hModule,
                                        const char* pGlobalName,
                                        size_t* pSize,
                                        void** pptr) {
  if (return_value == ZE_RESULT_SUCCESS && pptr != nullptr) {
    auto& sd = SD();
    auto& allocState = sd.Map<CAllocState>()[*pptr];
    allocState = std::make_unique<CAllocState>(hModule, pGlobalName, pSize ? *pSize : 0,
                                               AllocStateType::global_pointer);
    const auto& moduleState = sd.Get<CModuleState>(hModule, EXCEPTION_MESSAGE);
    allocState->hContext = moduleState.hContext;
    allocState->hDevice = moduleState.hDevice;
  }
}

inline void zeFenceReset_SD(ze_result_t return_value, ze_fence_handle_t hFence) {
  if (return_value == ZE_RESULT_SUCCESS && hFence != nullptr) {
    auto& fenceState = SD().Get<CFenceState>(hFence, EXCEPTION_MESSAGE);
    fenceState.canBeSynced = false;
  }
}

inline void zeDeviceGetProperties_SD(ze_result_t return_value,
                                     ze_device_handle_t hDevice,
                                     ze_device_properties_t* pDeviceProperties) {
  if (return_value == ZE_RESULT_SUCCESS) {
    SD().Get<CDeviceState>(hDevice, EXCEPTION_MESSAGE).properties = *pDeviceProperties;
  }
}

inline void zeDeviceGetSubDevices_SD(ze_result_t return_value,
                                     ze_device_handle_t hDevice,
                                     uint32_t* pCount,
                                     ze_device_handle_t* phSubdevices) {
  if (phSubdevices != nullptr && pCount != nullptr && return_value == ZE_RESULT_SUCCESS) {
    for (auto i = 0u; i < *pCount; i++) {
      auto& deviceState = SD().Map<CDeviceState>()[phSubdevices[i]];
      deviceState = std::make_unique<CDeviceState>(hDevice);
    }
  }
}

inline void zeKernelSetGlobalOffsetExp_SD(ze_result_t return_value,
                                          ze_kernel_handle_t hKernel,
                                          uint32_t offsetX,
                                          uint32_t offsetY,
                                          uint32_t offsetZ) {
  if (return_value == ZE_RESULT_SUCCESS) {
    auto& kernelState = SD().Get<CKernelState>(hKernel, EXCEPTION_MESSAGE);
    kernelState.currentKernelInfo->offsetX = offsetX;
    kernelState.currentKernelInfo->offsetY = offsetY;
    kernelState.currentKernelInfo->offsetZ = offsetZ;
    kernelState.currentKernelInfo->isOffsetSet = true;
  }
}

inline void zeContextCreateEx_SD(ze_result_t return_value,
                                 ze_driver_handle_t hDriver,
                                 const ze_context_desc_t* desc,
                                 uint32_t numDevices,
                                 ze_device_handle_t* phDevices,
                                 ze_context_handle_t* phContext) {
  if (return_value == ZE_RESULT_SUCCESS) {
    auto& contextState = SD().Map<CContextState>()[*phContext];
    contextState = std::make_unique<CContextState>(hDriver, *desc, numDevices, phDevices);
  }
}

inline void zeImageViewCreateExp_SD(ze_result_t return_value,
                                    ze_context_handle_t hContext,
                                    ze_device_handle_t hDevice,
                                    const ze_image_desc_t* desc,
                                    ze_image_handle_t hImage,
                                    ze_image_handle_t* phImageView) {
  if (return_value == ZE_RESULT_SUCCESS) {
    auto& imageState = SD().Map<CImageState>()[*phImageView];
    imageState = std::make_unique<CImageState>(hContext, hDevice, *desc);
    imageState->imageView = hImage;
  }
}

inline void zeKernelSchedulingHintExp_SD(ze_result_t return_value,
                                         ze_kernel_handle_t hKernel,
                                         ze_scheduling_hint_exp_desc_t* pHint) {
  if (return_value == ZE_RESULT_SUCCESS && pHint != nullptr) {
    auto& kernelState = SD().Get<CKernelState>(hKernel, EXCEPTION_MESSAGE);
    kernelState.currentKernelInfo->schedulingHintFlags = pHint->flags;
  }
}

inline void zeDeviceGetCommandQueueGroupProperties_SD(
    ze_result_t return_value,
    ze_device_handle_t hDevice,
    uint32_t* pCount,
    ze_command_queue_group_properties_t* pCommandQueueGroupProperties) {
  if (return_value == ZE_RESULT_SUCCESS && pCommandQueueGroupProperties != nullptr &&
      pCount != nullptr) {
    auto& deviceState = SD().Get<CDeviceState>(hDevice, EXCEPTION_MESSAGE);
    deviceState.cqGroupProperties.clear();
    for (auto i = 0U; i < *pCount; i++) {
      deviceState.cqGroupProperties.push_back(pCommandQueueGroupProperties[i]);
    }
  }
}

inline void zeCommandListAppendMemoryCopy_SD(ze_result_t return_value,
                                             ze_command_list_handle_t hCommandList,
                                             void* dstptr,
                                             const void* srcptr,
                                             [[maybe_unused]] size_t size,
                                             ze_event_handle_t hSignalEvent,
                                             uint32_t numWaitEvents,
                                             ze_event_handle_t* phWaitEvents) {
  if (return_value == ZE_RESULT_SUCCESS) {
    auto& sd = SD();
    RegisterEvents(sd, hCommandList, hSignalEvent, numWaitEvents, phWaitEvents);
    if (sd.Exists<CAllocState>(const_cast<void*>(srcptr))) {
      auto& allocState = sd.Get<CAllocState>(const_cast<void*>(srcptr), EXCEPTION_MESSAGE);
      if (allocState.allocType == AllocStateType::global_pointer) {
        SaveGlobalPointerAllocationToMemory(sd, allocState, srcptr);
      }
    }
    const auto& cfg = Config::Get();
    if (IsBruteForceScanForIndirectPointersEnabled(cfg)) {
      const auto allocInfo = GetAllocFromRegion(dstptr, sd);
      if (allocInfo.first != nullptr) {
        auto& allocState = sd.Get<CAllocState>(allocInfo.first, EXCEPTION_MESSAGE);
        allocState.modified = true;
      }
    }
  }
}

inline void zeImageViewCreateExt_SD(ze_result_t return_value,
                                    ze_context_handle_t hContext,
                                    ze_device_handle_t hDevice,
                                    const ze_image_desc_t* desc,
                                    ze_image_handle_t hImage,
                                    ze_image_handle_t* phImageView) {
  if (return_value == ZE_RESULT_SUCCESS) {
    auto& imageState = SD().Map<CImageState>()[*phImageView];
    imageState = std::make_unique<CImageState>(hContext, hDevice, *desc);
    imageState->imageView = hImage;
  }
}

inline void zeMemFreeExt_SD(ze_result_t return_value,
                            [[maybe_unused]] ze_context_handle_t hContext,
                            [[maybe_unused]] const ze_memory_free_ext_desc_t* pMemFreeDesc,
                            void* ptr) {
  if (return_value == ZE_RESULT_SUCCESS) {
    SD().Release<CAllocState>(ptr);
  }
}

inline void zeModuleDynamicLink_SD(ze_result_t return_value,
                                   uint32_t numModules,
                                   ze_module_handle_t* phModules,
                                   [[maybe_unused]] ze_module_build_log_handle_t* phLinkLog) {
  if (return_value == ZE_RESULT_SUCCESS) {
    auto& sd = SD();
    for (auto i = 0U; i < numModules; i++) {
      sd.Get<CModuleState>(phModules[i], EXCEPTION_MESSAGE).AddModuleLinks(numModules, phModules);
    }
  }
}

inline void zeCommandQueueSynchronize_SD(ze_result_t return_value,
                                         ze_command_queue_handle_t hCommandQueue,
                                         uint64_t timeout) {
  const auto failedSyncAttempt = return_value != ZE_RESULT_SUCCESS && timeout != UINT64_MAX;
  const auto& cfg = Config::Get();
  if ((return_value == ZE_RESULT_SUCCESS || failedSyncAttempt) && CaptureKernels(cfg)) {
    auto& sd = SD();
    auto& cqState = sd.Get<CCommandQueueState>(hCommandQueue, EXCEPTION_MESSAGE);
    for (const auto& cmdQueueListsInfo : cqState.queueSubmissionDumpState) {
      if (CheckWhetherDumpQueueSubmit(cfg, cmdQueueListsInfo->cmdQueueNumber)) {
        if (failedSyncAttempt) {
          drv.inject.zeCommandQueueSynchronize(hCommandQueue, UINT64_MAX);
        }
        DumpQueueSubmit(cfg, sd, hCommandQueue);
        break;
      }
    }
    cqState.notSyncedSubmissions.clear();
    cqState.queueSubmissionDumpState.clear();
  }
}

inline void zeFenceHostSynchronize_SD(ze_result_t return_value,
                                      ze_fence_handle_t hFence,
                                      uint64_t timeout) {
  const auto failedSyncAttempt = return_value != ZE_RESULT_SUCCESS && timeout != UINT64_MAX;
  const auto& cfg = Config::Get();
  auto& sd = SD();
  auto& fenceState = sd.Get<CFenceState>(hFence, EXCEPTION_MESSAGE);
  if ((return_value == ZE_RESULT_SUCCESS || failedSyncAttempt) && CaptureKernels(cfg)) {
    auto& cqState = sd.Get<CCommandQueueState>(fenceState.hCommandQueue, EXCEPTION_MESSAGE);
    for (const auto& cmdQueueListsInfo : cqState.queueSubmissionDumpState) {
      if (CheckWhetherDumpQueueSubmit(cfg, cmdQueueListsInfo->cmdQueueNumber)) {
        if (failedSyncAttempt) {
          return_value = drv.inject.zeFenceHostSynchronize(hFence, UINT64_MAX);
        }
        DumpQueueSubmit(cfg, sd, fenceState.hCommandQueue);
        break;
      }
    }
    cqState.notSyncedSubmissions.clear();
    cqState.queueSubmissionDumpState.clear();
  }
  if (return_value == ZE_RESULT_SUCCESS && fenceState.canBeSynced) {
    fenceState.executionIsSynced = true;
  }
}

inline void zeCommandListAppendSignalEvent_SD(ze_result_t return_value,
                                              ze_command_list_handle_t hCommandList,
                                              ze_event_handle_t hEvent) {
  if (return_value == ZE_RESULT_SUCCESS) {
    auto& cmdListState = SD().Get<CCommandListState>(hCommandList, EXCEPTION_MESSAGE);
    cmdListState.AddAction(hEvent, 0, nullptr, CCommandListState::Action::Type::Signal);
  }
}

inline void zeCommandListAppendWaitOnEvents_SD(ze_result_t return_value,
                                               ze_command_list_handle_t hCommandList,
                                               uint32_t numEvents,
                                               ze_event_handle_t* phEvents) {
  if (return_value == ZE_RESULT_SUCCESS) {
    auto& cmdListState = SD().Get<CCommandListState>(hCommandList, EXCEPTION_MESSAGE);
    cmdListState.AddAction(nullptr, numEvents, phEvents);
  }
}

inline void zeCommandListAppendBarrier_SD([[maybe_unused]] ze_result_t return_value,
                                          ze_command_list_handle_t hCommandList,
                                          ze_event_handle_t hSignalEvent,
                                          uint32_t numWaitEvents,
                                          ze_event_handle_t* phWaitEvents) {
  RegisterEvents(SD(), hCommandList, hSignalEvent, numWaitEvents, phWaitEvents);
}

inline void zeCommandListAppendImageCopy_SD([[maybe_unused]] ze_result_t return_value,
                                            ze_command_list_handle_t hCommandList,
                                            [[maybe_unused]] ze_image_handle_t hDstImage,
                                            [[maybe_unused]] ze_image_handle_t hSrcImage,
                                            ze_event_handle_t hSignalEvent,
                                            uint32_t numWaitEvents,
                                            ze_event_handle_t* phWaitEvents) {
  RegisterEvents(SD(), hCommandList, hSignalEvent, numWaitEvents, phWaitEvents);
}

inline void zeCommandListAppendImageCopyFromMemory_SD(
    [[maybe_unused]] ze_result_t return_value,
    ze_command_list_handle_t hCommandList,
    [[maybe_unused]] ze_image_handle_t hDstImage,
    [[maybe_unused]] const void* srcptr,
    [[maybe_unused]] const ze_image_region_t* pDstRegion,
    ze_event_handle_t hSignalEvent,
    uint32_t numWaitEvents,
    ze_event_handle_t* phWaitEvents) {
  RegisterEvents(SD(), hCommandList, hSignalEvent, numWaitEvents, phWaitEvents);
}

inline void zeCommandListAppendImageCopyFromMemoryExt_SD(
    [[maybe_unused]] ze_result_t return_value,
    ze_command_list_handle_t hCommandList,
    [[maybe_unused]] ze_image_handle_t hDstImage,
    [[maybe_unused]] const void* srcptr,
    [[maybe_unused]] const ze_image_region_t* pDstRegion,
    [[maybe_unused]] uint32_t srcRowPitch,
    [[maybe_unused]] uint32_t srcSlicePitch,
    ze_event_handle_t hSignalEvent,
    uint32_t numWaitEvents,
    ze_event_handle_t* phWaitEvents) {
  RegisterEvents(SD(), hCommandList, hSignalEvent, numWaitEvents, phWaitEvents);
}

inline void zeCommandListAppendImageCopyRegion_SD(
    [[maybe_unused]] ze_result_t return_value,
    ze_command_list_handle_t hCommandList,
    [[maybe_unused]] ze_image_handle_t hDstImage,
    [[maybe_unused]] ze_image_handle_t hSrcImage,
    [[maybe_unused]] const ze_image_region_t* pDstRegion,
    [[maybe_unused]] const ze_image_region_t* pSrcRegion,
    ze_event_handle_t hSignalEvent,
    uint32_t numWaitEvents,
    ze_event_handle_t* phWaitEvents) {
  RegisterEvents(SD(), hCommandList, hSignalEvent, numWaitEvents, phWaitEvents);
}

inline void zeCommandListAppendImageCopyToMemory_SD(
    [[maybe_unused]] ze_result_t return_value,
    ze_command_list_handle_t hCommandList,
    [[maybe_unused]] void* dstptr,
    [[maybe_unused]] ze_image_handle_t hSrcImage,
    [[maybe_unused]] const ze_image_region_t* pSrcRegion,
    ze_event_handle_t hSignalEvent,
    uint32_t numWaitEvents,
    ze_event_handle_t* phWaitEvents) {
  RegisterEvents(SD(), hCommandList, hSignalEvent, numWaitEvents, phWaitEvents);
}

inline void zeCommandListAppendImageCopyToMemoryExt_SD(
    [[maybe_unused]] ze_result_t return_value,
    ze_command_list_handle_t hCommandList,
    [[maybe_unused]] void* dstptr,
    [[maybe_unused]] ze_image_handle_t hSrcImage,
    [[maybe_unused]] const ze_image_region_t* pSrcRegion,
    [[maybe_unused]] uint32_t destRowPitch,
    [[maybe_unused]] uint32_t destSlicePitch,
    ze_event_handle_t hSignalEvent,
    uint32_t numWaitEvents,
    ze_event_handle_t* phWaitEvents) {
  RegisterEvents(SD(), hCommandList, hSignalEvent, numWaitEvents, phWaitEvents);
}

inline void zeCommandListAppendMemoryCopyFromContext_SD(
    [[maybe_unused]] ze_result_t return_value,
    ze_command_list_handle_t hCommandList,
    [[maybe_unused]] void* dstptr,
    [[maybe_unused]] ze_context_handle_t hContextSrc,
    [[maybe_unused]] const void* srcptr,
    [[maybe_unused]] size_t size,
    ze_event_handle_t hSignalEvent,
    uint32_t numWaitEvents,
    ze_event_handle_t* phWaitEvents) {
  RegisterEvents(SD(), hCommandList, hSignalEvent, numWaitEvents, phWaitEvents);
}

inline void zeCommandListAppendMemoryCopyRegion_SD(
    [[maybe_unused]] ze_result_t return_value,
    ze_command_list_handle_t hCommandList,
    [[maybe_unused]] void* dstptr,
    [[maybe_unused]] const ze_copy_region_t* dstRegion,
    [[maybe_unused]] uint32_t dstPitch,
    [[maybe_unused]] uint32_t dstSlicePitch,
    [[maybe_unused]] const void* srcptr,
    [[maybe_unused]] const ze_copy_region_t* srcRegion,
    [[maybe_unused]] uint32_t srcPitch,
    [[maybe_unused]] uint32_t srcSlicePitch,
    ze_event_handle_t hSignalEvent,
    uint32_t numWaitEvents,
    ze_event_handle_t* phWaitEvents) {
  RegisterEvents(SD(), hCommandList, hSignalEvent, numWaitEvents, phWaitEvents);
}

inline void zeCommandListAppendMemoryFill_SD([[maybe_unused]] ze_result_t return_value,
                                             ze_command_list_handle_t hCommandList,
                                             [[maybe_unused]] void* ptr,
                                             [[maybe_unused]] const void* pattern,
                                             [[maybe_unused]] size_t pattern_size,
                                             [[maybe_unused]] size_t size,
                                             ze_event_handle_t hSignalEvent,
                                             uint32_t numWaitEvents,
                                             ze_event_handle_t* phWaitEvents) {
  RegisterEvents(SD(), hCommandList, hSignalEvent, numWaitEvents, phWaitEvents);
}

inline void zeCommandListAppendMemoryRangesBarrier_SD([[maybe_unused]] ze_result_t return_value,
                                                      ze_command_list_handle_t hCommandList,
                                                      [[maybe_unused]] uint32_t numRanges,
                                                      [[maybe_unused]] const size_t* pRangeSizes,
                                                      [[maybe_unused]] const void** pRanges,
                                                      ze_event_handle_t hSignalEvent,
                                                      uint32_t numWaitEvents,
                                                      ze_event_handle_t* phWaitEvents) {
  RegisterEvents(SD(), hCommandList, hSignalEvent, numWaitEvents, phWaitEvents);
}

inline void zeCommandListAppendQueryKernelTimestamps_SD(
    [[maybe_unused]] ze_result_t return_value,
    ze_command_list_handle_t hCommandList,
    [[maybe_unused]] uint32_t numEvents,
    [[maybe_unused]] ze_event_handle_t* phEvents,
    [[maybe_unused]] void* dstptr,
    [[maybe_unused]] const size_t* pOffsets,
    ze_event_handle_t hSignalEvent,
    uint32_t numWaitEvents,
    ze_event_handle_t* phWaitEvents) {
  RegisterEvents(SD(), hCommandList, hSignalEvent, numWaitEvents, phWaitEvents);
}

inline void zeCommandListAppendWriteGlobalTimestamp_SD([[maybe_unused]] ze_result_t return_value,
                                                       ze_command_list_handle_t hCommandList,
                                                       [[maybe_unused]] uint64_t* dstptr,
                                                       ze_event_handle_t hSignalEvent,
                                                       uint32_t numWaitEvents,
                                                       ze_event_handle_t* phWaitEvents) {
  RegisterEvents(SD(), hCommandList, hSignalEvent, numWaitEvents, phWaitEvents);
}

inline void zetCommandListAppendMetricQueryEnd_SD(
    [[maybe_unused]] ze_result_t return_value,
    zet_command_list_handle_t hCommandList,
    [[maybe_unused]] zet_metric_query_handle_t hMetricQuery,
    ze_event_handle_t hSignalEvent,
    uint32_t numWaitEvents,
    ze_event_handle_t* phWaitEvents) {
  RegisterEvents(SD(), reinterpret_cast<ze_command_list_handle_t>(hCommandList), hSignalEvent,
                 numWaitEvents, phWaitEvents);
}

inline void zeCommandListAppendEventReset_SD(ze_result_t return_value,
                                             ze_command_list_handle_t hCommandList,
                                             ze_event_handle_t hEvent) {
  if (return_value == ZE_RESULT_SUCCESS) {
    auto& cmdListState = SD().Get<CCommandListState>(hCommandList, EXCEPTION_MESSAGE);
    cmdListState.AddAction(hEvent, 0, nullptr, CCommandListState::Action::Type::Reset);
  }
}

inline void zeFenceQueryStatus_SD(ze_result_t return_value, ze_fence_handle_t hFence) {
  if (return_value == ZE_RESULT_SUCCESS) {
    auto& sd = SD();
    auto& fenceState = sd.Get<CFenceState>(hFence, EXCEPTION_MESSAGE);
    const auto& cfg = Config::Get();
    if (!fenceState.executionIsSynced && fenceState.canBeSynced) {
      if (CaptureKernels(cfg)) {
        auto& cqState = sd.Get<CCommandQueueState>(fenceState.hCommandQueue, EXCEPTION_MESSAGE);
        for (const auto& cmdQueueListsInfo : cqState.queueSubmissionDumpState) {
          if (CheckWhetherDumpQueueSubmit(cfg, cmdQueueListsInfo->cmdQueueNumber)) {
            DumpQueueSubmit(cfg, sd, fenceState.hCommandQueue);
            break;
          }
        }
        cqState.notSyncedSubmissions.clear();
        cqState.queueSubmissionDumpState.clear();
      }
      fenceState.executionIsSynced = true;
    }
  }
}
inline void zeGitsOriginalQueueFamilyInfo_SD(
    [[maybe_unused]] ze_result_t return_value,
    ze_device_handle_t hDevice,
    uint32_t count,
    const ze_command_queue_group_properties_t* cqGroupProperties) {
  Log(TRACE) << "Updating original command queue group properties...";
  auto& originalQueueGroupProps =
      SD().Get<CDeviceState>(hDevice, EXCEPTION_MESSAGE).originalQueueGroupProperties;
  originalQueueGroupProps.clear();
  for (auto i = 0U; i < count; i++) {
    originalQueueGroupProps.push_back(cqGroupProperties[i]);
  }
}

inline void zeContextMakeImageResident_SD(ze_result_t return_value,
                                          ze_context_handle_t hContext,
                                          ze_device_handle_t hDevice,
                                          ze_image_handle_t hImage) {
  if (return_value == ZE_RESULT_SUCCESS) {
    auto& imageState = SD().Get<CImageState>(hImage, EXCEPTION_MESSAGE);
    imageState.residencyInfo = std::make_unique<CImageState::ResidencyInfo>(hContext, hDevice);
  }
}

inline void zeContextMakeMemoryResident_SD(ze_result_t return_value,
                                           ze_context_handle_t hContext,
                                           ze_device_handle_t hDevice,
                                           void* ptr,
                                           size_t size) {
  if (return_value == ZE_RESULT_SUCCESS) {
    auto& sd = SD();
    const auto allocInfo = GetAllocFromRegion(ptr, sd);
    auto& allocState = sd.Get<CAllocState>(allocInfo.first, EXCEPTION_MESSAGE);
    allocState.residencyInfo =
        std::make_unique<CAllocState::ResidencyInfo>(hContext, hDevice, size, allocInfo.second);
  }
}

inline void zeContextEvictImage_SD(ze_result_t return_value,
                                   [[maybe_unused]] ze_context_handle_t hContext,
                                   [[maybe_unused]] ze_device_handle_t hDevice,
                                   ze_image_handle_t hImage) {
  if (return_value == ZE_RESULT_SUCCESS) {
    auto& sd = SD();
    auto& imageState = sd.Get<CImageState>(hImage, EXCEPTION_MESSAGE);
    imageState.residencyInfo.release();
  }
}

inline void zeContextEvictMemory_SD(ze_result_t return_value,
                                    [[maybe_unused]] ze_context_handle_t hContext,
                                    [[maybe_unused]] ze_device_handle_t hDevice,
                                    void* ptr,
                                    size_t size) {
  if (return_value == ZE_RESULT_SUCCESS) {
    auto& sd = SD();
    const auto allocInfo = GetAllocFromRegion(ptr, sd);
    auto& allocState = sd.Get<CAllocState>(allocInfo.first, EXCEPTION_MESSAGE);
    if (allocState.residencyInfo && allocState.residencyInfo->size != size) {
      Log(WARN) << "Evicted memory size is not covering whole residency.";
    }
    allocState.residencyInfo.release();
  }
}
inline void zePhysicalMemCreate_SD(ze_result_t return_value,
                                   ze_context_handle_t hContext,
                                   ze_device_handle_t hDevice,
                                   ze_physical_mem_desc_t* desc,
                                   ze_physical_mem_handle_t* phPhysicalMemory) {
  if (return_value == ZE_RESULT_SUCCESS && phPhysicalMemory != nullptr && desc != nullptr) {
    auto& physicalMemState = SD().Map<CPhysicalMemState>()[*phPhysicalMemory];
    physicalMemState = std::make_unique<CPhysicalMemState>(hContext, hDevice, *desc);
  }
}

inline void zePhysicalMemDestroy_SD(ze_result_t return_value,
                                    [[maybe_unused]] ze_context_handle_t hContext,
                                    ze_physical_mem_handle_t hPhysicalMemory) {
  if (return_value == ZE_RESULT_SUCCESS) {
    SD().Release<CPhysicalMemState>(hPhysicalMemory);
  }
}

inline void zeVirtualMemFree_SD(ze_result_t return_value,
                                [[maybe_unused]] ze_context_handle_t hContext,
                                const void* ptr,
                                [[maybe_unused]] size_t size) {
  if (return_value == ZE_RESULT_SUCCESS && ptr != nullptr) {
    SD().Release<CAllocState>(const_cast<void*>(ptr));
  }
}

inline void zeVirtualMemMap_SD(ze_result_t return_value,
                               [[maybe_unused]] ze_context_handle_t hContext,
                               const void* ptr,
                               size_t size,
                               ze_physical_mem_handle_t hPhysicalMemory,
                               size_t offset,
                               ze_memory_access_attribute_t access) {
  if (return_value == ZE_RESULT_SUCCESS && ptr != nullptr && hPhysicalMemory != nullptr) {
    auto& sd = SD();
    const auto allocInfo = GetAllocFromRegion(const_cast<void*>(ptr), sd);
    if (allocInfo.first != nullptr) {
      auto& allocState = sd.Get<CAllocState>(allocInfo.first, EXCEPTION_MESSAGE);
      const auto virtualMemOffset = allocInfo.second;
      const auto virtualMemorySizeFromOffset = size;
      const auto physicalMemoryOffset = offset;
      allocState.memMaps[virtualMemOffset] = std::make_shared<CAllocState::VirtualMemMapInfo>(
          virtualMemorySizeFromOffset, hPhysicalMemory, physicalMemoryOffset, access);
      const auto& physicalMemState = sd.Get<CPhysicalMemState>(hPhysicalMemory, EXCEPTION_MESSAGE);
      allocState.hDevice = physicalMemState.hDevice;
    }
  }
}

inline void zeVirtualMemReserve_SD(ze_result_t return_value,
                                   ze_context_handle_t hContext,
                                   const void* pStart,
                                   size_t size,
                                   void** pptr) {
  if (return_value == ZE_RESULT_SUCCESS && pptr != nullptr) {
    auto& allocState = SD().Map<CAllocState>()[*pptr];
    allocState = std::make_unique<CAllocState>(hContext, size, pStart);
  }
}

inline void zeVirtualMemUnmap_SD(ze_result_t return_value,
                                 [[maybe_unused]] ze_context_handle_t hContext,
                                 const void* ptr,
                                 [[maybe_unused]] size_t size) {
  if (return_value == ZE_RESULT_SUCCESS) {
    auto& sd = SD();
    const auto allocInfo = GetAllocFromRegion(const_cast<void*>(ptr), sd);
    if (allocInfo.first != nullptr) {
      auto& allocState = SD().Get<CAllocState>(allocInfo.first, EXCEPTION_MESSAGE);
      allocState.memMaps.erase(allocInfo.second);
    }
  }
}

inline void zeVirtualMemSetAccessAttribute_SD(ze_result_t return_value,
                                              [[maybe_unused]] ze_context_handle_t hContext,
                                              const void* ptr,
                                              size_t size,
                                              ze_memory_access_attribute_t access) {
  if (return_value == ZE_RESULT_SUCCESS) {
    auto& sd = SD();
    const auto allocInfo = GetAllocFromRegion(const_cast<void*>(ptr), sd);
    if (allocInfo.first != nullptr) {
      auto& allocState = SD().Get<CAllocState>(allocInfo.first, EXCEPTION_MESSAGE);
      if (allocState.size != size) {
        Log(ERR)
            << "Setting only part of the virtual memory's memory access attribute is not supported";
        throw ENotImplemented(EXCEPTION_MESSAGE);
      }
      for (auto& memoryMap : allocState.memMaps) {
        memoryMap.second->access = access;
      }
    }
  }
}

} // namespace l0
} // namespace gits

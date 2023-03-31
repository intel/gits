// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "l0Arguments.h"
#include "l0Header.h"
#include "l0StateDynamic.h"
#include "l0Tools.h"
#include "l0Drivers.h"

namespace gits {
namespace l0 {
namespace {
void SaveGlobalPointerAllocationToMemory(CStateDynamic& sd,
                                         const ze_command_list_handle_t& hCommandList,
                                         CAllocState& allocState,
                                         const void* globalPtr) {
  const auto& commandListState = sd.Get<CCommandListState>(hCommandList, EXCEPTION_MESSAGE);
  ze_command_list_handle_t hCommandListImmediate =
      GetCommandListImmediate(sd, drv, commandListState.hContext);
  allocState.globalPtrAllocation.resize(allocState.size);
  ze_result_t err = drv.zeCommandListAppendMemoryCopy(
      hCommandListImmediate, allocState.globalPtrAllocation.data(), globalPtr, allocState.size,
      nullptr, 0, nullptr);
  sd.scanningGlobalPointersMode.insert(allocState.hModule);
  if (err != ZE_RESULT_SUCCESS) {
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
}
ze_event_pool_handle_t CreateGitsPoolEvent(const ze_context_handle_t hContext) {
  static std::unordered_map<ze_context_handle_t, ze_event_pool_handle_t> gitsPoolEventMap;
  if (gitsPoolEventMap.find(hContext) != gitsPoolEventMap.end()) {
    return gitsPoolEventMap[hContext];
  }
  ze_event_pool_desc_t desc = {};
  desc.stype = ZE_STRUCTURE_TYPE_EVENT_POOL_DESC;
  desc.pNext = nullptr;
  desc.count = 128;
  drv.zeEventPoolCreate(hContext, &desc, 0, nullptr, &gitsPoolEventMap[hContext]);
  Log(TRACE) << "^------------------ injected synchronization pool";
  return gitsPoolEventMap[hContext];
}
ze_event_handle_t CreateGitsEvent(const ze_context_handle_t hContext) {
  static std::unordered_map<ze_context_handle_t, ze_event_handle_t> gitsEventMap;
  if (hContext == nullptr) {
    return nullptr;
  }
  if (gitsEventMap.find(hContext) != gitsEventMap.end()) {
    return gitsEventMap[hContext];
  }
  auto eventPool = CreateGitsPoolEvent(hContext);
  ze_event_desc_t desc = {};
  desc.index = static_cast<uint32_t>(gitsEventMap.size());
  desc.pNext = nullptr;
  desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
  drv.zeEventCreate(eventPool, &desc, &gitsEventMap[hContext]);
  Log(TRACE) << "^------------------ injected synchronization event";
  return gitsEventMap[hContext];
}
bool CheckWhetherDumpQueueSubmit(uint32_t queueSubmitNumber) {
  const auto& cfg = Config::Get();
  auto cmdQueueList = cfg.IsPlayer() ? cfg.player.l0CaptureCommandQueues
                                     : cfg.recorder.levelZero.utilities.captureCommandQueues;
  return !cmdQueueList.empty() ? cmdQueueList[queueSubmitNumber] : false;
}

bool CheckWhetherDumpKernel(uint32_t kernelNumber, uint32_t cmdListNumber) {
  const auto& cfg = Config::Get();
  auto kernelList = cfg.IsPlayer() ? cfg.player.l0CaptureKernels
                                   : cfg.recorder.levelZero.utilities.captureKernels;
  auto cmdList = cfg.IsPlayer() ? cfg.player.l0CaptureCommandLists
                                : cfg.recorder.levelZero.utilities.captureCommandLists;
  return !kernelList.empty()
             ? (kernelList[kernelNumber] && (!cmdList.empty() ? cmdList[cmdListNumber] : false))
             : false;
}

bool CheckWhetherQueueRequiresSync(ze_command_list_handle_t* cmdLists, uint32_t numCommandLists) {
  bool requiresSync = false;
  for (auto i = 0u; i < numCommandLists; i++) {
    const auto& cmdListState = SD().Get<CCommandListState>(cmdLists[i], EXCEPTION_MESSAGE);
    for (const auto& kernel : cmdListState.appendedKernelsMap) {
      auto& kernelState = SD().Get<CKernelState>(kernel.second, EXCEPTION_MESSAGE);
      auto& kernelInfo = kernelState.executedKernels.at(kernel.first);
      if (CheckWhetherDumpKernel(kernelInfo.kernelNumber, cmdListState.cmdListNumber)) {
        requiresSync = true;
      }
    }
  }
  return requiresSync
             ? CheckWhetherDumpQueueSubmit(CGits::Instance().CurrentCommandQueueExecCount())
             : false;
}

bool CaptureAfterSubmit() {
  const auto& cfg = Config::Get();
  return cfg.IsPlayer() ? cfg.player.l0CaptureAfterSubmit
                        : cfg.recorder.levelZero.utilities.captureAfterSubmit;
}

inline void CommandListKernelInit(const ze_command_list_handle_t& commandList,
                                  const ze_kernel_handle_t& kernel,
                                  const ze_group_count_t*& pLaunchFuncArgs) {
  auto& kernelState = SD().Get<CKernelState>(kernel, EXCEPTION_MESSAGE);
  kernelState.currentKernelInfo.kernelNumber = CGits::Instance().CurrentKernelCount();
  if (pLaunchFuncArgs != nullptr) {
    kernelState.currentKernelInfo.launchFuncArgs = *pLaunchFuncArgs;
  }
  kernelState.executedKernels[kernelState.currentKernelInfo.kernelNumber] =
      kernelState.currentKernelInfo;
  auto& cmdListState = SD().Get<CCommandListState>(commandList, EXCEPTION_MESSAGE);
  cmdListState.appendedKernelsMap[kernelState.currentKernelInfo.kernelNumber] = kernel;

  Log(TRACE) << "--- Kernel append call #" << kernelState.currentKernelInfo.kernelNumber
             << ", kernel name \"" << kernelState.desc.pKernelName << "\" ---";
  if (cmdListState.isImmediate) {
    Log(TRACE) << "--- Queue #" << cmdListState.cmdQueueNumber << " / CommandList #"
               << cmdListState.cmdListNumber << " / Kernel #"
               << kernelState.currentKernelInfo.kernelNumber << " ---";
  }
}

inline void InjectReadsForArguments(std::vector<CKernelArgumentDump>& readyArgVec,
                                    const ze_command_list_handle_t& cmdList,
                                    const bool useBarrier,
                                    ze_context_handle_t hContext) {
  bool callBarrier = useBarrier;
  auto eventHandle = CreateGitsEvent(hContext);
  for (auto& argDump : readyArgVec) {
    if (argDump.injected) {
      continue;
    }
    argDump.injected = true;
    if (callBarrier) {
      drv.zeCommandListAppendBarrier(cmdList, nullptr, 0, nullptr);
    }
    if (argDump.argType == KernelArgType::buffer) {
      drv.zeCommandListAppendMemoryCopy(cmdList, const_cast<char*>(argDump.buffer.data()),
                                        argDump.h_buf, argDump.buffer.size(), eventHandle, 0,
                                        nullptr);
    } else if (argDump.argType == KernelArgType::image) {
      drv.zeCommandListAppendImageCopyToMemory(cmdList, const_cast<char*>(argDump.buffer.data()),
                                               argDump.h_img, nullptr, eventHandle, 0, nullptr);
    }
    Log(TRACE) << "^------------------ injected" << (callBarrier ? " barrier with " : " ")
               << "read";
    callBarrier = false;
    if (eventHandle != nullptr) {
      drv.zeEventHostSynchronize(eventHandle, UINT64_MAX);
      drv.zeEventHostReset(eventHandle);
      Log(TRACE) << "^------------------ injected synchronization with event reset";
    }
  }
}

inline void SaveKernelArguments(const ze_event_handle_t& hSignalEvent,
                                const ze_command_list_handle_t& hCommandList,
                                const CKernelState& kernelState,
                                const CCommandListState& cmdListState,
                                bool callOnce = true) {
  const auto& kernelInfo = kernelState.currentKernelInfo;
  const auto& cfg = Config::Get();
  const auto syncNeeded =
      CheckWhetherSync(cmdListState.isImmediate, cmdListState.isSync, hSignalEvent, callOnce);
  if (syncNeeded) {
    drv.zeEventHostSynchronize(hSignalEvent, UINT64_MAX);
    Log(TRACE) << "^------------------ injected synchronization";
  }
  auto& sd = SD();
  auto& readyArgVec = sd.Map<CKernelArgumentDump>()[hCommandList];
  PrepareArguments(kernelState, kernelInfo.kernelNumber, readyArgVec);
  InjectReadsForArguments(readyArgVec, hCommandList, cmdListState.isImmediate ? false : callOnce,
                          syncNeeded ? cmdListState.hContext : nullptr);
  if (cmdListState.isImmediate && CheckWhetherDumpQueueSubmit(cmdListState.cmdQueueNumber)) {
    for (const auto& arg : readyArgVec) {
      if (kernelInfo.kernelNumber == arg.kernelNumber) {
        sd.layoutBuilder.UpdateLayout(kernelState, kernelInfo, cmdListState.cmdQueueNumber,
                                      cmdListState.cmdListNumber, arg.kernelArgIndex);
      }
    }
    DumpReadyArguments(readyArgVec, cmdListState.cmdQueueNumber, cmdListState.cmdListNumber, cfg,
                       sd);
    sd.Release<CKernelArgumentDump>(hCommandList);
  }
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
  }
}

inline void zeCommandListCreate_SD(ze_result_t return_value,
                                   ze_context_handle_t hContext,
                                   ze_device_handle_t hDevice,
                                   const ze_command_list_desc_t* desc,
                                   ze_command_list_handle_t* phCommandList) {
  CGits::Instance().CommandListCountUp();
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
  CGits::Instance().CommandListCountUp();
  CGits::Instance().CommandQueueExecCountUp();
  if (return_value == ZE_RESULT_SUCCESS && phCommandList != nullptr && altdesc != nullptr) {
    auto& clState = SD().Map<CCommandListState>()[*phCommandList];
    clState = std::make_unique<CCommandListState>(hContext, hDevice, *altdesc);
    clState->cmdListNumber = CGits::Instance().CurrentCommandListCount();
    clState->cmdQueueNumber = CGits::Instance().CurrentCommandQueueExecCount();
  }
}
inline void AppendLaunchKernel(ze_result_t return_value,
                               ze_command_list_handle_t hCommandList,
                               ze_kernel_handle_t hKernel,
                               const ze_group_count_t* pLaunchFuncArgs,
                               ze_event_handle_t hSignalEvent,
                               uint32_t numWaitEvents,
                               ze_event_handle_t* phWaitEvents) {
  (void)return_value;
  (void)numWaitEvents;
  (void)phWaitEvents;
  CommandListKernelInit(hCommandList, hKernel, pLaunchFuncArgs);
  const auto& cmdListState = SD().Get<CCommandListState>(hCommandList, EXCEPTION_MESSAGE);
  auto& kernelState = SD().Get<CKernelState>(hKernel, EXCEPTION_MESSAGE);
  if (CheckWhetherDumpKernel(kernelState.currentKernelInfo.kernelNumber,
                             cmdListState.cmdListNumber) &&
      (cmdListState.isImmediate || !CaptureAfterSubmit())) {
    SaveKernelArguments(hSignalEvent, hCommandList, kernelState, cmdListState);
  }
}
inline void zeCommandListAppendLaunchKernel_SD(ze_result_t return_value,
                                               ze_command_list_handle_t hCommandList,
                                               ze_kernel_handle_t hKernel,
                                               const ze_group_count_t* pLaunchFuncArgs,
                                               ze_event_handle_t hSignalEvent,
                                               uint32_t numWaitEvents,
                                               ze_event_handle_t* phWaitEvents) {
  AppendLaunchKernel(return_value, hCommandList, hKernel, pLaunchFuncArgs, hSignalEvent,
                     numWaitEvents, phWaitEvents);
}

inline void zeCommandListAppendLaunchCooperativeKernel_SD(ze_result_t return_value,
                                                          ze_command_list_handle_t hCommandList,
                                                          ze_kernel_handle_t hKernel,
                                                          const ze_group_count_t* pLaunchFuncArgs,
                                                          ze_event_handle_t hSignalEvent,
                                                          uint32_t numWaitEvents,
                                                          ze_event_handle_t* phWaitEvents) {
  AppendLaunchKernel(return_value, hCommandList, hKernel, pLaunchFuncArgs, hSignalEvent,
                     numWaitEvents, phWaitEvents);
}

inline void zeCommandListAppendLaunchKernelIndirect_SD(
    ze_result_t return_value,
    ze_command_list_handle_t hCommandList,
    ze_kernel_handle_t hKernel,
    const ze_group_count_t* pLaunchArgumentsBuffer,
    ze_event_handle_t hSignalEvent,
    uint32_t numWaitEvents,
    ze_event_handle_t* phWaitEvents) {
  AppendLaunchKernel(return_value, hCommandList, hKernel, pLaunchArgumentsBuffer, hSignalEvent,
                     numWaitEvents, phWaitEvents);
}

inline void zeCommandListAppendLaunchMultipleKernelsIndirect_SD(
    ze_result_t return_value,
    ze_command_list_handle_t hCommandList,
    uint32_t numKernels,
    ze_kernel_handle_t* phKernels,
    const uint32_t* pCountBuffer,
    const ze_group_count_t* pLaunchArgumentsBuffer,
    ze_event_handle_t hSignalEvent,
    uint32_t numWaitEvents,
    ze_event_handle_t* phWaitEvents) {
  (void)return_value;
  (void)pCountBuffer;
  (void)numWaitEvents;
  (void)phWaitEvents;
  bool callOnce = true;
  const auto& cmdListState = SD().Get<CCommandListState>(hCommandList, EXCEPTION_MESSAGE);
  for (auto i = 0u; i < numKernels; i++) {
    CommandListKernelInit(hCommandList, phKernels[i], pLaunchArgumentsBuffer);
    auto& kernelState = SD().Get<CKernelState>((phKernels)[i], EXCEPTION_MESSAGE);

    if (CheckWhetherDumpKernel(kernelState.currentKernelInfo.kernelNumber,
                               cmdListState.cmdListNumber) &&
        (cmdListState.isImmediate || !CaptureAfterSubmit())) {
      SaveKernelArguments(hSignalEvent, hCommandList, kernelState, cmdListState, callOnce);
      callOnce = false;
    }
  }
}

inline void zeKernelSetArgumentValue_SD(ze_result_t return_value,
                                        ze_kernel_handle_t hKernel,
                                        uint32_t argIndex,
                                        size_t argSize,
                                        const void* pArgValue) {
  (void)return_value;
  SD().Get<CKernelState>(hKernel, EXCEPTION_MESSAGE)
      .currentKernelInfo.SetArgument(argIndex, argSize, pArgValue);
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

inline void zeCommandQueueExecuteCommandLists_SD(ze_result_t return_value,
                                                 ze_command_queue_handle_t hCommandQueue,
                                                 uint32_t numCommandLists,
                                                 ze_command_list_handle_t* phCommandLists,
                                                 ze_fence_handle_t hFence) {
  (void)return_value;
  const auto& cfg = Config::Get();
  auto& sd = SD();
  if (hFence != nullptr) {
    auto& fenceState = sd.Get<CFenceState>(hFence, EXCEPTION_MESSAGE);
    fenceState.canBeSynced = true;
  }
  auto& cqState = sd.Get<CCommandQueueState>(hCommandQueue, EXCEPTION_MESSAGE);
  if (cfg.IsPlayer()) {
    cqState.cmdQueueNumber = gits::CGits::Instance().CurrentCommandQueueExecCount();
  }
  if (!cqState.isSync && CheckWhetherQueueRequiresSync(phCommandLists, numCommandLists)) {
    drv.zeCommandQueueSynchronize(hCommandQueue, UINT64_MAX);
    Log(TRACE) << "^------------------ injected synchronization";
  }
  ze_command_list_handle_t tmpList = nullptr;
  ze_result_t err = ZE_RESULT_ERROR_UNINITIALIZED;
  for (auto i = 0u; i < numCommandLists; i++) {
    const auto& cmdListState = sd.Get<CCommandListState>(phCommandLists[i], EXCEPTION_MESSAGE);
    for (auto& kernel : cmdListState.appendedKernelsMap) {
      const auto& kernelState = sd.Get<CKernelState>(kernel.second, EXCEPTION_MESSAGE);
      const auto& kernelInfo = kernelState.executedKernels.at(kernel.first);
      Log(TRACE) << "--- Queue #" << cqState.cmdQueueNumber << " / CommandList #"
                 << cmdListState.cmdListNumber << " / Kernel #" << kernelInfo.kernelNumber
                 << " ---";
      if (!cmdListState.isImmediate && CheckWhetherDumpQueueSubmit(cqState.cmdQueueNumber) &&
          CheckWhetherDumpKernel(kernelInfo.kernelNumber, cmdListState.cmdListNumber) &&
          CaptureAfterSubmit()) {
        if (!tmpList) {
          ze_command_queue_desc_t queueDesc = {};
          queueDesc.mode = ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS;
          err = drv.zeCommandListCreateImmediate(cmdListState.hContext, cmdListState.hDevice,
                                                 &queueDesc, &tmpList);
          Log(TRACE) << "^------------------ injected command list";
        }
        auto& readyArgVec = sd.Map<CKernelArgumentDump>()[tmpList];
        PrepareArguments(kernelState, kernel.first, readyArgVec, true);
        InjectReadsForArguments(readyArgVec, tmpList, false, nullptr);
        for (const auto& arg : readyArgVec) {
          if (kernelInfo.kernelNumber == arg.kernelNumber) {
            sd.layoutBuilder.UpdateLayout(kernelState, kernelInfo, cqState.cmdQueueNumber,
                                          cmdListState.cmdListNumber, arg.kernelArgIndex);
          }
        }
        DumpReadyArguments(readyArgVec, cqState.cmdQueueNumber, cmdListState.cmdListNumber, cfg,
                           sd);
        sd.Release<CKernelArgumentDump>(tmpList);
      }
    }
    //Dump enqueued arguments
    if (sd.Exists<CKernelArgumentDump>(phCommandLists[i])) {
      if (CheckWhetherDumpQueueSubmit(cqState.cmdQueueNumber)) {
        for (auto& kernel : cmdListState.appendedKernelsMap) {
          const auto& kernelState = sd.Get<CKernelState>(kernel.second, EXCEPTION_MESSAGE);
          const auto& kernelInfo = kernelState.executedKernels.at(kernel.first);
          for (const auto& arg : sd.Map<CKernelArgumentDump>()[phCommandLists[i]]) {
            if (kernelInfo.kernelNumber == arg.kernelNumber) {
              sd.layoutBuilder.UpdateLayout(kernelState, kernelInfo, cqState.cmdQueueNumber,
                                            cmdListState.cmdListNumber, arg.kernelArgIndex);
            }
          }
        }
        DumpReadyArguments(sd.Map<CKernelArgumentDump>()[phCommandLists[i]], cqState.cmdQueueNumber,
                           cmdListState.cmdListNumber, cfg, sd);
      }
      sd.Release<CKernelArgumentDump>(phCommandLists[i]);
    }
  }
  if (err == ZE_RESULT_SUCCESS) {
    drv.zeCommandListDestroy(tmpList);
    Log(TRACE) << "^------------------ injected command list destroy";
  }
}

inline void zeMemFree_SD(ze_result_t return_value, ze_context_handle_t hContext, void* ptr) {
  (void)hContext;
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
    moduleState = std::make_unique<CModuleState>(hContext, hDevice, *desc,
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
    kernelState.currentKernelInfo.groupSizeX = groupSizeX;
    kernelState.currentKernelInfo.groupSizeY = groupSizeY;
    kernelState.currentKernelInfo.groupSizeZ = groupSizeZ;
    kernelState.isGroupSizeSet = true;
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
    char* strdupPtr =
        const_cast<char*>(SD().Get<CKernelState>(hKernel, EXCEPTION_MESSAGE).desc.pKernelName);
    if (strdupPtr != nullptr) {
      delete[] strdupPtr;
    }
    SD().Release<CKernelState>(hKernel);
  }
}

inline void zeModuleDestroy_SD(ze_result_t return_value, ze_module_handle_t hModule) {
  if (return_value == ZE_RESULT_SUCCESS) {
    auto& sd = SD();
    sd.scanningGlobalPointersMode.erase(hModule);
    std::vector<void*> globalPtrs;
    for (auto& allocState : sd.Map<CAllocState>()) {
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
    kernelState.currentKernelInfo.indirectUsmTypes |= static_cast<unsigned>(flags);
  }
}

inline void zeContextDestroy_SD(ze_result_t return_value, ze_context_handle_t hContext) {
  if (return_value == ZE_RESULT_SUCCESS) {
    const auto& list = SD().Get<CContextState>(hContext, EXCEPTION_MESSAGE).gitsImmediateList;
    if (list != nullptr) {
      drv.zeCommandListDestroy(list);
    }
    for (auto& state : SD().Map<CCommandListState>()) {
      if (state.second->hContext == hContext) {
        SD().Release<CCommandListState>(state.first);
      }
    }
    SD().Release<CContextState>(hContext);
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

inline void zeCommandListReset_SD(ze_result_t return_value, ze_command_list_handle_t hCommandList) {
  (void)return_value;
  auto& cmdListState = SD().Get<CCommandListState>(hCommandList, EXCEPTION_MESSAGE);
  cmdListState.RestoreReset();
  cmdListState.appendedKernelsMap.clear();
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
                                 uint32_t numDevices,
                                 ze_device_handle_t* phDevices,
                                 ze_event_pool_handle_t* phEventPool) {
  (void)numDevices;
  (void)phDevices;
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
  const auto isBufferTranslated = Config::IsRecorder();
  for (uint32_t i = 0U; i < numOffsets; i++) {
    allocState.indirectPointersOffsets[pOffsets[i]] = isBufferTranslated;
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
  if (return_value == ZE_RESULT_SUCCESS) {
    auto& sd = SD();
    auto& allocState = sd.Map<CAllocState>()[*pptr];
    allocState =
        std::make_unique<CAllocState>(hModule, pGlobalName, *pSize, AllocStateType::global_pointer);
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
    kernelState.currentKernelInfo.offsetX = offsetX;
    kernelState.currentKernelInfo.offsetX = offsetY;
    kernelState.currentKernelInfo.offsetX = offsetZ;
    kernelState.isOffsetSet = true;
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
    kernelState.currentKernelInfo.schedulingHintflags = pHint->flags;
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
    deviceState.cqGroupProperties.assign(pCommandQueueGroupProperties,
                                         pCommandQueueGroupProperties + *pCount);
  }
}

inline void zeCommandListAppendMemoryCopy_SD(ze_result_t return_value,
                                             ze_command_list_handle_t hCommandList,
                                             void* dstptr,
                                             const void* srcptr,
                                             size_t size,
                                             ze_event_handle_t hSignalEvent,
                                             uint32_t numWaitEvents,
                                             ze_event_handle_t* phWaitEvents) {
  (void)dstptr;
  (void)size;
  (void)hSignalEvent;
  (void)numWaitEvents;
  (void)phWaitEvents;
  if (return_value == ZE_RESULT_SUCCESS) {
    auto& sd = SD();
    if (sd.Exists<CAllocState>(const_cast<void*>(srcptr))) {
      auto& allocState = sd.Get<CAllocState>(const_cast<void*>(srcptr), EXCEPTION_MESSAGE);
      if (allocState.allocType == AllocStateType::global_pointer) {
        SaveGlobalPointerAllocationToMemory(sd, hCommandList, allocState, srcptr);
      }
    }
  }
}

} // namespace l0
} // namespace gits

// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   l0RecorderSubWrappers.h
*
* @brief API RECWRAP implementations.
*/

#pragma once

#include "apis_iface.h"
#include "exception.h"
#include "l0Functions.h"
#include "l0Header.h"
#include "l0StateDynamic.h"
#include "l0StateTracking.h"
#include "l0HelperFunctions.h"
#include "l0Drivers.h"
#include "l0Structs.h"
#include "l0Tools.h"
#include "openclTools.h"
#include "recorder.h"
#include "l0StateRestore.h"
#include <vector>

namespace gits {
namespace l0 {
namespace {
bool CheckWhetherUpdateUSM(const void* ptr) {
  bool update = false;
  auto& sd = SD();
  const auto allocInfo = GetAllocFromRegion(const_cast<void*>(ptr), sd);
  if (allocInfo.first != nullptr) {
    const auto& allocState = sd.Get<CAllocState>(allocInfo.first, EXCEPTION_MESSAGE);
    update = allocState.sniffedRegionHandle != nullptr &&
             !(**allocState.sniffedRegionHandle).GetTouchedPages().empty();
  }
  return update;
}

std::set<const void*> GetPointersToUpdate(ze_kernel_handle_t hKernel) {
  std::set<const void*> ptrsToUpdate;
  const auto& kernelState = SD().Get<CKernelState>(hKernel, EXCEPTION_MESSAGE);
  for (const auto& arg : kernelState.currentKernelInfo->GetArguments()) {
    if (arg.second.type == KernelArgType::buffer && CheckWhetherUpdateUSM(arg.second.argValue)) {
      ptrsToUpdate.insert(arg.second.argValue);
    }
  }
  if (kernelState.currentKernelInfo->indirectUsmTypes) {
    for (const auto& ptr : SD().Map<CAllocState>()) {
      if ((static_cast<unsigned>(ptr.second->memType) &
           kernelState.currentKernelInfo->indirectUsmTypes) &&
          CheckWhetherUpdateUSM(ptr.first)) {
        ptrsToUpdate.insert(ptr.first);
      }
    }
  }
  return ptrsToUpdate;
}
ze_command_list_handle_t GetCommandListImmediateRec(CStateDynamic& sd,
                                                    const CDriver& driver,
                                                    const ze_context_handle_t& context,
                                                    CRecorder* recorder) {
  ze_result_t err = ZE_RESULT_ERROR_UNINITIALIZED;
  auto commandList = GetCommandListImmediate(sd, driver, context, &err);
  if (err == ZE_RESULT_SUCCESS) {
    const auto device = GetGPUDevice(sd, driver);
    ze_command_queue_desc_t desc = {};
    desc.mode = ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS;
    recorder->Schedule(
        new CzeCommandListCreateImmediate(err, context, device, &desc, &commandList));
    zeCommandListCreateImmediate_SD(err, context, device, &desc, &commandList);
  }
  return commandList;
}
void UpdateOriginalQueueGroupProperties(CDeviceState& deviceState,
                                        const ze_device_handle_t& hDevice) {
  auto& groupProperties = deviceState.cqGroupProperties;
  if (deviceState.cqGroupProperties.empty()) {
    uint32_t numGroupProperties = 0U;
    drv.inject.zeDeviceGetCommandQueueGroupProperties(hDevice, &numGroupProperties, nullptr);
    groupProperties.resize(numGroupProperties);
    for (auto& property : groupProperties) {
      property.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_GROUP_PROPERTIES;
      property.pNext = nullptr;
    }
    const auto result = drv.inject.zeDeviceGetCommandQueueGroupProperties(
        hDevice, &numGroupProperties, groupProperties.data());
    if (result != ZE_RESULT_SUCCESS) {
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
    deviceState.originalQueueGroupProperties = groupProperties;
  }
}
void SaveKernelArgumentsForStateRestore(CStateDynamic& sd,
                                        const CDriver& driver,
                                        const ze_kernel_handle_t& hKernel,
                                        const ze_command_list_handle_t& hCommandList,
                                        const uint32_t numEvents,
                                        ze_event_handle_t* waitList) {
  auto& kernelState = sd.Get<CKernelState>(hKernel, EXCEPTION_MESSAGE);
  auto& stateRestoreBuffersSnapshot = kernelState.currentKernelInfo->stateRestoreBuffers;
  std::set<void*> restoredPtrs;
  if (kernelState.currentKernelInfo->indirectUsmTypes != 0) {
    for (const auto& allocState : sd.Map<CAllocState>()) {
      if (kernelState.currentKernelInfo->indirectUsmTypes &
          static_cast<unsigned>(allocState.second->memType)) {
        restoredPtrs.insert(allocState.first);
        stateRestoreBuffersSnapshot.emplace_back(
            std::make_unique<CKernelArgument>(allocState.second->size, allocState.first));
        driver.inject.zeCommandListAppendMemoryCopy(
            hCommandList, stateRestoreBuffersSnapshot.back()->buffer.data(), allocState.first,
            allocState.second->size, nullptr, numEvents, waitList);
      }
    }
  }
  for (const auto& arg : kernelState.currentKernelInfo->GetArguments()) {
    if (arg.second.type == KernelArgType::buffer) {
      const auto allocInfo = GetAllocFromRegion(const_cast<void*>(arg.second.argValue), sd);
      if (restoredPtrs.count(allocInfo.first) == 0) {
        void* ptr = GetOffsetPointer(allocInfo.first, allocInfo.second);
        const auto& allocState = sd.Get<CAllocState>(allocInfo.first, EXCEPTION_MESSAGE);
        stateRestoreBuffersSnapshot.emplace_back(
            std::make_unique<CKernelArgument>(allocState.size - allocInfo.second, ptr));
        driver.inject.zeCommandListAppendMemoryCopy(
            hCommandList, stateRestoreBuffersSnapshot.back()->buffer.data(), ptr,
            stateRestoreBuffersSnapshot.back()->buffer.size(), nullptr, numEvents, waitList);
      }
    } else if (arg.second.type == KernelArgType::image) {
      auto h_img = reinterpret_cast<ze_image_handle_t>(const_cast<void*>(arg.second.argValue));
      const auto& imageState = sd.Get<CImageState>(h_img, EXCEPTION_MESSAGE);
      stateRestoreBuffersSnapshot.emplace_back(
          std::make_unique<CKernelArgument>(CalculateImageSize(imageState.desc), h_img));
      driver.inject.zeCommandListAppendImageCopyToMemory(
          hCommandList, stateRestoreBuffersSnapshot.back()->buffer.data(), h_img, nullptr, nullptr,
          numEvents, waitList);
    }
  }
  driver.inject.zeCommandListAppendBarrier(hCommandList, nullptr, numEvents, waitList);
}
std::vector<ze_command_list_handle_t> GetCommandListToRestore(
    const ApisIface::ApiCompute& l0IFace,
    CStateDynamic& sd,
    uint32_t numCommandLists,
    ze_command_list_handle_t* phCommandLists) {
  std::vector<ze_command_list_handle_t> cmdLists;
  for (auto i = 0u; i < numCommandLists; i++) {
    const auto& cmdListState = sd.Get<CCommandListState>(phCommandLists[i], EXCEPTION_MESSAGE);
    if (l0IFace.CfgRec_IsCommandListToRecord(cmdListState.cmdListNumber)) {
      cmdLists.push_back(phCommandLists[i]);
    }
  }
  return cmdLists;
}
std::vector<ze_command_list_handle_t> GetCommandListsToSubcapture(
    const ApisIface::ApiCompute& l0IFace,
    CStateDynamic& sd,
    uint32_t numCommandLists,
    ze_command_list_handle_t* phCommandLists) {
  const auto cmdLists = GetCommandListToRestore(l0IFace, sd, numCommandLists, phCommandLists);
  for (const auto& cmdList : cmdLists) {
    const auto& cmdListState = sd.Get<CCommandListState>(cmdList, EXCEPTION_MESSAGE);
    for (const auto& kernelInfo : cmdListState.appendedKernels) {
      if (l0IFace.CfgRec_IsKernelToRecord(kernelInfo->kernelNumber)) {
        return cmdLists;
      }
    }
  }
  return cmdLists;
}

void SubcaptureLogicForImmediateCommandLists(CRecorder& recorder,
                                             const ApisIface::ApiCompute& l0IFace,
                                             CStateDynamic& sd,
                                             ze_command_list_handle_t hCommandList) {
  if (l0IFace.CfgRec_IsKernelsRangeMode() &&
      l0IFace.CfgRec_IsKernelToRecord(gits::CGits::Instance().CurrentKernelCount())) {
    auto& cmdListState = sd.Get<CCommandListState>(hCommandList, EXCEPTION_MESSAGE);
    if (cmdListState.isImmediate) {
      if (!cmdListState.isSync) {
        drv.inject.zeCommandListAppendBarrier(hCommandList, nullptr, 0, nullptr);
      }
      const auto kernelCount = gits::CGits::Instance().CurrentKernelCount();
      if (l0IFace.CfgRec_StartKernel() == kernelCount) {
        recorder.Start();
      }
      if (l0IFace.CfgRec_StopKernel() == kernelCount) {
        recorder.Stop();
        recorder.MarkForDeletion();
      }
    }
  }
}
} // namespace

inline void zeCommandListAppendLaunchKernel_RECWRAP_PRE(CRecorder& recorder,
                                                        ze_result_t return_value,
                                                        ze_command_list_handle_t hCommandList,
                                                        ze_kernel_handle_t hKernel,
                                                        const ze_group_count_t* pLaunchFuncArgs,
                                                        ze_event_handle_t hSignalEvent,
                                                        uint32_t numWaitEvents,
                                                        ze_event_handle_t* phWaitEvents) {
  std::ignore = return_value;
  std::ignore = hSignalEvent;
  std::ignore = recorder;
  std::ignore = pLaunchFuncArgs;
  auto& sd = SD();
  if (sd.nomenclatureCounting) {
    gits::CGits::Instance().KernelCountUp();
  }
  const auto& l0IFace = gits::CGits::Instance().apis.IfaceCompute();
  if (l0IFace.CfgRec_IsKernelsRangeMode() &&
      l0IFace.CfgRec_IsKernelToRecord(gits::CGits::Instance().CurrentKernelCount())) {
    SaveKernelArgumentsForStateRestore(sd, drv, hKernel, hCommandList, numWaitEvents, phWaitEvents);
  }
}

inline void zeCommandListAppendLaunchKernel_RECWRAP(CRecorder& recorder,
                                                    ze_result_t return_value,
                                                    ze_command_list_handle_t hCommandList,
                                                    ze_kernel_handle_t hKernel,
                                                    const ze_group_count_t* pLaunchFuncArgs,
                                                    ze_event_handle_t hSignalEvent,
                                                    uint32_t numWaitEvents,
                                                    ze_event_handle_t* phWaitEvents) {
  if (recorder.Running()) {
    for (const auto ptr : GetPointersToUpdate(hKernel)) {
      recorder.Schedule(new CGitsL0MemoryUpdate(ptr));
    }
    recorder.Schedule(new CzeCommandListAppendLaunchKernel(return_value, hCommandList, hKernel,
                                                           pLaunchFuncArgs, hSignalEvent,
                                                           numWaitEvents, phWaitEvents));
  }
  zeCommandListAppendLaunchKernel_SD(return_value, hCommandList, hKernel, pLaunchFuncArgs,
                                     hSignalEvent, numWaitEvents, phWaitEvents);
  const auto& l0IFace = gits::CGits::Instance().apis.IfaceCompute();
  SubcaptureLogicForImmediateCommandLists(recorder, l0IFace, SD(), hCommandList);
}

inline void zeCommandListAppendMemoryCopy_RECWRAP(CRecorder& recorder,
                                                  ze_result_t return_value,
                                                  ze_command_list_handle_t hCommandList,
                                                  void* dstptr,
                                                  const void* srcptr,
                                                  size_t size,
                                                  ze_event_handle_t hSignalEvent,
                                                  uint32_t numWaitEvents,
                                                  ze_event_handle_t* phWaitEvents) {
  if (recorder.Running()) {
    if (CheckWhetherUpdateUSM(srcptr)) {
      recorder.Schedule(new CGitsL0MemoryUpdate(srcptr));
    }
    recorder.Schedule(new CzeCommandListAppendMemoryCopy(return_value, hCommandList, dstptr, srcptr,
                                                         size, hSignalEvent, numWaitEvents,
                                                         phWaitEvents));
  }
  zeCommandListAppendMemoryCopy_SD(return_value, hCommandList, dstptr, srcptr, size, hSignalEvent,
                                   numWaitEvents, phWaitEvents);
  if (recorder.Running()) {
    auto& sd = SD();
    const auto allocInfo = GetAllocFromRegion(const_cast<void*>(srcptr), sd);
    if (allocInfo.first != nullptr) {
      const auto& allocState = sd.Get<CAllocState>(allocInfo.first, EXCEPTION_MESSAGE);
      if (allocState.allocType == AllocStateType::global_pointer) {
        recorder.Schedule(new CGitsL0MemoryRestore(srcptr, allocState.globalPtrAllocation));
      }
    }
  }
}

inline void zeCommandListAppendMemoryCopyRegion_RECWRAP(CRecorder& recorder,
                                                        ze_result_t return_value,
                                                        ze_command_list_handle_t hCommandList,
                                                        void* dstptr,
                                                        const ze_copy_region_t* dstRegion,
                                                        uint32_t dstPitch,
                                                        uint32_t dstSlicePitch,
                                                        const void* srcptr,
                                                        const ze_copy_region_t* srcRegion,
                                                        uint32_t srcPitch,
                                                        uint32_t srcSlicePitch,
                                                        ze_event_handle_t hSignalEvent,
                                                        uint32_t numWaitEvents,
                                                        ze_event_handle_t* phWaitEvents) {
  if (recorder.Running()) {
    if (CheckWhetherUpdateUSM(srcptr)) {
      recorder.Schedule(new CGitsL0MemoryUpdate(srcptr));
    }
    recorder.Schedule(new CzeCommandListAppendMemoryCopyRegion(
        return_value, hCommandList, dstptr, dstRegion, dstPitch, dstSlicePitch, srcptr, srcRegion,
        srcPitch, srcSlicePitch, hSignalEvent, numWaitEvents, phWaitEvents));
  }
}

inline void zeCommandListAppendLaunchCooperativeKernel_RECWRAP(
    CRecorder& recorder,
    ze_result_t return_value,
    ze_command_list_handle_t hCommandList,
    ze_kernel_handle_t hKernel,
    const ze_group_count_t* pLaunchFuncArgs,
    ze_event_handle_t hSignalEvent,
    uint32_t numWaitEvents,
    ze_event_handle_t* phWaitEvents) {
  if (recorder.Running()) {
    for (const auto ptr : GetPointersToUpdate(hKernel)) {
      recorder.Schedule(new CGitsL0MemoryUpdate(ptr));
    }
    recorder.Schedule(new CzeCommandListAppendLaunchCooperativeKernel(
        return_value, hCommandList, hKernel, pLaunchFuncArgs, hSignalEvent, numWaitEvents,
        phWaitEvents));
  }
  zeCommandListAppendLaunchCooperativeKernel_SD(return_value, hCommandList, hKernel,
                                                pLaunchFuncArgs, hSignalEvent, numWaitEvents,
                                                phWaitEvents);
  const auto& l0IFace = gits::CGits::Instance().apis.IfaceCompute();
  SubcaptureLogicForImmediateCommandLists(recorder, l0IFace, SD(), hCommandList);
}

inline void zeCommandListAppendLaunchKernelIndirect_RECWRAP(
    CRecorder& recorder,
    ze_result_t return_value,
    ze_command_list_handle_t hCommandList,
    ze_kernel_handle_t hKernel,
    const ze_group_count_t* pLaunchArgumentsBuffer,
    ze_event_handle_t hSignalEvent,
    uint32_t numWaitEvents,
    ze_event_handle_t* phWaitEvents) {
  if (recorder.Running()) {
    for (const auto ptr : GetPointersToUpdate(hKernel)) {
      recorder.Schedule(new CGitsL0MemoryUpdate(ptr));
    }
    recorder.Schedule(new CzeCommandListAppendLaunchKernelIndirect(
        return_value, hCommandList, hKernel, pLaunchArgumentsBuffer, hSignalEvent, numWaitEvents,
        phWaitEvents));
  }
  zeCommandListAppendLaunchKernelIndirect_SD(return_value, hCommandList, hKernel,
                                             pLaunchArgumentsBuffer, hSignalEvent, numWaitEvents,
                                             phWaitEvents);
  const auto& l0IFace = gits::CGits::Instance().apis.IfaceCompute();
  SubcaptureLogicForImmediateCommandLists(recorder, l0IFace, SD(), hCommandList);
}

inline void zeCommandListAppendLaunchMultipleKernelsIndirect_RECWRAP(
    CRecorder& recorder,
    ze_result_t return_value,
    ze_command_list_handle_t hCommandList,
    uint32_t numKernels,
    ze_kernel_handle_t* phKernels,
    const uint32_t* pCountBuffer,
    const ze_group_count_t* pLaunchArgumentsBuffer,
    ze_event_handle_t hSignalEvent,
    uint32_t numWaitEvents,
    ze_event_handle_t* phWaitEvents) {
  if (recorder.Running()) {
    for (auto i = 0u; i < numKernels; i++) {
      for (const auto ptr : GetPointersToUpdate(phKernels[i])) {
        recorder.Schedule(new CGitsL0MemoryUpdate(ptr));
      }
    }
    recorder.Schedule(new CzeCommandListAppendLaunchMultipleKernelsIndirect(
        return_value, hCommandList, numKernels, phKernels, pCountBuffer, pLaunchArgumentsBuffer,
        hSignalEvent, numWaitEvents, phWaitEvents));
  }
  zeCommandListAppendLaunchMultipleKernelsIndirect_SD(
      return_value, hCommandList, numKernels, phKernels, pCountBuffer, pLaunchArgumentsBuffer,
      hSignalEvent, numWaitEvents, phWaitEvents);
  const auto& l0IFace = gits::CGits::Instance().apis.IfaceCompute();
  SubcaptureLogicForImmediateCommandLists(recorder, l0IFace, SD(), hCommandList);
}

inline void zeCommandListAppendImageCopyFromMemory_RECWRAP(CRecorder& recorder,
                                                           ze_result_t return_value,
                                                           ze_command_list_handle_t hCommandList,
                                                           ze_image_handle_t hDstImage,
                                                           const void* srcptr,
                                                           const ze_image_region_t* pDstRegion,
                                                           ze_event_handle_t hSignalEvent,
                                                           uint32_t numWaitEvents,
                                                           ze_event_handle_t* phWaitEvents) {
  if (recorder.Running()) {
    if (CheckWhetherUpdateUSM(srcptr)) {
      recorder.Schedule(new CGitsL0MemoryUpdate(srcptr));
    }
    recorder.Schedule(new CzeCommandListAppendImageCopyFromMemory(
        return_value, hCommandList, hDstImage, srcptr, pDstRegion, hSignalEvent, numWaitEvents,
        phWaitEvents));
  }
}

inline void zeCommandListAppendMemoryFill_RECWRAP(CRecorder& recorder,
                                                  ze_result_t return_value,
                                                  ze_command_list_handle_t hCommandList,
                                                  void* ptr,
                                                  const void* pattern,
                                                  size_t pattern_size,
                                                  size_t size,
                                                  ze_event_handle_t hSignalEvent,
                                                  uint32_t numWaitEvents,
                                                  ze_event_handle_t* phWaitEvents) {
  if (recorder.Running()) {
    if (CheckWhetherUpdateUSM(ptr)) {
      recorder.Schedule(new CGitsL0MemoryUpdate(ptr));
    }
    recorder.Schedule(new CzeCommandListAppendMemoryFill_V1(
        return_value, hCommandList, ptr, pattern, pattern_size, size, hSignalEvent, numWaitEvents,
        phWaitEvents));
  }
}

inline void zeCommandQueueExecuteCommandLists_RECWRAP_PRE(CRecorder& recorder,
                                                          ze_result_t return_value,
                                                          ze_command_queue_handle_t hCommandQueue,
                                                          uint32_t numCommandLists,
                                                          ze_command_list_handle_t* phCommandLists,
                                                          ze_fence_handle_t hFence) {
  std::ignore = return_value;
  std::ignore = numCommandLists;
  std::ignore = phCommandLists;
  std::ignore = hFence;
  auto& sd = SD();
  if (sd.nomenclatureCounting) {
    gits::CGits::Instance().CommandQueueExecCountUp();
  }
  sd.Get<CCommandQueueState>(hCommandQueue, EXCEPTION_MESSAGE).cmdQueueNumber =
      gits::CGits::Instance().CurrentCommandQueueExecCount();
  if (recorder.Running()) {
    for (auto& allocState : sd.Map<CAllocState>()) {
      if (CheckWhetherUpdateUSM(allocState.first)) {
        recorder.Schedule(new CGitsL0MemoryUpdate(allocState.first));
      }
    }
  }
}

inline void zeCommandQueueExecuteCommandLists_RECWRAP(CRecorder& recorder,
                                                      ze_result_t return_value,
                                                      ze_command_queue_handle_t hCommandQueue,
                                                      uint32_t numCommandLists,
                                                      ze_command_list_handle_t* phCommandLists,
                                                      ze_fence_handle_t hFence) {
  const auto& l0IFace = gits::CGits::Instance().apis.IfaceCompute();
  auto& sd = SD();
  const auto subcaptureMode = l0IFace.CfgRec_IsKernelsRangeMode();
  if (subcaptureMode) {
    auto cmdLists = GetCommandListsToSubcapture(l0IFace, sd, numCommandLists, phCommandLists);
    if (!cmdLists.empty()) {
      if (l0IFace.CfgRec_IsStartQueueSubmit()) {
        recorder.Start();
        recorder.Schedule(new CzeCommandQueueExecuteCommandLists(
            return_value, hCommandQueue, static_cast<uint32_t>(cmdLists.size()), cmdLists.data(),
            hFence));
        recorder.Schedule(
            new CzeCommandQueueSynchronize(ZE_RESULT_SUCCESS, hCommandQueue, UINT64_MAX));
      }
      if (l0IFace.CfgRec_IsStopQueueSubmit()) {
        recorder.Stop();
        recorder.MarkForDeletion();
      }
    }
  } else if (recorder.Running()) {
    recorder.Schedule(new CzeCommandQueueExecuteCommandLists(
        return_value, hCommandQueue, numCommandLists, phCommandLists, hFence));
  }
  zeCommandQueueExecuteCommandLists_SD(return_value, hCommandQueue, numCommandLists, phCommandLists,
                                       hFence);
}

inline void zeCommandListAppendMemoryCopyFromContext_RECWRAP(CRecorder& recorder,
                                                             ze_result_t return_value,
                                                             ze_command_list_handle_t hCommandList,
                                                             void* dstptr,
                                                             ze_context_handle_t hContextSrc,
                                                             const void* srcptr,
                                                             size_t size,
                                                             ze_event_handle_t hSignalEvent,
                                                             uint32_t numWaitEvents,
                                                             ze_event_handle_t* phWaitEvents) {
  if (recorder.Running()) {
    if (CheckWhetherUpdateUSM(srcptr)) {
      recorder.Schedule(new CGitsL0MemoryUpdate(srcptr));
    }
    recorder.Schedule(new CzeCommandListAppendMemoryCopyFromContext(
        return_value, hCommandList, dstptr, hContextSrc, srcptr, size, hSignalEvent, numWaitEvents,
        phWaitEvents));
  }
}

inline void zeMemAllocHost_RECWRAP(CRecorder& recorder,
                                   ze_result_t return_value,
                                   ze_context_handle_t hContext,
                                   const ze_host_mem_alloc_desc_t* host_desc,
                                   size_t size,
                                   size_t alignment,
                                   void** pptr) {
  if (recorder.Running()) {
    recorder.Schedule(
        new CzeMemAllocHost(return_value, hContext, host_desc, size, alignment, pptr));
  }
  zeMemAllocHost_SD(return_value, hContext, host_desc, size, alignment, pptr);
  if (recorder.Running() && return_value == ZE_RESULT_SUCCESS &&
      CheckCfgZeroInitialization(Config::Get())) {
    const auto commandList = GetCommandListImmediateRec(SD(), drv, hContext, &recorder);
    if (commandList != nullptr) {
      ZeroInitializeUsm(drv, commandList, pptr, size, UnifiedMemoryType::host);
      recorder.Schedule(new CGitsL0MemoryRestore(*pptr, size));
    }
  }
  const auto& l0IFace = gits::CGits::Instance().apis.IfaceCompute();
  if (Config::Get().recorder.basic.enabled) {
    auto& sniffedRegionHandle = SD().Get<CAllocState>(*pptr, EXCEPTION_MESSAGE).sniffedRegionHandle;
    l0IFace.EnableMemorySnifferForPointer(*pptr, size, sniffedRegionHandle);
  }
}

inline void zeMemAllocDevice_RECWRAP(CRecorder& recorder,
                                     ze_result_t return_value,
                                     ze_context_handle_t hContext,
                                     const ze_device_mem_alloc_desc_t* device_desc,
                                     size_t size,
                                     size_t alignment,
                                     ze_device_handle_t hDevice,
                                     void** pptr) {
  if (recorder.Running()) {
    recorder.Schedule(
        new CzeMemAllocDevice(return_value, hContext, device_desc, size, alignment, hDevice, pptr));
  }
  zeMemAllocDevice_SD(return_value, hContext, device_desc, size, alignment, hDevice, pptr);
  if (recorder.Running() && return_value == ZE_RESULT_SUCCESS &&
      CheckCfgZeroInitialization(Config::Get())) {
    const auto commandList = GetCommandListImmediateRec(SD(), drv, hContext, &recorder);
    if (commandList != nullptr) {
      const auto zeroInitSucceed =
          ZeroInitializeUsm(drv, commandList, pptr, size, UnifiedMemoryType::device);
      if (zeroInitSucceed) {
        const auto zeroBuffer = std::vector<char>(size, 0);
        recorder.Schedule(new CzeCommandListAppendMemoryCopy(
            ZE_RESULT_SUCCESS, commandList, *pptr, zeroBuffer.data(), size, nullptr, 0, nullptr));
      }
    }
  }
}

inline void zeMemAllocShared_RECWRAP(CRecorder& recorder,
                                     ze_result_t return_value,
                                     ze_context_handle_t hContext,
                                     const ze_device_mem_alloc_desc_t* device_desc,
                                     const ze_host_mem_alloc_desc_t* host_desc,
                                     size_t size,
                                     size_t alignment,
                                     ze_device_handle_t hDevice,
                                     void** pptr) {
  const auto& cfg = Config::Get();
  if (recorder.Running()) {
    recorder.Schedule(new CzeMemAllocShared(return_value, hContext, device_desc, host_desc, size,
                                            alignment, hDevice, pptr));
  }
  zeMemAllocShared_SD(return_value, hContext, device_desc, host_desc, size, alignment, hDevice,
                      pptr);
  if (recorder.Running() && return_value == ZE_RESULT_SUCCESS && CheckCfgZeroInitialization(cfg)) {
    const auto commandList = GetCommandListImmediateRec(SD(), drv, hContext, &recorder);
    if (commandList != nullptr) {
      ZeroInitializeUsm(drv, commandList, pptr, size, UnifiedMemoryType::shared);
      recorder.Schedule(new CGitsL0MemoryRestore(*pptr, size));
    }
  }
  if (cfg.recorder.basic.enabled) {
    auto& sniffedRegionHandle = SD().Get<CAllocState>(*pptr, EXCEPTION_MESSAGE).sniffedRegionHandle;
    const auto& l0IFace = gits::CGits::Instance().apis.IfaceCompute();
    l0IFace.EnableMemorySnifferForPointer(*pptr, size, sniffedRegionHandle);
  }
}

inline void zeImageCreate_RECWRAP(CRecorder& recorder,
                                  ze_result_t return_value,
                                  ze_context_handle_t hContext,
                                  ze_device_handle_t hDevice,
                                  const ze_image_desc_t* desc,
                                  ze_image_handle_t* phImage) {
  if (recorder.Running()) {
    recorder.Schedule(new CzeImageCreate(return_value, hContext, hDevice, desc, phImage));
  }
  zeImageCreate_SD(return_value, hContext, hDevice, desc, phImage);
  if (return_value == ZE_RESULT_SUCCESS && recorder.Running() &&
      CheckCfgZeroInitialization(Config::Get())) {
    const auto commandList = GetCommandListImmediateRec(SD(), drv, hContext, &recorder);
    if (commandList != nullptr) {
      const auto zeroInitSucceed = ZeroInitializeImage(drv, commandList, phImage, desc);
      if (zeroInitSucceed) {
        const auto size = CalculateImageSize(*desc);
        const auto zeroBuffer = std::vector<char>(size, 0);
        ze_image_region_t region = {};
        region.width = static_cast<uint32_t>(desc->width);
        region.height = desc->height;
        region.depth = desc->depth;
        recorder.Schedule(new CzeCommandListAppendImageCopyFromMemory(
            ZE_RESULT_SUCCESS, commandList, *phImage, zeroBuffer.data(), &region, nullptr, 0,
            nullptr));
      }
    }
  }
}

inline void zeDeviceGetCommandQueueGroupProperties_RECWRAP(
    CRecorder& recorder,
    ze_result_t return_value,
    ze_device_handle_t hDevice,
    uint32_t* pCount,
    ze_command_queue_group_properties_t* pCommandQueueGroupProperties) {
  if (recorder.Running()) {
    if (pCommandQueueGroupProperties != nullptr) {
      recorder.Schedule(new CzeDeviceGetCommandQueueGroupProperties(return_value, hDevice, pCount,
                                                                    pCommandQueueGroupProperties));
    }
  }
}

inline void zesDevicePciGetBars_RECWRAP(CRecorder& recorder,
                                        ze_result_t return_value,
                                        zes_device_handle_t hDevice,
                                        uint32_t* pCount,
                                        zes_pci_bar_properties_t* pProperties) {
  if (recorder.Running()) {
    if (pProperties != nullptr) {
      recorder.Schedule(new CzesDevicePciGetBars(return_value, hDevice, pCount, pProperties));
    }
  }
}

inline void zeDriverGetExtensionProperties_RECWRAP(
    CRecorder& recorder,
    ze_result_t return_value,
    ze_driver_handle_t hDriver,
    uint32_t* pCount,
    ze_driver_extension_properties_t* pExtensionProperties) {
  if (recorder.Running()) {
    if (pExtensionProperties != nullptr) {
      recorder.Schedule(
          new CzeDriverGetExtensionProperties(return_value, hDriver, pCount, pExtensionProperties));
    }
  }
}

inline void zesDeviceProcessesGetState_RECWRAP(CRecorder& recorder,
                                               ze_result_t return_value,
                                               zes_device_handle_t hDevice,
                                               uint32_t* pCount,
                                               zes_process_state_t* pProcesses) {
  if (recorder.Running()) {
    if (pProcesses != nullptr) {
      recorder.Schedule(new CzesDeviceProcessesGetState(return_value, hDevice, pCount, pProcesses));
    }
  }
}

inline void zesFrequencyGetAvailableClocks_RECWRAP(CRecorder& recorder,
                                                   ze_result_t return_value,
                                                   zes_freq_handle_t hFrequency,
                                                   uint32_t* pCount,
                                                   double* phFrequency) {
  if (recorder.Running()) {
    if (phFrequency != nullptr) {
      recorder.Schedule(
          new CzesFrequencyGetAvailableClocks(return_value, hFrequency, pCount, phFrequency));
    }
  }
}

inline void zesDiagnosticsGetTests_RECWRAP(CRecorder& recorder,
                                           ze_result_t return_value,
                                           zes_diag_handle_t hDiagnostics,
                                           uint32_t* pCount,
                                           zes_diag_test_t* pTests) {
  if (recorder.Running()) {
    if (pTests != nullptr) {
      recorder.Schedule(new CzesDiagnosticsGetTests(return_value, hDiagnostics, pCount, pTests));
    }
  }
}

inline void zeDeviceGetCacheProperties_RECWRAP(CRecorder& recorder,
                                               ze_result_t return_value,
                                               ze_device_handle_t hDevice,
                                               uint32_t* pCount,
                                               ze_device_cache_properties_t* pCacheProperties) {
  if (recorder.Running()) {
    if (pCacheProperties != nullptr) {
      recorder.Schedule(
          new CzeDeviceGetCacheProperties(return_value, hDevice, pCount, pCacheProperties));
    }
  }
}

inline void zetDebugGetRegisterSetProperties_RECWRAP(
    CRecorder& recorder,
    ze_result_t return_value,
    zet_device_handle_t hDevice,
    uint32_t* pCount,
    zet_debug_regset_properties_t* pRegisterSetProperties) {
  if (recorder.Running()) {
    if (pRegisterSetProperties != nullptr) {
      recorder.Schedule(new CzetDebugGetRegisterSetProperties(return_value, hDevice, pCount,
                                                              pRegisterSetProperties));
    }
  }
}

inline void zeDeviceGetMemoryProperties_RECWRAP(CRecorder& recorder,
                                                ze_result_t return_value,
                                                ze_device_handle_t hDevice,
                                                uint32_t* pCount,
                                                ze_device_memory_properties_t* pMemProperties) {
  if (recorder.Running()) {
    if (pMemProperties != nullptr) {
      recorder.Schedule(
          new CzeDeviceGetMemoryProperties(return_value, hDevice, pCount, pMemProperties));
    }
  }
}

inline void zesDeviceEnumPerformanceFactorDomains_RECWRAP(CRecorder& recorder,
                                                          ze_result_t return_value,
                                                          zes_device_handle_t hDevice,
                                                          uint32_t* pCount,
                                                          zes_perf_handle_t* phPerf) {
  if (recorder.Running()) {
    if (phPerf != nullptr) {
      recorder.Schedule(
          new CzesDeviceEnumPerformanceFactorDomains(return_value, hDevice, pCount, phPerf));
    }
  }
}

inline void zesDeviceEnumRasErrorSets_RECWRAP(CRecorder& recorder,
                                              ze_result_t return_value,
                                              zes_device_handle_t hDevice,
                                              uint32_t* pCount,
                                              zes_ras_handle_t* phRas) {
  if (recorder.Running()) {
    if (phRas != nullptr) {
      recorder.Schedule(new CzesDeviceEnumRasErrorSets(return_value, hDevice, pCount, phRas));
    }
  }
}

inline void zesDeviceEnumLeds_RECWRAP(CRecorder& recorder,
                                      ze_result_t return_value,
                                      zes_device_handle_t hDevice,
                                      uint32_t* pCount,
                                      zes_led_handle_t* phLed) {
  if (recorder.Running()) {
    if (phLed != nullptr) {
      recorder.Schedule(new CzesDeviceEnumLeds(return_value, hDevice, pCount, phLed));
    }
  }
}

inline void zesDeviceEnumEngineGroups_RECWRAP(CRecorder& recorder,
                                              ze_result_t return_value,
                                              zes_device_handle_t hDevice,
                                              uint32_t* pCount,
                                              zes_engine_handle_t* phEngine) {
  if (recorder.Running()) {
    if (phEngine != nullptr) {
      recorder.Schedule(new CzesDeviceEnumEngineGroups(return_value, hDevice, pCount, phEngine));
    }
  }
}

inline void zesDeviceEnumFans_RECWRAP(CRecorder& recorder,
                                      ze_result_t return_value,
                                      zes_device_handle_t hDevice,
                                      uint32_t* pCount,
                                      zes_fan_handle_t* phFan) {
  if (recorder.Running()) {
    if (phFan != nullptr) {
      recorder.Schedule(new CzesDeviceEnumFans(return_value, hDevice, pCount, phFan));
    }
  }
}

inline void zesDeviceEnumSchedulers_RECWRAP(CRecorder& recorder,
                                            ze_result_t return_value,
                                            zes_device_handle_t hDevice,
                                            uint32_t* pCount,
                                            zes_sched_handle_t* phScheduler) {
  if (recorder.Running()) {
    if (phScheduler != nullptr) {
      recorder.Schedule(new CzesDeviceEnumSchedulers(return_value, hDevice, pCount, phScheduler));
    }
  }
}

inline void zeDriverGet_RECWRAP(CRecorder& recorder,
                                ze_result_t return_value,
                                uint32_t* pCount,
                                ze_driver_handle_t* phDrivers) {
  if (recorder.Running()) {
    if (phDrivers != nullptr) {
      recorder.Schedule(new CzeDriverGet(return_value, pCount, phDrivers));
    }
  }
  zeDriverGet_SD(return_value, pCount, phDrivers);
}

inline void zeModuleGetKernelNames_RECWRAP(CRecorder& recorder,
                                           ze_result_t return_value,
                                           ze_module_handle_t hModule,
                                           uint32_t* pCount,
                                           const char** pNames) {
  if (recorder.Running()) {
    if (pNames != nullptr) {
      recorder.Schedule(new CzeModuleGetKernelNames(return_value, hModule, pCount, pNames));
    }
  }
}

inline void zesDeviceEnumMemoryModules_RECWRAP(CRecorder& recorder,
                                               ze_result_t return_value,
                                               zes_device_handle_t hDevice,
                                               uint32_t* pCount,
                                               zes_mem_handle_t* phMemory) {
  if (recorder.Running()) {
    if (phMemory != nullptr) {
      recorder.Schedule(new CzesDeviceEnumMemoryModules(return_value, hDevice, pCount, phMemory));
    }
  }
}

inline void zesDeviceEnumPsus_RECWRAP(CRecorder& recorder,
                                      ze_result_t return_value,
                                      zes_device_handle_t hDevice,
                                      uint32_t* pCount,
                                      zes_psu_handle_t* phPsu) {
  if (recorder.Running()) {
    if (phPsu != nullptr) {
      recorder.Schedule(new CzesDeviceEnumPsus(return_value, hDevice, pCount, phPsu));
    }
  }
}

inline void zeDeviceGetSubDevices_RECWRAP(CRecorder& recorder,
                                          ze_result_t return_value,
                                          ze_device_handle_t hDevice,
                                          uint32_t* pCount,
                                          ze_device_handle_t* phSubdevices) {
  if (recorder.Running()) {
    if (phSubdevices != nullptr) {
      recorder.Schedule(new CzeDeviceGetSubDevices(return_value, hDevice, pCount, phSubdevices));
    }
  }
  zeDeviceGetSubDevices_SD(return_value, hDevice, pCount, phSubdevices);
}

inline void zesDeviceEnumFabricPorts_RECWRAP(CRecorder& recorder,
                                             ze_result_t return_value,
                                             zes_device_handle_t hDevice,
                                             uint32_t* pCount,
                                             zes_fabric_port_handle_t* phPort) {
  if (recorder.Running()) {
    if (phPort != nullptr) {
      recorder.Schedule(new CzesDeviceEnumFabricPorts(return_value, hDevice, pCount, phPort));
    }
  }
}

inline void zesDeviceEnumFrequencyDomains_RECWRAP(CRecorder& recorder,
                                                  ze_result_t return_value,
                                                  zes_device_handle_t hDevice,
                                                  uint32_t* pCount,
                                                  zes_freq_handle_t* phFrequency) {
  if (recorder.Running()) {
    if (phFrequency != nullptr) {
      recorder.Schedule(
          new CzesDeviceEnumFrequencyDomains(return_value, hDevice, pCount, phFrequency));
    }
  }
}

inline void zetMetricGet_RECWRAP(CRecorder& recorder,
                                 ze_result_t return_value,
                                 zet_metric_group_handle_t hMetricGroup,
                                 uint32_t* pCount,
                                 zet_metric_handle_t* phMetrics) {
  if (recorder.Running()) {
    if (phMetrics != nullptr) {
      recorder.Schedule(new CzetMetricGet(return_value, hMetricGroup, pCount, phMetrics));
    }
  }
}

inline void zeDeviceGet_RECWRAP(CRecorder& recorder,
                                ze_result_t return_value,
                                ze_driver_handle_t hDriver,
                                uint32_t* pCount,
                                ze_device_handle_t* phDevices) {
  if (recorder.Running()) {
    if (phDevices != nullptr) {
      recorder.Schedule(new CzeDeviceGet(return_value, hDriver, pCount, phDevices));
    }
  }
  zeDeviceGet_SD(return_value, hDriver, pCount, phDevices);
}

inline void zesDeviceEnumTemperatureSensors_RECWRAP(CRecorder& recorder,
                                                    ze_result_t return_value,
                                                    zes_device_handle_t hDevice,
                                                    uint32_t* pCount,
                                                    zes_temp_handle_t* phTemperature) {
  if (recorder.Running()) {
    if (phTemperature != nullptr) {
      recorder.Schedule(
          new CzesDeviceEnumTemperatureSensors(return_value, hDevice, pCount, phTemperature));
    }
  }
}

inline void zesDeviceEnumStandbyDomains_RECWRAP(CRecorder& recorder,
                                                ze_result_t return_value,
                                                zes_device_handle_t hDevice,
                                                uint32_t* pCount,
                                                zes_standby_handle_t* phStandby) {
  if (recorder.Running()) {
    if (phStandby != nullptr) {
      recorder.Schedule(new CzesDeviceEnumStandbyDomains(return_value, hDevice, pCount, phStandby));
    }
  }
}

inline void zesDeviceEnumFirmwares_RECWRAP(CRecorder& recorder,
                                           ze_result_t return_value,
                                           zes_device_handle_t hDevice,
                                           uint32_t* pCount,
                                           zes_firmware_handle_t* phFirmware) {
  if (recorder.Running()) {
    if (phFirmware != nullptr) {
      recorder.Schedule(new CzesDeviceEnumFirmwares(return_value, hDevice, pCount, phFirmware));
    }
  }
}

inline void zesDeviceEnumDiagnosticTestSuites_RECWRAP(CRecorder& recorder,
                                                      ze_result_t return_value,
                                                      zes_device_handle_t hDevice,
                                                      uint32_t* pCount,
                                                      zes_diag_handle_t* phDiagnostics) {
  if (recorder.Running()) {
    if (phDiagnostics != nullptr) {
      recorder.Schedule(
          new CzesDeviceEnumDiagnosticTestSuites(return_value, hDevice, pCount, phDiagnostics));
    }
  }
}

inline void zesDeviceEnumPowerDomains_RECWRAP(CRecorder& recorder,
                                              ze_result_t return_value,
                                              zes_device_handle_t hDevice,
                                              uint32_t* pCount,
                                              zes_pwr_handle_t* phPower) {
  if (recorder.Running()) {
    if (phPower != nullptr) {
      recorder.Schedule(new CzesDeviceEnumPowerDomains(return_value, hDevice, pCount, phPower));
    }
  }
}

inline void zetMetricGroupGet_RECWRAP(CRecorder& recorder,
                                      ze_result_t return_value,
                                      zet_device_handle_t hDevice,
                                      uint32_t* pCount,
                                      zet_metric_group_handle_t* phMetricGroups) {
  if (recorder.Running()) {
    if (phMetricGroups != nullptr) {
      recorder.Schedule(new CzetMetricGroupGet(return_value, hDevice, pCount, phMetricGroups));
    }
  }
}

inline void zeInit_RECWRAP(CRecorder& recorder, ze_result_t return_value, ze_init_flags_t flags) {
  if (recorder.Running()) {
    recorder.Schedule(new CzeInit(return_value, flags));
  }
  if (Config::Get().recorder.basic.enabled) {
    const auto& l0IFace = gits::CGits::Instance().apis.IfaceCompute();
    if (!l0IFace.MemorySnifferInstall()) {
      Log(WARN) << "Memory Sniffer installation failed";
    }
  }
}

inline void zeGitsIndirectAllocationOffsets_RECWRAP(CRecorder& recorder,
                                                    void* pAlloc,
                                                    uint32_t numOffsets,
                                                    size_t* pOffsets) {
  const auto pairAlloc = GetAllocFromRegion(pAlloc, SD());
  std::vector<size_t> offsets(numOffsets);
  for (uint32_t i = 0U; i < numOffsets; i++) {
    offsets[i] = pairAlloc.second + pOffsets[i];
  }
  if (pairAlloc.first == nullptr) {
    Log(ERR) << "Couldn't correlate pAlloc " << ToStringHelper(pAlloc)
             << " to any device allocation";
    return;
  }
  if (recorder.Running()) {
    recorder.Schedule(
        new CzeGitsIndirectAllocationOffsets(pairAlloc.first, numOffsets, offsets.data()));
  }
  zeGitsIndirectAllocationOffsets_SD(pairAlloc.first, numOffsets, offsets.data());
}

inline void zeCommandListCreate_RECWRAP(CRecorder& recorder,
                                        ze_result_t return_value,
                                        ze_context_handle_t hContext,
                                        ze_device_handle_t hDevice,
                                        const ze_command_list_desc_t* desc,
                                        ze_command_list_handle_t* phCommandList) {
  auto& sd = SD();
  if (sd.nomenclatureCounting) {
    CGits::Instance().CommandListCountUp();
  }
  if (recorder.Running()) {
    if (IsControlledSubmission(desc)) {
      auto& deviceState = sd.Get<CDeviceState>(hDevice, EXCEPTION_MESSAGE);
      if (deviceState.originalQueueGroupProperties.empty()) {
        UpdateOriginalQueueGroupProperties(deviceState, hDevice);
        recorder.Schedule(
            new CGitsL0OriginalQueueFamilyInfo(hDevice, deviceState.originalQueueGroupProperties));
      }
    }
    recorder.Schedule(
        new CzeCommandListCreate(return_value, hContext, hDevice, desc, phCommandList));
  }
  zeCommandListCreate_SD(return_value, hContext, hDevice, desc, phCommandList);
}

inline void zeCommandListCreateImmediate_RECWRAP(CRecorder& recorder,
                                                 ze_result_t return_value,
                                                 ze_context_handle_t hContext,
                                                 ze_device_handle_t hDevice,
                                                 const ze_command_queue_desc_t* altdesc,
                                                 ze_command_list_handle_t* phCommandList) {
  auto& sd = SD();
  if (sd.nomenclatureCounting) {
    CGits::Instance().CommandListCountUp();
    CGits::Instance().CommandQueueExecCountUp();
  }
  if (recorder.Running()) {
    if (IsControlledSubmission(altdesc)) {
      auto& deviceState = sd.Get<CDeviceState>(hDevice, EXCEPTION_MESSAGE);
      if (deviceState.originalQueueGroupProperties.empty()) {
        UpdateOriginalQueueGroupProperties(deviceState, hDevice);
        recorder.Schedule(
            new CGitsL0OriginalQueueFamilyInfo(hDevice, deviceState.originalQueueGroupProperties));
      }
    }
    recorder.Schedule(
        new CzeCommandListCreateImmediate(return_value, hContext, hDevice, altdesc, phCommandList));
  }
  zeCommandListCreateImmediate_SD(return_value, hContext, hDevice, altdesc, phCommandList);
}

inline void zeCommandQueueCreate_RECWRAP(CRecorder& recorder,
                                         ze_result_t return_value,
                                         ze_context_handle_t hContext,
                                         ze_device_handle_t hDevice,
                                         const ze_command_queue_desc_t* desc,
                                         ze_command_queue_handle_t* phCommandQueue) {
  if (recorder.Running()) {
    if (IsControlledSubmission(desc)) {
      auto& deviceState = SD().Get<CDeviceState>(hDevice, EXCEPTION_MESSAGE);
      if (deviceState.originalQueueGroupProperties.empty()) {
        UpdateOriginalQueueGroupProperties(deviceState, hDevice);
        recorder.Schedule(
            new CGitsL0OriginalQueueFamilyInfo(hDevice, deviceState.originalQueueGroupProperties));
      }
    }
    recorder.Schedule(
        new CzeCommandQueueCreate(return_value, hContext, hDevice, desc, phCommandQueue));
  }
  zeCommandQueueCreate_SD(return_value, hContext, hDevice, desc, phCommandQueue);
}

inline void zesInit_RECWRAP(CRecorder& recorder, ze_result_t return_value, zes_init_flags_t flags) {
  if (recorder.Running()) {
    recorder.Schedule(new CzesInit(return_value, flags));
  }
  if (Config::Get().recorder.basic.enabled) {
    const auto& l0IFace = gits::CGits::Instance().apis.IfaceCompute();
    if (!l0IFace.MemorySnifferInstall()) {
      Log(WARN) << "Memory Sniffer installation failed";
    }
  }
}

inline void zeGitsStopRecording_RECWRAP(CRecorder& recorder, ze_gits_recording_info_t properties) {
  const auto& l0IFace = gits::CGits::Instance().apis.IfaceCompute();
  auto& sd = SD();
  if (l0IFace.CfgRec_IsAllMode() || sd.stateRestoreFinished) {
    recorder.Pause();
  }
  if (properties & ZE_GITS_SWITCH_NOMENCLATURE_COUNTING) {
    sd.nomenclatureCounting = false;
  }
}

inline void zeGitsStartRecording_RECWRAP(CRecorder& recorder, ze_gits_recording_info_t properties) {
  const auto& l0IFace = gits::CGits::Instance().apis.IfaceCompute();
  auto& sd = SD();
  if (l0IFace.CfgRec_IsAllMode() || sd.stateRestoreFinished) {
    recorder.Continue();
  }
  if (properties & ZE_GITS_SWITCH_NOMENCLATURE_COUNTING) {
    sd.nomenclatureCounting = true;
  }
}

inline void zeModuleCreate_RECWRAP(CRecorder& recorder,
                                   ze_result_t return_value,
                                   ze_context_handle_t hContext,
                                   ze_device_handle_t hDevice,
                                   const ze_module_desc_t* desc,
                                   ze_module_handle_t* phModule,
                                   ze_module_build_log_handle_t* phBuildLog) {
  CFunction* token = nullptr;
  if (recorder.Running()) {
    token = new CzeModuleCreate_V1(return_value, hContext, hDevice, desc, phModule, phBuildLog);
    recorder.Schedule(token);
  }
  zeModuleCreate_SD(return_value, hContext, hDevice, desc, phModule, phBuildLog);
  if (token != nullptr) {
    const auto moduleFileName =
        token->Argument<Cze_module_desc_t_V1::CSArray>(2U).Vector()[0]->GetProgramSourceName();
    SD().Get<CModuleState>(*phModule, EXCEPTION_MESSAGE).moduleFileName = moduleFileName;
  }
}

} // namespace l0
} // namespace gits

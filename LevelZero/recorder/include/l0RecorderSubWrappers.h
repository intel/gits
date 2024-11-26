// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
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
#include "l0Log.h"
#include "l0StateDynamic.h"
#include "l0StateTracking.h"
#include "l0HelperFunctions.h"
#include "l0Drivers.h"
#include "l0Structs.h"
#include "l0Tools.h"
#include "recorder.h"
#include "l0StateRestore.h"
#include <cstdint>
#include <string>
#include <vector>
#include <future>

namespace gits {
namespace l0 {
void zeCommandListCreateImmediate_RECWRAP(CRecorder& recorder,
                                          ze_result_t return_value,
                                          ze_context_handle_t hContext,
                                          ze_device_handle_t hDevice,
                                          const ze_command_queue_desc_t* altdesc,
                                          ze_command_list_handle_t* phCommandList);
namespace {
bool CheckWhetherUpdateUSM(const void* ptr) {
  bool update = false;
  auto& sd = SD();
  const auto allocInfo = GetAllocFromRegion(const_cast<void*>(ptr), sd);
  if (allocInfo.first != nullptr) {
    auto& allocState = sd.Get<CAllocState>(allocInfo.first, EXCEPTION_MESSAGE);
    update = allocState.sniffedRegionHandle != nullptr &&
             !(**allocState.sniffedRegionHandle).GetTouchedPages().empty();
    const auto& cfg = Config::Get();
    if (update && IsBruteForceScanForIndirectPointersEnabled(cfg)) {
      allocState.modified = true;
    }
  }
  return update;
}

std::set<const void*> GetPointersToUpdate(CStateDynamic& sd, ze_kernel_handle_t hKernel) {
  std::set<const void*> ptrsToUpdate;
  const auto& kernelState = sd.Get<CKernelState>(hKernel, EXCEPTION_MESSAGE);
  for (const auto& arg : kernelState.currentKernelInfo->GetArguments()) {
    if (arg.second.type == KernelArgType::buffer && CheckWhetherUpdateUSM(arg.second.argValue)) {
      ptrsToUpdate.insert(arg.second.argValue);
    }
  }
  for (const auto& ptr : sd.Map<CAllocState>()) {
    const auto kernelContext =
        sd.Get<CModuleState>(kernelState.hModule, EXCEPTION_MESSAGE).hContext;
    const auto isResidencySet =
        ptr.second->residencyInfo && ptr.second->residencyInfo->hContext == kernelContext;
    const auto isIndirectTypeEnabled = static_cast<unsigned>(ptr.second->memType) &
                                       kernelState.currentKernelInfo->indirectUsmTypes;
    if ((isResidencySet || isIndirectTypeEnabled) && CheckWhetherUpdateUSM(ptr.first)) {
      ptrsToUpdate.insert(ptr.first);
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
    zeCommandListCreateImmediate_RECWRAP(*recorder, err, context, device, &desc, &commandList);
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
        if (allocState.second->allocType == AllocStateType::virtual_pointer) {
          for (const auto& memMap : allocState.second->memMaps) {
            const auto virtualPtrRegion = GetOffsetPointer(allocState.first, memMap.first);
            stateRestoreBuffersSnapshot.emplace_back(std::make_unique<CKernelArgument>(
                memMap.second->virtualMemorySizeFromOffset, virtualPtrRegion));
            driver.inject.zeCommandListAppendMemoryCopy(
                hCommandList, stateRestoreBuffersSnapshot.back()->buffer.data(), virtualPtrRegion,
                stateRestoreBuffersSnapshot.back()->buffer.size(), nullptr, numEvents, waitList);
          }
        } else {
          stateRestoreBuffersSnapshot.emplace_back(
              std::make_unique<CKernelArgument>(allocState.second->size, allocState.first));
          driver.inject.zeCommandListAppendMemoryCopy(
              hCommandList, stateRestoreBuffersSnapshot.back()->buffer.data(), allocState.first,
              allocState.second->size, nullptr, numEvents, waitList);
        }
      }
    }
  }
  for (const auto& arg : kernelState.currentKernelInfo->GetArguments()) {
    if (arg.second.type == KernelArgType::buffer) {
      const auto allocInfo = GetAllocFromRegion(const_cast<void*>(arg.second.argValue), sd);
      if (restoredPtrs.count(allocInfo.first) == 0) {
        void* ptr = GetOffsetPointer(allocInfo.first, allocInfo.second);
        const auto& allocState = sd.Get<CAllocState>(allocInfo.first, EXCEPTION_MESSAGE);
        if (allocState.allocType == AllocStateType::virtual_pointer) {
          for (const auto& memMap : allocState.memMaps) {
            const auto virtualPtrRegion = GetOffsetPointer(allocInfo.first, memMap.first);
            stateRestoreBuffersSnapshot.emplace_back(std::make_unique<CKernelArgument>(
                memMap.second->virtualMemorySizeFromOffset, virtualPtrRegion));
            driver.inject.zeCommandListAppendMemoryCopy(
                hCommandList, stateRestoreBuffersSnapshot.back()->buffer.data(), virtualPtrRegion,
                stateRestoreBuffersSnapshot.back()->buffer.size(), nullptr, numEvents, waitList);
          }
        } else {
          stateRestoreBuffersSnapshot.emplace_back(
              std::make_unique<CKernelArgument>(allocState.size - allocInfo.second, ptr));
          driver.inject.zeCommandListAppendMemoryCopy(
              hCommandList, stateRestoreBuffersSnapshot.back()->buffer.data(), ptr,
              stateRestoreBuffersSnapshot.back()->buffer.size(), nullptr, numEvents, waitList);
        }
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
void SaveAllBuffersForStateRestore(CStateDynamic& sd,
                                   const CDriver& driver,
                                   const ze_kernel_handle_t& hKernel,
                                   const ze_command_list_handle_t& hCommandList,
                                   const uint32_t numEvents,
                                   ze_event_handle_t* waitList) {
  auto& kernelState = sd.Get<CKernelState>(hKernel, EXCEPTION_MESSAGE);
  auto& stateRestoreBuffersSnapshot = kernelState.currentKernelInfo->stateRestoreBuffers;
  for (const auto& allocState : sd.Map<CAllocState>()) {
    if (!allocState.second->savedForStateRestore) {
      stateRestoreBuffersSnapshot.emplace_back(
          std::make_unique<CKernelArgument>(allocState.second->size, allocState.first));
      driver.inject.zeCommandListAppendMemoryCopy(
          hCommandList, stateRestoreBuffersSnapshot.back()->buffer.data(), allocState.first,
          allocState.second->size, nullptr, numEvents, waitList);
      allocState.second->savedForStateRestore = true;
    }
  }
  for (const auto& imageState : sd.Map<CImageState>()) {
    if (!imageState.second->savedForStateRestore) {
      stateRestoreBuffersSnapshot.emplace_back(std::make_unique<CKernelArgument>(
          CalculateImageSize(imageState.second->desc), imageState.first));
      driver.inject.zeCommandListAppendImageCopyToMemory(
          hCommandList, stateRestoreBuffersSnapshot.back()->buffer.data(), imageState.first,
          nullptr, nullptr, numEvents, waitList);
      imageState.second->savedForStateRestore = true;
    }
  }
  driver.inject.zeCommandListAppendBarrier(hCommandList, nullptr, numEvents, waitList);
}
std::vector<ze_command_list_handle_t> GetCommandListsToSubcapture(
    bool isRecorderRunning,
    const ApisIface::ApiCompute& l0IFace,
    CStateDynamic& sd,
    uint32_t numCommandLists,
    ze_command_list_handle_t* phCommandLists) {
  std::vector<ze_command_list_handle_t> cmdLists;
  bool record = isRecorderRunning;
  for (auto i = 0u; i < numCommandLists; i++) {
    const auto& cmdListState = sd.Get<CCommandListState>(phCommandLists[i], EXCEPTION_MESSAGE);
    if (cmdListState.cmdListNumber == l0IFace.CfgRec_StartCommandList()) {
      record = true;
    }
    if (record) {
      cmdLists.push_back(phCommandLists[i]);
      if (cmdListState.cmdListNumber == l0IFace.CfgRec_StopCommandList()) {
        return cmdLists;
      }
    }
  }
  return cmdLists;
}

bool CheckResidencyInContext(CStateDynamic& sd, const ze_context_handle_t& hContext) {
  for (const auto& allocPair : sd.Map<CAllocState>()) {
    if (allocPair.second->residencyInfo && allocPair.second->residencyInfo->hContext == hContext) {
      return true;
    }
  }
  return false;
}

bool ExistsAsKernelArgument(
    void* ptr, const std::vector<std::shared_ptr<CKernelExecutionInfo>>& executedKernels) {
  const auto& sd = SD();
  for (const auto& kernel : executedKernels) {
    for (const auto& arg : kernel->GetArguments()) {
      if (arg.second.type == KernelArgType::buffer) {
        const auto kernelArgValuePtr =
            GetAllocFromRegion(const_cast<void*>(arg.second.argValue), sd).first;
        if (kernelArgValuePtr == ptr) {
          return true;
        }
        const auto kernelArgOriginalValuePtr =
            GetAllocFromRegion(const_cast<void*>(arg.second.originalValue), sd).first;
        if (kernelArgOriginalValuePtr == ptr) {
          return true;
        }
      }
    }
  }
  return false;
}

std::vector<size_t> AsyncBruteForce(const CStateDynamic* sd,
                                    const void* ptr,
                                    const CAllocState* allocState,
                                    const CDriver* driver,
                                    const ze_command_list_handle_t* immediateSyncCommandListHandle,
                                    uintptr_t smallestPointerValue,
                                    uintptr_t highestPointerValue) {
  std::vector<size_t> offsets;
  uintptr_t potentialPointer = 0U;
  size_t i = 0U;
  std::vector<char> buffer;
  const char* pointerToData = nullptr;
  if (allocState->memType == UnifiedMemoryType::device) {
    buffer.resize(allocState->size);
    const auto ret = driver->inject.zeCommandListAppendMemoryCopy(
        *immediateSyncCommandListHandle, buffer.data(), ptr, buffer.size(), nullptr, 0, nullptr);
    if (ret != ZE_RESULT_SUCCESS) {
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
    pointerToData = buffer.data();
  } else {
    pointerToData = reinterpret_cast<const char*>(ptr);
  }
  while (allocState->size > i && allocState->size - i + 1 > sizeof(void*)) {
    std::memcpy(&potentialPointer, pointerToData + i, sizeof(uintptr_t));
    if (potentialPointer < smallestPointerValue || potentialPointer > highestPointerValue) {
      i++;
      continue;
    }
    const auto allocInfo = GetAllocFromRegion(reinterpret_cast<void*>(potentialPointer), *sd);
    if (allocInfo.first != nullptr) {
      offsets.push_back(i);
      Log(TRACEV) << "Scanning pointer: " << ToStringHelper(pointerToData)
                  << " -> Found pointer on offset " << std::to_string(i)
                  << " pointer: " << ToStringHelper(allocInfo.first)
                  << ", memory view: " << ToStringHelper(potentialPointer);
      i += sizeof(void*) - 1;
    }
    i++;
  }
  return offsets;
}

void BruteForceScanForIndirectAccess(
    CRecorder& recorder,
    CStateDynamic& sd,
    CDriver& driver,
    unsigned int indirectTypes,
    ze_context_handle_t hContext,
    const std::vector<std::shared_ptr<CKernelExecutionInfo>>& executedKernels) {
  if (indirectTypes == 0U && !CheckResidencyInContext(sd, hContext)) {
    return;
  }
  const auto& cfg = Config::Get();
  uintptr_t smallestPointerValue = UINTPTR_MAX;
  uintptr_t highestPointerValue = 0;
  for (const auto& allocState : sd.Map<CAllocState>()) {
    const auto pointerValueMin = reinterpret_cast<uintptr_t>(allocState.first);
    const auto pointerValueMax =
        reinterpret_cast<uintptr_t>(GetOffsetPointer(allocState.first, allocState.second->size));
    if (pointerValueMin < smallestPointerValue) {
      smallestPointerValue = pointerValueMin;
    }
    if (pointerValueMax > highestPointerValue) {
      highestPointerValue = pointerValueMax;
    }
  }
  std::map<void*, std::future<std::vector<size_t>>> futures;
  for (const auto& allocState : sd.Map<CAllocState>()) {
    if (!IsMemoryTypeIncluded(cfg.levelzero.recorder.bruteForceScanForIndirectPointers.memoryType,
                              allocState.second->memType)) {
      continue;
    }
    const auto modifiedAllocation =
        (allocState.second->modified ||
         (allocState.second->sniffedRegionHandle != nullptr &&
          !(**allocState.second->sniffedRegionHandle).GetTouchedPages().empty()));
    if ((static_cast<unsigned>(allocState.second->memType) & indirectTypes && modifiedAllocation) ||
        allocState.second->residencyInfo ||
        ExistsAsKernelArgument(allocState.first, executedKernels)) {
      if (BruteForceScanIterations(cfg)) {
        if (allocState.second->scannedTimes >= BruteForceScanIterations(cfg)) {
          continue;
        }
        allocState.second->scannedTimes++;
      }
      auto immediateCmdList = GetCommandListImmediate(sd, driver, allocState.second->hContext);
      futures[allocState.first] = std::async(
          std::launch::async, AsyncBruteForce, &sd, allocState.first, allocState.second.get(),
          &driver, &immediateCmdList, smallestPointerValue, highestPointerValue);
    }
  }
  for (auto& futureInfo : futures) {
    std::vector<size_t> offsets = futureInfo.second.get();
    auto* ptr = futureInfo.first;
    if (!offsets.empty()) {
      drv.zeGitsIndirectAllocationOffsets(ptr, offsets.size(), offsets.data());
      recorder.Schedule(new CzeGitsIndirectAllocationOffsets(ptr, offsets.size(), offsets.data()));
      zeGitsIndirectAllocationOffsets_SD(ptr, offsets.size(), offsets.data());
    }
  }
}

void SchedulePotentialMemoryUpdate(CStateDynamic& sd,
                                   CRecorder& recorder,
                                   const void* usmPtr,
                                   ze_command_list_handle_t hCommandList) {
  if (CheckWhetherUpdateUSM(usmPtr)) {
    const auto allocInfo = GetAllocFromRegion(const_cast<void*>(usmPtr), sd);
    auto& allocState = sd.Get<CAllocState>(allocInfo.first, EXCEPTION_MESSAGE);
    auto& commandListState = sd.Get<CCommandListState>(hCommandList, EXCEPTION_MESSAGE);
    if (commandListState.isImmediate || UnifiedMemoryType::host == allocState.memType) {
      recorder.Schedule(new CGitsL0MemoryUpdate(usmPtr));
    } else {
      commandListState.ptrsToUpdate.insert(usmPtr);
    }
  }
}

void PrepareCommandListsExecution(CStateDynamic& sd,
                                  CRecorder& recorder,
                                  ze_context_handle_t& hContext,
                                  uint32_t& numCommandLists,
                                  ze_command_list_handle_t* phCommandLists) {
  const auto& cfg = Config::Get();
  if (recorder.Running()) {
    if (IsBruteForceScanForIndirectPointersEnabled(cfg)) {
      unsigned int indirectTypes = 0U;
      std::vector<std::shared_ptr<CKernelExecutionInfo>> executedKernels;
      for (auto i = 0U; i < numCommandLists; i++) {
        const auto& commandListState =
            sd.Get<CCommandListState>(phCommandLists[i], EXCEPTION_MESSAGE);
        for (const auto& hKernelInfo : commandListState.appendedKernels) {
          indirectTypes |= hKernelInfo->indirectUsmTypes;
        }
        for (auto& kernel : commandListState.appendedKernels) {
          executedKernels.push_back(kernel);
        }
      }
      BruteForceScanForIndirectAccess(recorder, sd, drv, indirectTypes, hContext, executedKernels);
    }
    for (auto i = 0U; i < numCommandLists; i++) {
      auto& commandListState = sd.Get<CCommandListState>(phCommandLists[i], EXCEPTION_MESSAGE);
      auto& ptrsToUpdate = commandListState.ptrsToUpdate;
      for (auto& ptr : ptrsToUpdate) {
        recorder.Schedule(new CGitsL0MemoryUpdate(ptr));
      }
      ptrsToUpdate.clear();
      for (const auto& kernelInfo : commandListState.appendedKernels) {
        for (auto ptr : GetPointersToUpdate(sd, kernelInfo->handle)) {
          recorder.Schedule(new CGitsL0MemoryUpdate(ptr));
        }
      }
    }
  }
}
} // namespace

inline void zeCommandListAppendLaunchKernel_RECWRAP_PRE(CRecorder& recorder,
                                                        [[maybe_unused]] ze_result_t return_value,
                                                        ze_command_list_handle_t hCommandList,
                                                        ze_kernel_handle_t hKernel,
                                                        const ze_group_count_t* pLaunchFuncArgs,
                                                        ze_event_handle_t hSignalEvent,
                                                        uint32_t numWaitEvents,
                                                        ze_event_handle_t* phWaitEvents) {
  auto& sd = SD();
  if (sd.nomenclatureCounting) {
    gits::CGits::Instance().KernelCountUp();
  }
  const auto kernelCount = gits::CGits::Instance().CurrentKernelCount();
  const auto& l0IFace = gits::CGits::Instance().apis.IfaceCompute();
  auto& cmdListState = sd.Get<CCommandListState>(hCommandList, EXCEPTION_MESSAGE);
  if (!sd.stateRestoreFinished && l0IFace.CfgRec_IsSubcapture()) {
    if (l0IFace.CfgRec_IsSingleKernelMode() && kernelCount == l0IFace.CfgRec_StartKernel()) {
      if (cmdListState.isImmediate) {
        for (auto i = 0U; i < numWaitEvents; i++) {
          drv.inject.zeEventHostSynchronize(phWaitEvents[i], UINT64_MAX);
        }
        CommandListKernelInit(sd, hCommandList, hKernel, pLaunchFuncArgs, hSignalEvent);
        recorder.Start();
        recorder.Stop();
        recorder.MarkForDeletion();
      } else {
        SaveKernelArgumentsForStateRestore(sd, drv, hKernel, hCommandList, numWaitEvents,
                                           phWaitEvents);
      }
    } else {
      if (kernelCount == l0IFace.CfgRec_StartKernel() && cmdListState.isImmediate) {
        auto& kernelState = sd.Get<CKernelState>(hKernel, EXCEPTION_MESSAGE);
        if (pLaunchFuncArgs != nullptr) {
          kernelState.currentKernelInfo->launchFuncArgs = *pLaunchFuncArgs;
        }
        recorder.Start();
      } else if (kernelCount >= l0IFace.CfgRec_StartKernel() &&
                 kernelCount <= l0IFace.CfgRec_StopKernel()) {
        SaveAllBuffersForStateRestore(sd, drv, hKernel, hCommandList, numWaitEvents, phWaitEvents);
      }
    }
  }
  if (recorder.Running() && cmdListState.isImmediate) {
    for (const auto ptr : GetPointersToUpdate(sd, hKernel)) {
      recorder.Schedule(new CGitsL0MemoryUpdate(ptr));
    }
    if (IsBruteForceScanForIndirectPointersEnabled(Config::Get()) && cmdListState.isImmediate) {
      const auto& kernelState = sd.Get<CKernelState>(hKernel, EXCEPTION_MESSAGE);
      const auto& commandListState = sd.Get<CCommandListState>(hCommandList, EXCEPTION_MESSAGE);
      const auto& indirectTypes = kernelState.currentKernelInfo->indirectUsmTypes;
      BruteForceScanForIndirectAccess(recorder, sd, drv, indirectTypes, commandListState.hContext,
                                      commandListState.appendedKernels);
    }
  }
  const auto& cfg = Config::Get();
  if (CaptureKernels(cfg) && IsDumpInputMode(cfg)) {
    AppendLaunchKernel(hCommandList, hKernel, pLaunchFuncArgs, hSignalEvent, true);
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
    const auto& gitsInstance = gits::CGits::Instance();
    recorder.Schedule(new CzeCommandListAppendLaunchKernel(return_value, hCommandList, hKernel,
                                                           pLaunchFuncArgs, hSignalEvent,
                                                           numWaitEvents, phWaitEvents));
    if (IsCommandListImmediate(hCommandList, SD()) &&
        gitsInstance.apis.IfaceCompute().CfgRec_StopKernel() == gitsInstance.CurrentKernelCount()) {
      recorder.Schedule(
          new CzeCommandListHostSynchronize(ZE_RESULT_SUCCESS, hCommandList, UINT64_MAX));
      recorder.Stop();
      recorder.MarkForDeletion();
    }
  }
  zeCommandListAppendLaunchKernel_SD(return_value, hCommandList, hKernel, pLaunchFuncArgs,
                                     hSignalEvent, numWaitEvents, phWaitEvents);
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
    SchedulePotentialMemoryUpdate(SD(), recorder, srcptr, hCommandList);
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
    SchedulePotentialMemoryUpdate(SD(), recorder, srcptr, hCommandList);
    recorder.Schedule(new CzeCommandListAppendMemoryCopyRegion(
        return_value, hCommandList, dstptr, dstRegion, dstPitch, dstSlicePitch, srcptr, srcRegion,
        srcPitch, srcSlicePitch, hSignalEvent, numWaitEvents, phWaitEvents));
  }
  zeCommandListAppendMemoryCopyRegion_SD(return_value, hCommandList, dstptr, dstRegion, dstPitch,
                                         dstSlicePitch, srcptr, srcRegion, srcPitch, srcSlicePitch,
                                         hSignalEvent, numWaitEvents, phWaitEvents);
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
    const auto& gitsInstance = gits::CGits::Instance();
    recorder.Schedule(new CzeCommandListAppendLaunchCooperativeKernel(
        return_value, hCommandList, hKernel, pLaunchFuncArgs, hSignalEvent, numWaitEvents,
        phWaitEvents));
    if (IsCommandListImmediate(hCommandList, SD()) &&
        gitsInstance.apis.IfaceCompute().CfgRec_StopKernel() == gitsInstance.CurrentKernelCount()) {
      recorder.Schedule(
          new CzeCommandListHostSynchronize(ZE_RESULT_SUCCESS, hCommandList, UINT64_MAX));
      recorder.Stop();
      recorder.MarkForDeletion();
    }
  }
  zeCommandListAppendLaunchCooperativeKernel_SD(return_value, hCommandList, hKernel,
                                                pLaunchFuncArgs, hSignalEvent, numWaitEvents,
                                                phWaitEvents);
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
    const auto& gitsInstance = gits::CGits::Instance();
    recorder.Schedule(new CzeCommandListAppendLaunchKernelIndirect(
        return_value, hCommandList, hKernel, pLaunchArgumentsBuffer, hSignalEvent, numWaitEvents,
        phWaitEvents));
    if (IsCommandListImmediate(hCommandList, SD()) &&
        gitsInstance.apis.IfaceCompute().CfgRec_StopKernel() == gitsInstance.CurrentKernelCount()) {
      recorder.Schedule(
          new CzeCommandListHostSynchronize(ZE_RESULT_SUCCESS, hCommandList, UINT64_MAX));
      recorder.Stop();
      recorder.MarkForDeletion();
    }
  }
  zeCommandListAppendLaunchKernelIndirect_SD(return_value, hCommandList, hKernel,
                                             pLaunchArgumentsBuffer, hSignalEvent, numWaitEvents,
                                             phWaitEvents);
}

inline void zeCommandListAppendLaunchMultipleKernelsIndirect_RECWRAP_PRE(
    CRecorder& recorder,
    [[maybe_unused]] ze_result_t return_value,
    [[maybe_unused]] ze_command_list_handle_t hCommandList,
    uint32_t numKernels,
    ze_kernel_handle_t* phKernels,
    [[maybe_unused]] const uint32_t* pCountBuffer,
    [[maybe_unused]] const ze_group_count_t* pLaunchArgumentsBuffer,
    [[maybe_unused]] ze_event_handle_t hSignalEvent,
    [[maybe_unused]] uint32_t numWaitEvents,
    [[maybe_unused]] ze_event_handle_t* phWaitEvents) {
  auto& sd = SD();
  const auto& l0IFace = gits::CGits::Instance().apis.IfaceCompute();
  if (sd.nomenclatureCounting) {
    for (auto i = 0U; i < numKernels; i++) {
      gits::CGits::Instance().KernelCountUp();
      const auto kernelCount = gits::CGits::Instance().CurrentKernelCount();
      if (l0IFace.CfgRec_IsSubcapture() && l0IFace.CfgRec_StartKernel() == kernelCount) {
        throw ENotImplemented(
            "Subcapturing from zeCommandListAppendLaunchMultipleKernelsIndirect is not supported");
      }
    }
  }
  if (recorder.Running() && IsCommandListImmediate(hCommandList, sd)) {
    for (auto i = 0u; i < numKernels; i++) {
      for (const auto ptr : GetPointersToUpdate(sd, phKernels[i])) {
        recorder.Schedule(new CGitsL0MemoryUpdate(ptr));
      }
    }
  }
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
    recorder.Schedule(new CzeCommandListAppendLaunchMultipleKernelsIndirect(
        return_value, hCommandList, numKernels, phKernels, pCountBuffer, pLaunchArgumentsBuffer,
        hSignalEvent, numWaitEvents, phWaitEvents));
  }
  zeCommandListAppendLaunchMultipleKernelsIndirect_SD(
      return_value, hCommandList, numKernels, phKernels, pCountBuffer, pLaunchArgumentsBuffer,
      hSignalEvent, numWaitEvents, phWaitEvents);
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
    SchedulePotentialMemoryUpdate(SD(), recorder, srcptr, hCommandList);
    recorder.Schedule(new CzeCommandListAppendImageCopyFromMemory(
        return_value, hCommandList, hDstImage, srcptr, pDstRegion, hSignalEvent, numWaitEvents,
        phWaitEvents));
  }
  zeCommandListAppendImageCopyFromMemory_SD(return_value, hCommandList, hDstImage, srcptr,
                                            pDstRegion, hSignalEvent, numWaitEvents, phWaitEvents);
}

inline void zeCommandListAppendMemoryFill_RECWRAP_PRE(
    CRecorder& recorder,
    [[maybe_unused]] ze_result_t return_value,
    ze_command_list_handle_t hCommandList,
    void* ptr,
    [[maybe_unused]] const void* pattern,
    [[maybe_unused]] size_t pattern_size,
    [[maybe_unused]] size_t size,
    [[maybe_unused]] ze_event_handle_t hSignalEvent,
    [[maybe_unused]] uint32_t numWaitEvents,
    [[maybe_unused]] ze_event_handle_t* phWaitEvents) {
  if (recorder.Running()) {
    SchedulePotentialMemoryUpdate(SD(), recorder, ptr, hCommandList);
  }
}

inline void zeCommandQueueExecuteCommandLists_RECWRAP_PRE(
    CRecorder& recorder,
    [[maybe_unused]] ze_result_t return_value,
    ze_command_queue_handle_t hCommandQueue,
    uint32_t numCommandLists,
    ze_command_list_handle_t* phCommandLists,
    [[maybe_unused]] ze_fence_handle_t hFence) {
  auto& sd = SD();
  if (sd.nomenclatureCounting) {
    gits::CGits::Instance().CommandQueueExecCountUp();
  }
  auto& commandQueueState = sd.Get<CCommandQueueState>(hCommandQueue, EXCEPTION_MESSAGE);
  commandQueueState.cmdQueueNumber = gits::CGits::Instance().CurrentCommandQueueExecCount();
  PrepareCommandListsExecution(sd, recorder, commandQueueState.hContext, numCommandLists,
                               phCommandLists);
}

inline void zeCommandQueueExecuteCommandLists_RECWRAP(CRecorder& recorder,
                                                      ze_result_t return_value,
                                                      ze_command_queue_handle_t hCommandQueue,
                                                      uint32_t numCommandLists,
                                                      ze_command_list_handle_t* phCommandLists,
                                                      ze_fence_handle_t hFence) {
  const auto& l0IFace = gits::CGits::Instance().apis.IfaceCompute();
  auto& sd = SD();
  const auto subcaptureMode = l0IFace.CfgRec_IsSubcapture();
  if (!recorder.Running() && subcaptureMode && l0IFace.CfgRec_IsStartQueueSubmit()) {
    auto cmdLists = GetCommandListsToSubcapture(recorder.Running(), l0IFace, sd, numCommandLists,
                                                phCommandLists);
    if (!cmdLists.empty()) {
      recorder.Start();
      recorder.Schedule(new CzeCommandQueueExecuteCommandLists(
          return_value, hCommandQueue, static_cast<uint32_t>(cmdLists.size()), cmdLists.data(),
          hFence));
      recorder.Schedule(
          new CzeCommandQueueSynchronize(ZE_RESULT_SUCCESS, hCommandQueue, UINT64_MAX));
    } else {
      Log(ERR) << "Incorrect config LevelZero.Capture.Kernel.Range. The command list "
               << l0IFace.CfgRec_StartCommandList()
               << " doesn't exist inside command queue submission "
               << l0IFace.CfgRec_StartCommandQueueSubmit();
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
    if (l0IFace.CfgRec_IsStopQueueSubmit()) {
      recorder.Stop();
      recorder.MarkForDeletion();
    }
  } else if (recorder.Running()) {
    if (subcaptureMode && l0IFace.CfgRec_IsStopQueueSubmit()) {
      auto cmdLists = GetCommandListsToSubcapture(recorder.Running(), l0IFace, sd, numCommandLists,
                                                  phCommandLists);
      if (!cmdLists.empty()) {
        recorder.Schedule(new CzeCommandQueueExecuteCommandLists(
            return_value, hCommandQueue, static_cast<uint32_t>(cmdLists.size()), cmdLists.data(),
            hFence));
        recorder.Schedule(
            new CzeCommandQueueSynchronize(ZE_RESULT_SUCCESS, hCommandQueue, UINT64_MAX));
      } else {
        Log(ERR) << "Incorrect config LevelZero.Capture.Kernel.Range. The command list "
                 << l0IFace.CfgRec_StopCommandList()
                 << " doesn't exist inside command queue submission "
                 << l0IFace.CfgRec_StopCommandQueueSubmit();
        throw EOperationFailed(EXCEPTION_MESSAGE);
      }
      recorder.Stop();
      recorder.MarkForDeletion();
    } else {
      recorder.Schedule(new CzeCommandQueueExecuteCommandLists(
          return_value, hCommandQueue, numCommandLists, phCommandLists, hFence));
    }
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
    SchedulePotentialMemoryUpdate(SD(), recorder, srcptr, hCommandList);
    recorder.Schedule(new CzeCommandListAppendMemoryCopyFromContext(
        return_value, hCommandList, dstptr, hContextSrc, srcptr, size, hSignalEvent, numWaitEvents,
        phWaitEvents));
  }
  zeCommandListAppendMemoryCopyFromContext_SD(return_value, hCommandList, dstptr, hContextSrc,
                                              srcptr, size, hSignalEvent, numWaitEvents,
                                              phWaitEvents);
}

inline void zeMemAllocHost_RECWRAP(CRecorder& recorder,
                                   ze_result_t return_value,
                                   ze_context_handle_t hContext,
                                   const ze_host_mem_alloc_desc_t* host_desc,
                                   size_t size,
                                   size_t alignment,
                                   void** pptr) {
  const auto& cfg = Config::Get();
  if (recorder.Running()) {
    recorder.Schedule(
        new CzeMemAllocHost(return_value, hContext, host_desc, size, alignment, pptr));
  }
  zeMemAllocHost_SD(return_value, hContext, host_desc, size, alignment, pptr);
  if (recorder.Running() && return_value == ZE_RESULT_SUCCESS && CheckCfgZeroInitialization(cfg)) {
    const auto commandList = GetCommandListImmediateRec(SD(), drv, hContext, &recorder);
    if (commandList != nullptr) {
      ZeroInitializeUsm(drv, commandList, pptr, size, UnifiedMemoryType::host);
      recorder.Schedule(new CGitsL0MemoryRestore(*pptr, size));
    }
  }
  const auto& l0IFace = gits::CGits::Instance().apis.IfaceCompute();
  if (Config::Get().common.recorder.enabled) {
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
  if (cfg.common.recorder.enabled) {
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
  if (Config::Get().common.recorder.enabled) {
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
        recorder.Schedule(new CzeGitsOriginalQueueFamilyInfo(
            ZE_RESULT_SUCCESS, hDevice, deviceState.originalQueueGroupProperties.size(),
            deviceState.originalQueueGroupProperties.data()));
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
        recorder.Schedule(new CzeGitsOriginalQueueFamilyInfo(
            ZE_RESULT_SUCCESS, hDevice, deviceState.originalQueueGroupProperties.size(),
            deviceState.originalQueueGroupProperties.data()));
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
        recorder.Schedule(new CzeGitsOriginalQueueFamilyInfo(
            ZE_RESULT_SUCCESS, hDevice, deviceState.originalQueueGroupProperties.size(),
            deviceState.originalQueueGroupProperties.data()));
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
  if (Config::Get().common.recorder.enabled) {
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
  if (token != nullptr && return_value == ZE_RESULT_SUCCESS) {
    const auto moduleFileName =
        token->Argument<Cze_module_desc_t_V1::CSArray>(2U).Vector()[0]->GetProgramSourceName();
    SD().Get<CModuleState>(*phModule, EXCEPTION_MESSAGE).moduleFileName = std::move(moduleFileName);
  }
}

inline void zeCommandListImmediateAppendCommandListsExp_RECWRAP_PRE(
    CRecorder& recorder,
    [[maybe_unused]] ze_result_t return_value,
    ze_command_list_handle_t hCommandListImmediate,
    uint32_t numCommandLists,
    ze_command_list_handle_t* phCommandLists,
    [[maybe_unused]] ze_event_handle_t hSignalEvent,
    [[maybe_unused]] uint32_t numWaitEvents,
    [[maybe_unused]] ze_event_handle_t* phWaitEvents) {
  auto& sd = SD();
  auto& commandListImmediateState =
      sd.Get<CCommandListState>(hCommandListImmediate, EXCEPTION_MESSAGE);
  if (sd.nomenclatureCounting) {
    gits::CGits::Instance().CommandQueueExecCountUp();
  }
  PrepareCommandListsExecution(sd, recorder, commandListImmediateState.hContext, numCommandLists,
                               phCommandLists);
}

inline void zeCommandListImmediateAppendCommandListsExp_RECWRAP(
    CRecorder& recorder,
    ze_result_t return_value,
    ze_command_list_handle_t hCommandListImmediate,
    uint32_t numCommandLists,
    ze_command_list_handle_t* phCommandLists,
    ze_event_handle_t hSignalEvent,
    uint32_t numWaitEvents,
    ze_event_handle_t* phWaitEvents) {
  const auto& l0IFace = gits::CGits::Instance().apis.IfaceCompute();
  auto& sd = SD();
  const auto subcaptureMode = l0IFace.CfgRec_IsSubcapture();
  if (!recorder.Running() && subcaptureMode && l0IFace.CfgRec_IsStartQueueSubmit()) {
    auto cmdLists = GetCommandListsToSubcapture(recorder.Running(), l0IFace, sd, numCommandLists,
                                                phCommandLists);
    if (!cmdLists.empty()) {
      recorder.Start();
      recorder.Schedule(new CzeCommandListImmediateAppendCommandListsExp(
          return_value, hCommandListImmediate, static_cast<uint32_t>(cmdLists.size()),
          cmdLists.data(), hSignalEvent, numWaitEvents, phWaitEvents));
      recorder.Schedule(
          new CzeCommandListHostSynchronize(ZE_RESULT_SUCCESS, hCommandListImmediate, UINT64_MAX));
    } else {
      Log(ERR) << "Incorrect config LevelZero.Capture.Kernel.Range. The command list "
               << l0IFace.CfgRec_StartCommandList()
               << " doesn't exist inside immediate command list submission "
               << l0IFace.CfgRec_StartCommandQueueSubmit();
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
    if (l0IFace.CfgRec_IsStopQueueSubmit()) {
      recorder.Stop();
      recorder.MarkForDeletion();
    }
  } else if (recorder.Running()) {
    if (subcaptureMode && l0IFace.CfgRec_IsStopQueueSubmit()) {
      auto cmdLists = GetCommandListsToSubcapture(recorder.Running(), l0IFace, sd, numCommandLists,
                                                  phCommandLists);
      if (!cmdLists.empty()) {
        recorder.Schedule(new CzeCommandListImmediateAppendCommandListsExp(
            return_value, hCommandListImmediate, static_cast<uint32_t>(cmdLists.size()),
            cmdLists.data(), hSignalEvent, numWaitEvents, phWaitEvents));
        recorder.Schedule(new CzeCommandListHostSynchronize(ZE_RESULT_SUCCESS,
                                                            hCommandListImmediate, UINT64_MAX));
      } else {
        Log(ERR) << "Incorrect config LevelZero.Capture.Kernel.Range. The command list "
                 << l0IFace.CfgRec_StopCommandList()
                 << " doesn't exist inside immediate command list submission "
                 << l0IFace.CfgRec_StopCommandQueueSubmit();
        throw EOperationFailed(EXCEPTION_MESSAGE);
      }
      recorder.Stop();
      recorder.MarkForDeletion();
    } else {
      recorder.Schedule(new CzeCommandListImmediateAppendCommandListsExp(
          return_value, hCommandListImmediate, numCommandLists, phCommandLists, hSignalEvent,
          numWaitEvents, phWaitEvents));
    }
  }
  zeCommandListImmediateAppendCommandListsExp_SD(return_value, hCommandListImmediate,
                                                 numCommandLists, phCommandLists, hSignalEvent,
                                                 numWaitEvents, phWaitEvents);
}

inline void zeCommandListCreateCloneExp_RECWRAP(
    CRecorder& recorder,
    [[maybe_unused]] ze_result_t return_value,
    [[maybe_unused]] ze_command_list_handle_t hCommandList,
    [[maybe_unused]] ze_command_list_handle_t* phClonedCommandList) {
  if (recorder.Running()) {
    Log(ERR) << "zeCommandListCreateCloneExp is not implemented.";
    throw ENotImplemented(EXCEPTION_MESSAGE);
  }
}

inline void zeContextDestroy_RECWRAP_PRE(CRecorder& recorder,
                                         ze_result_t return_value,
                                         ze_context_handle_t hContext) {
  if (recorder.Running() && return_value == ZE_RESULT_SUCCESS) {
    auto& sd = SD();
    auto& contextState = sd.Get<CContextState>(hContext, EXCEPTION_MESSAGE);
    auto& hEventPool = contextState.gitsPoolEventHandle;
    if (hEventPool != nullptr) {
      drv.inject.zeEventPoolDestroy(hEventPool);
    }
    auto& hEvent = contextState.gitsEventHandle;
    if (hEvent != nullptr) {
      drv.inject.zeEventDestroy(hEvent);
    }
  }
}

inline void zeContextCreate_RECWRAP(CRecorder& recorder,
                                    ze_result_t return_value,
                                    ze_driver_handle_t hDriver,
                                    const ze_context_desc_t* desc,
                                    ze_context_handle_t* phContext) {
  if (recorder.Running()) {
    recorder.Schedule(new CzeContextCreate(return_value, hDriver, desc, phContext));
  }
  zeContextCreate_SD(return_value, hDriver, desc, phContext);
  const auto& cfg = Config::Get();
  const uint64_t deviceMemorySize =
      cfg.levelzero.recorder.disableAddressTranslation.virtualDeviceMemorySize;
  if (return_value == ZE_RESULT_SUCCESS &&
      IsMemoryTypeAddressTranslationDisabled(cfg, UnifiedMemoryType::device) &&
      deviceMemorySize != 0U) {
    auto& contextState = SD().Get<CContextState>(*phContext, EXCEPTION_MESSAGE);
    const auto alignedSize = gits::Align<gits::alignment::pageSize2MB>(deviceMemorySize);
    contextState.virtualMemorySize = alignedSize;
    drv.inject.zeVirtualMemReserve(*phContext, nullptr, alignedSize, &contextState.virtualMemory);
  }
}
} // namespace l0
} // namespace gits

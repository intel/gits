// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   l0PlayerRunWrap.h
*
* @brief API RUNWRAP implementations.
*/

#pragma once

#include "l0Drivers.h"
#include "l0Arguments.h"
#include "l0Structs.h"
#include "l0Header.h"
#include "l0StateTracking.h"
#include "l0ArgumentsAuto.h"
#include "l0Tools.h"

#ifdef WITH_OPENCL
#include "openclArguments.h"
#else
#define Ccl_context       COutArgument
#define Ccl_mem           COutArgument
#define Ccl_program       COutArgument
#define Ccl_command_queue COutArgument
#endif

namespace gits {
namespace l0 {
namespace {
void TranslatePointerOffsets(CStateDynamic& sd,
                             void* bufferPtr,
                             const std::map<size_t, bool>& offsetMap) {
  for (const auto& offsetInfo : offsetMap) {
    void* ptrLocation =
        reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(bufferPtr) + offsetInfo.first);
    void* ptrToFetch = nullptr;
    std::memcpy(&ptrToFetch, ptrLocation, sizeof(void*));
    const auto allocPair = GetAllocFromOriginalPtr(ptrToFetch, sd);
    ptrToFetch = GetOffsetPointer(allocPair.first, allocPair.second);
    if (ptrToFetch == nullptr) {
      Log(WARN) << "Couldn't translate pointer inside pAlloc: " << bufferPtr
                << " offset: " << offsetInfo.first;
    }
    std::memcpy(ptrLocation, &ptrToFetch, sizeof(void*));
  }
}
void TranslatePointers(CStateDynamic& sd) {
  for (const auto& allocState : sd.Map<CAllocState>()) {
    auto& indirectOffsets = allocState.second->indirectPointersOffsets;
    if (indirectOffsets.empty() ||
        indirectOffsets.end() == std::find_if(indirectOffsets.begin(), indirectOffsets.end(),
                                              [](auto&& i) { return !i.second; })) {
      continue;
    }
    if (allocState.second->memType == UnifiedMemoryType::device) {
      ze_command_list_handle_t list = GetCommandListImmediate(sd, drv, allocState.second->hContext);
      const auto size = allocState.second->size;
      auto* tmpBuffer = new char[size];
      drv.inject.zeCommandListAppendMemoryCopy(list, tmpBuffer, allocState.first, size, nullptr, 0,
                                               nullptr);
      TranslatePointerOffsets(sd, tmpBuffer, indirectOffsets);
      drv.inject.zeCommandListAppendMemoryCopy(list, allocState.first, tmpBuffer, size, nullptr, 0,
                                               nullptr);
      delete[] tmpBuffer;
    } else {
      TranslatePointerOffsets(sd, allocState.first, indirectOffsets);
    }
    std::for_each(indirectOffsets.begin(), indirectOffsets.end(),
                  [](auto&& i) { i.second = true; });
  }
}
bool UpdateDeviceQueueProperties(
    const ze_device_handle_t& hDevice,
    std::vector<ze_command_queue_group_properties_t>& groupProperties) {
  uint32_t numGroupProperties = 0U;
  drv.inject.zeDeviceGetCommandQueueGroupProperties(hDevice, &numGroupProperties, nullptr);
  groupProperties.resize(numGroupProperties);
  const auto ret = drv.inject.zeDeviceGetCommandQueueGroupProperties(hDevice, &numGroupProperties,
                                                                     groupProperties.data());
  return ret == ZE_RESULT_SUCCESS;
}

bool RedirectToOriginalQueueSubmission(ze_device_handle_t hDevice, uint32_t* descOrdinal) {
  auto& deviceState = SD().Get<CDeviceState>(hDevice, EXCEPTION_MESSAGE);
  const auto& originalProps = deviceState.originalQueueGroupProperties;
  if (originalProps.empty()) {
    return false;
  }
  auto& currentEngines = deviceState.cqGroupProperties;
  if (currentEngines.empty() && !UpdateDeviceQueueProperties(hDevice, currentEngines)) {
    throw EOperationFailed("Couldn't obtain queue family properties.");
  }
  const auto originalOrdinal = *descOrdinal;

  if (originalProps.size() <= originalOrdinal) {
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  const auto& originalEngine = originalProps.at(originalOrdinal);
  Log(TRACE) << "Original queue family properties: " << ToStringHelper(originalEngine.flags);
  auto& multiContextEngineMap = deviceState.multiContextEngineMap;
  if (multiContextEngineMap.find(originalOrdinal) != multiContextEngineMap.end()) {
    *descOrdinal = multiContextEngineMap.at(originalOrdinal);
  } else {
    const auto blockedOrdinals = GetMapValues(deviceState.multiContextEngineMap);
    *descOrdinal = GetMostCommonOrdinal(originalEngine.flags, currentEngines, blockedOrdinals);
    multiContextEngineMap[originalOrdinal] = *descOrdinal;
  }
  return true;
}

void RedirectToDefaultQueueFamily(const ze_device_handle_t& hDevice, uint32_t* descOrdinal) {
  auto& deviceState = SD().Get<CDeviceState>(hDevice, EXCEPTION_MESSAGE);
  auto& currentEngines = deviceState.cqGroupProperties;
  if (currentEngines.empty() && !UpdateDeviceQueueProperties(hDevice, currentEngines)) {
    throw EOperationFailed("Couldn't obtain queue family properties.");
  }
  const auto commonFlags = static_cast<ze_command_queue_group_property_flags_t>(
      ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE | ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY);
  const auto originalOrdinal = *descOrdinal;
  auto& multiContextEngineMap = deviceState.multiContextEngineMap;
  if (multiContextEngineMap.find(originalOrdinal) != multiContextEngineMap.end()) {
    *descOrdinal = multiContextEngineMap.at(originalOrdinal);
  } else {
    const auto blockedOrdinals = GetMapValues(deviceState.multiContextEngineMap);
    *descOrdinal = GetMostCommonOrdinal(commonFlags, currentEngines, blockedOrdinals);
    multiContextEngineMap[originalOrdinal] = *descOrdinal;
  }
  Log(WARN) << "Setting new command queue group ordinal: " << *descOrdinal
            << " with group flags: " << ToStringHelper(currentEngines[*descOrdinal].flags);
}
void ChooseQueueIndex(const ze_device_handle_t& hDevice, const uint32_t& ordinal, uint32_t* index) {
  auto& deviceState = SD().Get<CDeviceState>(hDevice, EXCEPTION_MESSAGE);
  auto& currentEngines = deviceState.cqGroupProperties;
  if (currentEngines.empty() && !UpdateDeviceQueueProperties(hDevice, currentEngines)) {
    throw EOperationFailed("Couldn't obtain queue family properties.");
  }
  const auto numQueues = currentEngines.at(ordinal).numQueues;
  if (*index >= numQueues) {
    *index = numQueues - 1U;
  }
}

inline void HandleDumpSpv(const ze_module_desc_t* desc) {
  if (ShouldDumpSpv(Config::Get().player.l0DumpSpv, desc)) {
    static int programSourceIdx = 0;
    std::stringstream stream;
    stream << "l0Programs/kernel_source_" << std::setfill('0') << std::setw(2) << programSourceIdx++
           << ".spv";
    CArgumentFileText(stream.str().c_str(), reinterpret_cast<const char*>(desc->pInputModule),
                      static_cast<unsigned int>(desc->inputSize));
  }
}
} // namespace

inline void zeCommandListAppendLaunchKernel_RUNWRAP(Cze_result_t& _return_value,
                                                    Cze_command_list_handle_t& _hCommandList,
                                                    Cze_kernel_handle_t& _hKernel,
                                                    Cze_group_count_t::CSArray& _pLaunchFuncArgs,
                                                    Cze_event_handle_t& _hSignalEvent,
                                                    Cuint32_t& _numWaitEvents,
                                                    Cze_event_handle_t::CSArray& _phWaitEvents) {
  auto& sd = SD();
  const auto isImmediate = IsCommandListImmediate(*_hCommandList, sd);
  KernelCountUp(CGits::Instance());
  if (isImmediate) {
    TranslatePointers(sd);
  }
  _return_value.Value() =
      drv.zeCommandListAppendLaunchKernel(*_hCommandList, *_hKernel, *_pLaunchFuncArgs,
                                          *_hSignalEvent, *_numWaitEvents, *_phWaitEvents);
  zeCommandListAppendLaunchKernel_SD(*_return_value, *_hCommandList, *_hKernel, *_pLaunchFuncArgs,
                                     *_hSignalEvent, *_numWaitEvents, *_phWaitEvents);
}

inline void zeCommandListAppendLaunchCooperativeKernel_RUNWRAP(
    Cze_result_t& _return_value,
    Cze_command_list_handle_t& _hCommandList,
    Cze_kernel_handle_t& _hKernel,
    Cze_group_count_t::CSArray& _pLaunchFuncArgs,
    Cze_event_handle_t& _hSignalEvent,
    Cuint32_t& _numWaitEvents,
    Cze_event_handle_t::CSArray& _phWaitEvents) {
  auto& sd = SD();
  const auto isImmediate = IsCommandListImmediate(*_hCommandList, sd);
  KernelCountUp(CGits::Instance());
  if (isImmediate) {
    TranslatePointers(sd);
  }
  _return_value.Value() = drv.zeCommandListAppendLaunchCooperativeKernel(
      *_hCommandList, *_hKernel, *_pLaunchFuncArgs, *_hSignalEvent, *_numWaitEvents,
      *_phWaitEvents);
  zeCommandListAppendLaunchCooperativeKernel_SD(*_return_value, *_hCommandList, *_hKernel,
                                                *_pLaunchFuncArgs, *_hSignalEvent, *_numWaitEvents,
                                                *_phWaitEvents);
}

inline void zeCommandListAppendLaunchKernelIndirect_RUNWRAP(
    Cze_result_t& _return_value,
    Cze_command_list_handle_t& _hCommandList,
    Cze_kernel_handle_t& _hKernel,
    Cze_group_count_t::CSArray& _pLaunchArgumentsBuffer,
    Cze_event_handle_t& _hSignalEvent,
    Cuint32_t& _numWaitEvents,
    Cze_event_handle_t::CSArray& _phWaitEvents) {
  auto& sd = SD();
  const auto isImmediate = IsCommandListImmediate(*_hCommandList, sd);
  KernelCountUp(CGits::Instance());
  if (isImmediate) {
    TranslatePointers(sd);
  }
  _return_value.Value() = drv.zeCommandListAppendLaunchKernelIndirect(
      *_hCommandList, *_hKernel, *_pLaunchArgumentsBuffer, *_hSignalEvent, *_numWaitEvents,
      *_phWaitEvents);
  zeCommandListAppendLaunchKernelIndirect_SD(*_return_value, *_hCommandList, *_hKernel,
                                             *_pLaunchArgumentsBuffer, *_hSignalEvent,
                                             *_numWaitEvents, *_phWaitEvents);
}

inline void zeCommandListAppendLaunchMultipleKernelsIndirect_RUNWRAP(
    Cze_result_t& _return_value,
    Cze_command_list_handle_t& _hCommandList,
    Cuint32_t& _numKernels,
    Cze_kernel_handle_t::CSArray& _phKernels,
    Cuint32_t::CSArray& _pCountBuffer,
    Cze_group_count_t::CSArray& _pLaunchArgumentsBuffer,
    Cze_event_handle_t& _hSignalEvent,
    Cuint32_t& _numWaitEvents,
    Cze_event_handle_t::CSArray& _phWaitEvents) {
  auto& sd = SD();
  const auto isImmediate = IsCommandListImmediate(*_hCommandList, sd);
  KernelCountUp(CGits::Instance());
  if (isImmediate) {
    TranslatePointers(sd);
  }
  _return_value.Value() = drv.zeCommandListAppendLaunchMultipleKernelsIndirect(
      *_hCommandList, *_numKernels, *_phKernels, *_pCountBuffer, *_pLaunchArgumentsBuffer,
      *_hSignalEvent, *_numWaitEvents, *_phWaitEvents);
  zeCommandListAppendLaunchMultipleKernelsIndirect_SD(
      *_return_value, *_hCommandList, *_numKernels, *_phKernels, *_pCountBuffer,
      *_pLaunchArgumentsBuffer, *_hSignalEvent, *_numWaitEvents, *_phWaitEvents);
}

inline void zeCommandQueueExecuteCommandLists_RUNWRAP(
    Cze_result_t& _return_value,
    Cze_command_queue_handle_t& _hCommandQueue,
    Cuint32_t& _numCommandLists,
    Cze_command_list_handle_t::CSArray& _phCommandLists,
    Cze_fence_handle_t& _hFence) {
  auto& sd = SD();
  CommandQueueExecCountUp(CGits::Instance());
  TranslatePointers(sd);
  _return_value.Value() = drv.zeCommandQueueExecuteCommandLists(*_hCommandQueue, *_numCommandLists,
                                                                *_phCommandLists, *_hFence);
  zeCommandQueueExecuteCommandLists_SD(*_return_value, *_hCommandQueue, *_numCommandLists,
                                       *_phCommandLists, *_hFence);
}

inline void zeModuleCreate_RUNWRAP(Cze_result_t& _return_value,
                                   Cze_context_handle_t& _hContext,
                                   Cze_device_handle_t& _hDevice,
                                   Cze_module_desc_t::CSArray& _desc,
                                   Cze_module_handle_t::CSMapArray& _phModule,
                                   Cze_module_build_log_handle_t::CSMapArray& _phBuildLog) {
  _return_value.Value() =
      drv.zeModuleCreate(*_hContext, *_hDevice, *_desc, *_phModule, *_phBuildLog);
  zeModuleCreate_SD(*_return_value, *_hContext, *_hDevice, *_desc, *_phModule, *_phBuildLog);
  HandleDumpSpv(*_desc);
}

inline void zeModuleCreate_V1_RUNWRAP(Cze_result_t& _return_value,
                                      Cze_context_handle_t& _hContext,
                                      Cze_device_handle_t& _hDevice,
                                      Cze_module_desc_t_V1::CSArray& _desc,
                                      Cze_module_handle_t::CSMapArray& _phModule,
                                      Cze_module_build_log_handle_t::CSMapArray& _phBuildLog) {
  _return_value.Value() =
      drv.zeModuleCreate(*_hContext, *_hDevice, *_desc, *_phModule, *_phBuildLog);
  zeModuleCreate_SD(*_return_value, *_hContext, *_hDevice, *_desc, *_phModule, *_phBuildLog);
  if (_return_value.Value() == ZE_RESULT_SUCCESS && *_phModule != nullptr && *_desc != nullptr &&
      !_desc.Vector().empty()) {
    const auto moduleFileName = _desc.Vector().front()->GetProgramSourceName();
    ze_module_handle_t* hModule = *_phModule;
    if (hModule != nullptr) {
      auto& moduleState = SD().Get<CModuleState>(*hModule, EXCEPTION_MESSAGE);
      moduleState.moduleFileName = moduleFileName;
    }
  }
}

inline void zeCommandQueueCreate_RUNWRAP(Cze_result_t& _return_value,
                                         Cze_context_handle_t& _hContext,
                                         Cze_device_handle_t& _hDevice,
                                         Cze_command_queue_desc_t::CSArray& _desc,
                                         Cze_command_queue_handle_t::CSMapArray& _phCommandQueue) {
  ze_command_queue_desc_t* descPtr = *_desc;
  if (descPtr == nullptr) {
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  ze_command_queue_desc_t desc = *descPtr;
  if (desc.ordinal != 0) {
    const auto result = RedirectToOriginalQueueSubmission(*_hDevice, &desc.ordinal);
    if (!result) {
      RedirectToDefaultQueueFamily(*_hDevice, &desc.ordinal);
    }
  }
  if (desc.index != 0) {
    ChooseQueueIndex(*_hDevice, desc.ordinal, &desc.index);
  }
  _return_value.Value() = drv.zeCommandQueueCreate(*_hContext, *_hDevice, &desc, *_phCommandQueue);
  zeCommandQueueCreate_SD(*_return_value, *_hContext, *_hDevice, &desc, *_phCommandQueue);
}

inline void zeMemAllocHost_RUNWRAP(Cze_result_t& _return_value,
                                   Cze_context_handle_t& _hContext,
                                   Cze_host_mem_alloc_desc_t::CSArray& _host_desc,
                                   Csize_t& _size,
                                   Csize_t& _alignment,
                                   CMappedPtr::CSMapArray& _pptr) {
  _return_value.Value() = drv.zeMemAllocHost(*_hContext, *_host_desc, *_size, *_alignment, *_pptr);
  if (*_return_value == ZE_RESULT_SUCCESS && CheckCfgZeroInitialization(Config::Get())) {
    const auto commandList = GetCommandListImmediate(SD(), drv, *_hContext);
    ZeroInitializeUsm(drv, commandList, *_pptr, *_size, UnifiedMemoryType::host);
  }
  zeMemAllocHost_SD(*_return_value, *_hContext, *_host_desc, *_size, *_alignment, *_pptr);
}

inline void zeMemAllocDevice_RUNWRAP(Cze_result_t& _return_value,
                                     Cze_context_handle_t& _hContext,
                                     Cze_device_mem_alloc_desc_t::CSArray& _device_desc,
                                     Csize_t& _size,
                                     Csize_t& _alignment,
                                     Cze_device_handle_t& _hDevice,
                                     CMappedPtr::CSMapArray& _pptr) {
  _return_value.Value() =
      drv.zeMemAllocDevice(*_hContext, *_device_desc, *_size, *_alignment, *_hDevice, *_pptr);
  if (*_return_value == ZE_RESULT_SUCCESS && CheckCfgZeroInitialization(Config::Get())) {
    const auto commandList = GetCommandListImmediate(SD(), drv, *_hContext);
    ZeroInitializeUsm(drv, commandList, *_pptr, *_size, UnifiedMemoryType::device);
  }
  zeMemAllocDevice_SD(*_return_value, *_hContext, *_device_desc, *_size, *_alignment, *_hDevice,
                      *_pptr);
}

inline void zeMemAllocShared_RUNWRAP(Cze_result_t& _return_value,
                                     Cze_context_handle_t& _hContext,
                                     Cze_device_mem_alloc_desc_t::CSArray& _device_desc,
                                     Cze_host_mem_alloc_desc_t::CSArray& _host_desc,
                                     Csize_t& _size,
                                     Csize_t& _alignment,
                                     Cze_device_handle_t& _hDevice,
                                     CMappedPtr::CSMapArray& _pptr) {
  _return_value.Value() = drv.zeMemAllocShared(*_hContext, *_device_desc, *_host_desc, *_size,
                                               *_alignment, *_hDevice, *_pptr);
  if (*_return_value == ZE_RESULT_SUCCESS && CheckCfgZeroInitialization(Config::Get())) {
    const auto commandList = GetCommandListImmediate(SD(), drv, *_hContext);
    ZeroInitializeUsm(drv, commandList, *_pptr, *_size, UnifiedMemoryType::shared);
  }
  zeMemAllocShared_SD(*_return_value, *_hContext, *_device_desc, *_host_desc, *_size, *_alignment,
                      *_hDevice, *_pptr);
}

inline void zeImageCreate_RUNWRAP(Cze_result_t& _return_value,
                                  Cze_context_handle_t& _hContext,
                                  Cze_device_handle_t& _hDevice,
                                  Cze_image_desc_t::CSArray& _desc,
                                  Cze_image_handle_t::CSMapArray& _phImage) {
  _return_value.Value() = drv.zeImageCreate(*_hContext, *_hDevice, *_desc, *_phImage);
  if (*_return_value == ZE_RESULT_SUCCESS && CheckCfgZeroInitialization(Config::Get())) {
    const auto commandList = GetCommandListImmediate(SD(), drv, *_hContext);
    ZeroInitializeImage(drv, commandList, *_phImage, *_desc);
  }
  zeImageCreate_SD(*_return_value, *_hContext, *_hDevice, *_desc, *_phImage);
}

inline void zeFenceQueryStatus_RUNWRAP(Cze_result_t& _return_value, Cze_fence_handle_t& _hFence) {
  _return_value.Value() = drv.zeFenceQueryStatus(*_hFence);
  if (_return_value.Value() != ZE_RESULT_SUCCESS && _return_value.Original() == ZE_RESULT_SUCCESS &&
      SD().Get<CFenceState>(*_hFence, EXCEPTION_MESSAGE).canBeSynced) {
    drv.inject.zeFenceHostSynchronize(*_hFence, UINT64_MAX);
  }
}

inline void zeDeviceGetSubDevices_RUNWRAP(Cze_result_t& _return_value,
                                          Cze_device_handle_t& _hDevice,
                                          Cuint32_t::CSArray& _pCount,
                                          Cze_device_handle_t::CSMapArray& _phSubdevices) {
  uint32_t count = 0;
  if (_phSubdevices.Original() != nullptr) {
    _return_value.Value() = drv.zeDeviceGetSubDevices(*_hDevice, &count, nullptr);
  } else {
    return;
  }
  auto* pCountOriginal = *_pCount;
  if (pCountOriginal != nullptr && *pCountOriginal > count) {
    Log(WARN) << "Stream was recorded on the device that could be partitioned into more devices("
              << ToStringHelper(*pCountOriginal) << ") than this one: " << ToStringHelper(count);
  }
  _return_value.Value() = drv.zeDeviceGetSubDevices(*_hDevice, &count, *_phSubdevices);
  if (pCountOriginal != nullptr && *pCountOriginal > 0 && *_hDevice != nullptr &&
      count < *pCountOriginal) {
    Log(WARN) << "Adjusting " << ToStringHelper(static_cast<uint32_t>(*pCountOriginal) - count)
              << " of out-devices mapping to the primary in-device ";
    for (uint32_t i = count; i < static_cast<uint32_t>(*pCountOriginal); i++) {
      Cze_device_handle_t::AddMapping(_phSubdevices._array[i], *_hDevice);
    }
  }
  zeDeviceGetSubDevices_SD(*_return_value, *_hDevice, &count, *_phSubdevices);
}

inline void zeCommandListCreate_RUNWRAP(Cze_result_t& _return_value,
                                        Cze_context_handle_t& _hContext,
                                        Cze_device_handle_t& _hDevice,
                                        Cze_command_list_desc_t::CSArray& _desc,
                                        Cze_command_list_handle_t::CSMapArray& _phCommandList) {
  ze_command_list_desc_t* descPtr = *_desc;
  if (descPtr == nullptr) {
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  ze_command_list_desc_t desc = *descPtr;
  if (desc.commandQueueGroupOrdinal != 0U) {
    const auto result =
        RedirectToOriginalQueueSubmission(*_hDevice, &desc.commandQueueGroupOrdinal);
    if (!result) {
      RedirectToDefaultQueueFamily(*_hDevice, &desc.commandQueueGroupOrdinal);
    }
  }
  _return_value.Value() = drv.zeCommandListCreate(*_hContext, *_hDevice, &desc, *_phCommandList);
  CGits::Instance().CommandListCountUp();
  zeCommandListCreate_SD(*_return_value, *_hContext, *_hDevice, &desc, *_phCommandList);
}

inline void zeCommandListCreateImmediate_RUNWRAP(
    Cze_result_t& _return_value,
    Cze_context_handle_t& _hContext,
    Cze_device_handle_t& _hDevice,
    Cze_command_queue_desc_t::CSArray& _altdesc,
    Cze_command_list_handle_t::CSMapArray& _phCommandList) {
  ze_command_queue_desc_t* descPtr = *_altdesc;
  if (descPtr == nullptr) {
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  ze_command_queue_desc_t desc = *descPtr;
  if (desc.ordinal != 0U) {
    const auto result = RedirectToOriginalQueueSubmission(*_hDevice, &desc.ordinal);
    if (!result) {
      RedirectToDefaultQueueFamily(*_hDevice, &desc.ordinal);
    }
  }
  if (desc.index != 0U) {
    ChooseQueueIndex(*_hDevice, desc.ordinal, &desc.index);
  }
  _return_value.Value() =
      drv.zeCommandListCreateImmediate(*_hContext, *_hDevice, &desc, *_phCommandList);
  CGits::Instance().CommandListCountUp();
  CGits::Instance().CommandQueueExecCountUp();
  zeCommandListCreateImmediate_SD(*_return_value, *_hContext, *_hDevice, &desc, *_phCommandList);
}

inline void zeDriverGetLastErrorDescription_RUNWRAP(Cze_result_t& _return_value,
                                                    Cze_driver_handle_t& _hDriver,
                                                    COutArgument& /*_ppString*/) {
  const char* ppString = nullptr;
  _return_value.Value() = drv.zeDriverGetLastErrorDescription(*_hDriver, &ppString);
}

inline void zeCommandListAppendQueryKernelTimestamps_RUNWRAP(
    Cze_result_t& _return_value,
    Cze_command_list_handle_t& _hCommandList,
    Cuint32_t& _numEvents,
    Cze_event_handle_t::CSArray& _phEvents,
    CvoidPtr& _dstptr,
    Csize_t::CSArray& _pOffsets,
    Cze_event_handle_t& _hSignalEvent,
    Cuint32_t& _numWaitEvents,
    Cze_event_handle_t::CSArray& _phWaitEvents) {
  if (*_hSignalEvent != nullptr) {
    drv.inject.zeEventHostSignal(*_hSignalEvent);
  }
  zeCommandListAppendQueryKernelTimestamps_SD(*_return_value, *_hCommandList, *_numEvents,
                                              *_phEvents, *_dstptr, *_pOffsets, *_hSignalEvent,
                                              *_numWaitEvents, *_phWaitEvents);
}

} // namespace l0
} // namespace gits

// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
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
#include "l0Log.h"
#include "l0StateDynamic.h"
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
#include "tools_lite.h"

namespace gits {
namespace l0 {
namespace {
void TranslatePointers(CStateDynamic& sd) {
  for (const auto& allocState : sd.Map<CAllocState>()) {
    auto& indirectOffsets = allocState.second->indirectPointersOffsets;
    if (indirectOffsets.empty()) {
      continue;
    }
    const auto all_pointers_translated = std::all_of(indirectOffsets.begin(), indirectOffsets.end(),
                                                     [](const auto& pair) { return pair.second; });
    if (all_pointers_translated) {
      continue;
    }

    Log(TRACEV) << "Translating pointer: " << ToStringHelper(allocState.first)
                << " size of indirect pointers: " << std::to_string(indirectOffsets.size());
    const auto size = allocState.second->size;
    if (allocState.second->memType == UnifiedMemoryType::device) {
      ze_command_list_handle_t list = GetCommandListImmediate(sd, drv, allocState.second->hContext);
      auto tmpBuffer = std::make_unique<char[]>(size);
      drv.inject.zeCommandListAppendMemoryCopy(list, tmpBuffer.get(), allocState.first, size,
                                               nullptr, 0, nullptr);
      const auto translations = TranslatePointerOffsets(sd, tmpBuffer.get(), indirectOffsets);
      if (translations > 0) {
        drv.inject.zeCommandListAppendMemoryCopy(list, allocState.first, tmpBuffer.get(), size,
                                                 nullptr, 0, nullptr);
      }
    } else {
      auto tmpBuffer = std::make_unique<char[]>(size);
      std::memcpy(tmpBuffer.get(), allocState.first, size);
      const auto translations = TranslatePointerOffsets(sd, tmpBuffer.get(), indirectOffsets);
      if (translations > 0) {
        std::memcpy(allocState.first, tmpBuffer.get(), size);
      }
    }
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
  if (ShouldDumpSpv(Config::Get().levelzero.player.dumpSpv, desc)) {
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
  if (*_hSignalEvent != nullptr) {
    const auto err = drv.inject.zeEventQueryStatus(*_hSignalEvent);
    if (err == ZE_RESULT_SUCCESS) {
      drv.inject.zeEventHostReset(*_hSignalEvent);
    }
  }
  auto& sd = SD();
  const auto isImmediate = IsCommandListImmediate(*_hCommandList, sd);
  KernelCountUp(CGits::Instance());
  if (isImmediate) {
    TranslatePointers(sd);
  }
  const auto& cfg = Config::Get();
  if (CaptureKernels(cfg) && IsDumpInputMode(cfg)) {
    AppendLaunchKernel(*_hCommandList, *_hKernel, *_pLaunchFuncArgs, *_hSignalEvent, true);
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
  const auto& cfg = Config::Get();
  if (CaptureKernels(cfg) && IsDumpInputMode(cfg)) {
    AppendLaunchKernel(*_hCommandList, *_hKernel, *_pLaunchFuncArgs, *_hSignalEvent, true);
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
  const auto& cfg = Config::Get();
  if (CaptureKernels(cfg) && IsDumpInputMode(cfg)) {
    AppendLaunchKernel(*_hCommandList, *_hKernel, *_pLaunchArgumentsBuffer, *_hSignalEvent, true);
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
  if (isImmediate) {
    TranslatePointers(sd);
  }
  for (auto i = 0U; i < *_numKernels; i++) {
    KernelCountUp(CGits::Instance());
    const auto& cfg = Config::Get();
    if (CaptureKernels(cfg) && IsDumpInputMode(cfg)) {
      AppendLaunchKernel(*_hCommandList, (*_phKernels)[i], *_pLaunchArgumentsBuffer, *_hSignalEvent,
                         true);
    }
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
  if (IsControlledSubmission(descPtr)) {
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
  auto size = *_size;
  const auto& cfg = Config::Get();
  _return_value.Value() = drv.zeMemAllocHost(*_hContext, *_host_desc, size, *_alignment, *_pptr);
  if (_return_value.Value() == ZE_RESULT_SUCCESS && CaptureKernels(cfg)) {
    Log(TRACEV) << "^-- Original pointer: " << ToStringHelper(CMappedPtr::GetOriginal((*_pptr)[0]));
  }
  if (*_return_value == ZE_RESULT_SUCCESS && CheckCfgZeroInitialization(cfg)) {
    const auto commandList = GetCommandListImmediate(SD(), drv, *_hContext);
    ZeroInitializeUsm(drv, commandList, *_pptr, size, UnifiedMemoryType::host);
  }
  zeMemAllocHost_SD(*_return_value, *_hContext, *_host_desc, size, *_alignment, *_pptr);
}

inline void zeMemAllocDevice_RUNWRAP(Cze_result_t& _return_value,
                                     Cze_context_handle_t& _hContext,
                                     Cze_device_mem_alloc_desc_t::CSArray& _device_desc,
                                     Csize_t& _size,
                                     Csize_t& _alignment,
                                     Cze_device_handle_t& _hDevice,
                                     CMappedPtr::CSMapArray& _pptr) {
  const auto& cfg = Config::Get();
  void* originalPtr = _pptr.Original()[0];
  const auto IsAddressTranslationDisabled =
      IsMemoryTypeAddressTranslationDisabled(cfg, UnifiedMemoryType::device);
  auto size = *_size;
  if (IsAddressTranslationDisabled) {
    void* virtualPtr = nullptr;
    size = gits::Align<gits::alignment::pageSize2MB>(size);
    auto retCode = drv.zeVirtualMemReserve(*_hContext, originalPtr, size, &virtualPtr);
    zeVirtualMemReserve_SD(retCode, *_hContext, originalPtr, size, &virtualPtr);
    if (retCode != ZE_RESULT_SUCCESS || virtualPtr != originalPtr) {
      if (retCode == ZE_RESULT_SUCCESS) {
        drv.zeVirtualMemFree(*_hContext, virtualPtr, size);
        Log(ERR) << "Could not reserve the same address as pStart";
      }
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
    ze_physical_mem_desc_t physical_desc = {};
    physical_desc.stype = ZE_STRUCTURE_TYPE_PHYSICAL_MEM_DESC;
    physical_desc.size = size;
    ze_physical_mem_handle_t hPhysicalMemory = nullptr;
    retCode = drv.zePhysicalMemCreate(*_hContext, *_hDevice, &physical_desc, &hPhysicalMemory);
    zePhysicalMemCreate_SD(retCode, *_hContext, *_hDevice, &physical_desc, &hPhysicalMemory);
    if (retCode != ZE_RESULT_SUCCESS) {
      Log(ERR) << "Failed to create physical memory";
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
    retCode = drv.zeVirtualMemMap(*_hContext, originalPtr, size, hPhysicalMemory, 0U,
                                  ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE);
    zeVirtualMemMap_SD(retCode, *_hContext, originalPtr, size, hPhysicalMemory, 0U,
                       ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE);
    if (retCode != ZE_RESULT_SUCCESS) {
      Log(ERR) << "Failed to mmap virtual memory to physical memory";
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
    CMappedPtr::AddMapping(_pptr._array[0], originalPtr);
  } else {
    _return_value.Value() =
        drv.zeMemAllocDevice(*_hContext, *_device_desc, *_size, *_alignment, *_hDevice, *_pptr);
    zeMemAllocDevice_SD(*_return_value, *_hContext, *_device_desc, size, *_alignment, *_hDevice,
                        *_pptr);
  }
  if (_return_value.Value() == ZE_RESULT_SUCCESS && CaptureKernels(cfg)) {
    Log(TRACEV) << "^-- Original pointer: " << ToStringHelper(CMappedPtr::GetOriginal((*_pptr)[0]));
  }

  if (*_return_value == ZE_RESULT_SUCCESS && CheckCfgZeroInitialization(cfg)) {
    const auto commandList = GetCommandListImmediate(SD(), drv, *_hContext);
    ZeroInitializeUsm(drv, commandList, *_pptr, *_size, UnifiedMemoryType::device);
  }
}

inline void zeMemAllocShared_RUNWRAP(Cze_result_t& _return_value,
                                     Cze_context_handle_t& _hContext,
                                     Cze_device_mem_alloc_desc_t::CSArray& _device_desc,
                                     Cze_host_mem_alloc_desc_t::CSArray& _host_desc,
                                     Csize_t& _size,
                                     Csize_t& _alignment,
                                     Cze_device_handle_t& _hDevice,
                                     CMappedPtr::CSMapArray& _pptr) {
  auto size = *_size;
  const auto& cfg = Config::Get();
  _return_value.Value() = drv.zeMemAllocShared(*_hContext, *_device_desc, *_host_desc, size,
                                               *_alignment, *_hDevice, *_pptr);
  if (_return_value.Value() == ZE_RESULT_SUCCESS && CaptureKernels(cfg)) {
    Log(TRACEV) << "^-- Original pointer: " << ToStringHelper(CMappedPtr::GetOriginal((*_pptr)[0]));
  }
  if (*_return_value == ZE_RESULT_SUCCESS && CheckCfgZeroInitialization(cfg)) {
    const auto commandList = GetCommandListImmediate(SD(), drv, *_hContext);
    ZeroInitializeUsm(drv, commandList, *_pptr, size, UnifiedMemoryType::shared);
  }
  zeMemAllocShared_SD(*_return_value, *_hContext, *_device_desc, *_host_desc, size, *_alignment,
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
  const auto originalRetValue = _return_value.Original();
  _return_value.Value() = drv.zeFenceQueryStatus(*_hFence);
  if (_return_value.Value() != ZE_RESULT_SUCCESS && originalRetValue == ZE_RESULT_SUCCESS) {
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
  if (IsControlledSubmission(descPtr)) {
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
  if (IsControlledSubmission(descPtr)) {
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
                                                    [[maybe_unused]] COutArgument& _ppString) {
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
inline void zeDriverGet_RUNWRAP(Cze_result_t& _return_value,
                                Cuint32_t::CSArray& _pCount,
                                Cze_driver_handle_t::CSMapArray& _phDrivers) {
  const auto originalCount = **_pCount;
  _return_value.Value() = drv.zeDriverGet(*_pCount, *_phDrivers);
  const auto currentCount = **_pCount;
  if (_return_value.Value() == ZE_RESULT_SUCCESS && _phDrivers.Size() > 0U &&
      originalCount > currentCount) {
    Log(WARN) << "Original application was recorded using more drivers.";
    const auto firstDriver = (*_phDrivers)[0];
    for (auto i = currentCount; i < originalCount; i++) {
      const auto originalDrivers = _phDrivers.Original();
      Cze_driver_handle_t::AddMapping(originalDrivers[i], firstDriver);
    }
  }
  zeDriverGet_SD(*_return_value, *_pCount, *_phDrivers);
}

inline void zeDeviceGet_RUNWRAP(Cze_result_t& _return_value,
                                Cze_driver_handle_t& _hDriver,
                                Cuint32_t::CSArray& _pCount,
                                Cze_device_handle_t::CSMapArray& _phDevices) {
  const auto originalCount = **_pCount;
  _return_value.Value() = drv.zeDeviceGet(*_hDriver, *_pCount, *_phDevices);
  const auto currentCount = **_pCount;
  if (_return_value.Value() == ZE_RESULT_SUCCESS && _phDevices.Size() > 0U &&
      originalCount > currentCount) {
    Log(WARN) << "Original application was recorded using more devices.";
    const auto gpuDevice = GetGPUDevice(SD(), drv);
    for (auto i = currentCount; i < originalCount; i++) {
      const auto originalDevices = _phDevices.Original();
      Cze_device_handle_t::AddMapping(originalDevices[i], gpuDevice);
    }
  }
  zeDeviceGet_SD(*_return_value, *_hDriver, *_pCount, *_phDevices);
}

inline void zeContextDestroy_RUNWRAP(Cze_result_t& _return_value, Cze_context_handle_t& _hContext) {
  if (*_hContext != nullptr) {
    auto& sd = SD();
    auto& contextState = sd.Get<CContextState>(*_hContext, EXCEPTION_MESSAGE);
    auto& hEventPool = contextState.gitsPoolEventHandle;
    if (hEventPool != nullptr) {
      drv.inject.zeEventPoolDestroy(hEventPool);
    }
    auto& hEvent = contextState.gitsEventHandle;
    if (hEvent != nullptr) {
      drv.inject.zeEventDestroy(hEvent);
    }
  }
  _return_value.Value() = drv.zeContextDestroy(*_hContext);
  zeContextDestroy_SD(*_return_value, *_hContext);
  _hContext.RemoveMapping();
}

inline void zeMemFree_RUNWRAP(Cze_result_t& _return_value,
                              Cze_context_handle_t& _hContext,
                              CMappedPtr& _ptr) {
  if (*_ptr != nullptr) {
    auto& allocState = SD().Get<CAllocState>(*_ptr, EXCEPTION_MESSAGE);
    const auto& cfg = Config::Get();
    const auto IsAddressTranslationDisabled =
        IsMemoryTypeAddressTranslationDisabled(cfg, allocState.memType);
    if (IsAddressTranslationDisabled && allocState.memType == UnifiedMemoryType::device) {
      for (const auto& memMap : allocState.memMaps) {
        const auto hPhysicalMemory = memMap.second->hPhysicalMemory;
        _return_value.Value() =
            drv.zeVirtualMemUnmap(*_hContext, GetOffsetPointer(*_ptr, memMap.first),
                                  memMap.second->virtualMemorySizeFromOffset);
        zeVirtualMemUnmap_SD(*_return_value, *_hContext, GetOffsetPointer(*_ptr, memMap.first),
                             memMap.second->virtualMemorySizeFromOffset);
        _return_value.Value() = drv.zePhysicalMemDestroy(*_hContext, hPhysicalMemory);
        zePhysicalMemDestroy_SD(*_return_value, *_hContext, hPhysicalMemory);
        _return_value.Value() = drv.zeVirtualMemFree(*_hContext, *_ptr, allocState.size);
        zeVirtualMemFree_SD(*_return_value, *_hContext, *_ptr, allocState.size);
      }
    } else {
      _return_value.Value() = drv.zeMemFree(*_hContext, *_ptr);
      zeMemFree_SD(*_return_value, *_hContext, *_ptr);
    }
  }
}

inline void zeVirtualMemReserve_V1_RUNWRAP(Cze_result_t& _return_value,
                                           Cze_context_handle_t& _hContext,
                                           Cuintptr_t& _pStart,
                                           Csize_t& _size,
                                           CMappedPtr::CSMapArray& _pptr) {
  _return_value.Value() = drv.zeVirtualMemReserve(*_hContext, *_pStart, *_size, *_pptr);
  const auto& cfg = Config::Get();
  if (_return_value.Value() == ZE_RESULT_SUCCESS &&
      !cfg.levelzero.player.omitOriginalAddressCheck) {
    const void* originalPtrValue = *_pStart;
    const void* virtualPtrReturnedByDriver = **_pptr;
    if (originalPtrValue != nullptr && virtualPtrReturnedByDriver != originalPtrValue) {
      drv.inject.zeVirtualMemFree(*_hContext, virtualPtrReturnedByDriver, *_size);
      Log(ERR) << "Could not reserve the same address as pStart";
      if (reinterpret_cast<uintptr_t>(virtualPtrReturnedByDriver) >
          reinterpret_cast<uintptr_t>(originalPtrValue)) {
        Log(ERR) << "Try to record application again with higher VirtualDeviceMemorySize";
      }
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
  }
  zeVirtualMemReserve_SD(*_return_value, *_hContext, *_pStart, *_size, *_pptr);
}

} // namespace l0
} // namespace gits

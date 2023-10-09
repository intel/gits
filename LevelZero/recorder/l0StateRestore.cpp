// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   l0StateRestore.cpp
*
* @brief Definition of l0 State Restore.
*
*/
#include "l0StateRestore.h"
#include "l0Drivers.h"
#include "l0Header.h"
#include "l0HelperFunctions.h"
#include "l0RecorderSubWrappers.h"
#include "l0RecorderWrapper.h"
#include "recorder.h"
#include <cstdint>
#ifdef WITH_OCLOC
#include "oclocFunctions.h"
#include "oclocStateDynamic.h"
#endif

#include "l0StateDynamic.h"
#include "l0Tools.h"
#include "l0Functions.h"
#include "recorder.h"

namespace gits {
namespace l0 {
namespace {
std::unordered_map<ze_event_handle_t, bool> availableEvents;
static ze_command_list_handle_t gitsImmediateCommandList = nullptr;
ze_command_list_handle_t GetImmediateCommandList(CScheduler& scheduler,
                                                 const ze_context_handle_t& hContext,
                                                 const ze_device_handle_t& hDevice) {
  if (gitsImmediateCommandList == nullptr) {
    ze_command_queue_desc_t desc = {};
    desc.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC;
    desc.mode = ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS;
    l0::drv.inject.zeCommandListCreateImmediate(hContext, hDevice, &desc,
                                                &gitsImmediateCommandList);
    scheduler.Register(new CzeCommandListCreateImmediate(ZE_RESULT_SUCCESS, hContext, hDevice,
                                                         &desc, &gitsImmediateCommandList));
  }
  return gitsImmediateCommandList;
}
#ifdef WITH_OCLOC
inline void ScheduleOclocInvoke(CScheduler& scheduler, ocloc::COclocState& oclocState) {
  std::vector<const char*> args;
  std::transform(oclocState.args.begin(), oclocState.args.end(), std::back_inserter(args),
                 [](auto& str) { return str.c_str(); });
  uint32_t outputNum = static_cast<uint32_t>(oclocState.outputData.size());
  std::vector<uint8_t*> outputData;
  std::transform(oclocState.outputData.begin(), oclocState.outputData.end(),
                 std::back_inserter(outputData),
                 [](std::vector<uint8_t>& vector) { return vector.data(); });
  uint8_t** outputDataPtr = outputData.data();

  auto outputDataLens = oclocState.outputLens.data();
  std::vector<char*> outputNames;
  std::transform(oclocState.outputNames.begin(), oclocState.outputNames.end(),
                 std::back_inserter(outputNames),
                 [](std::string& str) { return const_cast<char*>(str.c_str()); });
  auto outputNamesPtr = outputNames.data();
  std::vector<const char*> headerNames;
  std::transform(oclocState.headerNames.begin(), oclocState.headerNames.end(),
                 std::back_inserter(headerNames), [](std::string& str) { return str.c_str(); });
  const auto headerNamesPtr = headerNames.data();

  std::vector<const char*> sourceNames;
  std::transform(oclocState.sourceNames.begin(), oclocState.sourceNames.end(),
                 std::back_inserter(sourceNames), [](std::string& str) { return str.c_str(); });
  const auto sourceNamesPtr = sourceNames.data();

  std::vector<const uint8_t*> sourceData;
  std::transform(oclocState.sourceData.begin(), oclocState.sourceData.end(),
                 std::back_inserter(sourceData),
                 [](std::vector<uint8_t>& vector) { return vector.data(); });
  const auto sourceDataPtr = sourceData.data();

  std::vector<const uint8_t*> headerData;
  std::transform(oclocState.headerData.begin(), oclocState.headerData.end(),
                 std::back_inserter(headerData),
                 [](std::vector<uint8_t>& vector) { return vector.data(); });
  const auto headerDataPtr = headerData.data();

  scheduler.Register(
      new ocloc::CoclocInvoke_V1(0, static_cast<unsigned int>(args.size()), args.data(),
                                 static_cast<uint32_t>(oclocState.sourceData.size()), sourceDataPtr,
                                 oclocState.sourceLens.data(), sourceNamesPtr,
                                 static_cast<uint32_t>(oclocState.headerData.size()), headerDataPtr,
                                 oclocState.headerLens.data(), headerNamesPtr, &outputNum,
                                 &outputDataPtr, &outputDataLens, &outputNamesPtr));
}
#endif
void ScheduleSplitMemoryCopyFromHostPtr(CScheduler& scheduler,
                                        const void* srcPtr,
                                        const ze_command_list_handle_t& commandList,
                                        void* dstPtr,
                                        const uintptr_t offset,
                                        const size_t size) {
  auto tmpSize = size;
  if (size > UINT32_MAX) {
    tmpSize = UINT32_MAX;
    ScheduleSplitMemoryCopyFromHostPtr(scheduler, srcPtr, commandList, dstPtr, offset + UINT32_MAX,
                                       size - UINT32_MAX);
  }
  scheduler.Register(new CzeCommandListAppendMemoryCopy(
      ZE_RESULT_SUCCESS, commandList, GetOffsetPointer(dstPtr, offset),
      GetOffsetPointer((void*)srcPtr, offset), tmpSize, nullptr, 0, nullptr));
}

ze_event_handle_t GetAvailableSignalEvent(CScheduler& scheduler) {
  if (availableEvents.empty()) {
    return nullptr;
  }
  for (auto& event : availableEvents) {
    if (!event.second) {
      event.second = true;
      return event.first;
    }
  }
  auto firstEventPair = availableEvents.begin();
  scheduler.Register(new CzeEventHostReset(ZE_RESULT_SUCCESS, firstEventPair->first));
  return firstEventPair->first;
}
} // namespace
void RestoreDrivers(CScheduler& scheduler, CStateDynamic& sd) {
  scheduler.Register(new CzeInit(ZE_RESULT_SUCCESS, ZE_INIT_FLAG_GPU_ONLY));
  std::vector<ze_driver_handle_t> driverHandles;
  for (auto& state : sd.Map<CDriverState>()) {
    if (!state.second->Restored()) {
      driverHandles.push_back(state.first);
      state.second->RestoreFinished();
    }
  }
  uint32_t pCount = static_cast<uint32_t>(driverHandles.size());
  scheduler.Register(new CzeDriverGet(ZE_RESULT_SUCCESS, &pCount, driverHandles.data()));
}

void RestoreDevices(CScheduler& scheduler, CStateDynamic& sd) {
  std::map<ze_driver_handle_t, std::vector<ze_device_handle_t>> deviceHandles;
  std::map<ze_device_handle_t, std::vector<ze_device_handle_t>> subDeviceHandles;
  for (auto& state : sd.Map<CDeviceState>()) {
    if (!state.second->Restored() && state.second->hDriver != nullptr) {
      deviceHandles[state.second->hDriver].push_back(state.first);
      state.second->RestoreFinished();
    } else if (!state.second->Restored() && state.second->hDriver == nullptr) {
      subDeviceHandles[state.second->hDevice].push_back(state.first);
      state.second->RestoreFinished();
    }
  }
  for (auto& deviceHandle : deviceHandles) {
    uint32_t pCount = static_cast<uint32_t>(deviceHandle.second.size());
    scheduler.Register(new CzeDeviceGet(ZE_RESULT_SUCCESS, deviceHandle.first, &pCount,
                                        deviceHandle.second.data()));
  }
  for (auto& deviceHandle : subDeviceHandles) {
    uint32_t pCount = static_cast<uint32_t>(deviceHandle.second.size());
    scheduler.Register(new CzeDeviceGetSubDevices(ZE_RESULT_SUCCESS, deviceHandle.first, &pCount,
                                                  deviceHandle.second.data()));
  }
}

void RestoreContext(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& state : sd.Map<CContextState>()) {
    if (!state.second->Restored()) {
      auto stateInstance = state.first;
      if (state.second->hDevices.empty()) {
        scheduler.Register(new CzeContextCreate(ZE_RESULT_SUCCESS, state.second->hDriver,
                                                &state.second->desc, &stateInstance));
      } else {
        scheduler.Register(
            new CzeContextCreateEx(ZE_RESULT_SUCCESS, state.second->hDriver, &state.second->desc,
                                   static_cast<uint32_t>(state.second->hDevices.size()),
                                   state.second->hDevices.data(), &stateInstance));
      }
      state.second->RestoreFinished();
    }
  }
}

void RestorePointers(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& state : sd.Map<CAllocState>()) {
    if (!state.second->Restored()) {
      auto stateInstance = state.first;
      switch (state.second->memType) {
      case UnifiedMemoryType::device: {
        if (state.second->allocType == AllocStateType::pointer) {
          scheduler.Register(new CzeMemAllocDevice(
              ZE_RESULT_SUCCESS, state.second->hContext, &state.second->device_desc,
              state.second->size, state.second->alignment, state.second->hDevice, &stateInstance));
        } else if (state.second->allocType == AllocStateType::global_pointer) {
          scheduler.Register(new CzeModuleGetGlobalPointer(ZE_RESULT_SUCCESS, state.second->hModule,
                                                           state.second->name.c_str(),
                                                           &state.second->size, &stateInstance));
          if (!state.second->globalPtrAllocation.empty()) {
            scheduler.Register(
                new CGitsL0MemoryRestore(state.first, state.second->globalPtrAllocation));
          }
        } else if (state.second->allocType == AllocStateType::function_pointer) {
          scheduler.Register(
              new CzeModuleGetFunctionPointer(ZE_RESULT_SUCCESS, state.second->hModule,
                                              state.second->name.c_str(), &stateInstance));
        }
        break;
      }
      case UnifiedMemoryType::host: {
        scheduler.Register(new CzeMemAllocHost(ZE_RESULT_SUCCESS, state.second->hContext,
                                               &state.second->host_desc, state.second->size,
                                               state.second->alignment, &stateInstance));
        break;
      }
      case UnifiedMemoryType::shared: {
        scheduler.Register(new CzeMemAllocShared(
            ZE_RESULT_SUCCESS, state.second->hContext, &state.second->device_desc,
            &state.second->host_desc, state.second->size, state.second->alignment,
            state.second->hDevice, &stateInstance));
        break;
      }
      }
    }
  }

  const auto& l0IFace = gits::CGits::Instance().apis.IfaceCompute();
  for (const auto& state : sd.Map<CCommandListState>()) {
    const auto commandList =
        GetImmediateCommandList(scheduler, state.second->hContext, state.second->hDevice);
    if (l0IFace.CfgRec_IsCommandListToRecord(state.second->cmdListNumber)) {
      auto& appendedKernels = state.second->appendedKernels;
      for (auto& kernelInfo : appendedKernels) {
        if (l0IFace.CfgRec_IsKernelToRecord(kernelInfo->kernelNumber)) {
          auto& stateOfBuffersBeforeKernelLaunch = kernelInfo->stateRestoreBuffers;
          for (const auto& arg : stateOfBuffersBeforeKernelLaunch) {
            if (arg->argType == KernelArgType::buffer) {
              const auto allocInfo = GetAllocFromRegion(arg->h_buf, sd);
              auto& allocState = sd.Get<CAllocState>(allocInfo.first, EXCEPTION_MESSAGE);
              if (allocState.memType == UnifiedMemoryType::device) {
                ScheduleSplitMemoryCopyFromHostPtr(scheduler, arg->buffer.data(), commandList,
                                                   arg->h_buf, 0U, arg->buffer.size());
              } else {
                scheduler.Register(
                    new CGitsL0MemoryRestore(arg->h_buf, arg->buffer.data(), arg->buffer.size()));
              }
              allocState.RestoreFinished();
            } else if (arg->argType == KernelArgType::image) {
              scheduler.Register(new CzeCommandListAppendImageCopyFromMemory(
                  ZE_RESULT_SUCCESS, commandList, arg->h_img, arg->buffer.data(), nullptr, nullptr,
                  0, nullptr));
            }
          }
          if (!stateOfBuffersBeforeKernelLaunch.empty()) {
            scheduler.Register(new CzeCommandListAppendBarrier(ZE_RESULT_SUCCESS, commandList,
                                                               nullptr, 0, nullptr));
            stateOfBuffersBeforeKernelLaunch.clear();
          }
        }
      }
    }
  }

  for (auto& state : sd.Map<CAllocState>()) {
    if (!state.second->Restored()) {
      auto stateInstance = state.first;
      if (state.second->memType == UnifiedMemoryType::device) {
        if (state.second->allocType != AllocStateType::function_pointer) {
          const auto commandList =
              GetImmediateCommandList(scheduler, state.second->hContext, state.second->hDevice);
          std::vector<char> buffer(state.second->size);
          l0::drv.inject.zeCommandListAppendMemoryCopy(commandList, buffer.data(), state.first,
                                                       buffer.size(), nullptr, 0, nullptr);
          ScheduleSplitMemoryCopyFromHostPtr(scheduler, buffer.data(), commandList, state.first, 0U,
                                             buffer.size());
        }
      } else {
        scheduler.Register(new CGitsL0MemoryRestore(stateInstance, state.second->size));
      }
      state.second->RestoreFinished();
    }
  }

  for (const auto& state : sd.Map<CAllocState>()) {
    if (!state.second->indirectPointersOffsets.empty()) {
      std::vector<size_t> indirectPointersOffsets;
      for (const auto& indirectInfo : state.second->indirectPointersOffsets) {
        indirectPointersOffsets.push_back(indirectInfo.first);
      }
      scheduler.Register(new CzeGitsIndirectAllocationOffsets(
          state.first, indirectPointersOffsets.size(), indirectPointersOffsets.data()));
    }
  }
}

void RestoreModules(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& state : sd.Map<CModuleState>()) {
    if (!state.second->Restored()) {
#ifdef WITH_OCLOC
      auto& oclocState = state.second->oclocState;
      if (oclocState && !oclocState->args.empty()) {
        ScheduleOclocInvoke(scheduler, *oclocState);
      }
#endif
      auto stateInstance = state.first;
      scheduler.Register(new CzeModuleCreate_V1(ZE_RESULT_SUCCESS, state.second->hContext,
                                                state.second->hDevice, &state.second->desc,
                                                &stateInstance, &state.second->hBuildLog));
      state.second->RestoreFinished();
    }
  }
}

void RestoreImages(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& state : sd.Map<CImageState>()) {
    if (!state.second->Restored()) {
      auto stateInstance = state.first;
      if (state.second->imageView != nullptr) {
        scheduler.Register(new CzeImageViewCreateExt(ZE_RESULT_SUCCESS, state.second->hContext,
                                                     state.second->hDevice, &state.second->desc,
                                                     state.second->imageView, &stateInstance));
      } else {
        scheduler.Register(new CzeImageCreate(ZE_RESULT_SUCCESS, state.second->hContext,
                                              state.second->hDevice, &state.second->desc,
                                              &stateInstance));
      }
      state.second->RestoreFinished();
    }
  }
}

void RestoreEventPools(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& state : sd.Map<CEventPoolState>()) {
    if (!state.second->Restored()) {
      auto stateInstance = state.first;
      scheduler.Register(new CzeEventPoolCreate(ZE_RESULT_SUCCESS, state.second->hContext,
                                                &state.second->desc, 0, nullptr, &stateInstance));
      state.second->RestoreFinished();
    }
  }
}

void RestoreEvents(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& state : sd.Map<CEventState>()) {
    if (!state.second->Restored()) {
      auto stateInstance = state.first;
      scheduler.Register(new CzeEventCreate(ZE_RESULT_SUCCESS, state.second->hEventPool,
                                            &state.second->desc, &stateInstance));
      availableEvents[stateInstance] = false;
      state.second->RestoreFinished();
    }
  }
}

void RestoreKernels(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& state : sd.Map<CKernelState>()) {
    if (!state.second->Restored()) {
      auto stateInstance = state.first;
      scheduler.Register(new CzeKernelCreate(ZE_RESULT_SUCCESS, state.second->hModule,
                                             &state.second->desc, &stateInstance));
      state.second->RestoreFinished();
    }
  }
}

void RestoreCommandList(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& state : sd.Map<CCommandListState>()) {
    auto stateInstance = state.first;
    const auto& isImmediate = state.second->isImmediate;
    if (IsControlledSubmission(&state.second->listDesc) ||
        IsControlledSubmission(&state.second->queueDesc)) {
      const auto& deviceState = sd.Get<CDeviceState>(state.second->hDevice, EXCEPTION_MESSAGE);
      if (!deviceState.originalQueueGroupProperties.empty()) {
        scheduler.Register(new CGitsL0OriginalQueueFamilyInfo(
            state.second->hDevice, deviceState.originalQueueGroupProperties));
      }
    }
    if (isImmediate) {
      scheduler.Register(new CzeCommandListCreateImmediate(
          ZE_RESULT_SUCCESS, state.second->hContext, state.second->hDevice,
          &state.second->queueDesc, &stateInstance));
      if (gitsImmediateCommandList == nullptr) {
        gitsImmediateCommandList = stateInstance;
      }
    } else {
      scheduler.Register(new CzeCommandListCreate(ZE_RESULT_SUCCESS, state.second->hContext,
                                                  state.second->hDevice, &state.second->listDesc,
                                                  &stateInstance));
    }
    state.second->RestoreFinished();
  }
}

void RestoreCommandListBuffer(CScheduler& scheduler, CStateDynamic& sd) {
  const auto& l0IFace = gits::CGits::Instance().apis.IfaceCompute();
  for (auto& state : sd.Map<CCommandListState>()) {
    auto stateInstance = state.first;
    if (l0IFace.CfgRec_IsCommandListToRecord(state.second->cmdListNumber)) {
      const auto& isImmediate = state.second->isImmediate;
      const auto& appendedKernels = state.second->appendedKernels;
      std::unordered_map<ze_kernel_handle_t, ze_event_handle_t> kernelToEventMap;
      for (const auto& kernelInfo : appendedKernels) {
        if (l0IFace.CfgRec_IsKernelToRecord(kernelInfo->kernelNumber)) {
          if (kernelInfo->isGroupSizeSet) {
            scheduler.Register(new CzeKernelSetGroupSize(
                ZE_RESULT_SUCCESS, kernelInfo->handle, kernelInfo->groupSizeX,
                kernelInfo->groupSizeY, kernelInfo->groupSizeZ));
          }
          if (kernelInfo->isOffsetSet) {
            scheduler.Register(new CzeKernelSetGlobalOffsetExp(
                ZE_RESULT_SUCCESS, kernelInfo->handle, kernelInfo->offsetX, kernelInfo->offsetY,
                kernelInfo->offsetZ));
          }
          if (kernelInfo->indirectUsmTypes != 0) {
            const auto flags =
                static_cast<ze_kernel_indirect_access_flags_t>(kernelInfo->indirectUsmTypes);
            scheduler.Register(
                new CzeKernelSetIndirectAccess(ZE_RESULT_SUCCESS, kernelInfo->handle, flags));
          }
          if (kernelInfo->schedulingHintFlags != 0) {
            ze_scheduling_hint_exp_desc_t desc = {};
            desc.stype = ZE_STRUCTURE_TYPE_SCHEDULING_HINT_EXP_DESC;
            desc.pNext = nullptr;
            desc.flags = kernelInfo->schedulingHintFlags;
            scheduler.Register(
                new CzeKernelSchedulingHintExp(ZE_RESULT_SUCCESS, kernelInfo->handle, &desc));
          }
          auto signalEvent = GetAvailableSignalEvent(scheduler);
          for (const auto& arg : kernelInfo->GetArguments()) {
            scheduler.Register(new CzeKernelSetArgumentValue(ZE_RESULT_SUCCESS, kernelInfo->handle,
                                                             arg.first, arg.second.typeSize,
                                                             arg.second.originalValue));
          }
          const auto& launchArgs = kernelInfo->launchFuncArgs;
          scheduler.Register(new CzeCommandListAppendLaunchKernel(ZE_RESULT_SUCCESS, stateInstance,
                                                                  kernelInfo->handle, &launchArgs,
                                                                  signalEvent, 0, nullptr));
          if (isImmediate && !state.second->isSync) {
            scheduler.Register(
                new CzeEventHostSynchronize(ZE_RESULT_SUCCESS, signalEvent, UINT64_MAX));
          }
        }
      }
      if (!isImmediate) {
        scheduler.Register(new CzeCommandListClose(ZE_RESULT_SUCCESS, stateInstance));
      }
    }
  }
}

void RestoreCommandQueue(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& state : sd.Map<CCommandQueueState>()) {
    if (!state.second->Restored()) {
      auto stateInstance = state.first;
      if (IsControlledSubmission(&state.second->desc)) {
        const auto& deviceState = sd.Get<CDeviceState>(state.second->hDevice, EXCEPTION_MESSAGE);
        if (!deviceState.originalQueueGroupProperties.empty()) {
          scheduler.Register(new CGitsL0OriginalQueueFamilyInfo(
              state.second->hDevice, deviceState.originalQueueGroupProperties));
        }
      }
      scheduler.Register(new CzeCommandQueueCreate(ZE_RESULT_SUCCESS, state.second->hContext,
                                                   state.second->hDevice, &state.second->desc,
                                                   &stateInstance));
      state.second->RestoreFinished();
    }
  }
}

void RestoreFences(CScheduler& scheduler, CStateDynamic& sd) {
  for (auto& state : sd.Map<CFenceState>()) {
    if (!state.second->Restored()) {
      auto stateInstance = state.first;
      scheduler.Register(new CzeFenceCreate(ZE_RESULT_SUCCESS, state.second->hCommandQueue,
                                            &state.second->desc, &stateInstance));
      state.second->RestoreFinished();
    }
  }
}

void CRestoreState::Finish(CScheduler& scheduler) const {
  auto& sd = SD();
  for (auto& state : sd.Map<CEventState>()) {
    if (state.second->Restored()) {
      scheduler.Register(new CzeEventDestroy(ZE_RESULT_SUCCESS, state.first));
    }
  }
  for (auto& state : sd.Map<CEventPoolState>()) {
    if (state.second->Restored()) {
      scheduler.Register(new CzeEventPoolDestroy(ZE_RESULT_SUCCESS, state.first));
    }
  }
  for (auto& state : sd.Map<CFenceState>()) {
    if (state.second->Restored()) {
      scheduler.Register(new CzeFenceDestroy(ZE_RESULT_SUCCESS, state.first));
    }
  }
  for (auto& state : sd.Map<CCommandQueueState>()) {
    if (state.second->Restored()) {
      scheduler.Register(new CzeCommandQueueDestroy(ZE_RESULT_SUCCESS, state.first));
    }
  }
  for (auto& state : sd.Map<CCommandListState>()) {
    if (state.second->Restored()) {
      scheduler.Register(new CzeCommandListDestroy(ZE_RESULT_SUCCESS, state.first));
    }
  }
  for (auto& state : sd.Map<CKernelState>()) {
    if (state.second->Restored()) {
      scheduler.Register(new CzeKernelDestroy(ZE_RESULT_SUCCESS, state.first));
    }
  }
  for (auto& state : sd.Map<CAllocState>()) {
    if (state.second->Restored() && state.second->hModule == nullptr) {
      scheduler.Register(new CzeMemFree(ZE_RESULT_SUCCESS, state.second->hContext, state.first));
    }
  }
  for (auto& state : sd.Map<CImageState>()) {
    if (state.second->Restored()) {
      scheduler.Register(new CzeImageDestroy(ZE_RESULT_SUCCESS, state.first));
    }
  }
  for (auto& state : sd.Map<CModuleState>()) {
    if (state.second->Restored()) {
      scheduler.Register(new CzeModuleDestroy(ZE_RESULT_SUCCESS, state.first));
    }
  }
  for (auto& state : sd.Map<CContextState>()) {
    if (state.second->Restored()) {
      scheduler.Register(new CzeContextDestroy(ZE_RESULT_SUCCESS, state.first));
    }
  }
}
} // namespace l0
} // namespace gits

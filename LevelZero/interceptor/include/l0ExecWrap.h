// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "l0Drivers.h"
#include "l0RecorderWrapperIface.h"
#include "gitsPluginL0.h"

#include "platform.h"

#include <mutex>

namespace {
std::recursive_mutex globalMutex;
// Avoid recording API - recursive functions.
uint32_t recursionDepth = 0;
const uint32_t disableDepth = 1000;
} // namespace

using namespace gits::l0;

void PrePostDisableLevelZero() {
  recursionDepth = disableDepth;
}

#define GITS_WRAPPER_PRE                                                                           \
  --recursionDepth;                                                                                \
  if (CGitsPlugin::Configuration().recorder.basic.enabled && (recursionDepth == 0)) {              \
    try {                                                                                          \
      wrapper.TrackThread();

#define GITS_WRAPPER_POST                                                                          \
  wrapper.CloseRecorderIfRequired();                                                               \
  }                                                                                                \
  catch (...) {                                                                                    \
    topmost_exception_handler(__FUNCTION__);                                                       \
  }                                                                                                \
  }

#define GITS_ENTRY                                                                                 \
  ++recursionDepth;                                                                                \
  CGitsPlugin::Initialize();                                                                       \
  IRecorderWrapper& wrapper = CGitsPlugin::RecorderWrapper();                                      \
  CDriver& driver = wrapper.Drivers();                                                             \
  wrapper.InitializeDriver();

#define GITS_MUTEX    std::unique_lock<std::recursive_mutex> lock(globalMutex);
#define GITS_ENTRY_L0 GITS_MUTEX GITS_ENTRY

#ifdef GITS_PLATFORM_WINDOWS
#define VISIBLE __declspec(dllexport)
#endif

namespace gits {
namespace l0 {
void* GetExtensionFunction(const char* name);

inline ze_result_t zeCommandQueueExecuteCommandLists_RECEXECWRAP(
    ze_command_queue_handle_t hCommandQueue,
    uint32_t numCommandLists,
    ze_command_list_handle_t* phCommandLists,
    ze_fence_handle_t hFence) {
  GITS_ENTRY_L0
  auto return_value = ZE_RESULT_SUCCESS;
  GITS_WRAPPER_PRE
  wrapper.UnProtectMemoryPointers();
  wrapper.zeCommandQueueExecuteCommandLists_pre(return_value, hCommandQueue, numCommandLists,
                                                phCommandLists, hFence);
  return_value = driver.zeCommandQueueExecuteCommandLists(hCommandQueue, numCommandLists,
                                                          phCommandLists, hFence);
  wrapper.zeCommandQueueExecuteCommandLists(return_value, hCommandQueue, numCommandLists,
                                            phCommandLists, hFence);
  wrapper.ProtectMemoryPointers();
  GITS_WRAPPER_POST
  else {
    return_value = driver.zeCommandQueueExecuteCommandLists(hCommandQueue, numCommandLists,
                                                            phCommandLists, hFence);
  }
  return return_value;
}

inline ze_result_t zeCommandListAppendLaunchKernel_RECEXECWRAP(
    ze_command_list_handle_t hCommandList,
    ze_kernel_handle_t hKernel,
    const ze_group_count_t* pLaunchFuncArgs,
    ze_event_handle_t hSignalEvent,
    uint32_t numWaitEvents,
    ze_event_handle_t* phWaitEvents) {
  GITS_ENTRY_L0
  auto return_value = ZE_RESULT_SUCCESS;
  GITS_WRAPPER_PRE
  wrapper.UnProtectMemoryPointers(hCommandList);
  wrapper.zeCommandListAppendLaunchKernel_pre(return_value, hCommandList, hKernel, pLaunchFuncArgs,
                                              hSignalEvent, numWaitEvents, phWaitEvents);
  return_value = driver.zeCommandListAppendLaunchKernel(hCommandList, hKernel, pLaunchFuncArgs,
                                                        hSignalEvent, numWaitEvents, phWaitEvents);
  wrapper.zeCommandListAppendLaunchKernel(return_value, hCommandList, hKernel, pLaunchFuncArgs,
                                          hSignalEvent, numWaitEvents, phWaitEvents);
  wrapper.ProtectMemoryPointers(hCommandList);
  GITS_WRAPPER_POST
  else {
    return_value = driver.zeCommandListAppendLaunchKernel(
        hCommandList, hKernel, pLaunchFuncArgs, hSignalEvent, numWaitEvents, phWaitEvents);
  }
  return return_value;
}

inline ze_result_t zeCommandListAppendLaunchCooperativeKernel_RECEXECWRAP(
    ze_command_list_handle_t hCommandList,
    ze_kernel_handle_t hKernel,
    const ze_group_count_t* pLaunchFuncArgs,
    ze_event_handle_t hSignalEvent,
    uint32_t numWaitEvents,
    ze_event_handle_t* phWaitEvents) {
  GITS_ENTRY_L0
  auto return_value = ZE_RESULT_SUCCESS;
  GITS_WRAPPER_PRE
  wrapper.UnProtectMemoryPointers(hCommandList);
  wrapper.zeCommandListAppendLaunchKernel_pre(return_value, hCommandList, hKernel, pLaunchFuncArgs,
                                              hSignalEvent, numWaitEvents, phWaitEvents);
  return_value = driver.zeCommandListAppendLaunchCooperativeKernel(
      hCommandList, hKernel, pLaunchFuncArgs, hSignalEvent, numWaitEvents, phWaitEvents);
  wrapper.zeCommandListAppendLaunchCooperativeKernel(return_value, hCommandList, hKernel,
                                                     pLaunchFuncArgs, hSignalEvent, numWaitEvents,
                                                     phWaitEvents);
  wrapper.ProtectMemoryPointers(hCommandList);
  GITS_WRAPPER_POST
  else {
    return_value = driver.zeCommandListAppendLaunchCooperativeKernel(
        hCommandList, hKernel, pLaunchFuncArgs, hSignalEvent, numWaitEvents, phWaitEvents);
  }
  return return_value;
}

inline ze_result_t zeCommandListAppendLaunchKernelIndirect_RECEXECWRAP(
    ze_command_list_handle_t hCommandList,
    ze_kernel_handle_t hKernel,
    const ze_group_count_t* pLaunchArgumentsBuffer,
    ze_event_handle_t hSignalEvent,
    uint32_t numWaitEvents,
    ze_event_handle_t* phWaitEvents) {
  GITS_ENTRY_L0
  auto return_value = ZE_RESULT_SUCCESS;
  GITS_WRAPPER_PRE
  wrapper.UnProtectMemoryPointers(hCommandList);
  wrapper.zeCommandListAppendLaunchKernel_pre(return_value, hCommandList, hKernel,
                                              pLaunchArgumentsBuffer, hSignalEvent, numWaitEvents,
                                              phWaitEvents);
  return_value = driver.zeCommandListAppendLaunchKernelIndirect(
      hCommandList, hKernel, pLaunchArgumentsBuffer, hSignalEvent, numWaitEvents, phWaitEvents);
  wrapper.zeCommandListAppendLaunchKernelIndirect(return_value, hCommandList, hKernel,
                                                  pLaunchArgumentsBuffer, hSignalEvent,
                                                  numWaitEvents, phWaitEvents);
  wrapper.ProtectMemoryPointers(hCommandList);
  GITS_WRAPPER_POST
  else {
    return_value = driver.zeCommandListAppendLaunchKernelIndirect(
        hCommandList, hKernel, pLaunchArgumentsBuffer, hSignalEvent, numWaitEvents, phWaitEvents);
  }
  return return_value;
}

inline ze_result_t zeCommandListAppendLaunchMultipleKernelsIndirect_RECEXECWRAP(
    ze_command_list_handle_t hCommandList,
    uint32_t numKernels,
    ze_kernel_handle_t* phKernels,
    const uint32_t* pCountBuffer,
    const ze_group_count_t* pLaunchArgumentsBuffer,
    ze_event_handle_t hSignalEvent,
    uint32_t numWaitEvents,
    ze_event_handle_t* phWaitEvents) {
  GITS_ENTRY_L0
  auto return_value = ZE_RESULT_SUCCESS;
  GITS_WRAPPER_PRE
  wrapper.UnProtectMemoryPointers(hCommandList);
  for (auto i = 0u; i < numKernels; i++) {
    wrapper.zeCommandListAppendLaunchKernel_pre(return_value, hCommandList, phKernels[i],
                                                pLaunchArgumentsBuffer, hSignalEvent, numWaitEvents,
                                                phWaitEvents);
  }
  return_value = driver.zeCommandListAppendLaunchMultipleKernelsIndirect(
      hCommandList, numKernels, phKernels, pCountBuffer, pLaunchArgumentsBuffer, hSignalEvent,
      numWaitEvents, phWaitEvents);
  wrapper.zeCommandListAppendLaunchMultipleKernelsIndirect(
      return_value, hCommandList, numKernels, phKernels, pCountBuffer, pLaunchArgumentsBuffer,
      hSignalEvent, numWaitEvents, phWaitEvents);
  wrapper.ProtectMemoryPointers(hCommandList);
  GITS_WRAPPER_POST
  else {
    return_value = driver.zeCommandListAppendLaunchMultipleKernelsIndirect(
        hCommandList, numKernels, phKernels, pCountBuffer, pLaunchArgumentsBuffer, hSignalEvent,
        numWaitEvents, phWaitEvents);
  }
  return return_value;
}

inline ze_result_t zeDriverGetExtensionFunctionAddress_RECEXECWRAP(ze_driver_handle_t hDriver,
                                                                   const char* name,
                                                                   void** ppFunctionAddress) {
  GITS_ENTRY_L0(void) driver; // unused variable WA
  (void)hDriver;
  auto return_value = ZE_RESULT_SUCCESS;
  *ppFunctionAddress = GetExtensionFunction(name);
  if (*ppFunctionAddress == nullptr) {
    return_value = ZE_RESULT_ERROR_UNSUPPORTED_FEATURE;
  }
  GITS_WRAPPER_PRE
  GITS_WRAPPER_POST
  return return_value;
}

inline ze_result_t zeMemFree_RECEXECWRAP(ze_context_handle_t hContext, void* ptr) {
  GITS_ENTRY_L0
  ze_result_t return_value = ZE_RESULT_SUCCESS;
  GITS_WRAPPER_PRE
  wrapper.DestroySniffedRegion(ptr);
  return_value = driver.zeMemFree(hContext, ptr);
  wrapper.zeMemFree(return_value, hContext, ptr);
  GITS_WRAPPER_POST
  else {
    return_value = driver.zeMemFree(hContext, ptr);
  }
  return return_value;
}

inline ze_result_t zeCommandQueueSynchronize_RECEXECWRAP(ze_command_queue_handle_t hCommandQueue,
                                                         uint64_t timeout) {
  GITS_ENTRY_L0
  auto return_value = ZE_RESULT_SUCCESS;
  GITS_WRAPPER_PRE
  wrapper.UnProtectMemoryPointers();
  return_value = driver.zeCommandQueueSynchronize(hCommandQueue, timeout);
  wrapper.zeCommandQueueSynchronize(return_value, hCommandQueue, timeout);
  wrapper.ProtectMemoryPointers();
  GITS_WRAPPER_POST
  else {
    return_value = driver.zeCommandQueueSynchronize(hCommandQueue, timeout);
  }
  return return_value;
}

inline ze_result_t zeMemFreeExt_RECEXECWRAP(ze_context_handle_t hContext,
                                            const ze_memory_free_ext_desc_t* pMemFreeDesc,
                                            void* ptr) {
  GITS_ENTRY_L0
  ze_result_t return_value = ZE_RESULT_SUCCESS;
  GITS_WRAPPER_PRE
  wrapper.DestroySniffedRegion(ptr);
  return_value = driver.zeMemFreeExt(hContext, pMemFreeDesc, ptr);
  wrapper.zeMemFreeExt(return_value, hContext, pMemFreeDesc, ptr);
  GITS_WRAPPER_POST
  else {
    return_value = driver.zeMemFreeExt(hContext, pMemFreeDesc, ptr);
  }
  return return_value;
}

inline void zeGitsStopRecording_RECEXECWRAP(ze_gits_recording_info_t properties) {
  GITS_ENTRY_L0(void) driver;
  GITS_WRAPPER_PRE
  wrapper.zeGitsStopRecording(properties);
  GITS_WRAPPER_POST
}

inline void zeGitsStartRecording_RECEXECWRAP(ze_gits_recording_info_t properties) {
  GITS_ENTRY_L0(void) driver;
  GITS_WRAPPER_PRE
  wrapper.zeGitsStartRecording(properties);
  GITS_WRAPPER_POST
}
} // namespace l0
} // namespace gits

// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

${AUTO_GENERATED_HEADER}


#include "vulkanExecWrap.h"
#include "vulkanThreadSync.h"

using namespace gits::Vulkan;

void thread_tracker() {
  static std::map<std::thread::id, int> threadIdMap;
  static int currentThreadId = 0;
  static int generatedThreadId = 0;

  //Add thread to map if not mapped
  if (threadIdMap.find(std::this_thread::get_id()) == threadIdMap.end()) {
    threadIdMap[std::this_thread::get_id()] = generatedThreadId;
    generatedThreadId++;
  }

  //If thread changed schedule thread change
  if (currentThreadId != threadIdMap[std::this_thread::get_id()]) {
    currentThreadId = threadIdMap[std::this_thread::get_id()];
    Log(TRACE) << "ThreadID: " << currentThreadId;
  }
}

namespace {
  // Avoid recording API - recursive functions.
  thread_local uint32_t recursionDepth = 0;
  const uint32_t disableDepth = 1000;

  // Global thread synchronization
  std::recursive_mutex globalMutex;
  std::condition_variable_any globalConditionVariable;
  std::unordered_map<VkSemaphore, uint64_t> globalSemaphoreConditionMap;
} // namespace

void PrePostDisableVulkan() {
  recursionDepth = disableDepth;
  CGitsPluginVulkan::_recorderFinished = true;
}

void EndFramePost() {
  CGitsPluginVulkan::RecorderWrapper().EndFramePost();
}

void CloseRecorderIfRequired() {
  CGitsPluginVulkan::RecorderWrapper().CloseRecorderIfRequired();
}

#define VKATTRIB

#define GITS_WRAPPER_PRE                                            ${'\\'}
  --recursionDepth;                                                 ${'\\'}
  if (CGitsPluginVulkan::Configuration().common.recorder.enabled) { ${'\\'}
    if (recursionDepth == 0) {                                      ${'\\'}
      try {

#define GITS_WRAPPER_POST                                           ${'\\'}
        CloseRecorderIfRequired();                                  ${'\\'}
      }                                                             ${'\\'}
      catch (...) {                                                 ${'\\'}
        topmost_exception_handler(__FUNCTION__);                    ${'\\'}
      }                                                             ${'\\'}
    }                                                               ${'\\'}
  }

#define GITS_ENTRY                                                  ${'\\'}
  ++recursionDepth;                                                 ${'\\'}
  thread_tracker();                                                 ${'\\'}
  IRecorderWrapper& wrapper = CGitsPluginVulkan::RecorderWrapper();

#define GITS_MUTEX std::unique_lock<std::recursive_mutex> lock(globalMutex);
#define GITS_ENTRY_VK GITS_MUTEX GITS_ENTRY

extern "C" {
% for token in vk_functions:
<%
    has_retval: bool = token.return_value.type != 'void'
    retval_and_args: list[Argument]
    if has_retval:
        retval_and_args = [token.return_value] + token.args
    else:
        retval_and_args = token.args

    params: str = args_to_str(token.args, '{type} {name}{array}, ', ', ')
    normal_args: str = args_to_str(token.args, '{name}, ', ', ')
    normal_retval_and_args: str = args_to_str(retval_and_args, '{name}, ', ', ')

    store_return_value: str = 'auto return_value = ' if has_retval else ''
    call_prefix: str = 'recExecWrap_' if token.recorder_exec_wrap else 'wrapper.Drivers().'
    plugin_call_prefix: str = 'pluginWrap_' if token.plugin_wrap else 'drvVk.'

    call: str = f'{store_return_value}{call_prefix}{token.name}({normal_args});'

    wrapper_call_args: str
    if token.exec_post_recorder_wrap:
        if token.return_value.type == 'VkResult':
            wrapper_call_args = 'VK_SUCCESS, ' + normal_args
        elif token.return_value.type == 'void':
            wrapper_call_args = normal_args  # Same as normal_retval_and_args.
        else:
            raise NotImplementedError(
                f"Fake return value undefined for type {token.return_value.type}")
    else:
        wrapper_call_args = normal_retval_and_args
%>\

VKATTRIB VISIBLE ${token.return_value.type} VKAPI_CALL ${token.name}(${params}) {
  % if token.level == FuncLevel.GLOBAL:
  CGitsPluginVulkan::Initialize();
  % endif
  GITS_ENTRY_VK
  % if token.wait_operation:
  waitOperation_${token.name}(${normal_args}, lock);
  % endif
  % if not token.exec_post_recorder_wrap:
  ${call}
  % endif
  GITS_WRAPPER_PRE
  wrapper.${token.name}(${wrapper_call_args});
  GITS_WRAPPER_POST
  % if token.end_frame_tag:
  EndFramePost();
  % endif
  % if token.exec_post_recorder_wrap:
  ${call}
  % endif
  % if token.signal_operation:
  signalOperation_${token.name}(${normal_args});
  % endif
  % if has_retval:
  return return_value;
  % endif
}
% endfor
} // extern "C"

const std::unordered_map<std::string, PFN_vkVoidFunction> interceptorExportedFunctions = {
% for token in vk_functions:
  {"${token.name}", (PFN_vkVoidFunction)${token.name}},
% endfor
};

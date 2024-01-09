// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "vulkanHeader.h"
#include "dynamic_linker.h"

#include <memory>
#include <unordered_map>

#ifndef VOID_T_DEFINED
#define VOID_T_DEFINED
typedef struct void_type_tag {
}* void_t;
#endif

namespace gits {
namespace Vulkan {

using std::uint64_t;

struct CVkGlobalDispatchTable {
#define VK_GLOBAL_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call)   \
  return_type(STDCALL* function_name) function_arguments;

#include "vulkanDriversAuto.inl"
};

struct CVkInstanceDispatchTable {
  VkInstance instanceHandle;
  uint32_t majorVersion;
  uint32_t minorVersion;

#define VK_INSTANCE_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call, \
                                   first_argument_name)                                            \
  return_type(STDCALL* function_name) function_arguments;

#include "vulkanDriversAuto.inl"
};

struct CVkDeviceDispatchTable {
  VkInstance instanceHandle;
  VkPhysicalDevice physicalDeviceHandle;
  VkDevice deviceHandle;

#define VK_DEVICE_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call,   \
                                 first_argument_name)                                              \
  return_type(STDCALL* function_name) function_arguments;

#include "vulkanDriversAuto.inl"
};

class CVkDriver {
public:
  enum class DriverMode { INTERCEPTOR, LAYER };
  CVkDriver();
  ~CVkDriver();
  CVkDriver(const CVkDriver& other) = delete;
  CVkDriver& operator=(const CVkDriver& other) = delete;

  void Initialize();
  VkResult Initialize(const VkInstanceCreateInfo* pCreateInfo,
                      const VkAllocationCallbacks* pAllocator,
                      VkInstance* pInstance);
  VkResult Initialize(VkPhysicalDevice physicalDevice,
                      const VkDeviceCreateInfo* pCreateInfo,
                      const VkAllocationCallbacks* pAllocator,
                      VkDevice* pDevice);
  void InitializeUnifiedAPI(const VkDeviceCreateInfo* pCreateInfo,
                            CVkInstanceDispatchTable& instanceDispatchTable,
                            CVkDeviceDispatchTable& deviceDispatchTable);
  void Destroy(VkInstance instance, const VkAllocationCallbacks* pAllocator);
  void Destroy(VkDevice device, const VkAllocationCallbacks* pAllocator);

  void SetMode(DriverMode mode);
  void SetDispatchKey(VkDevice device, void* obj);

  CVkGlobalDispatchTable& GetGlobalDispatchTable() {
    return GlobalDispatchTable;
  }

  template <typename HANDLE>
  CVkInstanceDispatchTable& GetInstanceDispatchTable(HANDLE handle) {
    auto dispatchMapKey = GetDispatchKey(handle);
    auto iterator = InstanceDispatchTables.find(dispatchMapKey);
    if (iterator == InstanceDispatchTables.end()) {
      auto insert_result = InstanceDispatchTables.insert(
          std::make_pair(dispatchMapKey, std::make_unique<CVkInstanceDispatchTable>()));
      *insert_result.first->second = {};
      iterator = insert_result.first;
    }
    return *iterator->second;
  }

  template <typename HANDLE>
  CVkDeviceDispatchTable& GetDeviceDispatchTable(HANDLE handle) {
    auto dispatchMapKey = GetDispatchKey(handle);
    auto iterator = DeviceDispatchTables.find(dispatchMapKey);
    if (iterator == DeviceDispatchTables.end()) {
      auto insert_result = DeviceDispatchTables.insert(
          std::make_pair(dispatchMapKey, std::make_unique<CVkDeviceDispatchTable>()));
      *insert_result.first->second = {};
      iterator = insert_result.first;
    }
    return *iterator->second;
  }

#define VK_GLOBAL_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call)   \
  return_type(STDCALL* function_name) function_arguments;
#define VK_INSTANCE_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call, \
                                   first_argument_name)                                            \
  return_type(STDCALL* function_name) function_arguments;
#define VK_DEVICE_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call,   \
                                 first_argument_name)                                              \
  return_type(STDCALL* function_name) function_arguments;

#include "vulkanDriversAuto.inl"

private:
  static inline void* GetDispatchKey(const void* object) {
    return (void*)*(CVkDeviceDispatchTable**)object;
  }

  dl::SharedObject _lib;
  DriverMode Mode;
  CVkGlobalDispatchTable GlobalDispatchTable;
  std::unordered_map<void*, std::unique_ptr<CVkInstanceDispatchTable>> InstanceDispatchTables;
  std::unordered_map<void*, std::unique_ptr<CVkDeviceDispatchTable>> DeviceDispatchTables;
};

extern CVkDriver drvVk;

/**
    * @brief List of GITS-specific, fake function names used to communicate GITS-specific behavior to VK Shims or GITS recorder.
    *
    * They are defined here because this file is shared between both GITS and VK Shims
    */

#define VK_UNWIND_QUEUE_PRESENT_GITS_FUNCTION_NAME                                                 \
  "vkUnwindQueuePresentGITS" // Used for rewinding swapchain image indices
#define VK_FAKE_QUEUE_PRESENT_GITS_FUNCTION_NAME                                                   \
  "vkFakeQueuePresentGITS" // Used for offscreen rendering
#define VK_SWAP_AFTER_PREPARE_GITS_FUNCTION_NAME                                                   \
  "vkSwapAfterPrepareGITS" // Used for swapAfterPrepare marker
#define VK_STATE_RESTORE_BEGIN_GITS_FUNCTION_NAME "vkStateRestoreBeginGITS"
#define VK_STATE_RESTORE_END_GITS_FUNCTION_NAME   "vkStateRestoreEndGITS"
#define VK_PASS_PHYSICAL_DEVICE_MEMORY_PROPERTIES_GITS_FUNCTION_NAME                               \
  "vkPassPhysicalDeviceMemoryPropertiesGITS"
#define VK_TAG_MEMORY_CONTENTS_UPDATE_GITS_FUNCTION_NAME "vkTagMemoryContentsUpdateGITS"
#define VK_I_AM_GITS_FUNCTION_NAME                       "vkIAmGITS"
#define VK_PAUSE_RECORDING_GITS_FUNCTION_NAME            "vkPauseRecordingGITS"
#define VK_CONTINUE_RECORDING_GITS_FUNCTION_NAME         "vkContinueRecordingGITS"
} // namespace Vulkan
} // namespace gits

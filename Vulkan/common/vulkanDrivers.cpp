// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   vulkanDrivers.cpp
*
* @brief
*/

#include "vk_layer.h"
#include "vulkanDrivers.h"
#include "vulkanTracerAuto.h"

#include "gits.h"
#include "configurationLib.h"
#include "lua_bindings.h"
#include "vulkanTools.h"

namespace gits {
namespace lua {
#include "vulkanLuaEnums.h"
}
} // namespace gits

namespace gits {
namespace Vulkan {
using std::uint64_t;

NOINLINE void LogFunctionBeforeContext(const char* func) {
  LOG_WARNING << "Function " << func << " called before any context creation";
}

//==========================================================================================================//
// LUA
//==========================================================================================================//

using namespace lua;
static bool bypass_luascript;

#define LUA_FUNCTION(return_type, function_name, function_arguments)                               \
  int lua_##function_name(lua_State* L) {                                                          \
    int top = lua_gettop(L);                                                                       \
    if (top != Argnum<return_type function_arguments>::value) {                                    \
      luaL_error(L, "invalid number of parameters");                                               \
    }                                                                                              \
                                                                                                   \
    FuncToTuple<return_type function_arguments>::type args;                                        \
    fill_tuple(L, args);                                                                           \
    bypass_luascript = true;                                                                       \
    return_type ret = call_tuple<return_type>(gits::Vulkan::drvVk.function_name, args);            \
    bypass_luascript = false;                                                                      \
    gits::lua::lua_push(L, ret);                                                                   \
                                                                                                   \
    return 1;                                                                                      \
  }

#define VK_GLOBAL_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call)   \
  LUA_FUNCTION(return_type, function_name, function_arguments)
#define VK_INSTANCE_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call, \
                                   first_argument_name)                                            \
  LUA_FUNCTION(return_type, function_name, function_arguments)
#define VK_DEVICE_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call,   \
                                 first_argument_name)                                              \
  LUA_FUNCTION(return_type, function_name, function_arguments)

#include "vulkanDriversAuto.inl"

namespace {

int export_WaitForAllVkDevices(lua_State* L) {
  waitForAllDevices();
  return 0;
}

const luaL_Reg exports[] = {
#define VK_GLOBAL_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call)   \
  {#function_name, lua_##function_name},
#define VK_INSTANCE_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call, \
                                   first_argument_name)                                            \
  {#function_name, lua_##function_name},
#define VK_DEVICE_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call,   \
                                 first_argument_name)                                              \
  {#function_name, lua_##function_name},

#include "vulkanDriversAuto.inl"
    {"waitForAllVkDevices", export_WaitForAllVkDevices}, {nullptr, nullptr}};
} // namespace

void RegisterLuaVulkanDriverFunctions() {
  auto L = CGits::Instance().GetLua();
  luaL_newlib(L.get(), exports);
  lua_setglobal(L.get(), "drvVk");
}

//==========================================================================================================//
// Default dispatch functions
//==========================================================================================================//

// Manually handled default_ API functions

VkResult STDCALL
default_vkNegotiateLoaderLayerInterfaceVersion(VkNegotiateLayerInterface* /* pVersionStruct */) {
  return VK_SUCCESS;
}

PFN_vkVoidFunction STDCALL default_vkGetInstanceProcAddr(VkInstance instance, const char* pName) {
  drvVk.Initialize();

  auto& dispatchTable = drvVk.GetGlobalDispatchTable();
  if (dispatchTable.vkGetInstanceProcAddr) {
    return dispatchTable.vkGetInstanceProcAddr(instance, pName);
  }
  return nullptr;
}

VkResult STDCALL default_vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo,
                                          const VkAllocationCallbacks* pAllocator,
                                          VkInstance* pInstance) {
  drvVk.Initialize();
  return drvVk.Initialize(pCreateInfo, pAllocator, pInstance);
}

void_t STDCALL default_vkDestroyInstance(VkInstance instance,
                                         const VkAllocationCallbacks* pAllocator) {
  drvVk.Destroy(instance, pAllocator);
  return nullptr;
}

PFN_vkVoidFunction STDCALL default_GetPhysicalDeviceProcAddr(VkInstance instance,
                                                             const char* pName) {
  auto& dispatchTable = drvVk.GetInstanceDispatchTable(instance);
  if (dispatchTable.GetPhysicalDeviceProcAddr) {
    return dispatchTable.GetPhysicalDeviceProcAddr(instance, pName);
  }
  return nullptr;
}

PFN_vkVoidFunction STDCALL default_vkGetDeviceProcAddr(VkDevice device, const char* pName) {
  auto& dispatchTable = drvVk.GetDeviceDispatchTable(device);
  if (dispatchTable.vkGetDeviceProcAddr) {
    return dispatchTable.vkGetDeviceProcAddr(device, pName);
  }
  return nullptr;
}

void_t STDCALL default_vkPassPhysicalDeviceMemoryPropertiesGITS(
    VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties* pMemoryProperties) {
  // After added mapping for memory type indexes function is ignored
  return nullptr;
}

VkResult STDCALL default_vkCreateDevice(VkPhysicalDevice physicalDevice,
                                        const VkDeviceCreateInfo* pCreateInfo,
                                        const VkAllocationCallbacks* pAllocator,
                                        VkDevice* pDevice) {
  return drvVk.Initialize(physicalDevice, pCreateInfo, pAllocator, pDevice);
}

void_t STDCALL default_vkDestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator) {
  drvVk.Destroy(device, pAllocator);
  return nullptr;
}

void_t STDCALL default_vkTagMemoryContentsUpdateGITS(VkDevice device,
                                                     VkDeviceMemory memory,
                                                     uint32_t regionCount,
                                                     const VkBufferCopy* pRegions) {
  auto& dispatchTable = drvVk.GetDeviceDispatchTable(device);
  if (dispatchTable.vkTagMemoryContentsUpdateGITS) {
    dispatchTable.vkTagMemoryContentsUpdateGITS(device, memory, regionCount, pRegions);
  }
  return nullptr;
}

void_t STDCALL default_vkPauseRecordingGITS() {
  auto& dispatchTable = drvVk.GetGlobalDispatchTable();
  if (dispatchTable.vkPauseRecordingGITS) {
    dispatchTable.vkPauseRecordingGITS();
  }
  return nullptr;
}

void_t STDCALL default_vkContinueRecordingGITS() {
  auto& dispatchTable = drvVk.GetGlobalDispatchTable();
  if (dispatchTable.vkContinueRecordingGITS) {
    dispatchTable.vkContinueRecordingGITS();
  }
  return nullptr;
}

void_t STDCALL default_vkIAmRecorderGITS() {
  // No call on purpose (function is exported from a GITS recorder and is used
  // only to indicate whether the recorder is attached to a player)
  return nullptr;
}

#define DEFAULT_FUNCTION(return_type, function_name, function_arguments, arguments_call, level,    \
                         first_argument_name, drv_initialization)                                  \
  return_type STDCALL default_##function_name function_arguments {                                 \
    drv_initialization /* <- used in global_level functions only */                                \
        return drvVk.Get##level##DispatchTable(first_argument_name)                                \
            .function_name arguments_call;                                                         \
  }

// Prepare default_ API for all other Vulkan functions (but skip custom
// functions - custom functions are handled manually)

#define VK_GLOBAL_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call)   \
  DEFAULT_FUNCTION(return_type, function_name, function_arguments, arguments_call, Global,         \
                   /* empty */, drvVk.Initialize();)
#define VK_INSTANCE_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call, \
                                   first_argument_name)                                            \
  DEFAULT_FUNCTION(return_type, function_name, function_arguments, arguments_call, Instance,       \
                   first_argument_name,                                                            \
                   /* empty */)
#define VK_DEVICE_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call,   \
                                 first_argument_name)                                              \
  DEFAULT_FUNCTION(return_type, function_name, function_arguments, arguments_call, Device,         \
                   first_argument_name,                                                            \
                   /* empty */)
#define VK_CUSTOM_GLOBAL_LEVEL_FUNCTION(return_type, function_name, function_arguments,            \
                                        arguments_call) /* intentionally left blank */
#define VK_CUSTOM_INSTANCE_LEVEL_FUNCTION(return_type, function_name, function_arguments,          \
                                          arguments_call,                                          \
                                          first_argument_name) /* intentionally left blank */
#define VK_CUSTOM_DEVICE_LEVEL_FUNCTION(return_type, function_name, function_arguments,            \
                                        arguments_call,                                            \
                                        first_argument_name) /* intentionally left blank */

#include "vulkanDriversAuto.inl"

//==========================================================================================================//
// Special dispatch functions (lua script calling and tracing)
//==========================================================================================================//

NOINLINE bool UseSpecial(const char* func) {
  const auto& cfg = Configurator::Get();
  return log::ShouldLog(LogLevel::TRACE) ||
         (cfg.common.shared.useEvents &&
          lua::FunctionExists(func, CGits::Instance().GetLua().get())) ||
         (cfg.common.player.exitOnError) || (!cfg.common.player.traceSelectedFrames.empty());
}

template <class T>
void checkReturnValue(const char* name, T gits_ret) {
  if (gits_ret < 0) {
    std::ostringstream error;
    error << name << " function returned the " << gits_ret << " error!\n";
    error << "Play the stream with \"--trace\" option for more details!\n";
    throw std::runtime_error(error.str());
  }
}

template <>
void checkReturnValue<void_t>(const char*, void_t) {}

template <>
void checkReturnValue<PFN_vkVoidFunction>(const char*, PFN_vkVoidFunction) {}

#define SPECIAL_FUNCTION(return_type, function_name, function_arguments, arguments_call,           \
                         first_argument_name)                                                      \
  return_type STDCALL special_##function_name function_arguments {                                 \
    const Configuration& gits_cfg = Configurator::Get();                                           \
    bool doTrace = log::ShouldLog(LogLevel::TRACE);                                                \
    if (doTrace) {                                                                                 \
      LOG_FORMAT_RAW                                                                               \
      LOG_TRACE << #function_name;                                                                 \
    }                                                                                              \
    return_type gits_ret = (return_type)0;                                                         \
    bool call_shd = true;                                                                          \
    if (gits_cfg.common.shared.useEvents && !bypass_luascript) {                                   \
      auto L = CGits::Instance().GetLua().get();                                                   \
      bool exists = !doTrace || lua::FunctionExists(#function_name, L);                            \
      if (exists) {                                                                                \
        LUA_CALL_FUNCTION(L, #function_name, arguments_call, function_arguments)                   \
        call_shd = false;                                                                          \
        int top = lua_gettop(L);                                                                   \
        gits_ret = lua::lua_to<return_type>(L, top);                                               \
        lua_pop(L, top);                                                                           \
      }                                                                                            \
    }                                                                                              \
    if (call_shd) {                                                                                \
      gits_ret = default_##function_name arguments_call;                                           \
    }                                                                                              \
    if (doTrace) {                                                                                 \
      function_name##_trace arguments_call;                                                        \
      trace_return_value(gits_ret);                                                                \
    }                                                                                              \
    if (gits_cfg.common.player.exitOnError) {                                                      \
      checkReturnValue(#function_name, gits_ret);                                                  \
    }                                                                                              \
    return gits_ret;                                                                               \
  }

#define VK_GLOBAL_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call)   \
  SPECIAL_FUNCTION(return_type, function_name, function_arguments, arguments_call, )
#define VK_INSTANCE_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call, \
                                   first_argument_name)                                            \
  SPECIAL_FUNCTION(return_type, function_name, function_arguments, arguments_call,                 \
                   first_argument_name)
#define VK_DEVICE_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call,   \
                                 first_argument_name)                                              \
  SPECIAL_FUNCTION(return_type, function_name, function_arguments, arguments_call,                 \
                   first_argument_name)

#include "vulkanDriversAuto.inl"

//==========================================================================================================//
// CVkDriver member functions definitions
//==========================================================================================================//

namespace {
VkLayerDeviceCreateInfo* GetChainInfo(const VkDeviceCreateInfo* pCreateInfo) {
  auto chainInfo = (VkLayerDeviceCreateInfo*)pCreateInfo->pNext;

  while (chainInfo && !(chainInfo->sType == VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO &&
                        chainInfo->function == VK_LAYER_LINK_INFO)) {
    chainInfo = (VkLayerDeviceCreateInfo*)chainInfo->pNext;
  }
  return chainInfo;
}
} // namespace

CVkDriver::CVkDriver() : _lib(nullptr), Mode(DriverMode::INTERCEPTOR), GlobalDispatchTable() {
#define VK_GLOBAL_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call)   \
  this->function_name = default_##function_name;
#define VK_INSTANCE_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call, \
                                   first_argument_name)                                            \
  this->function_name = default_##function_name;
#define VK_DEVICE_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call,   \
                                 first_argument_name)                                              \
  this->function_name = default_##function_name;

#include "vulkanDriversAuto.inl"

  CGits::Instance().RegisterLuaFunctionsRegistrator(RegisterLuaVulkanDriverFunctions);
}

CVkDriver::~CVkDriver() {
  dl::close_library(_lib);
}

void CVkDriver::Initialize() {
  CALL_ONCE[&] {
    Configurator::Get();
    LOG_INFO << "Initializing Vulkan API";

#define VK_GLOBAL_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call)   \
  if (UseSpecial(#function_name)) {                                                                \
    this->function_name = special_##function_name;                                                 \
  }
#define VK_INSTANCE_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call, \
                                   first_argument_name)                                            \
  if (UseSpecial(#function_name)) {                                                                \
    this->function_name = special_##function_name;                                                 \
  }
#define VK_DEVICE_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call,   \
                                 first_argument_name)                                              \
  if (UseSpecial(#function_name)) {                                                                \
    this->function_name = special_##function_name;                                                 \
  }

#include "vulkanDriversAuto.inl"

    if (Mode == DriverMode::INTERCEPTOR) {
      _lib = dl::open_library(Configurator::Get().common.shared.libVK.string().c_str());
      if (!_lib) {
        std::string errorMessage =
            std::string("Could not find Vulkan Loader in the specified path: ") +
            Configurator::Get().common.shared.libVK.string();
        throw std::runtime_error(errorMessage);
      }

      GlobalDispatchTable.vkGetInstanceProcAddr =
          (PFN_vkGetInstanceProcAddr)dl::load_symbol(_lib, "vkGetInstanceProcAddr");

#define VK_GLOBAL_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call)   \
  if (GlobalDispatchTable.function_name == nullptr) {                                              \
    GlobalDispatchTable.function_name =                                                            \
        (return_type(STDCALL*) function_arguments)GlobalDispatchTable.vkGetInstanceProcAddr(       \
            VK_NULL_HANDLE, #function_name);                                                       \
  }                                                                                                \
  if (GlobalDispatchTable.function_name == nullptr) {                                              \
    GlobalDispatchTable.function_name =                                                            \
        (return_type(STDCALL*) function_arguments)dl::load_symbol(_lib, #function_name);           \
  }

#include "vulkanDriversAuto.inl"
    }

    // Inform GITS recorder that this is the GITS Player
    if (GlobalDispatchTable.vkIAmGITS) {
      GlobalDispatchTable.vkIAmGITS();
    }

    return true;
  };
}

VkResult CVkDriver::Initialize(const VkInstanceCreateInfo* pCreateInfo,
                               const VkAllocationCallbacks* pAllocator,
                               VkInstance* pInstance) {
  if (Mode == DriverMode::LAYER) {
    auto chainInfo = (VkLayerInstanceCreateInfo*)pCreateInfo->pNext;

    while (chainInfo && !(chainInfo->sType == VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO &&
                          chainInfo->function == VK_LAYER_LINK_INFO)) {
      chainInfo = (VkLayerInstanceCreateInfo*)chainInfo->pNext;
    }

    if (chainInfo != nullptr) {
      GlobalDispatchTable.vkGetInstanceProcAddr =
          chainInfo->u.pLayerInfo->pfnNextGetInstanceProcAddr;
      GlobalDispatchTable.vkCreateInstance =
          (PFN_vkCreateInstance)GlobalDispatchTable.vkGetInstanceProcAddr(nullptr,
                                                                          "vkCreateInstance");
      if (GlobalDispatchTable.vkCreateInstance == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
      }
      chainInfo->u.pLayerInfo = chainInfo->u.pLayerInfo->pNext;
    } else {
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }
  }

  VkResult return_value = GlobalDispatchTable.vkCreateInstance(pCreateInfo, pAllocator, pInstance);

  if ((return_value == VK_SUCCESS) && (*pInstance != VK_NULL_HANDLE)) {

    auto& instanceDispatchTable = GetInstanceDispatchTable(*pInstance);
    instanceDispatchTable.instanceHandle = *pInstance;
    if (pCreateInfo->pApplicationInfo != nullptr) {
      instanceDispatchTable.majorVersion =
          VK_VERSION_MAJOR(pCreateInfo->pApplicationInfo->apiVersion);
      instanceDispatchTable.minorVersion =
          VK_VERSION_MINOR(pCreateInfo->pApplicationInfo->apiVersion);
    } else {
      instanceDispatchTable.majorVersion = 1;
      instanceDispatchTable.minorVersion = 0;
    }

#define VK_INSTANCE_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call, \
                                   first_argument_name)                                            \
  if (instanceDispatchTable.function_name == nullptr) {                                            \
    instanceDispatchTable.function_name =                                                          \
        (return_type(STDCALL*) function_arguments)GlobalDispatchTable.vkGetInstanceProcAddr(       \
            *pInstance, #function_name);                                                           \
  }

#include "vulkanDriversAuto.inl"
  }
  return return_value;
}

VkResult CVkDriver::Initialize(VkPhysicalDevice physicalDevice,
                               const VkDeviceCreateInfo* pCreateInfo,
                               const VkAllocationCallbacks* pAllocator,
                               VkDevice* pDevice) {
  auto& instanceDispatchTable = GetInstanceDispatchTable(physicalDevice);

  if (Mode == DriverMode::LAYER) {
    VkLayerDeviceCreateInfo* chainInfo = GetChainInfo(pCreateInfo);
    GlobalDispatchTable.vkGetInstanceProcAddr = chainInfo->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    instanceDispatchTable.vkCreateDevice =
        (PFN_vkCreateDevice)GlobalDispatchTable.vkGetInstanceProcAddr(
            instanceDispatchTable.instanceHandle, "vkCreateDevice");
    if (instanceDispatchTable.vkCreateDevice == nullptr) {
      return VK_ERROR_INITIALIZATION_FAILED;
    }
  }

  VkResult return_value =
      instanceDispatchTable.vkCreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);

  if ((return_value == VK_SUCCESS) && (*pDevice != VK_NULL_HANDLE)) {
    auto& deviceDispatchTable = GetDeviceDispatchTable(*pDevice);
    deviceDispatchTable.instanceHandle = instanceDispatchTable.instanceHandle;
    deviceDispatchTable.physicalDeviceHandle = physicalDevice;
    deviceDispatchTable.deviceHandle = *pDevice;

    if (Mode == DriverMode::LAYER) {
      VkLayerDeviceCreateInfo* chainInfo = GetChainInfo(pCreateInfo);
      deviceDispatchTable.vkGetDeviceProcAddr = chainInfo->u.pLayerInfo->pfnNextGetDeviceProcAddr;
      chainInfo->u.pLayerInfo = chainInfo->u.pLayerInfo->pNext;
    } else {
      deviceDispatchTable.vkGetDeviceProcAddr =
          (PFN_vkVoidFunction(STDCALL*)(VkDevice device, const char* pName))GlobalDispatchTable
              .vkGetInstanceProcAddr(instanceDispatchTable.instanceHandle, "vkGetDeviceProcAddr");
    }

#define VK_DEVICE_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call,   \
                                 first_argument_name)                                              \
  if (deviceDispatchTable.function_name == nullptr) {                                              \
    deviceDispatchTable.function_name =                                                            \
        (return_type(STDCALL*)                                                                     \
             function_arguments)deviceDispatchTable.vkGetDeviceProcAddr(*pDevice, #function_name); \
  }

#include "vulkanDriversAuto.inl"

    InitializeUnifiedAPI(pCreateInfo, instanceDispatchTable, deviceDispatchTable);
  }

  return return_value;
}

void CVkDriver::InitializeUnifiedAPI(const VkDeviceCreateInfo* pCreateInfo,
                                     CVkInstanceDispatchTable& instanceDispatchTable,
                                     CVkDeviceDispatchTable& deviceDispatchTable) {
  // There are different variations of various functions, introduced by
  // extensions and later incorporated into higher, core versions i.e.
  // vkGetBufferDeviceAddress() and vkGetBufferDeviceAddressKHR(), or
  // vkCmdPipelineBarrier2() and vkCmdPipelineBarrier2KHR(). To ease the
  // record/playback, and to not have to check every time which version of
  // the function should be called (for example in state restore), a custom
  // unified function is introduced, which is being initialized here
  // depending on the API version and extensions enabled by the app.
  //
  // This way GITS can call only a single, "unified" function but underneath
  // the pointer is initialized according to API version and available ext.

  // Core 1.3+ version
  if (instanceDispatchTable.minorVersion >= 3) {
    deviceDispatchTable.vkCmdPipelineBarrier2UnifiedGITS =
        deviceDispatchTable.vkCmdPipelineBarrier2;
  }

  // Core 1.2+ version
  if (instanceDispatchTable.minorVersion >= 2) {
    deviceDispatchTable.vkGetBufferDeviceAddressUnifiedGITS =
        deviceDispatchTable.vkGetBufferDeviceAddress;
    deviceDispatchTable.vkGetBufferOpaqueCaptureAddressUnifiedGITS =
        deviceDispatchTable.vkGetBufferOpaqueCaptureAddress;
    deviceDispatchTable.vkGetDeviceMemoryOpaqueCaptureAddressUnifiedGITS =
        deviceDispatchTable.vkGetDeviceMemoryOpaqueCaptureAddress;
    deviceDispatchTable.vkWaitSemaphoresUnifiedGITS = deviceDispatchTable.vkWaitSemaphores;
    deviceDispatchTable.vkGetSemaphoreCounterValueUnifiedGITS =
        deviceDispatchTable.vkGetSemaphoreCounterValue;
  }

  // Core 1.0 version
  if ((instanceDispatchTable.majorVersion == 1) && (instanceDispatchTable.minorVersion == 0)) {
    deviceDispatchTable.vkGetDeviceQueue2 = nullptr;
  }

  // Enabled extensions
  for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; ++i) {
    auto element = pCreateInfo->ppEnabledExtensionNames[i];
    // VK_KHR_buffer_device_address
    if (strcmp(element, VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) == 0) {
      deviceDispatchTable.vkGetBufferDeviceAddressUnifiedGITS =
          deviceDispatchTable.vkGetBufferDeviceAddressKHR;
      deviceDispatchTable.vkGetBufferOpaqueCaptureAddressUnifiedGITS =
          deviceDispatchTable.vkGetBufferOpaqueCaptureAddressKHR;
      deviceDispatchTable.vkGetDeviceMemoryOpaqueCaptureAddressUnifiedGITS =
          deviceDispatchTable.vkGetDeviceMemoryOpaqueCaptureAddressKHR;
    }
    // VK_EXT_buffer_device_address
    else if (strcmp(element, VK_EXT_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) == 0) {
      deviceDispatchTable.vkGetBufferDeviceAddressUnifiedGITS =
          deviceDispatchTable.vkGetBufferDeviceAddressEXT;
    }
    // VK_KHR_synchronization2
    else if (strcmp(element, VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME) == 0) {
      deviceDispatchTable.vkCmdPipelineBarrier2UnifiedGITS =
          deviceDispatchTable.vkCmdPipelineBarrier2KHR;
    }
    // VK_KHR_timeline_semaphore
    else if (strcmp(element, VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME) == 0) {
      deviceDispatchTable.vkWaitSemaphoresUnifiedGITS = deviceDispatchTable.vkWaitSemaphoresKHR;
      deviceDispatchTable.vkGetSemaphoreCounterValueUnifiedGITS =
          deviceDispatchTable.vkGetSemaphoreCounterValueKHR;
    }
  }

  // Ray tracing - only KHR version available at the moment, but core version expected to be added in the future

  for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; ++i) {
    auto element = pCreateInfo->ppEnabledExtensionNames[i];
    // KHR version
    if (strcmp(element, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) == 0) {
      deviceDispatchTable.vkGetAccelerationStructureDeviceAddressUnifiedGITS =
          deviceDispatchTable.vkGetAccelerationStructureDeviceAddressKHR;
      break;
    }
  }
}

void CVkDriver::Destroy(VkInstance instance, const VkAllocationCallbacks* pAllocator) {
  if (instance == nullptr) {
    LOG_INFO << "vkDestroyInstance was called on nullptr handle";
    return;
  }

  auto dispatchMapKey = GetDispatchKey(instance);

  drvVk.GetInstanceDispatchTable(instance).vkDestroyInstance(instance, pAllocator);

  InstanceDispatchTables.erase(dispatchMapKey);
}

void CVkDriver::Destroy(VkDevice device, const VkAllocationCallbacks* pAllocator) {
  if (device == nullptr) {
    LOG_INFO << "vkDestroyDevice was called on nullptr handle";
    return;
  }

  auto dispatchMapKey = GetDispatchKey(device);

  drvVk.GetDeviceDispatchTable(device).vkDestroyDevice(device, pAllocator);

  DeviceDispatchTables.erase(dispatchMapKey);
}

void CVkDriver::SetMode(DriverMode mode) {
  Mode = mode;
}

void CVkDriver::SetDispatchKey(VkDevice device, void* obj) {
  if (Mode == DriverMode::LAYER) {
    auto dispatchMapKey = GetDispatchKey(device);
    *((const void**)obj) = (const void*)dispatchMapKey;
  }
}

CVkDriver drvVk;

} // namespace Vulkan
} // namespace gits

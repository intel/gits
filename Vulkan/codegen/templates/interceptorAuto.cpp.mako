// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#define VK_NO_PROTOTYPES
#include "vulkanHeader2.h"
#include "gitsLoader.h"
#include "vulkanRecorderInterface.h"

#include <mutex>

#if defined(GITS_PLATFORM_WINDOWS)
#define VK_INTERCEPTOR_EXPORT extern "C" __declspec(dllexport)
#elif defined(GITS_PLATFORM_X11)
#define VK_INTERCEPTOR_EXPORT extern "C" __attribute__((visibility("default")))
#endif

std::unique_ptr<gits::CGitsLoader> g_GitsLoader;
gits::vulkan::IRecorderWrapper* g_RecorderWrapper = nullptr;
PFN_vkGetInstanceProcAddr g_vkGetInstanceProcAddr = nullptr;

class VulkanLibHandler {
public:
  explicit VulkanLibHandler(const char* name) : m_VulkanLib(dl::open_library(name)) {}
  ~VulkanLibHandler() {
    if (m_VulkanLib) {
      dl::close_library(m_VulkanLib);
      m_VulkanLib = nullptr;
    }
  }

  VulkanLibHandler(const VulkanLibHandler&) = delete;
  VulkanLibHandler& operator=(const VulkanLibHandler&) = delete;

  dl::SharedObject Get() const { return m_VulkanLib; }

private:
  dl::SharedObject m_VulkanLib = nullptr;
};

std::unique_ptr<VulkanLibHandler> g_VulkanLibHandler; // Ensures Vulkan library is closed when the program exits

void Initialize() {
  static std::once_flag initFlag;
  std::call_once(initFlag, []() {
    g_GitsLoader = std::make_unique<gits::CGitsLoader>("GITSRecorderVulkan2", false);
    g_RecorderWrapper = reinterpret_cast<gits::vulkan::IRecorderWrapper*>(g_GitsLoader->GetRecorderWrapperPtr());
    auto& cfg = g_GitsLoader->GetConfiguration();
    g_VulkanLibHandler = std::make_unique<VulkanLibHandler>(cfg.common.recorder.libVK.string().c_str());
    GITS_ASSERT(g_VulkanLibHandler->Get() != nullptr);
    g_vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(dl::load_symbol(g_VulkanLibHandler->Get(), "vkGetInstanceProcAddr"));
    GITS_ASSERT(g_vkGetInstanceProcAddr != nullptr);
    g_RecorderWrapper->LoadGlobalLevelFunctions(g_vkGetInstanceProcAddr);
  });
}

VK_INTERCEPTOR_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo,
                                                const VkAllocationCallbacks* pAllocator,
                                                VkInstance* pInstance) {
  Initialize();
  auto vkCreateInstanceWrapper = reinterpret_cast<PFN_vkCreateInstance>(
      g_RecorderWrapper->GetFunctionWrapper("vkCreateInstance"));
  if (vkCreateInstanceWrapper == nullptr) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  VkResult result = vkCreateInstanceWrapper(pCreateInfo, pAllocator, pInstance);
  if (result != VK_SUCCESS) {
    return result;
  }
  g_RecorderWrapper->LoadInstanceLevelFunctions(g_vkGetInstanceProcAddr, *pInstance);
  return result;
}

VK_INTERCEPTOR_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateDevice(VkPhysicalDevice physicalDevice,
                                              const VkDeviceCreateInfo* pCreateInfo,
                                              const VkAllocationCallbacks* pAllocator,
                                              VkDevice* pDevice) {
  auto vkCreateDeviceWrapper =
      reinterpret_cast<PFN_vkCreateDevice>(g_RecorderWrapper->GetFunctionWrapper("vkCreateDevice"));
  if (vkCreateDeviceWrapper == nullptr) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  VkResult result = vkCreateDeviceWrapper(physicalDevice, pCreateInfo, pAllocator, pDevice);
  if (result != VK_SUCCESS) {
    return result;
  }
  void* dispatchKey = *reinterpret_cast<void**>(physicalDevice);
  g_RecorderWrapper->LoadDeviceLevelFunctions(dispatchKey, *pDevice);
  return result;
}

VK_INTERCEPTOR_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice device, const char* pName) {
  Initialize();
  if (strcmp(pName, "vkGetDeviceProcAddr") == 0) {
    return reinterpret_cast<PFN_vkVoidFunction>(vkGetDeviceProcAddr);
  }
  return g_RecorderWrapper->GetFunctionWrapper(pName);
}

VK_INTERCEPTOR_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
vkGetInstanceProcAddr(VkInstance instance, const char* pName) {
  Initialize();

  if (strcmp(pName, "vkCreateInstance") == 0) {
    return reinterpret_cast<PFN_vkVoidFunction>(vkCreateInstance);
  } else if (strcmp(pName, "vkCreateDevice") == 0) {
    return reinterpret_cast<PFN_vkVoidFunction>(vkCreateDevice);
  } else if (strcmp(pName, "vkGetDeviceProcAddr") == 0) {
    return reinterpret_cast<PFN_vkVoidFunction>(vkGetDeviceProcAddr);
  }

  return g_RecorderWrapper->GetFunctionWrapper(pName);
}

VK_INTERCEPTOR_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkEnumerateInstanceVersion(uint32_t* pApiVersion) {
  Initialize();
  auto vkEnumerateInstanceVersionWrapper = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(
      g_RecorderWrapper->GetFunctionWrapper("vkEnumerateInstanceVersion"));
  if (vkEnumerateInstanceVersionWrapper == nullptr) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  return vkEnumerateInstanceVersionWrapper(pApiVersion);
}

VK_INTERCEPTOR_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkEnumerateInstanceLayerProperties(uint32_t* pPropertyCount, VkLayerProperties* pProperties) {
  Initialize();
  auto vkEnumerateInstanceLayerPropertiesWrapper =
      reinterpret_cast<PFN_vkEnumerateInstanceLayerProperties>(
          g_RecorderWrapper->GetFunctionWrapper("vkEnumerateInstanceLayerProperties"));
  if (vkEnumerateInstanceLayerPropertiesWrapper == nullptr) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  return vkEnumerateInstanceLayerPropertiesWrapper(pPropertyCount, pProperties);
}

VK_INTERCEPTOR_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(
    const char* pLayerName, uint32_t* pPropertyCount, VkExtensionProperties* pProperties) {
  Initialize();
  auto vkEnumerateInstanceExtensionPropertiesWrapper =
      reinterpret_cast<PFN_vkEnumerateInstanceExtensionProperties>(
          g_RecorderWrapper->GetFunctionWrapper("vkEnumerateInstanceExtensionProperties"));
  if (vkEnumerateInstanceExtensionPropertiesWrapper == nullptr) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  return vkEnumerateInstanceExtensionPropertiesWrapper(pLayerName, pPropertyCount, pProperties);
}

<%
skip = ['vkGetInstanceProcAddr', 'vkGetDeviceProcAddr', 'vkCreateInstance', 'vkCreateDevice', 'vkEnumerateInstanceVersion', 'vkEnumerateInstanceLayerProperties', 'vkEnumerateInstanceExtensionProperties']
%>

% for command in commands:
% if command.name not in skip:
<% define = get_define(command.platform) %>\
% if define:
#ifdef ${define}
% endif
VK_INTERCEPTOR_EXPORT VKAPI_ATTR ${command.return_type} VKAPI_CALL ${command.name}(
  % for param in command.params:
    ${param.full_type}${',' if not loop.last else ''}
  % endfor
  ) {
  auto ${command.name}Wrapper = reinterpret_cast<PFN_${command.name}>(
      g_RecorderWrapper->GetFunctionWrapper("${command.name}"));
  if (${command.name}Wrapper == nullptr) {
    % if command.return_type == 'void':
    return;
    % elif command.return_type == 'PFN_vkVoidFunction':
    return nullptr;
    % else:
    return VK_ERROR_INITIALIZATION_FAILED;
    % endif
  }
  ${'return ' if command.return_type != 'void' else ''}${command.name}Wrapper(
    % for param in command.params:
      ${param.name}${',' if not loop.last else ''}
    % endfor
  );
}
% if define:
#endif
% endif

% endif
% endfor

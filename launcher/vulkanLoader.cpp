// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "vulkanLoader.h"

#ifdef _WIN32
#include <windows.h>
#define VK_LIBRARY_NAME "vulkan-1.dll"
#else
#include <dlfcn.h>
#define VK_LIBRARY_NAME "libvulkan.so.1"
#endif

#include "log.h"

// Global variables
static void* g_vulkan_library = nullptr;

// Define global function pointers
PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;

// Global functions
#define VK_GLOBAL_FUNCTION(name) PFN_##name name = nullptr;
#include "vulkanFunctions.inl"

// Instance functions
#define VK_INSTANCE_FUNCTION(name) PFN_##name name = nullptr;
#include "vulkanFunctions.inl"

// Device functions
#define VK_DEVICE_FUNCTION(name) PFN_##name name = nullptr;
#include "vulkanFunctions.inl"

bool LoadVulkanLibrary() {
  if (g_vulkan_library) {
    LOG_DEBUG << "Vulkan library already loaded";
    return true; // Already loaded
  }

  LOG_INFO << "Loading Vulkan library: " << VK_LIBRARY_NAME;

#ifdef _WIN32
  g_vulkan_library = LoadLibraryA(VK_LIBRARY_NAME);
  if (!g_vulkan_library) {
    DWORD error = GetLastError();
    LOG_ERROR << "Failed to load " << VK_LIBRARY_NAME << " (Error code: " << error << ")";
    return false;
  }

  vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
      GetProcAddress(static_cast<HMODULE>(g_vulkan_library), "vkGetInstanceProcAddr"));
#else
  g_vulkan_library = dlopen(VK_LIBRARY_NAME, RTLD_NOW | RTLD_LOCAL);
  if (!g_vulkan_library) {
    LOG_ERROR << "Failed to load " << VK_LIBRARY_NAME << ": " << dlerror();
    return false;
  }

  vkGetInstanceProcAddr =
      reinterpret_cast<PFN_vkGetInstanceProcAddr>(dlsym(g_vulkan_library, "vkGetInstanceProcAddr"));
#endif

  if (!vkGetInstanceProcAddr) {
    LOG_ERROR << "Failed to load vkGetInstanceProcAddr from " << VK_LIBRARY_NAME;
    UnloadVulkanLibrary();
    return false;
  }

  LOG_INFO << "Successfully loaded Vulkan library";
  return LoadVulkanGlobalFunctions();
}

void UnloadVulkanLibrary() {
  if (g_vulkan_library) {
    LOG_INFO << "Unloading Vulkan library";

#ifdef _WIN32
    FreeLibrary(static_cast<HMODULE>(g_vulkan_library));
#else
    dlclose(g_vulkan_library);
#endif
    g_vulkan_library = nullptr;
    LOG_DEBUG << "Vulkan library unloaded";
  }

  // Reset all function pointers
  LOG_DEBUG << "Resetting Vulkan function pointers";
  vkGetInstanceProcAddr = nullptr;

#define VK_GLOBAL_FUNCTION(name) name = nullptr;
#include "vulkanFunctions.inl"

#define VK_INSTANCE_FUNCTION(name) name = nullptr;
#include "vulkanFunctions.inl"

#define VK_DEVICE_FUNCTION(name) name = nullptr;
#include "vulkanFunctions.inl"
}

bool LoadVulkanGlobalFunctions() {
  if (!vkGetInstanceProcAddr) {
    LOG_ERROR << "Cannot load global functions: vkGetInstanceProcAddr is null";
    return false;
  }

  LOG_INFO << "Loading Vulkan global functions";
  bool success = true;
  int loaded_count = 0;
  int failed_count = 0;

#define VK_GLOBAL_FUNCTION(name)                                                                   \
  name = reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr(nullptr, #name));                      \
  if (!name) {                                                                                     \
    LOG_ERROR << "Failed to load global function: " #name;                                         \
    success = false;                                                                               \
    failed_count++;                                                                                \
  } else {                                                                                         \
    loaded_count++;                                                                                \
    LOG_DEBUG << "Loaded global function: " #name;                                                 \
  }
#include "vulkanFunctions.inl"

  if (success) {
    LOG_INFO << "Successfully loaded " << loaded_count << " global Vulkan functions";
  } else {
    LOG_ERROR << "Failed to load " << failed_count << " global functions out of "
              << (loaded_count + failed_count);
  }

  return success;
}

bool LoadVulkanInstanceFunctions(VkInstance instance) {
  if (!vkGetInstanceProcAddr) {
    LOG_ERROR << "Cannot load instance functions: vkGetInstanceProcAddr is null";
    return false;
  }

  if (!instance) {
    LOG_ERROR << "Cannot load instance functions: VkInstance is null";
    return false;
  }

  LOG_INFO << "Loading Vulkan instance functions";
  bool success = true;
  int loaded_count = 0;
  int failed_count = 0;

#define VK_INSTANCE_FUNCTION(name)                                                                 \
  name = reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr(instance, #name));                     \
  if (!name) {                                                                                     \
    LOG_WARNING << "Failed to load instance function: " #name " (may not be supported)";           \
    failed_count++;                                                                                \
  } else {                                                                                         \
    loaded_count++;                                                                                \
    LOG_DEBUG << "Loaded instance function: " #name;                                               \
  }
#include "vulkanFunctions.inl"

  // Check for critical functions
  if (!vkDestroyInstance || !vkEnumeratePhysicalDevices || !vkCreateDevice) {
    LOG_ERROR << "Failed to load critical instance functions";
    success = false;
  }

  if (success) {
    LOG_INFO << "Successfully loaded " << loaded_count << " instance functions (" << failed_count
             << " optional functions not available)";
  } else {
    LOG_ERROR << "Failed to load critical instance functions";
  }

  return success;
}

bool LoadVulkanDeviceFunctions(VkDevice device) {
  if (!vkGetDeviceProcAddr) {
    LOG_ERROR << "Cannot load device functions: vkGetDeviceProcAddr is null";
    return false;
  }

  if (!device) {
    LOG_ERROR << "Cannot load device functions: VkDevice is null";
    return false;
  }

  LOG_INFO << "Loading Vulkan device functions";
  bool success = true;
  int loaded_count = 0;
  int failed_count = 0;

#define VK_DEVICE_FUNCTION(name)                                                                   \
  name = reinterpret_cast<PFN_##name>(vkGetDeviceProcAddr(device, #name));                         \
  if (!name) {                                                                                     \
    LOG_WARNING << "Failed to load device function: " #name " (may not be supported)";             \
    failed_count++;                                                                                \
  } else {                                                                                         \
    loaded_count++;                                                                                \
    LOG_DEBUG << "Loaded device function: " #name;                                                 \
  }
#include "vulkanFunctions.inl"

  // Check for critical functions
  if (!vkDestroyDevice || !vkGetDeviceQueue || !vkCreateBuffer || !vkCreateImage) {
    LOG_ERROR << "Failed to load critical device functions";
    success = false;
  }

  if (success) {
    LOG_INFO << "Successfully loaded " << loaded_count << " device functions (" << failed_count
             << " optional functions not available)";
  } else {
    LOG_ERROR << "Failed to load critical device functions";
  }

  return success;
}

// Custom loader function for ImGui
PFN_vkVoidFunction VulkanLoaderFunction(const char* name, void* user_data) {
  if (!name) {
    LOG_WARNING << "VulkanLoaderFunction called with null function name";
    return nullptr;
  }

  VkInstance instance = static_cast<VkInstance>(user_data);
  PFN_vkVoidFunction func = vkGetInstanceProcAddr(instance, name);

  if (!func) {
    LOG_DEBUG << "VulkanLoaderFunction: Function not found: " << name;
  } else {
    LOG_DEBUG << "VulkanLoaderFunction: Loaded function: " << name;
  }

  return func;
}

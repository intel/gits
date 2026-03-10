// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#define VK_NO_PROTOTYPES
#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#else
#define VK_USE_PLATFORM_XLIB_KHR
#endif

#include <vulkan/vulkan.h>

// Declare global function pointers
extern PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;

// Global functions
#define VK_GLOBAL_FUNCTION(name) extern PFN_##name name;
#include "vulkanFunctions.inl"

// Instance functions
#define VK_INSTANCE_FUNCTION(name) extern PFN_##name name;
#include "vulkanFunctions.inl"

// Device functions
#define VK_DEVICE_FUNCTION(name) extern PFN_##name name;
#include "vulkanFunctions.inl"

// Loading functions
bool LoadVulkanLibrary();
void UnloadVulkanLibrary();
bool LoadVulkanGlobalFunctions();
bool LoadVulkanInstanceFunctions(VkInstance instance);
bool LoadVulkanDeviceFunctions(VkDevice device);

// Custom loader function for ImGui
PFN_vkVoidFunction VulkanLoaderFunction(const char* name, void* user_data);

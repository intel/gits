// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "vulkanHeader.h"
#include <vector>
#include <unordered_map>

namespace gits {
namespace Vulkan {

class VulkanHudHelper {
public:
  VulkanHudHelper();
  void OnVkCreateInstance(const VkInstanceCreateInfo* pCreateInfo,
                          const VkAllocationCallbacks* pAllocator,
                          VkInstance* pInstance);
  void SetWindowHandle(HWND hwnd);
  void OnVkCreateDevice(VkPhysicalDevice physicalDevice,
                        const VkDeviceCreateInfo* pCreateInfo,
                        const VkAllocationCallbacks* pAllocator,
                        VkDevice* pDevice);
  void OnVkCreateSwapchainKHR(VkDevice device,
                              const VkSwapchainCreateInfoKHR* pCreateInfo,
                              const VkAllocationCallbacks* pAllocator,
                              VkSwapchainKHR* pSwapchain);
  void OnVkGetDeviceQueue(VkDevice device,
                          uint32_t queueFamilyIndex,
                          uint32_t queueIndex,
                          VkQueue* pQueue);
  void OnVkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR* pPresentInfo);

private:
  void CreateHudDescriptorPool();
  void SelectHudQueue();
  void CreateHudCommandPool();
  void CreateHudCommandBuffers();
  void CreateHudFences();
  void CreateHudRenderPass();
  void CreateHudFramebuffers();
  void CreateHudResources();
  void DestroyHudResources();
  void RecreateHudResources();
  void Initialize();
  void ChangeHudQueue(uint32_t queueFamilyIndex, VkQueue queue);

  bool m_Initialized = false;

  HWND m_Hwnd = NULL;

  uint32_t m_ApiVersion = 0;
  VkInstance m_Instance = VK_NULL_HANDLE;
  VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
  VkDevice m_Device = VK_NULL_HANDLE;
  std::unordered_map<VkQueue, uint32_t> m_QueueFamilyForQueue = {};
  VkQueue m_Queue = VK_NULL_HANDLE;
  uint32_t m_HudQueueFamily = 0;
  VkQueue m_HudQueue = VK_NULL_HANDLE;
  VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
  VkRenderPass m_RenderPass = VK_NULL_HANDLE;
  uint32_t m_MinImageCount = 0;
  uint32_t m_ImageCount = 0;

  VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
  VkExtent2D m_SwapchainExtent = {0, 0};
  std::vector<VkImage> m_SwapchainImages;
  VkCommandPool m_CommandPool = VK_NULL_HANDLE;
  std::vector<VkCommandBuffer> m_CommandBuffers;
  std::vector<VkFence> m_Fences;
  VkSurfaceFormatKHR m_SurfaceFormat = {VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_MAX_ENUM_KHR};
  std::vector<VkFramebuffer> m_Framebuffers;
  std::vector<VkImageView> m_ImageViews;
};

} // namespace Vulkan
} // namespace gits

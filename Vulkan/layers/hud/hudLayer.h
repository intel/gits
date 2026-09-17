// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "dispatchTablesHolder.h"

#include <chrono>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace gits {
namespace vulkan {

class HudLayer : public Layer {
public:
  static constexpr const char* LAYER_NAME = "Hud";

  // Layers must use proper instance/device level dispatch tables.
  // Cannot use application dispatch table from vulkan-1.dll, because
  // GITS layer has its own dispatch table and wrapped handles
  HudLayer(DispatchTablesHolder& dispatchTablesHolder);

  static PFN_vkVoidFunction ImGuiVulkanLoader(const char* functionName, void* userData);

  void Pre(vkQueuePresentKHRCommand& command) override;
  void Pre(vkDestroyDeviceCommand& command) override;

  void Post(vkCreateInstanceCommand& command) override;
#if defined(_WIN32)
  void Post(vkCreateWin32SurfaceKHRCommand& command) override;
#endif
  void Post(vkCreateDeviceCommand& command) override;
  void Post(vkCreateSwapchainKHRCommand& command) override;
  void Post(vkGetDeviceQueueCommand& command) override;
  void Post(vkGetDeviceQueue2Command& command) override;
  void Post(vkQueuePresentKHRCommand& command) override;

  // Queue access is locked across each Pre/driver call/Post pair
  void Pre(vkQueueSubmitCommand& command) override;
  void Pre(vkQueueSubmit2Command& command) override;
  void Pre(vkQueueSubmit2KHRCommand& command) override;
  void Pre(vkQueueBindSparseCommand& command) override;
  void Pre(vkQueueWaitIdleCommand& command) override;
  void Post(vkQueueSubmitCommand& command) override;
  void Post(vkQueueSubmit2Command& command) override;
  void Post(vkQueueSubmit2KHRCommand& command) override;
  void Post(vkQueueBindSparseCommand& command) override;
  void Post(vkQueueWaitIdleCommand& command) override;

private:
  void Initialize();
  void InitializeImGuiVulkanBackend();
#if defined(_WIN32)
  void SetWindowHandle(HWND hwnd);
#endif
  bool SelectHudQueue();
  void ChangeHudQueue(uint32_t queueFamilyIndex, VkQueue queue);

  void CreateHudResources();
  void DestroyHudResources();
  void RecreateHudResources(bool reselectQueue);
  void CreateHudDescriptorPool();
  void CreateHudCommandPool();
  void CreateHudCommandBuffers();
  void CreateHudFences();
  void CreateHudSemaphores();
  void CreateHudRenderPass();
  void CreateHudFramebuffers();
  void RecordHudDraw(VkCommandBuffer commandBuffer, uint32_t imageIndex);

  std::mutex& GetQueueMutex(VkQueue queue);
  // Retains entries for the layer lifetime so returned mutex references remain valid
  std::unordered_map<VkQueue, std::mutex> m_QueueMutexes;
  std::mutex m_MutexMapMutex;
  // HUD state and queue host access are protected independently
  // HUD state helper callers should lock m_HudResourcesMutex before calling them
  std::mutex m_HudResourcesMutex;

  DispatchTablesHolder& m_DispatchTablesHolder;

  // HUD lifecycle
  bool m_Initialized = false;
  bool m_ImGuiFunctionsLoaded = false;
  bool m_HudDisabled = false;
  bool m_SkipHudForSwapchain = false;

  // Application device and queue information
#if defined(_WIN32)
  HWND m_Hwnd = nullptr;
#else
  std::chrono::steady_clock::time_point m_PreviousFrameTime{};
#endif

  uint32_t m_ApiVersion = 0;
  VkInstance m_Instance = VK_NULL_HANDLE;
  VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
  VkDevice m_Device = VK_NULL_HANDLE;
  std::unordered_map<VkQueue, uint32_t> m_QueueFamilyForQueue;
  std::vector<VkQueueFamilyProperties> m_QueueFamilyProperties;
  std::unordered_set<uint32_t> m_RequestedQueueFamilies;

  // Selected graphics queue
  uint32_t m_HudQueueFamily = 0;
  VkQueue m_HudQueue = VK_NULL_HANDLE;

  // Tracked swapchain (the HUD currently assumes one swapchain)
  VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
  VkSharingMode m_SwapchainSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  // Queue families allowed to access the images when sharing mode is concurrent
  std::unordered_set<uint32_t> m_SwapchainConcurrentQueueFamilies;
  VkExtent2D m_SwapchainExtent = {0, 0};
  std::vector<VkImage> m_SwapchainImages;
  VkSurfaceFormatKHR m_SurfaceFormat = {VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_MAX_ENUM_KHR};
  uint32_t m_MinImageCount = 0;
  uint32_t m_ImageCount = 0;

  // HUD rendering resources, with per-image command buffers and synchronization
  VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
  VkRenderPass m_RenderPass = VK_NULL_HANDLE;
  VkCommandPool m_CommandPool = VK_NULL_HANDLE;
  std::vector<VkCommandBuffer> m_CommandBuffers;
  std::vector<VkFence> m_Fences;
  std::vector<VkFramebuffer> m_Framebuffers;
  std::vector<VkImageView> m_ImageViews;
  std::vector<VkSemaphore> m_ReadyToPresentSemaphores;

  // Additional resources for exclusive queue-family ownership transfers
  uint32_t m_PresentQueueFamily = UINT32_MAX;
  VkCommandPool m_PresentFamilyCommandPool = VK_NULL_HANDLE;
  std::vector<VkCommandBuffer> m_ReleaseCommandBuffers;
  std::vector<VkCommandBuffer> m_AcquireBackCommandBuffers;
  std::vector<VkSemaphore> m_ReleaseSemaphores;
  std::vector<VkSemaphore> m_HudFinishedSemaphores;
};

} // namespace vulkan
} // namespace gits

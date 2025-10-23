// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   vulkanRecorderWrapperIface.h
*
* @brief Declaration of Vulkan recorder wrapper interface.
*/

#pragma once

#include "configurationLib.h"
#include "vulkanDrivers.h"

#include <functional>

namespace gits {
namespace Vulkan {
class IRecorderWrapper {
public:
  virtual void PauseRecording() = 0;
  virtual void ContinueRecording() = 0;
  virtual bool IsPaused() const = 0;
  virtual void StreamFinishedEvent(std::function<void()> e) = 0;
  virtual void CloseRecorderIfRequired() = 0;
  virtual CVkDriver& Drivers() const = 0;
  virtual void SetDriverMode(CVkDriver::DriverMode mode) const = 0;
  virtual void* GetShadowMemory(VkDeviceMemory memory,
                                void* orig,
                                uint64_t size,
                                uint64_t offset) = 0;
  virtual uint64_t GetWholeMemorySize(VkDeviceMemory memory) const = 0;
  virtual void dumpScreenshot(VkQueue queue,
                              VkCommandBuffer cmdBuffer,
                              uint32_t commandBufferBatchCounter,
                              uint32_t commandBufferCounter) = 0;
  virtual void resetMemoryAfterQueueSubmit(VkQueue queue,
                                           uint32_t submitCount,
                                           const VkSubmitInfo* pSubmits) = 0;
  virtual void resetMemoryAfterQueueSubmit2(VkQueue queue,
                                            uint32_t submitCount,
                                            const VkSubmitInfo2* pSubmits) = 0;
  virtual VkResult CheckFenceStatus(VkDevice device, VkFence fence) const = 0;
  virtual void EndFramePost() const = 0;
  virtual void SuppressPhysicalDeviceFeatures(std::vector<std::string> const& suppressFeatures,
                                              VkPhysicalDeviceFeatures* features) const = 0;
  virtual VkPhysicalDevice GetPhysicalDevice(VkDevice device) const = 0;
  virtual void IgnoreNextQueuePresentGITS() = 0;
  virtual bool IsNextQueuePresentIgnored() = 0;
  virtual void AcceptNextQueuePresentGITS() = 0;
  virtual bool CheckMemoryMappingFeasibility(VkDevice device,
                                             uint32_t memoryTypeIndex,
                                             bool throwException = true) const = 0;
  virtual bool AreDeviceExtensionsSupported(VkPhysicalDevice physicalDevice,
                                            uint32_t requestedExtensionsCount,
                                            char const* const* requestedExtensions) const = 0;
  virtual const void* GetPNextStructure(const void* pNext, VkStructureType structureType) const = 0;
  virtual bool IsImagePresentable(const VkImageCreateInfo* pCreateInfo) const = 0;
  virtual void DisableConfigOptions() const = 0;
  virtual void* CreateExternalMemory(VkDeviceSize size) const = 0;
  virtual void FreeExternalMemory(VkDeviceMemory memory) const = 0;
  virtual void TrackMemoryState(VkResult return_value,
                                VkDevice device,
                                const VkMemoryAllocateInfo* pAllocateInfo,
                                const VkAllocationCallbacks* pAllocator,
                                VkDeviceMemory* pMemory,
                                void* externalMemory) const = 0;
  virtual bool IsVulkanAPIVersionSupported(uint32_t major,
                                           uint32_t minor,
                                           VkPhysicalDevice physicalDevice) const = 0;
  virtual void SetConfig(Config const& cfg) const = 0;
  virtual bool IsUseExternalMemoryExtensionUsed() const = 0;
  virtual bool IsSubcaptureBeforeRestorationPhase() const = 0;
  virtual void HudOnVkCreateInstance(const VkInstanceCreateInfo* pCreateInfo,
                                     const VkAllocationCallbacks* pAllocator,
                                     VkInstance* pInstance) = 0;
  virtual void HudOnVkCreateWin32SurfaceKHR(VkInstance instance,
                                            const VkWin32SurfaceCreateInfoKHR* pCreateInfo,
                                            const VkAllocationCallbacks* pAllocator,
                                            VkSurfaceKHR* pSurface) = 0;
  virtual void HudOnVkCreateDevice(VkPhysicalDevice physicalDevice,
                                   const VkDeviceCreateInfo* pCreateInfo,
                                   const VkAllocationCallbacks* pAllocator,
                                   VkDevice* pDevice) = 0;
  virtual void HudOnVkCreateSwapchainKHR(VkDevice device,
                                         const VkSwapchainCreateInfoKHR* pCreateInfo,
                                         const VkAllocationCallbacks* pAllocator,
                                         VkSwapchainKHR* pSwapchain) = 0;
  virtual void HudOnVkGetDeviceQueue(VkDevice device,
                                     uint32_t queueFamilyIndex,
                                     uint32_t queueIndex,
                                     VkQueue* pQueue) = 0;
  virtual void HudOnVkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR* pPresentInfo) = 0;
#include "vulkanRecorderWrapperIfaceAuto.h"
};
} // namespace Vulkan
} // namespace gits

typedef gits::Vulkan::IRecorderWrapper*(STDCALL* FGITSRecoderVulkan)();

extern "C" {
gits::Vulkan::IRecorderWrapper* STDCALL GITSRecorderVulkan() VISIBLE;
}

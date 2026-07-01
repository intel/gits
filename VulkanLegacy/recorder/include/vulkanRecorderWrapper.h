// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   vulkanRecorderWrapper.h
*
* @brief Declaration of Vulkan recorder wrapper.
*/

#pragma once

#include "tools_lite.h"
#include "vulkanRecorderWrapperIface.h"

#if defined GITS_PLATFORM_WINDOWS
#include "vulkanHudHelper.h"
#endif

namespace gits {
class VulkanQueueSubmitPrePost : private gits::noncopyable {
public:
  VulkanQueueSubmitPrePost();
  ~VulkanQueueSubmitPrePost();
};
class VulkanCreateImagePrePost : private gits::noncopyable {
public:
  VulkanCreateImagePrePost();
  ~VulkanCreateImagePrePost();
};
class VulkanCreateBufferPrePost : private gits::noncopyable {
public:
  VulkanCreateBufferPrePost();
  ~VulkanCreateBufferPrePost();
};
} // namespace gits
#define QUEUE_SUBMIT_WRAPPER_PRE_POST  VulkanQueueSubmitPrePost vulkanQueueSubmitPrePost;
#define CREATE_IMAGE_WRAPPER_PRE_POST  VulkanCreateImagePrePost vulkanCreateImagePrePost;
#define CREATE_BUFFER_WRAPPER_PRE_POST VulkanCreateBufferPrePost vulkanCreateBufferPrePost;
namespace gits {

class CRecorder;
namespace Vulkan {

class CRecorderWrapper : public IRecorderWrapper {
  CRecorder& _recorder;
  bool _ignoreNextQueuePresentGITS;
#if defined GITS_PLATFORM_WINDOWS
  VulkanHudHelper _hudHelper;
#endif

public:
  CRecorderWrapper(CRecorder& recorder);
  ~CRecorderWrapper() = default;
  CRecorderWrapper(const CRecorderWrapper& ref) = delete;
  CRecorderWrapper& operator=(const CRecorderWrapper& ref) = delete;
  void PauseRecording() override;
  void ContinueRecording() override;
  bool IsPaused() const override;
  void StreamFinishedEvent(std::function<void()> e);
  void CloseRecorderIfRequired() override;
  CVkDriver& Drivers() const override;
  void SetDriverMode(CVkDriver::DriverMode mode) const override;
  void* GetShadowMemory(VkDeviceMemory memory, void* orig, uint64_t size, uint64_t offset) override;
  uint64_t GetWholeMemorySize(VkDeviceMemory memory) const override;
  void dumpScreenshot(VkQueue queue,
                      VkCommandBuffer cmdBuffer,
                      uint32_t commandBufferBatchCounter,
                      uint32_t commandBufferCounter) override;
  void resetMemoryAfterQueueSubmit(VkQueue queue,
                                   uint32_t submitCount,
                                   const VkSubmitInfo* pSubmits) override;
  void resetMemoryAfterQueueSubmit2(VkQueue queue,
                                    uint32_t submitCount,
                                    const VkSubmitInfo2* pSubmits) override;
  VkResult CheckFenceStatus(VkDevice device, VkFence fence) const override;
  void EndFramePost() const override;
  void SuppressPhysicalDeviceFeatures(std::vector<std::string> const& suppressFeatures,
                                      VkPhysicalDeviceFeatures* features) const override;
  VkPhysicalDevice GetPhysicalDevice(VkDevice device) const override;
  void IgnoreNextQueuePresentGITS() override;
  bool IsNextQueuePresentIgnored() override;
  void AcceptNextQueuePresentGITS() override;
  bool CheckMemoryMappingFeasibility(VkDevice device,
                                     uint32_t memoryTypeIndex,
                                     bool throwException = true) const override;
  bool AreDeviceExtensionsSupported(VkPhysicalDevice physicalDevice,
                                    uint32_t requestedExtensionsCount,
                                    char const* const* requestedExtensions) const override;
  const void* GetPNextStructure(const void* pNext, VkStructureType structureType) const override;
  bool IsImagePresentable(const VkImageCreateInfo* pCreateInfo) const override;
  void DisableConfigOptions() const override;
  void* CreateExternalMemory(VkDeviceSize size) const override;
  void FreeExternalMemory(VkDeviceMemory memory) const override;
  void TrackMemoryState(VkResult return_value,
                        VkDevice device,
                        const VkMemoryAllocateInfo* pAllocateInfo,
                        const VkAllocationCallbacks* pAllocator,
                        VkDeviceMemory* pMemory,
                        void* externalMemory) const override;
  bool IsVulkanAPIVersionSupported(uint32_t major,
                                   uint32_t minor,
                                   VkPhysicalDevice physicalDevice) const override;
  void SetConfig(Config const& cfg) const override;
  bool IsUseExternalMemoryExtensionUsed() const override;
  bool IsSubcaptureBeforeRestorationPhase() const override;
  void HudOnVkCreateInstance(const VkInstanceCreateInfo* pCreateInfo,
                             const VkAllocationCallbacks* pAllocator,
                             VkInstance* pInstance) override;
  void HudOnVkCreateWin32SurfaceKHR(VkInstance instance,
                                    const VkWin32SurfaceCreateInfoKHR* pCreateInfo,
                                    const VkAllocationCallbacks* pAllocator,
                                    VkSurfaceKHR* pSurface) override;
  void HudOnVkCreateDevice(VkPhysicalDevice physicalDevice,
                           const VkDeviceCreateInfo* pCreateInfo,
                           const VkAllocationCallbacks* pAllocator,
                           VkDevice* pDevice) override;
  void HudOnVkCreateSwapchainKHR(VkDevice device,
                                 const VkSwapchainCreateInfoKHR* pCreateInfo,
                                 const VkAllocationCallbacks* pAllocator,
                                 VkSwapchainKHR* pSwapchain) override;
  void HudOnVkGetDeviceQueue(VkDevice device,
                             uint32_t queueFamilyIndex,
                             uint32_t queueIndex,
                             VkQueue* pQueue) override;
  void HudOnVkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR* pPresentInfo) override;
#include "vulkanRecorderWrapperAuto.h"
};
} // namespace Vulkan
} // namespace gits

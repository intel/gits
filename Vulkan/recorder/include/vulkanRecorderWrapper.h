// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
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

#include "vulkanRecorderWrapperIface.h"
#include "tools_lite.h"

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

  CRecorderWrapper(const CRecorderWrapper& ref);            // do not allow copy construction
  CRecorderWrapper& operator=(const CRecorderWrapper& ref); // do not allow class assignment

public:
  CRecorderWrapper(CRecorder& recorder);
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
  void StartStateRestore() const override;
  void EndStateRestore() const override;
  bool IsCCodeStateRestore() const override;
  void StartFrame() const override;
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
#include "vulkanRecorderWrapperAuto.h"
};
} // namespace Vulkan
} // namespace gits

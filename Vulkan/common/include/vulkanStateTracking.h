// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   vulkanStateTracking.h
*
* @brief Functions for tracking current state of Vulkan resources
*
*/
#pragma once

#include "vulkanStateDynamic.h"
#include "vulkanDrivers.h"
#include "vulkanTools.h"
#include "istdhash.h"
#include "gits.h"

namespace gits {

namespace Vulkan {

namespace {

inline bool isRecorder() {
  static bool isRecorder = Config::Get().IsRecorder();
  return isRecorder;
}

inline bool isSubcaptureBeforeRestorationPhase() {
  static bool isSubcapture = CGits::Instance().apis.Iface3D().CfgRec_IsSubcapture();
  if (!isSubcapture) {
    return false;
  }
  return !SD().stateRestoreFinished;
}

inline bool updateOnlyUsedMemory() {
  static bool updateOnlyUsedMemory = TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED ==
                                     Config::Get().recorder.vulkan.utilities.memoryUpdateState;
  return updateOnlyUsedMemory;
}

inline bool captureRenderPasses() {
  static bool captureRenderPasses = !Config::Get().player.captureVulkanSubmits.empty() ||
                                    !Config::Get().player.captureVulkanRenderPasses.empty();
  return captureRenderPasses;
}

inline bool captureVulkanSubmitsResources() {
  static bool captureVulkanSubmitsResources =
      !Config::Get().player.captureVulkanSubmitsResources.empty();
  return captureVulkanSubmitsResources;
}

inline bool crossPlatformStateRestoration() {
  static bool crossPlatformStateRestoration =
      Config::Get().recorder.vulkan.utilities.crossPlatformStateRestoration.images;
  return crossPlatformStateRestoration;
}
#ifdef GITS_PLATFORM_WINDOWS
inline bool usePresentSrcLayoutTransitionAsAFrameBoundary() {
  static bool usePresentSrcLayoutTransitionAsAFrameBoundary =
      Config::Get().recorder.vulkan.utilities.usePresentSrcLayoutTransitionAsAFrameBoundary;
  return usePresentSrcLayoutTransitionAsAFrameBoundary;
}
#endif

} // namespace

inline void vkIAmGITS_SD() {
  SD().internalResources.attachedToGITS = true;
}

// Instance

inline void vkCreateInstance_SD(VkResult return_value,
                                const VkInstanceCreateInfo* pCreateInfo,
                                const VkAllocationCallbacks* pAllocator,
                                VkInstance* pInstance) {
#ifdef GITS_PLATFORM_WINDOWS
  auto offscreenApp =
      getPNextStructure(pCreateInfo->pNext, VK_STRUCTURE_TYPE_WIN32_INSTANCE_CREATE_INFO_INTEL);
  if (offscreenApp != nullptr) {
    // Automatically enable path for offscreen applications
    auto cfg = Config::Get();
    cfg.recorder.vulkan.utilities.usePresentSrcLayoutTransitionAsAFrameBoundary = true;
    Config::Set(cfg);
  }
#endif

  if ((return_value == VK_SUCCESS) && (*pInstance != VK_NULL_HANDLE)) {
    auto instanceState = std::make_shared<CInstanceState>(pInstance, pCreateInfo);
    if (pCreateInfo->pApplicationInfo) {
      instanceState->vulkanVersionMajor =
          VK_VERSION_MAJOR(pCreateInfo->pApplicationInfo->apiVersion);
      instanceState->vulkanVersionMinor =
          VK_VERSION_MINOR(pCreateInfo->pApplicationInfo->apiVersion);
    }
    SD()._instancestates.emplace(*pInstance, instanceState);

    if (Config::Get().player.skipNonDeterministicImages) {
      for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
        auto element = pCreateInfo->ppEnabledExtensionNames[i];
        if (strcmp(element, VK_EXT_DEPTH_RANGE_UNRESTRICTED_EXTENSION_NAME) == 0) {
          SD().depthRangeUnrestrictedEXTEnabled = true;
          break;
        }
      }
    }
  }
}

inline void vkDestroyInstance_SD(VkInstance instance, const VkAllocationCallbacks* pAllocator) {
  // Remove tracked data for destroyed instance
  {
    std::vector<VkPhysicalDevice> physicalDevicesToRemove;

    for (auto& physicalDeviceState : SD()._physicaldevicestates) {
      if (physicalDeviceState.second->instanceStateStore->instanceHandle == instance) {
        physicalDevicesToRemove.push_back(physicalDeviceState.first);
      }
    }
    for (auto physicalDevice : physicalDevicesToRemove) {
      SD()._physicaldevicestates.erase(physicalDevice);
    }
  }

  SD()._instancestates.erase(instance);
}

// Physical devices

namespace {
void getSupportedExtensions(std::shared_ptr<CPhysicalDeviceState>& physicalDeviceState) {
  uint32_t supportedExtensionsCount = 0;
  if ((drvVk.vkEnumerateDeviceExtensionProperties(physicalDeviceState->physicalDeviceHandle,
                                                  nullptr, &supportedExtensionsCount,
                                                  nullptr) != VK_SUCCESS) ||
      (supportedExtensionsCount == 0)) {
    return;
  }

  std::vector<VkExtensionProperties> supportedExtensions(supportedExtensionsCount);
  if (drvVk.vkEnumerateDeviceExtensionProperties(physicalDeviceState->physicalDeviceHandle, nullptr,
                                                 &supportedExtensionsCount,
                                                 supportedExtensions.data()) != VK_SUCCESS) {
    return;
  }

  physicalDeviceState->supportedExtensions.reserve(supportedExtensionsCount);
  for (auto& extension : supportedExtensions) {
    physicalDeviceState->supportedExtensions.emplace_back(extension.extensionName);
  }
}
} // namespace

inline void vkEnumeratePhysicalDevices_SD(VkResult return_value,
                                          VkInstance instance,
                                          uint32_t* pPhysicalDeviceCount,
                                          VkPhysicalDevice* pPhysicalDevices) {
  // pPhysicalDevices == NULL means the app is only querying the number of available devices rather than obtaining a list of those devices
  if (pPhysicalDevices == NULL) {
    return;
  }
  for (uint32_t i = 0; i < *pPhysicalDeviceCount; ++i) {
    if (SD()._physicaldevicestates.find(pPhysicalDevices[i]) == SD()._physicaldevicestates.end()) {
      auto physicalDeviceState = std::make_shared<CPhysicalDeviceState>(
          pPhysicalDevices[i], SD()._instancestates[instance]);
      SD()._physicaldevicestates.emplace(pPhysicalDevices[i], physicalDeviceState);
      getSupportedExtensions(physicalDeviceState);
    }
  }
}

inline void vkEnumeratePhysicalDeviceGroups_SD(
    VkResult return_value,
    VkInstance instance,
    uint32_t* pPhysicalDeviceGroupCount,
    VkPhysicalDeviceGroupProperties* pPhysicalDeviceGroupProperties) {
  // pPhysicalDevices == NULL means the app is only querying the number of available devices rather than obtaining a list of those devices
  if (pPhysicalDeviceGroupProperties == NULL) {
    return;
  }

  for (uint32_t g = 0; g < *pPhysicalDeviceGroupCount; ++g) {
    for (uint32_t i = 0; i < pPhysicalDeviceGroupProperties[g].physicalDeviceCount; ++i) {
      VkPhysicalDevice physicalDevice = pPhysicalDeviceGroupProperties[g].physicalDevices[i];
      if (SD()._physicaldevicestates.find(physicalDevice) == SD()._physicaldevicestates.end()) {
        auto physicalDeviceState =
            std::make_shared<CPhysicalDeviceState>(physicalDevice, SD()._instancestates[instance]);
        SD()._physicaldevicestates.emplace(physicalDevice, physicalDeviceState);
        getSupportedExtensions(physicalDeviceState);
      }
    }
  }
}

inline void vkEnumeratePhysicalDeviceGroupsKHR_SD(
    VkResult return_value,
    VkInstance instance,
    uint32_t* pPhysicalDeviceGroupCount,
    VkPhysicalDeviceGroupProperties* pPhysicalDeviceGroupProperties) {
  vkEnumeratePhysicalDeviceGroups_SD(return_value, instance, pPhysicalDeviceGroupCount,
                                     pPhysicalDeviceGroupProperties);
}

inline void vkPassPhysicalDeviceMemoryPropertiesGITS_SD(
    VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties* pMemoryProperties) {
  if (pMemoryProperties == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  SD()._physicaldevicestates[physicalDevice]->memoryProperties = *pMemoryProperties;
}

// Surface

inline void vkCreateWin32SurfaceKHR_SD(VkResult return_value,
                                       VkInstance instance,
                                       const VkWin32SurfaceCreateInfoKHR* pCreateInfo,
                                       const VkAllocationCallbacks* pAllocator,
                                       VkSurfaceKHR* pSurface) {
  if ((return_value == VK_SUCCESS) && (*pSurface != VK_NULL_HANDLE)) {
    SD()._surfacekhrstates.emplace(
        *pSurface,
        std::make_shared<CSurfaceKHRState>(pSurface, pCreateInfo, SD()._instancestates[instance]));
  }
}

inline void vkCreateXcbSurfaceKHR_SD(VkResult return_value,
                                     VkInstance instance,
                                     const VkXcbSurfaceCreateInfoKHR* pCreateInfo,
                                     const VkAllocationCallbacks* pAllocator,
                                     VkSurfaceKHR* pSurface) {
  if ((return_value == VK_SUCCESS) && (*pSurface != VK_NULL_HANDLE)) {
    SD()._surfacekhrstates.emplace(
        *pSurface,
        std::make_shared<CSurfaceKHRState>(pSurface, pCreateInfo, SD()._instancestates[instance]));
  }
}

inline void vkCreateXlibSurfaceKHR_SD(VkResult return_value,
                                      VkInstance instance,
                                      const VkXlibSurfaceCreateInfoKHR* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator,
                                      VkSurfaceKHR* pSurface) {
  if ((return_value == VK_SUCCESS) && (*pSurface != VK_NULL_HANDLE)) {
    SD()._surfacekhrstates.emplace(
        *pSurface,
        std::make_shared<CSurfaceKHRState>(pSurface, pCreateInfo, SD()._instancestates[instance]));
  }
}

inline void vkDestroySurfaceKHR_SD(VkInstance instance,
                                   VkSurfaceKHR surface,
                                   const VkAllocationCallbacks* pAllocator) {
  if (surface) {
    auto& surfaceState = SD()._surfacekhrstates[surface];
#ifdef GITS_PLATFORM_WINDOWS
    SD()._hwndstates.erase(surfaceState->surfaceCreateInfoWin32Data.Value()->hwnd);
#endif
#ifdef GITS_PLATFORM_X11
    VkXcbSurfaceCreateInfoKHR* xcbCreateInfoPtr = surfaceState->surfaceCreateInfoXcbData.Value();
    VkXlibSurfaceCreateInfoKHR* xlibCreateInfoPtr = surfaceState->surfaceCreateInfoXlibData.Value();
    if (xcbCreateInfoPtr != nullptr) {
      SD()._hwndstates.erase(xcbCreateInfoPtr->window);
    } else if (xlibCreateInfoPtr != nullptr) {
      SD()._hwndstates.erase(xlibCreateInfoPtr->window);
    }
#endif
    SD()._surfacekhrstates.erase(surface);
  } else {
    Log(WARN) << "Destroying null VkSurfaceKHR";
  }
}

// Device

inline void vkCreateDevice_SD(VkResult return_value,
                              VkPhysicalDevice physicalDevice,
                              const VkDeviceCreateInfo* pCreateInfo,
                              const VkAllocationCallbacks* pAllocator,
                              VkDevice* pDevice) {
  if ((return_value == VK_SUCCESS) && (*pDevice != VK_NULL_HANDLE)) {
    auto deviceState = std::make_shared<CDeviceState>(pDevice, pCreateInfo,
                                                      SD()._physicaldevicestates[physicalDevice]);

    std::vector<VkQueueFamilyProperties> queueFamilies;
    {
      uint32_t queueFamiliesCount = 0;
      drvVk.vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamiliesCount, nullptr);

      queueFamilies.resize(queueFamiliesCount);
      drvVk.vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamiliesCount,
                                                     queueFamilies.data());
    }

    for (uint32_t qci = 0; qci < pCreateInfo->queueCreateInfoCount; ++qci) {
      for (uint32_t qc = 0; qc < pCreateInfo->pQueueCreateInfos[qci].queueCount; ++qc) {
        VkQueue queue = VK_NULL_HANDLE;
        uint32_t queueFamilyIndex = pCreateInfo->pQueueCreateInfos[qci].queueFamilyIndex;
        uint32_t queueIndex = qc;

        if (0 != pCreateInfo->pQueueCreateInfos[qci].flags) {
          VkDeviceQueueInfo2 deviceQueueInfo2 = {
              VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,     // VkStructureType sType;
              nullptr,                                   // const void* pNext;
              pCreateInfo->pQueueCreateInfos[qci].flags, // VkDeviceQueueCreateFlags flags;
              queueFamilyIndex,                          // uint32_t queueFamilyIndex;
              queueIndex                                 // uint32_t queueIndex;
          };
          drvVk.vkGetDeviceQueue2(*pDevice, &deviceQueueInfo2, &queue);
        } else {
          drvVk.vkGetDeviceQueue(*pDevice, queueFamilyIndex, queueIndex, &queue);
        }

        auto queueState =
            std::make_shared<CQueueState>(&queue, queueFamilyIndex, queueIndex,
                                          queueFamilies[queueFamilyIndex].queueFlags, deviceState);
        SD()._queuestates.emplace(queue, queueState);
        deviceState->queueStateStoreList.push_back(queueState);
      }
    }

    SD()._devicestates.emplace(*pDevice, deviceState);
    for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; ++i) {
      deviceState->enabledExtensions.emplace_back(pCreateInfo->ppEnabledExtensionNames[i]);
    }
  }
}

inline void vkDestroyDevice_SD(VkDevice device, const VkAllocationCallbacks* pAllocator) {
  SD()._devicestates.erase(device);
}

// Queue

inline void vkQueueBindSparse_SD(VkResult returnValue,
                                 VkQueue queue,
                                 uint32_t bindInfoCount,
                                 const VkBindSparseInfo* pBindInfo,
                                 VkFence fence) {
  if (VK_NULL_HANDLE != fence) {
    SD()._fencestates[fence]->fenceUsed = true;
  }

  if (Config::Get().IsRecorder()) {
    for (uint32_t bic = 0; bic < bindInfoCount; ++bic) {
      auto& bindInfo = pBindInfo[bic];

      for (uint32_t b = 0; b < bindInfo.bufferBindCount; ++b) {
        auto& bufferState = SD()._bufferstates[bindInfo.pBufferBinds[b].buffer];

        for (uint32_t bc = 0; bc < bindInfo.pBufferBinds[b].bindCount; ++bc) {
          bufferState->sparseBindings.emplace_back(
              std::make_shared<CVkSparseMemoryBindData>(&bindInfo.pBufferBinds[b].pBinds[bc]));
        }
      }

      for (uint32_t o = 0; o < bindInfo.imageOpaqueBindCount; o++) {
        auto& imageState = SD()._imagestates[bindInfo.pImageOpaqueBinds[o].image];

        for (uint32_t bc = 0; bc < bindInfo.pImageOpaqueBinds[o].bindCount; ++bc) {
          imageState->sparseBindings.emplace_back(
              std::make_shared<CVkSparseMemoryBindData>(&bindInfo.pImageOpaqueBinds[o].pBinds[bc]),
              std::shared_ptr<CVkSparseImageMemoryBindData>());
        }
      }

      for (uint32_t i = 0; i < bindInfo.imageBindCount; ++i) {
        auto& imageState = SD()._imagestates[bindInfo.pImageBinds[i].image];

        for (uint32_t bc = 0; bc < bindInfo.pImageBinds[i].bindCount; ++bc) {
          imageState->sparseBindings.emplace_back(
              std::shared_ptr<CVkSparseMemoryBindData>(),
              std::make_shared<CVkSparseImageMemoryBindData>(&bindInfo.pImageBinds[i].pBinds[bc]));
        }
      }
    }
  }
}

// Swapchain

inline void vkCreateSwapchainKHR_SD(VkResult return_value,
                                    VkDevice device,
                                    const VkSwapchainCreateInfoKHR* pCreateInfo,
                                    const VkAllocationCallbacks* pAllocator,
                                    VkSwapchainKHR* pSwapchain) {
  if ((return_value == VK_SUCCESS) && (*pSwapchain != VK_NULL_HANDLE)) {
    auto swapchainState =
        std::make_shared<CSwapchainKHRState>(pSwapchain, pCreateInfo, SD()._devicestates[device],
                                             SD()._surfacekhrstates[pCreateInfo->surface]);

    {
      std::vector<VkImage> swapchainImages;
      uint32_t swapchainImageCount;
      drvVk.vkGetSwapchainImagesKHR(device, *pSwapchain, &swapchainImageCount, nullptr);
      swapchainImages.resize(swapchainImageCount);
      drvVk.vkGetSwapchainImagesKHR(device, *pSwapchain, &swapchainImageCount,
                                    swapchainImages.data());

      for (auto image : swapchainImages) {
        if (SD().imageCounter.find(image) == SD().imageCounter.end()) {
          CGits::Instance().vkCounters.ImageCountUp();
          SD().imageCounter[image] = CGits::Instance().vkCounters.CurrentImageCount();
          Log(TRACE) << "Image nr: " << SD().imageCounter[image] << " (Swapchain image: " << image
                     << " )";
        }
        auto imageState = std::make_shared<CImageState>(&image, swapchainState);
        SD()._imagestates.emplace(image, imageState);
        swapchainState->imageStateStoreList.push_back(imageState);
      }
    }

    SD()._swapchainkhrstates.emplace(*pSwapchain, swapchainState);
  }
}

inline void vkCreateFakeSwapchainKHR_SD(VkDevice device,
                                        const VkSwapchainCreateInfoKHR* pCreateInfo,
                                        VkSwapchainKHR* pSwapchain,
                                        std::vector<VkImage>& swapchainImages) {
  auto swapchainState =
      std::make_shared<CSwapchainKHRState>(pSwapchain, pCreateInfo, SD()._devicestates[device],
                                           SD()._surfacekhrstates[pCreateInfo->surface]);

  for (auto image : swapchainImages) {
    auto imageState = std::make_shared<CImageState>(&image, swapchainState);
    SD()._imagestates.emplace(image, imageState);
    swapchainState->imageStateStoreList.push_back(imageState);
  }

  SD()._swapchainkhrstates.emplace(*pSwapchain, swapchainState);
}

inline void vkAcquireNextImageKHR_SD(VkResult return_value,
                                     VkDevice device,
                                     VkSwapchainKHR swapchain,
                                     uint64_t timeout,
                                     VkSemaphore semaphore,
                                     VkFence fence,
                                     uint32_t* pImageIndex) {
  if (VK_NULL_HANDLE != semaphore) {
    SD()._semaphorestates[semaphore]->semaphoreUsed = true;
  }
  if (VK_NULL_HANDLE != fence) {
    SD()._fencestates[fence]->fenceUsed = true;
  }
  SD()._swapchainkhrstates[swapchain]->acquiredImages.insert(*pImageIndex);
}

inline void vkAcquireNextImage2KHR_SD(VkResult return_value,
                                      VkDevice device,
                                      const VkAcquireNextImageInfoKHR* pAcquireInfo,
                                      uint32_t* pImageIndex) {
  if (VK_NULL_HANDLE != pAcquireInfo->semaphore) {
    SD()._semaphorestates[pAcquireInfo->semaphore]->semaphoreUsed = true;
  }
  if (VK_NULL_HANDLE != pAcquireInfo->fence) {
    SD()._fencestates[pAcquireInfo->fence]->fenceUsed = true;
  }
  SD()._swapchainkhrstates[pAcquireInfo->swapchain]->acquiredImages.insert(*pImageIndex);
}

inline void vkQueuePresentKHR_SD(VkResult return_value,
                                 VkQueue queue,
                                 const VkPresentInfoKHR* pPresentInfo) {
  for (uint32_t i = 0; i < pPresentInfo->waitSemaphoreCount; i++) {
    SD()._semaphorestates[pPresentInfo->pWaitSemaphores[i]]->semaphoreUsed = false;
  }
  for (uint32_t i = 0; i < pPresentInfo->swapchainCount; ++i) {
    SD()._swapchainkhrstates[pPresentInfo->pSwapchains[i]]->acquiredImages.erase(
        pPresentInfo->pImageIndices[i]);
  }
}

inline void vkDestroySwapchainKHR_SD(VkDevice device,
                                     VkSwapchainKHR swapchain,
                                     const VkAllocationCallbacks* pAllocator) {
  for (auto& imageState : SD()._swapchainkhrstates[swapchain]->imageStateStoreList) {
    SD()._imagestates.erase(imageState->imageHandle);
  }
  SD()._swapchainkhrstates.erase(swapchain);
}

// Descriptor pool

inline void vkCreateDescriptorPool_SD(VkResult return_value,
                                      VkDevice device,
                                      const VkDescriptorPoolCreateInfo* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator,
                                      VkDescriptorPool* pDescriptorPool) {
  if ((return_value == VK_SUCCESS) && (*pDescriptorPool != VK_NULL_HANDLE)) {
    SD()._descriptorpoolstates.emplace(
        *pDescriptorPool, std::make_shared<CDescriptorPoolState>(pDescriptorPool, pCreateInfo,
                                                                 SD()._devicestates[device]));
  }
}

inline void vkResetDescriptorPool_SD(VkResult return_value,
                                     VkDevice device,
                                     VkDescriptorPool descriptorPool,
                                     VkDescriptorPoolResetFlags flags) {
  if (Config::Get().IsRecorder() || !Config::Get().player.captureVulkanSubmitsResources.empty()) {
    auto& descriptorSetStateList =
        SD()._descriptorpoolstates[descriptorPool]->descriptorSetStateStoreList;

    for (auto& descriptorSetState : descriptorSetStateList) {
      SD()._descriptorsetstates.erase(descriptorSetState->descriptorSetHandle); // Stardust
    }
    descriptorSetStateList.clear();
  }
}

inline void vkDestroyDescriptorPool_SD(VkDevice device,
                                       VkDescriptorPool descriptorPool,
                                       const VkAllocationCallbacks* pAllocator) {
  for (auto& descriptorSetState :
       SD()._descriptorpoolstates[descriptorPool]->descriptorSetStateStoreList) {
    SD()._descriptorsetstates.erase(descriptorSetState->descriptorSetHandle); // Stardust
  }
  SD()._descriptorpoolstates.erase(descriptorPool);
}

// Command pool

inline void vkCreateCommandPool_SD(VkResult return_value,
                                   VkDevice device,
                                   const VkCommandPoolCreateInfo* pCreateInfo,
                                   const VkAllocationCallbacks* pAllocator,
                                   VkCommandPool* pCmdPool) {
  if ((return_value == VK_SUCCESS) && (*pCmdPool != VK_NULL_HANDLE)) {
    SD()._commandpoolstates.emplace(
        *pCmdPool,
        std::make_shared<CCommandPoolState>(pCmdPool, pCreateInfo, SD()._devicestates[device]));
  }
}

inline void vkResetCommandBuffer_SD(VkResult /* return_value */,
                                    VkCommandBuffer commandBuffer,
                                    VkCommandBufferResetFlags /* flags */);

inline void vkResetCommandPool_SD(VkResult return_value,
                                  VkDevice device,
                                  VkCommandPool commandPool,
                                  VkCommandPoolResetFlags flags) {
  for (auto& commandBufferState :
       SD()._commandpoolstates[commandPool]->commandBufferStateStoreList) {
    vkResetCommandBuffer_SD(return_value, commandBufferState->commandBufferHandle, flags);
  }
}

inline void vkDestroyCommandPool_SD(VkDevice device,
                                    VkCommandPool commandPool,
                                    const VkAllocationCallbacks* pAllocator) {
  if (commandPool != 0) {
    auto& commandPoolState = SD()._commandpoolstates[commandPool];
    for (auto& commandBufferState : commandPoolState->commandBufferStateStoreList) {
      SD()._commandbufferstates.erase(commandBufferState->commandBufferHandle);

      if ((TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED ==
           Config::Get().recorder.vulkan.utilities.memoryUpdateState) ||
          isSubcaptureBeforeRestorationPhase()) {
        SD().bindingBuffers.erase(commandBufferState->commandBufferHandle);
        SD().bindingImages.erase(commandBufferState->commandBufferHandle);
      }

      if (Config::Get().recorder.vulkan.utilities.memorySegmentSize ||
          Config::Get().recorder.vulkan.utilities.shadowMemory) {
        SD().updatedMemoryInCmdBuffer.erase(commandBufferState->commandBufferHandle);
      }
    }

    SD()._commandpoolstates.erase(commandPool);
  }
}

// Sampler

inline void vkCreateSampler_SD(VkResult return_value,
                               VkDevice device,
                               const VkSamplerCreateInfo* pCreateInfo,
                               const VkAllocationCallbacks* pAllocator,
                               VkSampler* pSampler) {
  if (((Config::Get().IsPlayer()) && (Config::Get().player.cleanResourcesOnExit)) ||
      (isSubcaptureBeforeRestorationPhase())) {
    if ((return_value == VK_SUCCESS) && (*pSampler != VK_NULL_HANDLE)) {
      SD()._samplerstates.emplace(
          *pSampler,
          std::make_shared<CSamplerState>(pSampler, pCreateInfo, SD()._devicestates[device]));
    }
  }
}

inline void vkDestroySampler_SD(VkDevice device,
                                VkSampler sampler,
                                const VkAllocationCallbacks* pAllocator) {
  if (((Config::Get().IsPlayer()) && (Config::Get().player.cleanResourcesOnExit)) ||
      (isSubcaptureBeforeRestorationPhase())) {
    SD()._samplerstates.erase(sampler);
  }
}

// Device memory

inline void vkMapMemory_SD(VkResult return_value,
                           VkDevice device,
                           VkDeviceMemory memory,
                           VkDeviceSize offset,
                           VkDeviceSize size,
                           VkMemoryMapFlags flags,
                           void** ppData) {
  auto& memoryState = SD()._devicememorystates[memory];

  if (VK_SUCCESS != return_value) {
    return;
  }

  uint64_t unmapSize = size;
  if (unmapSize == 0xFFFFFFFFFFFFFFFF) {
    unmapSize = memoryState->memoryAllocateInfoData.Value()->allocationSize - offset;
  }

  memoryState->mapping.reset(new CDeviceMemoryState::CMapping(&offset, &unmapSize, &flags, ppData));

  if (Config::Get().player.printMemUsageVk) {
    SD().currentlyMappedMemory += unmapSize;
    Log(INFO) << "Currently Allocated Memory TOTAL: " << SD().currentlyAllocatedMemoryAll / 1000000
              << " MB; GPU_ONLY: " << SD().currentlyAllocatedMemoryGPU / 1000000
              << " MB; CPU_GPU_Shared: " << SD().currentlyAllocatedMemoryCPU_GPU / 1000000
              << " MB; Currently mapped memory: " << SD().currentlyMappedMemory / 1000000 << " MB";
  }

  if (Config::Get().IsRecorder()) {
    if (Config::Get().recorder.vulkan.utilities.useExternalMemoryExtension) {
      ExternalMemoryRegion::ResetTouchedPages(*ppData, unmapSize);
    } else if (Config::Get().recorder.vulkan.utilities.memoryAccessDetection) {
      MemorySniffer::Install();

      auto& sniffedRegionHandle = memoryState->mapping->sniffedRegionHandle;
      sniffedRegionHandle = MemorySniffer::Get().CreateRegion(*ppData, (size_t)unmapSize);
      if ((0 == sniffedRegionHandle) || (0 == *sniffedRegionHandle)) {
        Log(ERR) << "MemorySniffer setup for memory: " << memory << " with mapped ptr: " << *ppData
                 << " failed.";
        throw std::runtime_error(EXCEPTION_MESSAGE);
      }
      if (!MemorySniffer::Get().Protect(sniffedRegionHandle)) {
        Log(ERR) << "Protecting memory region: " << (**sniffedRegionHandle).BeginAddress() << " - "
                 << (**sniffedRegionHandle).EndAddress() << " FAILED!.";
        if (!Config::Get().recorder.vulkan.utilities.shadowMemory) {
          Log(ERR) << "Please try to record with enabled ShadowMemory option.";
        }
      }
    }
  }
}

inline void vkUnmapMemory_SD(VkDevice device, VkDeviceMemory memory) {
  auto& memoryState = SD()._devicememorystates[memory];

  if (memoryState->IsMapped()) {
    if (Config::Get().IsRecorder() &&
        Config::Get().recorder.vulkan.utilities.memoryAccessDetection) {
      MemorySniffer::Get().RemoveRegion(memoryState->mapping->sniffedRegionHandle);
    }

    if (Config::Get().player.printMemUsageVk) {
      SD().currentlyMappedMemory -= memoryState->mapping->sizeData.Value();
    }

    memoryState->mapping.reset();
  }

  if (Config::Get().IsRecorder() && Config::Get().recorder.vulkan.utilities.shadowMemory) {
    memoryState->shadowMemory.reset();
  }
}

inline void vkAllocateMemory_SD(VkResult return_value,
                                VkDevice device,
                                const VkMemoryAllocateInfo* pAllocInfo,
                                const VkAllocationCallbacks* pAllocator,
                                VkDeviceMemory* pMemory,
                                void* externalMemory = nullptr) {
  if ((return_value == VK_SUCCESS) && (*pMemory != VK_NULL_HANDLE)) {
    SD()._devicememorystates.emplace(
        *pMemory, std::make_shared<CDeviceMemoryState>(pMemory, pAllocInfo,
                                                       SD()._devicestates[device], externalMemory));
    if (Config::Get().player.printMemUsageVk) {
      SD().currentlyAllocatedMemoryAll += pAllocInfo->allocationSize;
      if (checkMemoryMappingFeasibility(device, pAllocInfo->memoryTypeIndex, false)) {
        SD().currentlyAllocatedMemoryCPU_GPU += pAllocInfo->allocationSize;
      } else {
        SD().currentlyAllocatedMemoryGPU += pAllocInfo->allocationSize;
      }
      Log(INFO) << "Currently Allocated Memory TOTAL: "
                << SD().currentlyAllocatedMemoryAll / 1000000
                << " MB; GPU_ONLY: " << SD().currentlyAllocatedMemoryGPU / 1000000
                << " MB; CPU_GPU_Shared: " << SD().currentlyAllocatedMemoryCPU_GPU / 1000000
                << " MB; Currently mapped memory: " << SD().currentlyMappedMemory / 1000000
                << " MB";
    }
  }
}

inline void vkFreeMemory_SD(VkDevice device,
                            VkDeviceMemory memory,
                            const VkAllocationCallbacks* pAllocator) {
  vkUnmapMemory_SD(device, memory);
  if (Config::Get().player.printMemUsageVk) {
    SD().currentlyAllocatedMemoryAll -=
        SD()._devicememorystates[memory]->memoryAllocateInfoData.Value()->allocationSize;
    if (checkMemoryMappingFeasibility(
            device,
            SD()._devicememorystates[memory]->memoryAllocateInfoData.Value()->memoryTypeIndex,
            false)) {
      SD().currentlyAllocatedMemoryCPU_GPU -=
          SD()._devicememorystates[memory]->memoryAllocateInfoData.Value()->allocationSize;
    } else {
      SD().currentlyAllocatedMemoryGPU -=
          SD()._devicememorystates[memory]->memoryAllocateInfoData.Value()->allocationSize;
    }
    Log(INFO) << "Currently Allocated Memory TOTAL: " << SD().currentlyAllocatedMemoryAll / 1000000
              << " MB; GPU_ONLY: " << SD().currentlyAllocatedMemoryGPU / 1000000
              << " MB; CPU_GPU_Shared: " << SD().currentlyAllocatedMemoryCPU_GPU / 1000000
              << " MB; Currently mapped memory: " << SD().currentlyMappedMemory / 1000000 << " MB";
  }
  SD()._devicememorystates.erase(memory); // Stardust
}

// Image

inline void vkCreateImage_SD(VkResult return_value,
                             VkDevice device,
                             const VkImageCreateInfo* pCreateInfo,
                             const VkAllocationCallbacks* pAllocator,
                             VkImage* pImage) {
  if ((return_value == VK_SUCCESS) && (*pImage != VK_NULL_HANDLE)) {
    VkImage image = *pImage;
    SD().imageCounter[image] = CGits::Instance().vkCounters.CurrentImageCount();
    SD()._imagestates.emplace(
        image, std::make_shared<CImageState>(&image, pCreateInfo, SD()._devicestates[device]));
  }
}

inline void vkDestroyImage_SD(VkDevice device,
                              VkImage image,
                              const VkAllocationCallbacks* pAllocator) {
  if (Config::Get().IsRecorder() && isSubcaptureBeforeRestorationPhase()) {
    auto iterator = SD()._imagestates.find(image);
    if ((iterator != SD()._imagestates.end()) && (iterator->second->binding != nullptr)) {
      iterator->second->binding->deviceMemoryStateStore->aliasingTracker.RemoveImage(
          iterator->second->binding->memoryOffset, iterator->second->binding->memorySizeRequirement,
          image);
    }
  }
  SD().nonDeterministicImages.erase(image);
  SD()._imagestates.erase(image); // Stardust ImageView
}

inline void vkGetImageMemoryRequirements_SD(VkDevice device,
                                            VkImage image,
                                            VkMemoryRequirements* pMemoryRequirements) {
  SD()._imagestates[image]->memoryRequirements = *pMemoryRequirements;
}

inline void vkGetImageMemoryRequirements2_SD(VkDevice device,
                                             const VkImageMemoryRequirementsInfo2* pInfo,
                                             VkMemoryRequirements2* pMemoryRequirements) {
  if (pInfo == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  vkGetImageMemoryRequirements_SD(device, pInfo->image, &pMemoryRequirements->memoryRequirements);
}

inline void vkGetImageMemoryRequirements2KHR_SD(VkDevice device,
                                                const VkImageMemoryRequirementsInfo2* pInfo,
                                                VkMemoryRequirements2* pMemoryRequirements) {
  if (pInfo == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  vkGetImageMemoryRequirements_SD(device, pInfo->image, &pMemoryRequirements->memoryRequirements);
}

namespace {
void BindImageMemory_SDHelper(VkImage image, VkDeviceMemory memory, VkDeviceSize memOffset) {
  auto& imageState = SD()._imagestates[image];
  imageState->binding.reset(new CMemoryBinding(memOffset, imageState->memoryRequirements.size,
                                               SD()._devicememorystates[memory]));

  if (Config::Get().IsRecorder() && isSubcaptureBeforeRestorationPhase()) {
    imageState->binding->deviceMemoryStateStore->aliasingTracker.AddImage(
        memOffset, imageState->memoryRequirements.size, image);
  }
}
} // namespace

inline void vkBindImageMemory_SD(VkResult return_value,
                                 VkDevice device,
                                 VkImage image,
                                 VkDeviceMemory mem,
                                 VkDeviceSize memOffset) {
  BindImageMemory_SDHelper(image, mem, memOffset);
}

inline void vkBindImageMemory2_SD(VkResult return_value,
                                  VkDevice device,
                                  uint32_t bindInfoCount,
                                  const VkBindImageMemoryInfo* pBindInfos) {
  for (uint32_t i = 0; i < bindInfoCount; ++i) {
    VkBindImageMemoryInfo const& bindInfo = pBindInfos[i];
    BindImageMemory_SDHelper(bindInfo.image, bindInfo.memory, bindInfo.memoryOffset);
  }
}

inline void vkBindImageMemory2KHR_SD(VkResult return_value,
                                     VkDevice device,
                                     uint32_t bindInfoCount,
                                     const VkBindImageMemoryInfo* pBindInfos) {
  vkBindImageMemory2_SD(return_value, device, bindInfoCount, pBindInfos);
}

// Image view

inline void vkCreateImageView_SD(VkResult return_value,
                                 VkDevice device,
                                 const VkImageViewCreateInfo* pCreateInfo,
                                 const VkAllocationCallbacks* pAllocator,
                                 VkImageView* pView) {
  if ((return_value == VK_SUCCESS) && (*pView != VK_NULL_HANDLE)) {
    SD()._imageviewstates.emplace(
        *pView, std::make_shared<CImageViewState>(pView, pCreateInfo, SD()._devicestates[device],
                                                  SD()._imagestates[pCreateInfo->image]));
  }
}

inline void vkDestroyImageView_SD(VkDevice device,
                                  VkImageView imageView,
                                  const VkAllocationCallbacks* pAllocator) {
  SD()._imageviewstates.erase(imageView); // Stardust UpdateDescription
}

// Buffer

inline void vkCreateBuffer_SD(VkResult return_value,
                              VkDevice device,
                              const VkBufferCreateInfo* pCreateInfo,
                              const VkAllocationCallbacks* pAllocator,
                              VkBuffer* pBuffer) {
  if ((return_value == VK_SUCCESS) && (*pBuffer != VK_NULL_HANDLE)) {
    VkBuffer buffer = *pBuffer;
    SD().bufferCounter[buffer] = CGits::Instance().vkCounters.CurrentBufferCount();
    auto newBufferState =
        std::make_shared<CBufferState>(&buffer, pCreateInfo, SD()._devicestates[device]);
    SD()._bufferstates.emplace(buffer, newBufferState);

    if (Config::Get().IsRecorder() &&
        ((pCreateInfo->usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) ==
         VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)) {
      CBufferState::shaderDeviceAddressBuffers[buffer] = newBufferState;
    }
  }
}

inline void vkDestroyBuffer_SD(VkDevice device,
                               VkBuffer buffer,
                               const VkAllocationCallbacks* pAllocator) {
  if (Config::Get().IsRecorder() && isSubcaptureBeforeRestorationPhase()) {
    auto iterator = SD()._bufferstates.find(buffer);
    if ((iterator != SD()._bufferstates.end()) && (iterator->second->binding != nullptr)) {
      iterator->second->binding->deviceMemoryStateStore->aliasingTracker.RemoveBuffer(
          iterator->second->binding->memoryOffset, iterator->second->binding->memorySizeRequirement,
          buffer);
    }
  }

  CBufferState::shaderDeviceAddressBuffers.erase(buffer);
  CBufferState::deviceAddressesMap.erase(SD()._bufferstates[buffer]->deviceAddress);
  SD()._bufferstates.erase(buffer); //SDK
}

inline void vkGetBufferMemoryRequirements_SD(VkDevice device,
                                             VkBuffer buffer,
                                             VkMemoryRequirements* pMemoryRequirements) {
  SD()._bufferstates[buffer]->memoryRequirements = *pMemoryRequirements;
}

inline void vkGetBufferMemoryRequirements2_SD(VkDevice device,
                                              const VkBufferMemoryRequirementsInfo2* pInfo,
                                              VkMemoryRequirements2* pMemoryRequirements) {
  if (pInfo == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  vkGetBufferMemoryRequirements_SD(device, pInfo->buffer, &pMemoryRequirements->memoryRequirements);
}

inline void vkGetBufferMemoryRequirements2KHR_SD(VkDevice device,
                                                 const VkBufferMemoryRequirementsInfo2* pInfo,
                                                 VkMemoryRequirements2* pMemoryRequirements) {
  if (pInfo == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  vkGetBufferMemoryRequirements_SD(device, pInfo->buffer, &pMemoryRequirements->memoryRequirements);
}

namespace {
void BindBufferMemory_SDHelper(VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memOffset) {
  auto& bufferState = SD()._bufferstates[buffer];
  bufferState->binding.reset(new CMemoryBinding(memOffset, bufferState->memoryRequirements.size,
                                                SD()._devicememorystates[memory]));

  if (Config::Get().IsRecorder() && isSubcaptureBeforeRestorationPhase()) {
    bufferState->binding->deviceMemoryStateStore->aliasingTracker.AddBuffer(
        memOffset, bufferState->memoryRequirements.size, buffer);
  }
}
} // namespace

inline void vkBindBufferMemory_SD(VkResult return_value,
                                  VkDevice device,
                                  VkBuffer buffer,
                                  VkDeviceMemory mem,
                                  VkDeviceSize memOffset) {
  BindBufferMemory_SDHelper(buffer, mem, memOffset);
}

inline void vkBindBufferMemory2_SD(VkResult return_value,
                                   VkDevice device,
                                   uint32_t bindInfoCount,
                                   const VkBindBufferMemoryInfo* pBindInfos) {
  for (uint32_t i = 0; i < bindInfoCount; ++i) {
    VkBindBufferMemoryInfo const& bindInfo = pBindInfos[i];
    BindBufferMemory_SDHelper(bindInfo.buffer, bindInfo.memory, bindInfo.memoryOffset);
  }
}

inline void vkBindBufferMemory2KHR_SD(VkResult return_value,
                                      VkDevice device,
                                      uint32_t bindInfoCount,
                                      const VkBindBufferMemoryInfo* pBindInfos) {
  vkBindBufferMemory2_SD(return_value, device, bindInfoCount, pBindInfos);
}

inline void vkGetBufferDeviceAddressUnifiedGITS_SD(VkDeviceAddress return_value,
                                                   VkDevice device,
                                                   const VkBufferDeviceAddressInfo* pInfo) {
  if (pInfo == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  auto& bufferState = SD()._bufferstates[pInfo->buffer];
  bufferState->deviceAddress =
      return_value; // <- We need this data to properly replay streams (see CBufferDeviceAddressObject class)

  if (Config::Get().IsRecorder()) {
    CBufferState::deviceAddressesMap[return_value] = {
        return_value + bufferState->bufferCreateInfoData.Value()->size, bufferState};
  }
}

// Buffer view

inline void vkCreateBufferView_SD(VkResult return_value,
                                  VkDevice device,
                                  const VkBufferViewCreateInfo* pCreateInfo,
                                  const VkAllocationCallbacks* pAllocator,
                                  VkBufferView* pView) {
  if ((return_value == VK_SUCCESS) && (*pView != VK_NULL_HANDLE)) {
    SD()._bufferviewstates.emplace(
        *pView, std::make_shared<CBufferViewState>(pView, pCreateInfo, SD()._devicestates[device],
                                                   SD()._bufferstates[pCreateInfo->buffer]));
  }
}

inline void vkDestroyBufferView_SD(VkDevice device,
                                   VkBufferView bufferView,
                                   const VkAllocationCallbacks* pAllocator) {
  SD()._bufferviewstates.erase(bufferView);
}

// Descriptor set layout

inline void vkCreateDescriptorSetLayout_SD(VkResult return_value,
                                           VkDevice device,
                                           const VkDescriptorSetLayoutCreateInfo* pCreateInfo,
                                           const VkAllocationCallbacks* pAllocator,
                                           VkDescriptorSetLayout* pSetLayout) {
  if ((return_value == VK_SUCCESS) && (*pSetLayout != VK_NULL_HANDLE)) {
    SD()._descriptorsetlayoutstates.emplace(
        *pSetLayout, std::make_shared<CDescriptorSetLayoutState>(pSetLayout, pCreateInfo,
                                                                 SD()._devicestates[device]));
  }
}

inline void vkDestroyDescriptorSetLayout_SD(VkDevice device,
                                            VkDescriptorSetLayout descriptorSetLayout,
                                            const VkAllocationCallbacks* pAllocator) {
  SD()._descriptorsetlayoutstates.erase(descriptorSetLayout);
}

// Descriptor set

inline void vkAllocateDescriptorSets_SD(VkResult return_value,
                                        VkDevice device,
                                        const VkDescriptorSetAllocateInfo* pAllocateInfo,
                                        VkDescriptorSet* pDescriptorSets) {
  if ((Config::Get().IsRecorder() || !Config::Get().player.captureVulkanSubmitsResources.empty()) &&
      (return_value == VK_SUCCESS)) {

    const VkDescriptorSetVariableDescriptorCountAllocateInfo* variableDescriptorCount =
        (const VkDescriptorSetVariableDescriptorCountAllocateInfo*)getPNextStructure(
            pAllocateInfo->pNext,
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO);
    if ((variableDescriptorCount != nullptr) &&
        (variableDescriptorCount->descriptorSetCount == 0)) {
      variableDescriptorCount = nullptr;
    }

    for (unsigned int i = 0; i < pAllocateInfo->descriptorSetCount; i++) {
      if (pDescriptorSets[i] == VK_NULL_HANDLE) {
        continue;
      }

      // Store extension data if needed
      std::shared_ptr<VkDescriptorSetVariableDescriptorCountAllocateInfo> pNextChain;
      if (variableDescriptorCount != nullptr) {
        VkDescriptorSetVariableDescriptorCountAllocateInfo variableDescriptorCountAllocateInfo = {
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO, // VkStructureType sType;
            nullptr,                                       // const void* pNext;
            1,                                             // uint32_t descriptorSetCount;
            &variableDescriptorCount->pDescriptorCounts[i] // const uint32_t* pDescriptorCounts;
        };
        pNextChain = std::make_shared<VkDescriptorSetVariableDescriptorCountAllocateInfo>(
            variableDescriptorCountAllocateInfo);
      }

      auto descriptorSetState = std::make_shared<CDescriptorSetState>(
          &pDescriptorSets[i], pNextChain.get(),
          SD()._descriptorpoolstates[pAllocateInfo->descriptorPool],
          SD()._descriptorsetlayoutstates[pAllocateInfo->pSetLayouts[i]]);
      SD()._descriptorsetstates.emplace(pDescriptorSets[i], descriptorSetState);
      SD()._descriptorpoolstates[pAllocateInfo->descriptorPool]->descriptorSetStateStoreList.insert(
          descriptorSetState);

      if (isSubcaptureBeforeRestorationPhase()) {
        VkDescriptorSetLayoutCreateInfo* layoutCreateInfo =
            descriptorSetState->descriptorSetLayoutStateStore->descriptorSetLayoutCreateInfoData
                .Value();
        const VkDescriptorSetLayoutBindingFlagsCreateInfo* bindingFlagsCreateInfo =
            (const VkDescriptorSetLayoutBindingFlagsCreateInfo*)getPNextStructure(
                layoutCreateInfo->pNext,
                VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO);
        if ((bindingFlagsCreateInfo != nullptr) && (bindingFlagsCreateInfo->bindingCount == 0)) {
          bindingFlagsCreateInfo = nullptr;
        }

        for (uint32_t j = 0; j < layoutCreateInfo->bindingCount; j++) {
          auto& layoutBindingData = layoutCreateInfo->pBindings[j];
          auto& descriptorSetBinding =
              descriptorSetState->descriptorSetBindings[layoutBindingData.binding];
          uint32_t descriptorCount = layoutBindingData.descriptorCount;

          // Check if the number of descriptors is modified by the VK_EXT_descriptor_indexing extension (VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT flag)
          if (variableDescriptorCount != nullptr) {
            // Flags are specified for each descriptor set / for each binding
            VkDescriptorBindingFlagsEXT bindingFlags =
                (bindingFlagsCreateInfo != nullptr) ? bindingFlagsCreateInfo->pBindingFlags[j] : 0;

            // Variable descriptor count is specified for each descriptor set (only one value for the whole descriptor set, just for the last binding)
            if (bindingFlags & VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT) {
              descriptorCount = variableDescriptorCount->pDescriptorCounts[i];
            }
          }

          descriptorSetBinding.descriptorType = layoutBindingData.descriptorType;
          descriptorSetBinding.descriptorCount = descriptorCount;

          switch (layoutBindingData.descriptorType) {
          case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK:
            descriptorSetBinding.descriptorData.resize(1);
            descriptorSetBinding.descriptorData[0].inlineUniformBlockData.resize(descriptorCount);
            break;
          default:
            descriptorSetBinding.descriptorData.resize(descriptorCount);
          }
        }
      }
    }
  }
}

inline void vkUpdateDescriptorSets_SD(VkDevice device,
                                      uint32_t descriptorWriteCount,
                                      const VkWriteDescriptorSet* pDescriptorWrites,
                                      uint32_t descriptorCopyCount,
                                      const VkCopyDescriptorSet* pDescriptorCopies) {
  if (Config::Get().IsRecorder() || !Config::Get().player.captureVulkanSubmitsResources.empty()) {
    for (unsigned int i = 0; i < descriptorWriteCount; i++) {
      auto& descriptorSetState = SD()._descriptorsetstates[pDescriptorWrites[i].dstSet];

      uint32_t currentBinding = pDescriptorWrites[i].dstBinding;
      uint32_t currentArrayOffset = pDescriptorWrites[i].dstArrayElement;
      const auto descriptorType = pDescriptorWrites[i].descriptorType;

      if (descriptorType == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK) {
        if (isSubcaptureBeforeRestorationPhase()) {
          auto* descriptorSetBinding = &descriptorSetState->descriptorSetBindings[currentBinding];
          if (pDescriptorWrites[i].pNext != nullptr) {
            auto block = static_cast<const VkWriteDescriptorSetInlineUniformBlock*>(
                pDescriptorWrites[i].pNext);
            auto dst_ptr =
                &descriptorSetBinding->descriptorData[0].inlineUniformBlockData[currentArrayOffset];
            memcpy(dst_ptr, block->pData, block->dataSize);
          } else {
            Log(ERR) << "Inline uniform block descriptor write is missing "
                        "VkWriteDescriptorSetInlineUniformBlock structure";
            throw std::runtime_error(EXCEPTION_MESSAGE);
          }
        }
      } else {
        for (unsigned int j = 0; j < pDescriptorWrites[i].descriptorCount; j++) {
          auto* descriptorSetBinding = &descriptorSetState->descriptorSetBindings[currentBinding];
          CDescriptorSetState::CDescriptorSetBindingData::CDescriptorData* currentDescriptorData =
              nullptr;
          if (isSubcaptureBeforeRestorationPhase()) {
            assert(descriptorSetBinding->descriptorData.size() > 0);
            while (descriptorSetBinding->descriptorCount <= currentArrayOffset) {
              currentArrayOffset =
                  currentArrayOffset - (uint32_t)descriptorSetBinding->descriptorCount;
              currentBinding++;
              descriptorSetBinding = &descriptorSetState->descriptorSetBindings[currentBinding];
            }

            currentDescriptorData = &descriptorSetBinding->descriptorData[currentArrayOffset];

            currentDescriptorData->bufferStateStore.reset();
            currentDescriptorData->bufferViewStateStore.reset();
            currentDescriptorData->imageViewStateStore.reset();
            currentDescriptorData->pBufferInfo.reset();
            currentDescriptorData->pImageInfo.reset();
            currentDescriptorData->pTexelBufferView.reset();
            currentDescriptorData->samplerStateStore.reset();
          }

          switch (descriptorType) {
          case VK_DESCRIPTOR_TYPE_SAMPLER:
            if ((pDescriptorWrites[i].pImageInfo != nullptr) &&
                (pDescriptorWrites[i].pImageInfo[j].sampler != VK_NULL_HANDLE) &&
                isSubcaptureBeforeRestorationPhase() && (currentDescriptorData != nullptr)) {
              currentDescriptorData->pImageInfo.reset(new CVkDescriptorImageInfoData(
                  &pDescriptorWrites[i].pImageInfo[j], descriptorType));
              currentDescriptorData->samplerStateStore =
                  SD()._samplerstates[pDescriptorWrites[i].pImageInfo[j].sampler];
            }
            break;
          case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
          case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
          case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
          case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
            if ((pDescriptorWrites[i].pImageInfo != nullptr) &&
                (pDescriptorWrites[i].pImageInfo[j].imageView != VK_NULL_HANDLE)) {
              auto& imageViewState =
                  SD()._imageviewstates[pDescriptorWrites[i].pImageInfo[j].imageView];
              auto& imageState = imageViewState->imageStateStore;

              if (isSubcaptureBeforeRestorationPhase() && (currentDescriptorData != nullptr)) {
                currentDescriptorData->pImageInfo.reset(new CVkDescriptorImageInfoData(
                    &pDescriptorWrites[i].pImageInfo[j], descriptorType));
                currentDescriptorData->imageViewStateStore = imageViewState;
                if ((descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) &&
                    ((pDescriptorWrites[i].pImageInfo[j].sampler != VK_NULL_HANDLE))) {
                  currentDescriptorData->samplerStateStore =
                      SD()._samplerstates[pDescriptorWrites[i].pImageInfo[j].sampler];
                }
              }

              if ((Config::Get().recorder.vulkan.utilities.memoryUpdateState ==
                   TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED) ||
                  (isSubcaptureBeforeRestorationPhase())) {
                descriptorSetState->descriptorImages[pDescriptorWrites[i].dstBinding] =
                    imageState->imageHandle;
              }

              if (descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) {
                descriptorSetState->descriptorWriteImages[pDescriptorWrites[i].dstBinding] = {
                    VULKAN_STORAGE_IMAGE, imageState->imageHandle};

                if ((Config::Get().recorder.vulkan.utilities.memorySegmentSize ||
                     Config::Get().recorder.vulkan.utilities.shadowMemory) &&
                    (imageState->binding)) {
                  VkDeviceSize dstOffsetFinal = imageState->binding->memoryOffset;
                  VkDeviceMemory dstDeviceMemory =
                      imageState->binding->deviceMemoryStateStore->deviceMemoryHandle;
                  VkMemoryRequirements memRequirements = {};
                  drvVk.vkGetImageMemoryRequirements(imageState->deviceStateStore->deviceHandle,
                                                     imageState->imageHandle, &memRequirements);
                  //TODO : call vkGetImageSubresourceLayout when tiling is Linear

                  boost::icl::interval<uint64_t>::type interv(dstOffsetFinal, memRequirements.size +
                                                                                  dstOffsetFinal);
                  descriptorSetState->descriptorMapMemory[pDescriptorWrites[i].dstBinding].clear();
                  descriptorSetState
                      ->descriptorMapMemory[pDescriptorWrites[i].dstBinding][dstDeviceMemory]
                      .insert(interv);
                }
              }
            }
            break;
          case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
          case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
            if ((pDescriptorWrites[i].pTexelBufferView != nullptr) &&
                (pDescriptorWrites[i].pTexelBufferView[j]) != VK_NULL_HANDLE) {
              auto& bufferViewState =
                  SD()._bufferviewstates[pDescriptorWrites[i].pTexelBufferView[j]];

              if (isSubcaptureBeforeRestorationPhase() && (currentDescriptorData != nullptr)) {
                currentDescriptorData->pTexelBufferView.reset(
                    new CVkBufferViewDataArray(1, &pDescriptorWrites[i].pTexelBufferView[j]));
                currentDescriptorData->bufferViewStateStore = bufferViewState;
              }

              if ((Config::Get().recorder.vulkan.utilities.memoryUpdateState ==
                   TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED) ||
                  (isSubcaptureBeforeRestorationPhase())) {
                descriptorSetState->descriptorBuffers[pDescriptorWrites[i].dstBinding] =
                    bufferViewState->bufferStateStore->bufferHandle;
              }
              if (descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER) {
                descriptorSetState->descriptorWriteBuffers[pDescriptorWrites[i].dstBinding] = {
                    VULKAN_STORAGE_TEXEL_BUFFER, bufferViewState->bufferStateStore->bufferHandle};

                if ((Config::Get().recorder.vulkan.utilities.memorySegmentSize ||
                     Config::Get().recorder.vulkan.utilities.shadowMemory) &&
                    (bufferViewState->bufferStateStore->binding)) {
                  VkDeviceSize dstOffsetFinal =
                      bufferViewState->bufferStateStore->binding->memoryOffset +
                      bufferViewState->bufferViewCreateInfoData.Value()->offset;
                  VkDeviceSize dstFinalSize;
                  if (bufferViewState->bufferViewCreateInfoData.Value()->range ==
                      0xFFFFFFFFFFFFFFFF) {
                    dstFinalSize =
                        bufferViewState->bufferStateStore->bufferCreateInfoData.Value()->size -
                        bufferViewState->bufferViewCreateInfoData.Value()->offset;
                  } else {
                    dstFinalSize = bufferViewState->bufferViewCreateInfoData.Value()->range;
                  }

                  boost::icl::interval<uint64_t>::type interv(dstOffsetFinal,
                                                              dstFinalSize + dstOffsetFinal);
                  descriptorSetState->descriptorMapMemory[pDescriptorWrites[i].dstBinding].clear();
                  descriptorSetState
                      ->descriptorMapMemory[pDescriptorWrites[i].dstBinding]
                                           [bufferViewState->bufferStateStore->binding
                                                ->deviceMemoryStateStore->deviceMemoryHandle]
                      .insert(interv);
                }
              }
            }
            break;
          case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
          case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
          case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
          case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            if ((pDescriptorWrites[i].pBufferInfo != nullptr) &&
                (pDescriptorWrites[i].pBufferInfo[j].buffer != VK_NULL_HANDLE)) {
              auto& bufferState = SD()._bufferstates[pDescriptorWrites[i].pBufferInfo[j].buffer];

              if (isSubcaptureBeforeRestorationPhase() && (currentDescriptorData != nullptr)) {
                currentDescriptorData->pBufferInfo.reset(
                    new CVkDescriptorBufferInfoData(&pDescriptorWrites[i].pBufferInfo[j]));
                currentDescriptorData->bufferStateStore = bufferState;
              }

              if ((Config::Get().recorder.vulkan.utilities.memoryUpdateState ==
                   TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED) ||
                  (isSubcaptureBeforeRestorationPhase())) {
                descriptorSetState->descriptorBuffers[pDescriptorWrites[i].dstBinding] =
                    pDescriptorWrites[i].pBufferInfo[j].buffer;
              }
              if ((descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) ||
                  (descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)) {
                VulkanResourceType resType = descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
                                                 ? VULKAN_STORAGE_BUFFER
                                                 : VULKAN_STORAGE_BUFFER_DYNAMIC;
                descriptorSetState->descriptorWriteBuffers[pDescriptorWrites[i].dstBinding] = {
                    resType, pDescriptorWrites[i].pBufferInfo[j].buffer};

                if ((Config::Get().recorder.vulkan.utilities.memorySegmentSize ||
                     Config::Get().recorder.vulkan.utilities.shadowMemory) &&
                    bufferState->binding) {
                  VkDeviceSize dstOffsetFinal = bufferState->binding->memoryOffset +
                                                pDescriptorWrites[i].pBufferInfo[j].offset;
                  VkDeviceSize dstFinalSize;
                  if (pDescriptorWrites[i].pBufferInfo[j].range == 0xFFFFFFFFFFFFFFFF) {
                    dstFinalSize = bufferState->bufferCreateInfoData.Value()->size -
                                   pDescriptorWrites[i].pBufferInfo[j].offset;
                  } else {
                    dstFinalSize = pDescriptorWrites[i].pBufferInfo[j].range;
                  }
                  boost::icl::interval<uint64_t>::type interv(dstOffsetFinal,
                                                              dstFinalSize + dstOffsetFinal);
                  descriptorSetState->descriptorMapMemory[pDescriptorWrites[i].dstBinding].clear();
                  descriptorSetState
                      ->descriptorMapMemory[pDescriptorWrites[i].dstBinding]
                                           [bufferState->binding->deviceMemoryStateStore
                                                ->deviceMemoryHandle]
                      .insert(interv);
                }
              }
            }
            break;
          default:
            Log(TRACE) << "Not handled VkDescriptorType enumeration: " +
                              std::to_string(descriptorType);
            break;
          }
          currentArrayOffset++;
        }
      }
    }

    for (unsigned int i = 0; i < descriptorCopyCount; i++) {
      auto& srcDescriptorSetState = SD()._descriptorsetstates[pDescriptorCopies[i].srcSet];
      auto& dstDescriptorSetState = SD()._descriptorsetstates[pDescriptorCopies[i].dstSet];

      if (isSubcaptureBeforeRestorationPhase()) {
        uint32_t srcArrayOffset = pDescriptorCopies[i].srcArrayElement;
        uint32_t dstArrayOffset = pDescriptorCopies[i].dstArrayElement;
        auto& srcDescriptorSetBinding =
            srcDescriptorSetState->descriptorSetBindings[pDescriptorCopies[i].srcBinding];
        auto& dstDescriptorSetBinding =
            dstDescriptorSetState->descriptorSetBindings[pDescriptorCopies[i].dstBinding];
        switch (srcDescriptorSetBinding.descriptorType) {
        case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK: {
          auto dst_ptr =
              &dstDescriptorSetBinding.descriptorData[0].inlineUniformBlockData[dstArrayOffset];
          auto src_ptr =
              &srcDescriptorSetBinding.descriptorData[0].inlineUniformBlockData[srcArrayOffset];
          memcpy(dst_ptr, src_ptr, pDescriptorCopies[i].descriptorCount);
          break;
        }
        default: {
          for (unsigned int j = 0; j < pDescriptorCopies[i].descriptorCount; ++j) {
            if ((dstArrayOffset + j < dstDescriptorSetBinding.descriptorCount) &&
                (srcArrayOffset + j < srcDescriptorSetBinding.descriptorCount)) {
              dstDescriptorSetBinding.descriptorData[dstArrayOffset + j] =
                  srcDescriptorSetBinding.descriptorData[srcArrayOffset + j];
            }
          }
        }
        }
      }

      if ((Config::Get().recorder.vulkan.utilities.memoryUpdateState ==
           TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED) ||
          (isSubcaptureBeforeRestorationPhase())) {
        for (auto obj : srcDescriptorSetState->descriptorBuffers) {
          dstDescriptorSetState->descriptorBuffers[obj.first] = obj.second;
        }

        for (auto obj : srcDescriptorSetState->descriptorImages) {
          dstDescriptorSetState->descriptorImages[obj.first] = obj.second;
        }
      }
      if (Config::Get().recorder.vulkan.utilities.memorySegmentSize ||
          Config::Get().recorder.vulkan.utilities.shadowMemory) {
        for (auto binding : srcDescriptorSetState->descriptorMapMemory) {
          dstDescriptorSetState->descriptorMapMemory[binding.first].clear();
          for (auto obj : binding.second) {
            for (auto obj2 : obj.second) {
              dstDescriptorSetState->descriptorMapMemory[binding.first][obj.first].insert(obj2);
            }
          }
        }
      }

      for (auto obj : srcDescriptorSetState->descriptorWriteBuffers) {
        dstDescriptorSetState->descriptorWriteBuffers[obj.first] = obj.second;
      }
      for (auto obj : srcDescriptorSetState->descriptorWriteImages) {
        dstDescriptorSetState->descriptorWriteImages[obj.first] = obj.second;
      }
    }
  }
}

inline void vkFreeDescriptorSets_SD(VkResult return_value,
                                    VkDevice device,
                                    VkDescriptorPool descriptorPool,
                                    uint32_t descriptorSetCount,
                                    const VkDescriptorSet* pDescriptorSets) {
  if (Config::Get().IsRecorder() || !Config::Get().player.captureVulkanSubmitsResources.empty()) {
    for (unsigned int i = 0; i < descriptorSetCount; i++) {
      TODO("Check if descriptor set is properly removed (if std::shared_ptr may be used correctly "
           "with std::set")
      SD()._descriptorpoolstates[descriptorPool]->descriptorSetStateStoreList.erase(
          SD()._descriptorsetstates
              [pDescriptorSets[i]]); // <- check if descriptorSetState is removed correctly
      SD()._descriptorsetstates.erase(pDescriptorSets[i]); // Stardust
    }
  }
}

// Pipeline layout

inline void vkCreatePipelineLayout_SD(VkResult return_value,
                                      VkDevice device,
                                      const VkPipelineLayoutCreateInfo* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator,
                                      VkPipelineLayout* pPipelineLayout) {
  if ((return_value == VK_SUCCESS) && (*pPipelineLayout != VK_NULL_HANDLE)) {
    auto state = std::make_shared<CPipelineLayoutState>(pPipelineLayout, pCreateInfo,
                                                        SD()._devicestates[device]);
    if (Config::Get().IsRecorder()) {
      for (uint32_t i = 0; i < pCreateInfo->setLayoutCount; i++) {
        if (pCreateInfo->pSetLayouts[i] == VK_NULL_HANDLE) {
          state->descriptorSetLayoutStates.push_back(0);
          continue;
        }

        const auto& it = SD()._descriptorsetlayoutstates.find(pCreateInfo->pSetLayouts[i]);
        if (it != SD()._descriptorsetlayoutstates.end()) {
          state->descriptorSetLayoutStates.push_back(it->second);
        } else {
          Log(ERR) << "Couldn't find state for VkDescriptorSetLayout: "
                   << pCreateInfo->pSetLayouts[i];
          throw std::runtime_error(EXCEPTION_MESSAGE);
        }
      }
    }
    SD()._pipelinelayoutstates.emplace(*pPipelineLayout, state);
  }
}

inline void vkDestroyPipelineLayout_SD(VkDevice device,
                                       VkPipelineLayout pipelineLayout,
                                       const VkAllocationCallbacks* pAllocator) {
  SD()._pipelinelayoutstates.erase(pipelineLayout);
}

// Descriptor update template

namespace {
inline void CreateDescriptorUpdateTemplate_SDHelper(
    VkDevice device,
    const VkDescriptorUpdateTemplateCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDescriptorUpdateTemplate* pDescriptorUpdateTemplate,
    CreationFunction createdWith) {
  if (VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_PUSH_DESCRIPTORS_KHR == pCreateInfo->templateType) {
    SD()._descriptorupdatetemplatestates.emplace(
        *pDescriptorUpdateTemplate, std::make_shared<CDescriptorUpdateTemplateState>(
                                        pDescriptorUpdateTemplate, pCreateInfo, createdWith,
                                        SD()._pipelinelayoutstates[pCreateInfo->pipelineLayout]));
  } else {
    SD()._descriptorupdatetemplatestates.emplace(
        *pDescriptorUpdateTemplate,
        std::make_shared<CDescriptorUpdateTemplateState>(pDescriptorUpdateTemplate, pCreateInfo,
                                                         createdWith, SD()._devicestates[device]));
  }
}
} // namespace

inline void vkCreateDescriptorUpdateTemplate_SD(
    VkResult return_value,
    VkDevice device,
    const VkDescriptorUpdateTemplateCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDescriptorUpdateTemplate* pDescriptorUpdateTemplate) {
  if (((Config::Get().IsPlayer()) &&
       (Config::Get().player.cleanResourcesOnExit ||
        !Config::Get().player.captureVulkanSubmitsResources.empty())) ||
      (Config::Get().IsRecorder())) {
    if ((return_value == VK_SUCCESS) && (*pDescriptorUpdateTemplate != VK_NULL_HANDLE)) {
      CreateDescriptorUpdateTemplate_SDHelper(
          device, pCreateInfo, pAllocator, pDescriptorUpdateTemplate, CreationFunction::CORE_1_1);
    }
  }
}

inline void vkCreateDescriptorUpdateTemplateKHR_SD(
    VkResult return_value,
    VkDevice device,
    const VkDescriptorUpdateTemplateCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDescriptorUpdateTemplate* pDescriptorUpdateTemplate) {
  if (((Config::Get().IsPlayer()) &&
       (Config::Get().player.cleanResourcesOnExit ||
        !Config::Get().player.captureVulkanSubmitsResources.empty())) ||
      (Config::Get().IsRecorder())) {
    if ((return_value == VK_SUCCESS) && (*pDescriptorUpdateTemplate != VK_NULL_HANDLE)) {
      CreateDescriptorUpdateTemplate_SDHelper(device, pCreateInfo, pAllocator,
                                              pDescriptorUpdateTemplate,
                                              CreationFunction::KHR_EXTENSION);
    }
  }
}

inline void vkDestroyDescriptorUpdateTemplate_SD(
    VkDevice device,
    VkDescriptorUpdateTemplate descriptorUpdateTemplate,
    const VkAllocationCallbacks* pAllocator) {
  if (((Config::Get().IsPlayer()) && (Config::Get().player.cleanResourcesOnExit)) ||
      (Config::Get().IsRecorder())) {
    SD()._descriptorupdatetemplatestates.erase(descriptorUpdateTemplate);
  }
}

inline void vkDestroyDescriptorUpdateTemplateKHR_SD(
    VkDevice device,
    VkDescriptorUpdateTemplate descriptorUpdateTemplate,
    const VkAllocationCallbacks* pAllocator) {
  if (((Config::Get().IsPlayer()) && (Config::Get().player.cleanResourcesOnExit)) ||
      (Config::Get().IsRecorder())) {
    vkDestroyDescriptorUpdateTemplate_SD(device, descriptorUpdateTemplate, pAllocator);
  }
}

inline void vkUpdateDescriptorSetWithTemplate_SD(
    VkDevice device,
    VkDescriptorSet descriptorSet,
    VkDescriptorUpdateTemplate descriptorUpdateTemplate,
    const void* pData) {
  if (Config::Get().IsRecorder() || !Config::Get().player.captureVulkanSubmitsResources.empty()) {
    auto& descriptorSetState = SD()._descriptorsetstates[descriptorSet];
    auto& descriptorUpdateTemplateState =
        SD()._descriptorupdatetemplatestates[descriptorUpdateTemplate];
    auto descriptorUpdateTemplateCreateInfoData =
        descriptorUpdateTemplateState->descriptorUpdateTemplateCreateInfoData.Value();

    for (unsigned int i = 0; i < descriptorUpdateTemplateCreateInfoData->descriptorUpdateEntryCount;
         i++) {
      auto& descriptorSetBinding =
          descriptorSetState->descriptorSetBindings
              [descriptorUpdateTemplateCreateInfoData->pDescriptorUpdateEntries[i].dstBinding];

      if (descriptorSetBinding.descriptorData.size() <
          descriptorUpdateTemplateCreateInfoData->pDescriptorUpdateEntries[i].descriptorCount) {
        descriptorSetBinding.descriptorData.resize(
            descriptorUpdateTemplateCreateInfoData->pDescriptorUpdateEntries[i].descriptorCount);
      }
      const auto descriptorType =
          descriptorUpdateTemplateCreateInfoData->pDescriptorUpdateEntries[i].descriptorType;
      descriptorSetBinding.descriptorType = descriptorType;

      uint32_t arrayOffset =
          descriptorUpdateTemplateCreateInfoData->pDescriptorUpdateEntries[i].dstArrayElement;

      for (unsigned int j = 0;
           j < descriptorUpdateTemplateCreateInfoData->pDescriptorUpdateEntries[i].descriptorCount;
           j++) {
        if (arrayOffset + j >= descriptorSetBinding.descriptorData.size()) {
          break;
        }
        CDescriptorSetState::CDescriptorSetBindingData::CDescriptorData* currentDescriptorData =
            nullptr;
        if (isSubcaptureBeforeRestorationPhase()) {

          currentDescriptorData = &descriptorSetBinding.descriptorData[arrayOffset + j];

          currentDescriptorData->bufferStateStore.reset();
          currentDescriptorData->bufferViewStateStore.reset();
          currentDescriptorData->imageViewStateStore.reset();
          currentDescriptorData->pBufferInfo.reset();
          currentDescriptorData->pImageInfo.reset();
          currentDescriptorData->pTexelBufferView.reset();
          currentDescriptorData->samplerStateStore.reset();
        }

        switch (descriptorType) {
        case VK_DESCRIPTOR_TYPE_SAMPLER:
          if (isSubcaptureBeforeRestorationPhase()) {
            VkDescriptorImageInfo* descriptorImageInfo =
                (VkDescriptorImageInfo*)((char*)pData +
                                         descriptorUpdateTemplateCreateInfoData
                                             ->pDescriptorUpdateEntries[i]
                                             .offset +
                                         descriptorUpdateTemplateCreateInfoData
                                                 ->pDescriptorUpdateEntries[i]
                                                 .stride *
                                             j);

            if (descriptorImageInfo->sampler != VK_NULL_HANDLE &&
                (currentDescriptorData != nullptr)) {
              currentDescriptorData->pImageInfo.reset(
                  new CVkDescriptorImageInfoData(descriptorImageInfo, descriptorType));
              currentDescriptorData->samplerStateStore =
                  SD()._samplerstates[descriptorImageInfo->sampler];
            }
          }
          break;
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: {
          VkDescriptorImageInfo* descriptorImageInfo =
              (VkDescriptorImageInfo*)((char*)pData +
                                       descriptorUpdateTemplateCreateInfoData
                                           ->pDescriptorUpdateEntries[i]
                                           .offset +
                                       descriptorUpdateTemplateCreateInfoData
                                               ->pDescriptorUpdateEntries[i]
                                               .stride *
                                           j);
          if (descriptorImageInfo->imageView != VK_NULL_HANDLE) {
            auto& imageViewState = SD()._imageviewstates[descriptorImageInfo->imageView];

            if (isSubcaptureBeforeRestorationPhase() && (currentDescriptorData != nullptr)) {
              currentDescriptorData->pImageInfo.reset(
                  new CVkDescriptorImageInfoData(descriptorImageInfo, descriptorType));
              currentDescriptorData->imageViewStateStore = imageViewState;
              if ((descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) &&
                  (descriptorImageInfo->sampler != VK_NULL_HANDLE)) {
                currentDescriptorData->samplerStateStore =
                    SD()._samplerstates[descriptorImageInfo->sampler];
              }
            }

            auto& imageState = imageViewState->imageStateStore;

            if ((Config::Get().recorder.vulkan.utilities.memoryUpdateState ==
                 TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED) ||
                (isSubcaptureBeforeRestorationPhase())) {
              descriptorSetState->descriptorImages
                  [descriptorUpdateTemplateCreateInfoData->pDescriptorUpdateEntries[i].dstBinding] =
                  imageState->imageHandle;
            }
            if (descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) {
              descriptorSetState->descriptorWriteImages[descriptorUpdateTemplateCreateInfoData
                                                            ->pDescriptorUpdateEntries[i]
                                                            .dstBinding] = {
                  VULKAN_STORAGE_IMAGE, imageState->imageHandle};

              if ((Config::Get().recorder.vulkan.utilities.memorySegmentSize ||
                   Config::Get().recorder.vulkan.utilities.shadowMemory) &&
                  (imageState->binding)) {
                VkDeviceSize dstOffsetFinal = imageState->binding->memoryOffset;
                VkDeviceMemory dstDeviceMemory =
                    imageState->binding->deviceMemoryStateStore->deviceMemoryHandle;
                VkMemoryRequirements memRequirements = {};
                drvVk.vkGetImageMemoryRequirements(imageState->deviceStateStore->deviceHandle,
                                                   imageState->imageHandle, &memRequirements);
                //TODO : call vkGetImageSubresourceLayout when tiling is Linear

                boost::icl::interval<uint64_t>::type interv(dstOffsetFinal,
                                                            memRequirements.size + dstOffsetFinal);
                descriptorSetState
                    ->descriptorMapMemory[descriptorUpdateTemplateCreateInfoData
                                              ->pDescriptorUpdateEntries[i]
                                              .dstBinding]
                    .clear();
                descriptorSetState
                    ->descriptorMapMemory[descriptorUpdateTemplateCreateInfoData
                                              ->pDescriptorUpdateEntries[i]
                                              .dstBinding][dstDeviceMemory]
                    .insert(interv);
              }
            }
          }
        } break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: {
          VkBufferView* bufferView =
              (VkBufferView*)((char*)pData +
                              descriptorUpdateTemplateCreateInfoData->pDescriptorUpdateEntries[i]
                                  .offset +
                              descriptorUpdateTemplateCreateInfoData->pDescriptorUpdateEntries[i]
                                      .stride *
                                  j);

          if (*bufferView != VK_NULL_HANDLE) {
            auto& bufferViewState = SD()._bufferviewstates[*bufferView];

            if (isSubcaptureBeforeRestorationPhase() && (currentDescriptorData != nullptr)) {
              currentDescriptorData->pTexelBufferView.reset(
                  new CVkBufferViewDataArray(1, bufferView));
              currentDescriptorData->bufferViewStateStore = bufferViewState;
            }

            if ((Config::Get().recorder.vulkan.utilities.memoryUpdateState ==
                 TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED) ||
                (isSubcaptureBeforeRestorationPhase())) {
              descriptorSetState->descriptorBuffers
                  [descriptorUpdateTemplateCreateInfoData->pDescriptorUpdateEntries[i].dstBinding] =
                  bufferViewState->bufferStateStore->bufferHandle;
            }
            if (descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER) {
              descriptorSetState->descriptorWriteBuffers[descriptorUpdateTemplateCreateInfoData
                                                             ->pDescriptorUpdateEntries[i]
                                                             .dstBinding] = {
                  VULKAN_STORAGE_TEXEL_BUFFER, bufferViewState->bufferStateStore->bufferHandle};

              if ((Config::Get().recorder.vulkan.utilities.memorySegmentSize ||
                   Config::Get().recorder.vulkan.utilities.shadowMemory) &&
                  (bufferViewState->bufferStateStore->binding)) {
                VkDeviceSize dstOffsetFinal =
                    bufferViewState->bufferStateStore->binding->memoryOffset +
                    bufferViewState->bufferViewCreateInfoData.Value()->offset;
                VkDeviceSize dstFinalSize;
                if (bufferViewState->bufferViewCreateInfoData.Value()->range ==
                    0xFFFFFFFFFFFFFFFF) {
                  dstFinalSize =
                      bufferViewState->bufferStateStore->bufferCreateInfoData.Value()->size -
                      bufferViewState->bufferViewCreateInfoData.Value()->offset;
                } else {
                  dstFinalSize = bufferViewState->bufferViewCreateInfoData.Value()->range;
                }

                boost::icl::interval<uint64_t>::type interv(dstOffsetFinal,
                                                            dstFinalSize + dstOffsetFinal);
                descriptorSetState
                    ->descriptorMapMemory[descriptorUpdateTemplateCreateInfoData
                                              ->pDescriptorUpdateEntries[i]
                                              .dstBinding]
                    .clear();
                descriptorSetState
                    ->descriptorMapMemory[descriptorUpdateTemplateCreateInfoData
                                              ->pDescriptorUpdateEntries[i]
                                              .dstBinding]
                                         [bufferViewState->bufferStateStore->binding
                                              ->deviceMemoryStateStore->deviceMemoryHandle]
                    .insert(interv);
              }
            }
          }
        } break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: {
          VkDescriptorBufferInfo* descBufferInfo =
              (VkDescriptorBufferInfo*)((char*)pData +
                                        descriptorUpdateTemplateCreateInfoData
                                            ->pDescriptorUpdateEntries[i]
                                            .offset +
                                        descriptorUpdateTemplateCreateInfoData
                                                ->pDescriptorUpdateEntries[i]
                                                .stride *
                                            j);

          if (descBufferInfo->buffer != VK_NULL_HANDLE) {
            auto& bufferState = SD()._bufferstates[descBufferInfo->buffer];

            if (isSubcaptureBeforeRestorationPhase() && (currentDescriptorData != nullptr)) {
              currentDescriptorData->pBufferInfo.reset(
                  new CVkDescriptorBufferInfoData(descBufferInfo));
              currentDescriptorData->bufferStateStore = bufferState;
            }

            if ((Config::Get().recorder.vulkan.utilities.memoryUpdateState ==
                 TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED) ||
                (isSubcaptureBeforeRestorationPhase())) {
              descriptorSetState->descriptorBuffers
                  [descriptorUpdateTemplateCreateInfoData->pDescriptorUpdateEntries[i].dstBinding] =
                  descBufferInfo->buffer;
            }

            if ((descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) ||
                (descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)) {
              VulkanResourceType resType = descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
                                               ? VULKAN_STORAGE_BUFFER
                                               : VULKAN_STORAGE_BUFFER_DYNAMIC;
              descriptorSetState->descriptorWriteBuffers[descriptorUpdateTemplateCreateInfoData
                                                             ->pDescriptorUpdateEntries[i]
                                                             .dstBinding] = {
                  resType, descBufferInfo->buffer};

              if ((Config::Get().recorder.vulkan.utilities.memorySegmentSize ||
                   Config::Get().recorder.vulkan.utilities.shadowMemory) &&
                  bufferState->binding) {
                VkDeviceSize dstOffsetFinal =
                    bufferState->binding->memoryOffset + descBufferInfo->offset;
                VkDeviceSize dstFinalSize;
                if (descBufferInfo->range == 0xFFFFFFFFFFFFFFFF) {
                  dstFinalSize =
                      bufferState->bufferCreateInfoData.Value()->size - descBufferInfo->offset;
                } else {
                  dstFinalSize = descBufferInfo->range;
                }

                boost::icl::interval<uint64_t>::type interv(dstOffsetFinal,
                                                            dstFinalSize + dstOffsetFinal);
                descriptorSetState
                    ->descriptorMapMemory[descriptorUpdateTemplateCreateInfoData
                                              ->pDescriptorUpdateEntries[i]
                                              .dstBinding]
                    .clear();
                descriptorSetState
                    ->descriptorMapMemory
                        [descriptorUpdateTemplateCreateInfoData->pDescriptorUpdateEntries[i]
                             .dstBinding]
                        [bufferState->binding->deviceMemoryStateStore->deviceMemoryHandle]
                    .insert(interv);
              }
            }
          }
        } break;
        default:
          Log(TRACE) << "Not handled VkDescriptorType enumeration: " +
                            std::to_string(descriptorType);
          break;
        }
      }
    }
  }
}

inline void vkUpdateDescriptorSetWithTemplateKHR_SD(
    VkDevice device,
    VkDescriptorSet descriptorSet,
    VkDescriptorUpdateTemplate descriptorUpdateTemplate,
    const void* pData) {
  vkUpdateDescriptorSetWithTemplate_SD(device, descriptorSet, descriptorUpdateTemplate, pData);
}

// Pipeline cache

inline void vkCreatePipelineCache_SD(VkResult return_value,
                                     VkDevice device,
                                     const VkPipelineCacheCreateInfo* pCreateInfo,
                                     const VkAllocationCallbacks* pAllocator,
                                     VkPipelineCache* pPipelineCache) {
  if (((Config::Get().IsPlayer()) && (Config::Get().player.cleanResourcesOnExit)) ||
      (isSubcaptureBeforeRestorationPhase())) {
    if ((return_value == VK_SUCCESS) && (*pPipelineCache != VK_NULL_HANDLE)) {
      SD()._pipelinecachestates.emplace(
          *pPipelineCache, std::make_shared<CPipelineCacheState>(pPipelineCache, pCreateInfo,
                                                                 SD()._devicestates[device]));
    }
  }
}

inline void vkDestroyPipelineCache_SD(VkDevice device,
                                      VkPipelineCache pipelineCache,
                                      const VkAllocationCallbacks* pAllocator) {
  if (((Config::Get().IsPlayer()) && (Config::Get().player.cleanResourcesOnExit)) ||
      (isSubcaptureBeforeRestorationPhase())) {
    SD()._pipelinecachestates.erase(pipelineCache);
  }
}

// Shader module

inline void vkCreateShaderModule_SD(VkResult return_value,
                                    VkDevice device,
                                    const VkShaderModuleCreateInfo* pCreateInfo,
                                    const VkAllocationCallbacks* pAllocator,
                                    VkShaderModule* pShaderModule) {
  if ((return_value == VK_SUCCESS) && (*pShaderModule != VK_NULL_HANDLE)) {
    uint32_t hash;
    if (!(pCreateInfo->codeSize % sizeof(uint32_t))) {
      hash = (uint32_t)ISTDHash(reinterpret_cast<const char*>(pCreateInfo->pCode),
                                static_cast<uint32_t>(pCreateInfo->codeSize));
    } else {
      hash = (uint32_t)ISTDHashPadding(reinterpret_cast<const char*>(pCreateInfo->pCode),
                                       static_cast<uint32_t>(pCreateInfo->codeSize));
    }

    SD()._shadermodulestates.emplace(
        *pShaderModule, std::make_shared<CShaderModuleState>(pShaderModule, pCreateInfo, hash,
                                                             SD()._devicestates[device]));
  }
}

inline void vkDestroyShaderModule_SD(VkDevice device,
                                     VkShaderModule shaderModule,
                                     const VkAllocationCallbacks* pAllocator) {
  SD()._shadermodulestates.erase(shaderModule);
}

// Render pass

inline void vkCreateRenderPass_SD(VkResult return_value,
                                  VkDevice device,
                                  const VkRenderPassCreateInfo* pCreateInfo,
                                  const VkAllocationCallbacks* pAllocator,
                                  VkRenderPass* pRenderPass) {
  if ((return_value == VK_SUCCESS) && (*pRenderPass != VK_NULL_HANDLE)) {
    SD()._renderpassstates.emplace(*pRenderPass,
                                   std::make_shared<CRenderPassState>(pRenderPass, pCreateInfo,
                                                                      CreationFunction::CORE_1_0,
                                                                      SD()._devicestates[device]));
  }
}

inline void vkCreateRenderPass2_SD(VkResult return_value,
                                   VkDevice device,
                                   const VkRenderPassCreateInfo2* pCreateInfo,
                                   const VkAllocationCallbacks* pAllocator,
                                   VkRenderPass* pRenderPass) {
  if ((return_value == VK_SUCCESS) && (*pRenderPass != VK_NULL_HANDLE)) {
    SD()._renderpassstates.emplace(*pRenderPass,
                                   std::make_shared<CRenderPassState>(pRenderPass, pCreateInfo,
                                                                      CreationFunction::CORE_1_2,
                                                                      SD()._devicestates[device]));
  }
}

inline void vkCreateRenderPass2KHR_SD(VkResult return_value,
                                      VkDevice device,
                                      const VkRenderPassCreateInfo2* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator,
                                      VkRenderPass* pRenderPass) {
  if ((return_value == VK_SUCCESS) && (*pRenderPass != VK_NULL_HANDLE)) {
    SD()._renderpassstates.emplace(
        *pRenderPass, std::make_shared<CRenderPassState>(pRenderPass, pCreateInfo,
                                                         CreationFunction::KHR_EXTENSION,
                                                         SD()._devicestates[device]));
  }
}

inline void vkDestroyRenderPass_SD(VkDevice device,
                                   VkRenderPass renderPass,
                                   const VkAllocationCallbacks* pAllocator) {
  SD()._renderpassstates.erase(renderPass);
}

// Pipeline

inline void vkCreateGraphicsPipelines_SD(VkResult return_value,
                                         VkDevice device,
                                         VkPipelineCache pipelineCache,
                                         uint32_t count,
                                         const VkGraphicsPipelineCreateInfo* pCreateInfos,
                                         const VkAllocationCallbacks* pAllocator,
                                         VkPipeline* pPipelines) {
  if (VK_SUCCESS == return_value) {
    for (uint32_t i = 0; i < count; i++) {
      if (pPipelines[i] == VK_NULL_HANDLE) {
        continue;
      }

      VkGraphicsPipelineCreateInfo const* createInfo = &pCreateInfos[i];

      std::shared_ptr<CRenderPassState> renderPassObj = NULL;
      if (createInfo->renderPass) {
        renderPassObj = SD()._renderpassstates[createInfo->renderPass];
      }
      auto pipelineState = std::make_shared<CPipelineState>(
          &pPipelines[i], createInfo, SD()._devicestates[device],
          SD()._pipelinelayoutstates[createInfo->layout], renderPassObj);

      for (unsigned int j = 0; j < createInfo->stageCount; j++) {
        auto& stageInfo = createInfo->pStages[j];

        if (stageInfo.module != VK_NULL_HANDLE) {
          auto& shaderModuleState = SD()._shadermodulestates[stageInfo.module];
          pipelineState->shaderModuleStateStoreList.push_back(shaderModuleState);
          pipelineState->stageShaderHashMapping[stageInfo.stage] = shaderModuleState->shaderHash;
        } else {
          pipelineState->shaderModuleStateStoreList.push_back(nullptr);
        }
      }

      SD()._pipelinestates.emplace(pPipelines[i], pipelineState);
    }
  }
}

inline void vkCreateComputePipelines_SD(VkResult return_value,
                                        VkDevice device,
                                        VkPipelineCache pipelineCache,
                                        uint32_t createInfoCount,
                                        const VkComputePipelineCreateInfo* pCreateInfos,
                                        const VkAllocationCallbacks* pAllocator,
                                        VkPipeline* pPipelines) {
  if (VK_SUCCESS == return_value) {
    for (unsigned int i = 0; i < createInfoCount; i++) {
      if (pPipelines[i] == VK_NULL_HANDLE) {
        continue;
      }

      VkComputePipelineCreateInfo const* createInfo = &pCreateInfos[i];

      auto pipelineState =
          std::make_shared<CPipelineState>(&pPipelines[i], createInfo, SD()._devicestates[device],
                                           SD()._pipelinelayoutstates[createInfo->layout]);

      {
        auto& stageInfo = createInfo->stage;
        if (stageInfo.module != VK_NULL_HANDLE) {
          auto& shaderModuleState = SD()._shadermodulestates[stageInfo.module];
          pipelineState->shaderModuleStateStoreList.push_back(shaderModuleState);
          pipelineState->stageShaderHashMapping[stageInfo.stage] = shaderModuleState->shaderHash;
        } else {
          pipelineState->shaderModuleStateStoreList.push_back(nullptr);
        }
      }

      SD()._pipelinestates.emplace(pPipelines[i], pipelineState);
    }
  }
}

inline void vkDestroyPipeline_SD(VkDevice device,
                                 VkPipeline pipeline,
                                 const VkAllocationCallbacks* pAllocator) {
  SD()._pipelinestates.erase(pipeline);
}

// Framebuffer

inline void vkCreateFramebuffer_SD(VkResult return_value,
                                   VkDevice device,
                                   const VkFramebufferCreateInfo* pCreateInfo,
                                   const VkAllocationCallbacks* pAllocator,
                                   VkFramebuffer* pFramebuffer) {
  if ((return_value == VK_SUCCESS) && (*pFramebuffer != VK_NULL_HANDLE)) {
    auto framebufferState =
        std::make_shared<CFramebufferState>(pFramebuffer, pCreateInfo, SD()._devicestates[device],
                                            SD()._renderpassstates[pCreateInfo->renderPass]);

    if (!(pCreateInfo->flags & VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT)) {
      for (uint32_t i = 0; i < pCreateInfo->attachmentCount; ++i) {
        framebufferState->imageViewStateStoreList.push_back(
            SD()._imageviewstates[pCreateInfo->pAttachments[i]]);
      }
    }

    SD()._framebufferstates.emplace(*pFramebuffer, framebufferState);
  }
}

inline void vkDestroyFramebuffer_SD(VkDevice device,
                                    VkFramebuffer framebuffer,
                                    const VkAllocationCallbacks* pAllocator) {
  SD()._framebufferstates.erase(framebuffer); //Stardust BeginRenderPass
}

// Fence

inline void vkCreateFence_SD(VkResult return_value,
                             VkDevice device,
                             const VkFenceCreateInfo* pCreateInfo,
                             const VkAllocationCallbacks* pAllocator,
                             VkFence* pFence) {
  if ((return_value == VK_SUCCESS) && (*pFence != VK_NULL_HANDLE)) {
    SD()._fencestates.emplace(
        *pFence, std::make_shared<CFenceState>(pFence, pCreateInfo, SD()._devicestates[device]));
  }
}

inline void vkResetFences_SD(VkResult return_value,
                             VkDevice device,
                             uint32_t fenceCount,
                             const VkFence* pFences) {
  for (uint32_t i = 0; i < fenceCount; i++) {
    SD()._fencestates[pFences[i]]->fenceUsed = false;
    SD()._fencestates[pFences[i]]->delayChecksCount = 0;
  }
}

inline void vkDeviceWaitIdle_SD(VkResult return_value, VkDevice device) {
  if (Config::Get().recorder.vulkan.utilities.delayFenceChecksCount > 0) {
    for (auto& fenceState : SD()._fencestates) {
      if ((fenceState.second->deviceStateStore->deviceHandle == device) &&
          fenceState.second->fenceUsed) {
        fenceState.second->delayChecksCount =
            Config::Get().recorder.vulkan.utilities.delayFenceChecksCount;
      }
    }
  }
}

inline void vkQueueWaitIdle_SD(VkResult return_value, VkQueue queue) {
  if (Config::Get().recorder.vulkan.utilities.delayFenceChecksCount > 0) {
    for (auto& fenceState : SD()._fencestates) {
      if ((fenceState.second->deviceStateStore->deviceHandle ==
           SD()._queuestates[queue]->deviceStateStore->deviceHandle) &&
          fenceState.second->fenceUsed) {
        fenceState.second->delayChecksCount =
            Config::Get().recorder.vulkan.utilities.delayFenceChecksCount;
      }
    }
  }
}

inline void vkDestroyFence_SD(VkDevice device,
                              VkFence fence,
                              const VkAllocationCallbacks* pAllocator) {
  SD()._fencestates.erase(fence);
}

// Event

inline void vkCreateEvent_SD(VkResult return_value,
                             VkDevice device,
                             const VkEventCreateInfo* pCreateInfo,
                             const VkAllocationCallbacks* pAllocator,
                             VkEvent* pEvent) {
  if ((return_value == VK_SUCCESS) && (*pEvent != VK_NULL_HANDLE)) {
    SD()._eventstates.emplace(
        *pEvent, std::make_shared<CEventState>(pEvent, pCreateInfo, SD()._devicestates[device]));
  }
}

inline void vkDestroyEvent_SD(VkDevice device,
                              VkEvent event,
                              const VkAllocationCallbacks* pAllocator) {
  SD()._eventstates.erase(event);
}

inline void vkSetEvent_SD(VkResult return_value, VkDevice device, VkEvent event) {
  SD()._eventstates[event]->eventUsed = true;
}

inline void vkResetEvent_SD(VkResult return_value, VkDevice device, VkEvent event) {
  SD()._eventstates[event]->eventUsed = false;
}

// Semaphore

inline void vkCreateSemaphore_SD(VkResult return_value,
                                 VkDevice device,
                                 const VkSemaphoreCreateInfo* pCreateInfo,
                                 const VkAllocationCallbacks* pAllocator,
                                 VkSemaphore* pSemaphore) {
  if ((return_value == VK_SUCCESS) && (*pSemaphore != VK_NULL_HANDLE)) {
    auto semaphoreState =
        std::make_shared<CSemaphoreState>(pSemaphore, pCreateInfo, SD()._devicestates[device]);
    SD()._semaphorestates.emplace(*pSemaphore, semaphoreState);
    auto semaphoreTypeCreateInfo = (VkSemaphoreTypeCreateInfo*)getPNextStructure(
        pCreateInfo->pNext, VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO);
    if (semaphoreTypeCreateInfo &&
        semaphoreTypeCreateInfo->semaphoreType == VK_SEMAPHORE_TYPE_TIMELINE) {
      semaphoreState->isTimeline = true;
      semaphoreState->timelineSemaphoreValue = semaphoreTypeCreateInfo->initialValue;
    }
  }
}

inline void vkDestroySemaphore_SD(VkDevice device,
                                  VkSemaphore semaphore,
                                  const VkAllocationCallbacks* pAllocator) {
  SD()._semaphorestates.erase(semaphore);
}

// Query pool

inline void vkCreateQueryPool_SD(VkResult return_value,
                                 VkDevice device,
                                 const VkQueryPoolCreateInfo* pCreateInfo,
                                 const VkAllocationCallbacks* pAllocator,
                                 VkQueryPool* pQueryPool) {
  if (((Config::Get().IsPlayer()) && (Config::Get().player.cleanResourcesOnExit)) ||
      (isSubcaptureBeforeRestorationPhase())) {
    if ((return_value == VK_SUCCESS) && (*pQueryPool != VK_NULL_HANDLE)) {
      SD()._querypoolstates.emplace(
          *pQueryPool,
          std::make_shared<CQueryPoolState>(pQueryPool, pCreateInfo, SD()._devicestates[device]));
    }
  }
}

inline void vkDestroyQueryPool_SD(VkDevice device,
                                  VkQueryPool queryPool,
                                  const VkAllocationCallbacks* pAllocator) {
  if (((Config::Get().IsPlayer()) && (Config::Get().player.cleanResourcesOnExit)) ||
      (isSubcaptureBeforeRestorationPhase())) {
    SD()._querypoolstates.erase(queryPool);
  }
}

inline void vkResetQueryPool_SD(VkDevice device,
                                VkQueryPool queryPool,
                                uint32_t firstQuery,
                                uint32_t queryCount) {
  if (((Config::Get().IsPlayer()) && (Config::Get().player.cleanResourcesOnExit)) ||
      (isSubcaptureBeforeRestorationPhase())) {
    auto& state = SD()._querypoolstates[queryPool];
    for (auto i = firstQuery; i < firstQuery + queryCount; i++) {
      state->resetQueries[i] = true;
      state->usedQueries[i] = false;
    }
  }
}

inline void vkResetQueryPoolEXT_SD(VkDevice device,
                                   VkQueryPool queryPool,
                                   uint32_t firstQuery,
                                   uint32_t queryCount) {
  vkResetQueryPool_SD(device, queryPool, firstQuery, queryCount);
}

// Command buffer

inline void vkAllocateCommandBuffers_SD(VkResult return_value,
                                        VkDevice device,
                                        const VkCommandBufferAllocateInfo* pAllocateInfo,
                                        VkCommandBuffer* pCommandBuffers) {
  if (return_value == VK_SUCCESS) {
    for (uint32_t i = 0; i < pAllocateInfo->commandBufferCount; i++) {
      if (pCommandBuffers[i] == VK_NULL_HANDLE) {
        continue;
      }

      auto commandBufferState = std::make_shared<CCommandBufferState>(
          &pCommandBuffers[i], pAllocateInfo, SD()._commandpoolstates[pAllocateInfo->commandPool]);

      SD()._commandbufferstates.emplace(pCommandBuffers[i], commandBufferState);
      SD()._commandpoolstates[pAllocateInfo->commandPool]->commandBufferStateStoreList.insert(
          commandBufferState);
    }
  }
}

inline void vkFreeCommandBuffers_SD(VkDevice device,
                                    VkCommandPool commandPool,
                                    uint32_t commandBufferCount,
                                    const VkCommandBuffer* pCommandBuffers) {
  for (uint32_t i = 0; i < commandBufferCount; i++) {
    SD()._commandpoolstates[commandPool]->commandBufferStateStoreList.erase(
        SD()._commandbufferstates[pCommandBuffers[i]]);
    SD()._commandbufferstates.erase(pCommandBuffers[i]);

    if (Config::Get().IsRecorder()) {
      SD().bindingBuffers.erase(pCommandBuffers[i]);
      SD().bindingImages.erase(pCommandBuffers[i]);

      if (Config::Get().recorder.vulkan.utilities.memorySegmentSize ||
          Config::Get().recorder.vulkan.utilities.shadowMemory) {
        SD().updatedMemoryInCmdBuffer.erase(pCommandBuffers[i]);
      }
    }
  }
}

inline void vkResetCommandBuffer_SD(VkResult /* return_value */,
                                    VkCommandBuffer commandBuffer,
                                    VkCommandBufferResetFlags /* flags */) {
  auto& commandBufferState = SD()._commandbufferstates[commandBuffer];

  commandBufferState->beginCommandBuffer.reset();
  commandBufferState->beginRenderPassesList.clear();
  commandBufferState->ended = false;
  commandBufferState->submitted = false;
  commandBufferState->restored = false;
  commandBufferState->currentPipeline = VK_NULL_HANDLE;
  commandBufferState->tokensBuffer.Clear();
  commandBufferState->eventStatesAfterSubmit.clear();
  commandBufferState->resetQueriesAfterSubmit.clear(); // per query pool, per query index
  commandBufferState->usedQueriesAfterSubmit.clear();  // per query pool, per query index
  commandBufferState->imageLayoutAfterSubmit.clear();  // per layer, per mipmap
  commandBufferState->descriptorSetStateStoreList.clear();
  commandBufferState->pipelineStateStoreList.clear();
  commandBufferState->secondaryCommandBuffersStateStoreList.clear();
  commandBufferState->resourceWriteBuffers.clear();
  commandBufferState->resourceWriteImages.clear();
  commandBufferState->touchedResources.clear();
  commandBufferState->secondaryCommandBuffers.clear();

  if (Config::Get().IsRecorder()) {
    SD().bindingBuffers[commandBuffer].clear();
    SD().bindingImages[commandBuffer].clear();

    if (Config::Get().recorder.vulkan.utilities.memorySegmentSize ||
        Config::Get().recorder.vulkan.utilities.shadowMemory) {
      SD().updatedMemoryInCmdBuffer.erase(commandBuffer);
    }
  }
}

inline void vkBeginCommandBuffer_SD(VkResult /* return_value */,
                                    VkCommandBuffer cmdBuffer,
                                    const VkCommandBufferBeginInfo* pBeginInfo) {
  vkResetCommandBuffer_SD(VK_SUCCESS, cmdBuffer, 0);

  if (pBeginInfo == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  auto& commandBufferState = SD()._commandbufferstates[cmdBuffer];

  commandBufferState->beginCommandBuffer.reset(
      new CCommandBufferState::CBeginCommandBuffer(pBeginInfo));

  if (Config::Get().IsRecorder()) {
    if (nullptr != pBeginInfo->pInheritanceInfo) {
      if (VK_NULL_HANDLE != pBeginInfo->pInheritanceInfo->framebuffer) {
        for (auto& imageViewState :
             SD()._framebufferstates[pBeginInfo->pInheritanceInfo->framebuffer]
                 ->imageViewStateStoreList) {
          auto& imageState = imageViewState->imageStateStore;

          if ((TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED ==
               Config::Get().recorder.vulkan.utilities.memoryUpdateState) ||
              isSubcaptureBeforeRestorationPhase()) {
            SD().bindingImages[cmdBuffer].insert(imageState->imageHandle);
          }
          if ((Config::Get().recorder.vulkan.utilities.memorySegmentSize ||
               Config::Get().recorder.vulkan.utilities.shadowMemory) &&
              (imageState->binding)) {
            VkDeviceSize dstOffsetFinal = imageState->binding->memoryOffset;
            VkDeviceMemory dstDeviceMemory =
                imageState->binding->deviceMemoryStateStore->deviceMemoryHandle;
            VkMemoryRequirements memRequirements = {};
            drvVk.vkGetImageMemoryRequirements(imageState->deviceStateStore->deviceHandle,
                                               imageState->imageHandle, &memRequirements);
            //TODO : call vkGetImageSubresourceLayout when tiling is Linear
            SD().updatedMemoryInCmdBuffer[cmdBuffer].AddToMap(dstDeviceMemory, dstOffsetFinal,
                                                              memRequirements.size);
          }
        }
      }
    }
  }
}

inline void vkEndCommandBuffer_SD(VkResult return_value, VkCommandBuffer commandBuffer) {
  SD()._commandbufferstates[commandBuffer]->ended = true;
}

// Queue submit

namespace {
inline void vkQueueSubmit_setImageLayout(std::shared_ptr<CCommandBufferState>& commandBufferState,
                                         uint32_t queueFamilyIndex) {
  if ((isSubcaptureBeforeRestorationPhase() &&
       Config::Get().recorder.vulkan.utilities.crossPlatformStateRestoration.images) ||
      captureRenderPasses() || !Config::Get().player.captureVulkanSubmitsResources.empty()) {

    for (auto& imageLayoutAfterSubmit : commandBufferState->imageLayoutAfterSubmit) {
      auto& imageState = SD()._imagestates[imageLayoutAfterSubmit.first];

      for (uint32_t l = 0; l < imageLayoutAfterSubmit.second.size(); ++l) {
        for (uint32_t m = 0; m < imageLayoutAfterSubmit.second[l].size(); ++m) {
          if (imageLayoutAfterSubmit.second[l][m].Layout != (VkImageLayout)-1) {
            imageState->currentLayout[l][m].Layout = imageLayoutAfterSubmit.second[l][m].Layout;
            imageState->currentLayout[l][m].Access = imageLayoutAfterSubmit.second[l][m].Access;
          }
          if (imageLayoutAfterSubmit.second[l][m].QueueFamilyIndex != VK_QUEUE_FAMILY_IGNORED) {
            imageState->currentLayout[l][m].QueueFamilyIndex =
                imageLayoutAfterSubmit.second[l][m].QueueFamilyIndex;
          } else {
            imageState->currentLayout[l][m].QueueFamilyIndex = queueFamilyIndex;
          }
        }
      }
    }
  }
}

inline void vkQueueSubmit_setQueryPoolState(
    std::shared_ptr<CCommandBufferState>& commandBufferState) {
  if (isSubcaptureBeforeRestorationPhase()) {
    for (auto& resetQueryAfterSubmit : commandBufferState->resetQueriesAfterSubmit) {
      auto& queryPoolState = SD()._querypoolstates[resetQueryAfterSubmit.first];

      for (auto& queryIndex : resetQueryAfterSubmit.second) {
        queryPoolState->resetQueries[queryIndex] = true;
        queryPoolState->usedQueries[queryIndex] = false;
      }
    }

    for (auto& usedQueryAfterSubmit : commandBufferState->usedQueriesAfterSubmit) {
      auto& queryPoolState = SD()._querypoolstates[usedQueryAfterSubmit.first];

      for (auto& queryIndex : usedQueryAfterSubmit.second) {
        queryPoolState->usedQueries[queryIndex] = true;
      }
    }
  }
}

inline void vkQueueSubmit_setTimestamps(std::shared_ptr<CCommandBufferState>& commandBufferState) {
  if (isSubcaptureBeforeRestorationPhase()) {
    for (uint32_t i = 0; i < commandBufferState->touchedResources.size(); ++i) {
      auto& touchedResource = commandBufferState->touchedResources[i];
      if (touchedResource.second) {
        SD()._imagestates[(VkImage)touchedResource.first]->timestamp =
            SD().internalResources.timestamp + i;
      } else {
        SD()._bufferstates[(VkBuffer)touchedResource.first]->timestamp =
            SD().internalResources.timestamp + i;
      }
    }

    SD().internalResources.timestamp += commandBufferState->touchedResources.size();
  }
}

inline void vkQueueSubmit_updateNonDeterministicImages(
    std::shared_ptr<CCommandBufferState>& commandBufferState) {
  if (Config::Get().player.skipNonDeterministicImages) {
    for (auto obj : commandBufferState->clearedImages) {
      SD().nonDeterministicImages.erase(obj);
    }
  }
}
} // namespace

inline void vkQueueSubmit_SD(VkResult return_value,
                             VkQueue queue,
                             uint32_t submitCount,
                             const VkSubmitInfo* pSubmits,
                             VkFence fence) {
  if (Config::Get().IsRecorder() || captureRenderPasses() ||
      !Config::Get().player.captureVulkanSubmitsResources.empty()) {
    if (pSubmits != NULL) {
      for (uint32_t s = 0; s < submitCount; ++s) {
        for (uint32_t c = 0; c < pSubmits[s].commandBufferCount; ++c) {
          auto& commandBufferState = SD()._commandbufferstates[pSubmits[s].pCommandBuffers[c]];

          commandBufferState->submitted = true;

          for (auto& secondaryCommandBufferState :
               commandBufferState->secondaryCommandBuffersStateStoreList) {
            secondaryCommandBufferState.second->submitted = true;
          }
          for (auto& eventState : commandBufferState->eventStatesAfterSubmit) {
            SD()._eventstates[eventState.first]->eventUsed = eventState.second;
          }

          // Query pool state
          vkQueueSubmit_setQueryPoolState(commandBufferState);

          // Image layout state
          vkQueueSubmit_setImageLayout(commandBufferState,
                                       SD()._queuestates[queue]->queueFamilyIndex);

          // Image usage
          vkQueueSubmit_setTimestamps(commandBufferState);

          // Non deterministic images list update
          vkQueueSubmit_updateNonDeterministicImages(commandBufferState);
        }

        // Semaphore state
        for (uint32_t i = 0; i < pSubmits[s].waitSemaphoreCount; i++) {
          SD()._semaphorestates[pSubmits[s].pWaitSemaphores[i]]->semaphoreUsed = false;
        }

        for (uint32_t i = 0; i < pSubmits[s].signalSemaphoreCount; i++) {
          SD()._semaphorestates[pSubmits[s].pSignalSemaphores[i]]->semaphoreUsed = true;
        }

        auto timelineSemaphoreSubmitInfo = (VkTimelineSemaphoreSubmitInfo*)getPNextStructure(
            pSubmits[s].pNext, VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO);
        if (timelineSemaphoreSubmitInfo) {
          for (uint32_t t = 0; t < timelineSemaphoreSubmitInfo->signalSemaphoreValueCount; ++t) {
            SD()._semaphorestates[pSubmits[s].pSignalSemaphores[t]]->timelineSemaphoreValue =
                timelineSemaphoreSubmitInfo->pSignalSemaphoreValues[t];
          }
        }
      }
    }
  }

  if (pSubmits != NULL) {
    for (uint32_t s = 0; s < submitCount; ++s) {
      for (uint32_t c = 0; c < pSubmits[s].commandBufferCount; ++c) {
        auto& cmdBufferState = SD()._commandbufferstates[pSubmits[s].pCommandBuffers[c]];

        for (auto& eventState : cmdBufferState->eventStatesAfterSubmit) {
          SD()._eventstates[eventState.first]->eventUsed = eventState.second;
        }
      }
    }
  }

  if (VK_NULL_HANDLE != fence) {
    SD()._fencestates[fence]->fenceUsed = true;
  }
}

inline void vkQueueSubmit2_SD(VkResult return_value,
                              VkQueue queue,
                              uint32_t submitCount,
                              const VkSubmitInfo2* pSubmits,
                              VkFence fence) {
  if (Config::Get().IsRecorder() || captureRenderPasses() ||
      !Config::Get().player.captureVulkanSubmitsResources.empty()) {
    if (pSubmits != NULL) {
      for (uint32_t s = 0; s < submitCount; ++s) {
        for (uint32_t c = 0; c < pSubmits[s].commandBufferInfoCount; ++c) {
          auto& commandBufferState =
              SD()._commandbufferstates[pSubmits[s].pCommandBufferInfos[c].commandBuffer];

          commandBufferState->submitted = true;

          for (auto& secondaryCommandBufferState :
               commandBufferState->secondaryCommandBuffersStateStoreList) {
            secondaryCommandBufferState.second->submitted = true;
          }
          for (auto& eventState : commandBufferState->eventStatesAfterSubmit) {
            SD()._eventstates[eventState.first]->eventUsed = eventState.second;
          }

          // Query pool state
          vkQueueSubmit_setQueryPoolState(commandBufferState);

          // Image layout state
          vkQueueSubmit_setImageLayout(commandBufferState,
                                       SD()._queuestates[queue]->queueFamilyIndex);

          // Image usage
          vkQueueSubmit_setTimestamps(commandBufferState);

          // Non deterministic images list update
          vkQueueSubmit_updateNonDeterministicImages(commandBufferState);
        }

        // Semaphore state
        for (uint32_t i = 0; i < pSubmits[s].waitSemaphoreInfoCount; i++) {
          SD()._semaphorestates[pSubmits[s].pWaitSemaphoreInfos[i].semaphore]->semaphoreUsed =
              false;
        }

        for (uint32_t i = 0; i < pSubmits[s].signalSemaphoreInfoCount; i++) {
          auto& signalInfo = pSubmits[s].pSignalSemaphoreInfos[i];
          auto& semaphoreState = SD()._semaphorestates[signalInfo.semaphore];

          semaphoreState->semaphoreUsed = true;
          if (semaphoreState->isTimeline) {
            semaphoreState->timelineSemaphoreValue = signalInfo.value;
          }
        }
      }
    }
  }

  if (pSubmits != NULL) {
    for (uint32_t s = 0; s < submitCount; ++s) {
      for (uint32_t c = 0; c < pSubmits[s].commandBufferInfoCount; ++c) {
        auto& cmdBufferState =
            SD()._commandbufferstates[pSubmits[s].pCommandBufferInfos[c].commandBuffer];

        for (auto& eventState : cmdBufferState->eventStatesAfterSubmit) {
          SD()._eventstates[eventState.first]->eventUsed = eventState.second;
        }
      }
    }
  }

  if (VK_NULL_HANDLE != fence) {
    SD()._fencestates[fence]->fenceUsed = true;
  }
}

//RenderPass helper functions
namespace {
inline void vkEndRenderPass_setImageLayout(
    std::shared_ptr<CCommandBufferState>& commandBufferState) {
  if (!Config::Get().player.captureVulkanRenderPasses.empty()) {
    for (auto& imageLayoutAfterSubmit : commandBufferState->imageLayoutAfterSubmit) {
      auto& imageState = SD()._imagestates[imageLayoutAfterSubmit.first];

      for (uint32_t l = 0; l < imageLayoutAfterSubmit.second.size(); ++l) {
        for (uint32_t m = 0; m < imageLayoutAfterSubmit.second[l].size(); ++m) {
          if (imageLayoutAfterSubmit.second[l][m].Layout != (VkImageLayout)-1) {
            imageState->currentLayout[l][m].Layout = imageLayoutAfterSubmit.second[l][m].Layout;
            imageState->currentLayout[l][m].Access = imageLayoutAfterSubmit.second[l][m].Access;
          }
        }
      }
    }
  }
}

inline void vkEndRenderPass_updateNonDeterministicImages(
    std::shared_ptr<CCommandBufferState>& commandBufferState) {
  if (Config::Get().player.skipNonDeterministicImages &&
      commandBufferState->beginRenderPassesList.size()) {
    auto& framebufferState =
        commandBufferState->beginRenderPassesList.back()->framebufferStateStore;
    uint32_t imageViewSize;
    if (framebufferState && !(framebufferState->framebufferCreateInfoData.Value()->flags &
                              VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT)) {
      imageViewSize = (uint32_t)framebufferState->imageViewStateStoreList.size();
    } else {
      imageViewSize = (uint32_t)commandBufferState->beginRenderPassesList.back()
                          ->imageViewStateStoreListKHR.size();
    }
    for (uint32_t imageview = 0; imageview < imageViewSize; ++imageview) {
      std::string fileName;
      VkImage imageHandle;
      VkAttachmentLoadOp imageLoadOption =
          commandBufferState->beginRenderPassesList.back()->imageLoadOp[imageview];
      if (framebufferState && !(framebufferState->framebufferCreateInfoData.Value()->flags &
                                VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT)) {
        imageHandle =
            framebufferState->imageViewStateStoreList[imageview]->imageStateStore->imageHandle;
      } else {
        imageHandle = commandBufferState->beginRenderPassesList.back()
                          ->imageViewStateStoreListKHR[imageview]
                          ->imageStateStore->imageHandle;
      }
      if (imageLoadOption == VK_ATTACHMENT_LOAD_OP_CLEAR) {
        commandBufferState->clearedImages.insert(imageHandle);
      }
    }
    if (!Config::Get().player.captureVulkanRenderPasses.empty()) {
      for (auto obj : commandBufferState->clearedImages) {
        SD().nonDeterministicImages.erase(obj);
      }
    }
  }
}
} // namespace

// Command buffer recording commands

inline void vkCmdBeginRenderPass_SD(VkCommandBuffer cmdBuffer,
                                    const VkRenderPassBeginInfo* pRenderPassBegin,
                                    VkSubpassContents contents) {
  if (pRenderPassBegin == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  auto beginRenderPassState = std::make_shared<CCommandBufferState::CBeginRenderPass>(
      pRenderPassBegin, &contents, SD()._renderpassstates[pRenderPassBegin->renderPass],
      SD()._framebufferstates[pRenderPassBegin->framebuffer]);
  SD()._commandbufferstates[cmdBuffer]->beginRenderPassesList.push_back(beginRenderPassState);
  if (Config::Get().IsRecorder() || captureRenderPasses() ||
      !Config::Get().player.captureVulkanSubmitsResources.empty()) {
    if (SD()._framebufferstates[pRenderPassBegin->framebuffer]
            ->framebufferCreateInfoData.Value()
            ->flags &
        VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT) {
      VkRenderPassAttachmentBeginInfo* rpAttBeginInfo =
          (VkRenderPassAttachmentBeginInfo*)pRenderPassBegin->pNext;
      for (uint32_t i = 0; i < rpAttBeginInfo->attachmentCount; i++) {
        beginRenderPassState->imageViewStateStoreListKHR.push_back(
            SD()._imageviewstates[rpAttBeginInfo->pAttachments[i]]);
        VkAttachmentLoadOp loadOp;
        VkAttachmentStoreOp storeOp;
        auto& state = SD()._renderpassstates[pRenderPassBegin->renderPass];
        if (getFormatAspectFlags(SD()._imageviewstates[rpAttBeginInfo->pAttachments[i]]
                                     ->imageStateStore->imageFormat) &
            VK_IMAGE_ASPECT_STENCIL_BIT) {
          if (state->renderPassCreateInfoData.Value()) {
            loadOp = state->renderPassCreateInfoData.Value()->pAttachments[i].stencilLoadOp;
            storeOp = state->renderPassCreateInfoData.Value()->pAttachments[i].stencilStoreOp;
          } else {
            loadOp = state->renderPassCreateInfo2Data.Value()->pAttachments[i].stencilLoadOp;
            storeOp = state->renderPassCreateInfo2Data.Value()->pAttachments[i].stencilStoreOp;
          }
        } else {
          if (state->renderPassCreateInfoData.Value()) {
            loadOp = state->renderPassCreateInfoData.Value()->pAttachments[i].loadOp;
            storeOp = state->renderPassCreateInfoData.Value()->pAttachments[i].storeOp;
          } else {
            loadOp = state->renderPassCreateInfo2Data.Value()->pAttachments[i].loadOp;
            storeOp = state->renderPassCreateInfo2Data.Value()->pAttachments[i].storeOp;
          }
        }
        beginRenderPassState->imageLoadOp.push_back(loadOp);
        beginRenderPassState->imageStoreOp.push_back(storeOp);
      }
    } else {
      for (uint32_t i = 0;
           i <
           SD()._framebufferstates[pRenderPassBegin->framebuffer]->imageViewStateStoreList.size();
           ++i) {
        auto imageViewStateStore =
            SD()._framebufferstates[pRenderPassBegin->framebuffer]->imageViewStateStoreList[i];
        VkAttachmentLoadOp loadOp;
        VkAttachmentStoreOp storeOp;

        auto& state = SD()._renderpassstates[pRenderPassBegin->renderPass];
        if (getFormatAspectFlags(imageViewStateStore->imageStateStore->imageFormat) &
            VK_IMAGE_ASPECT_STENCIL_BIT) {
          if (state->renderPassCreateInfoData.Value()) {
            loadOp = state->renderPassCreateInfoData.Value()->pAttachments[i].stencilLoadOp;
            storeOp = state->renderPassCreateInfoData.Value()->pAttachments[i].stencilStoreOp;
          } else {
            loadOp = state->renderPassCreateInfo2Data.Value()->pAttachments[i].stencilLoadOp;
            storeOp = state->renderPassCreateInfo2Data.Value()->pAttachments[i].stencilStoreOp;
          }
        } else {
          if (state->renderPassCreateInfoData.Value()) {
            loadOp = state->renderPassCreateInfoData.Value()->pAttachments[i].loadOp;
            storeOp = state->renderPassCreateInfoData.Value()->pAttachments[i].storeOp;
          } else {
            loadOp = state->renderPassCreateInfo2Data.Value()->pAttachments[i].loadOp;
            storeOp = state->renderPassCreateInfo2Data.Value()->pAttachments[i].storeOp;
          }
        }
        beginRenderPassState->imageLoadOp.push_back(loadOp);
        beginRenderPassState->imageStoreOp.push_back(storeOp);
      }
    }
  }
  if (Config::Get().IsRecorder()) {
    if (!(SD()._framebufferstates[pRenderPassBegin->framebuffer]
              ->framebufferCreateInfoData.Value()
              ->flags &
          VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT)) {
      for (auto& imageViewState :
           SD()._framebufferstates[pRenderPassBegin->framebuffer]->imageViewStateStoreList) {
        auto& imageState = imageViewState->imageStateStore;

        if ((TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED ==
             Config::Get().recorder.vulkan.utilities.memoryUpdateState) ||
            (isSubcaptureBeforeRestorationPhase())) {
          SD().bindingImages[cmdBuffer].insert(imageState->imageHandle);
        }
        if ((Config::Get().recorder.vulkan.utilities.memorySegmentSize ||
             (Config::Get().recorder.vulkan.utilities.shadowMemory)) &&
            (imageState->binding)) {
          VkDeviceSize dstOffsetFinal = imageState->binding->memoryOffset;
          VkDeviceMemory dstDeviceMemory =
              imageState->binding->deviceMemoryStateStore->deviceMemoryHandle;
          VkMemoryRequirements memRequirements = {};
          drvVk.vkGetImageMemoryRequirements(imageState->deviceStateStore->deviceHandle,
                                             imageState->imageHandle, &memRequirements);
          //TODO : call vkGetImageSubresourceLayout when tiling is Linear
          SD().updatedMemoryInCmdBuffer[cmdBuffer].AddToMap(dstDeviceMemory, dstOffsetFinal,
                                                            memRequirements.size);
        }
      }
    } else {
      for (uint32_t i = 0; i < beginRenderPassState->imageViewStateStoreListKHR.size(); i++) {
        auto& imageState = beginRenderPassState->imageViewStateStoreListKHR[i]->imageStateStore;
        if ((TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED ==
             Config::Get().recorder.vulkan.utilities.memoryUpdateState) ||
            (isSubcaptureBeforeRestorationPhase())) {
          SD().bindingImages[cmdBuffer].insert(imageState->imageHandle);
        }
        if ((Config::Get().recorder.vulkan.utilities.memorySegmentSize ||
             (Config::Get().recorder.vulkan.utilities.shadowMemory)) &&
            (imageState->binding)) {
          VkDeviceSize dstOffsetFinal = imageState->binding->memoryOffset;
          VkDeviceMemory dstDeviceMemory =
              imageState->binding->deviceMemoryStateStore->deviceMemoryHandle;
          VkMemoryRequirements memRequirements = {};
          drvVk.vkGetImageMemoryRequirements(imageState->deviceStateStore->deviceHandle,
                                             imageState->imageHandle, &memRequirements);
          //TODO : call vkGetImageSubresourceLayout when tiling is Linear
          SD().updatedMemoryInCmdBuffer[cmdBuffer].AddToMap(dstDeviceMemory, dstOffsetFinal,
                                                            memRequirements.size);
        }
      }
    }
  }
}

inline void vkCmdBeginRenderPass2_SD(VkCommandBuffer commandBuffer,
                                     const VkRenderPassBeginInfo* pRenderPassBegin,
                                     const VkSubpassBeginInfo* pSubpassBeginInfo) {
  if (pSubpassBeginInfo == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  vkCmdBeginRenderPass_SD(commandBuffer, pRenderPassBegin, pSubpassBeginInfo->contents);
}

inline void vkCmdBeginRenderPass2KHR_SD(VkCommandBuffer commandBuffer,
                                        const VkRenderPassBeginInfo* pRenderPassBegin,
                                        const VkSubpassBeginInfo* pSubpassBeginInfo) {
  if (pSubpassBeginInfo == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  vkCmdBeginRenderPass_SD(commandBuffer, pRenderPassBegin, pSubpassBeginInfo->contents);
}

inline void vkCmdEndRenderPass_SD(VkCommandBuffer commandBuffer) {
  if (((Config::Get().recorder.basic.enabled) && (isSubcaptureBeforeRestorationPhase()) &&
       (Config::Get().recorder.vulkan.utilities.crossPlatformStateRestoration.images)) ||
      (captureRenderPasses() || !Config::Get().player.captureVulkanSubmitsResources.empty())) {
    auto& commandBufferState = SD()._commandbufferstates[commandBuffer];

    if (commandBufferState->beginRenderPassesList.size()) {
      auto& renderPassState =
          commandBufferState->beginRenderPassesList.back()->renderPassStateStore;
      auto& framebufferState =
          commandBufferState->beginRenderPassesList.back()->framebufferStateStore;

      for (uint32_t i = 0; i < renderPassState->finalImageLayoutList.size(); ++i) {
        VkImageSubresourceRange imageViewSubresourceRange;
        std::shared_ptr<CImageState> imageState;
        if (!(framebufferState->framebufferCreateInfoData.Value()->flags &
              VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT)) {
          imageViewSubresourceRange = framebufferState->imageViewStateStoreList[i]
                                          ->imageViewCreateInfoData.Value()
                                          ->subresourceRange;
          imageState = framebufferState->imageViewStateStoreList[i]->imageStateStore;
        } else {
          imageViewSubresourceRange = commandBufferState->beginRenderPassesList.back()
                                          ->imageViewStateStoreListKHR[i]
                                          ->imageViewCreateInfoData.Value()
                                          ->subresourceRange;
          imageState = commandBufferState->beginRenderPassesList.back()
                           ->imageViewStateStoreListKHR[i]
                           ->imageStateStore;
        }
        if (imageViewSubresourceRange.levelCount == VK_REMAINING_MIP_LEVELS) {
          imageViewSubresourceRange.levelCount =
              imageState->mipLevels - imageViewSubresourceRange.baseMipLevel;
        }
        if (imageViewSubresourceRange.layerCount == VK_REMAINING_ARRAY_LAYERS) {
          imageViewSubresourceRange.layerCount =
              imageState->arrayLayers - imageViewSubresourceRange.baseArrayLayer;
        }
        auto& imageLayoutData = imageState->currentLayout;
        if (!imageLayoutData.size()) {
          continue;
        }

        commandBufferState->touchedResources.push_back({(uint64_t)imageState->imageHandle, true});
        auto& commandBufferImageState =
            commandBufferState->imageLayoutAfterSubmit[imageState->imageHandle];
        if (!commandBufferImageState.size()) {
          commandBufferImageState.resize(imageLayoutData.size());
          for (uint32_t l = 0; l < imageLayoutData.size(); ++l) {
            commandBufferImageState[l].resize(imageLayoutData[l].size(),
                                              {(VkImageLayout)-1, 0, VK_QUEUE_FAMILY_IGNORED});
          }
        }

        if (VK_IMAGE_TYPE_3D == imageState->imageType) {
          commandBufferImageState[0][imageViewSubresourceRange.baseMipLevel] = {
              renderPassState->finalImageLayoutList[i],
              getLayoutAccessFlags(renderPassState->finalImageLayoutList[i]),
              VK_QUEUE_FAMILY_IGNORED};
        } else {
          for (uint32_t layer = imageViewSubresourceRange.baseArrayLayer;
               layer <
               imageViewSubresourceRange.baseArrayLayer + imageViewSubresourceRange.layerCount;
               ++layer) {
            commandBufferImageState[layer][imageViewSubresourceRange.baseMipLevel] = {
                renderPassState->finalImageLayoutList[i],
                getLayoutAccessFlags(renderPassState->finalImageLayoutList[i]),
                VK_QUEUE_FAMILY_IGNORED};
          }
        }
      }
    }
    vkEndRenderPass_setImageLayout(commandBufferState);
    vkEndRenderPass_updateNonDeterministicImages(commandBufferState);
  }
}

inline void vkCmdEndRenderPass2_SD(VkCommandBuffer commandBuffer,
                                   const VkSubpassEndInfo* pSubpassEndInfo) {
  vkCmdEndRenderPass_SD(commandBuffer);
}

inline void vkCmdEndRenderPass2KHR_SD(VkCommandBuffer commandBuffer,
                                      const VkSubpassEndInfo* pSubpassEndInfo) {
  vkCmdEndRenderPass_SD(commandBuffer);
}

inline void vkCmdBeginRendering_SD(VkCommandBuffer commandBuffer,
                                   const VkRenderingInfo* pRenderingInfo) {
  auto beginRenderPassState =
      std::make_shared<CCommandBufferState::CBeginRenderPass>(pRenderingInfo);
  SD()._commandbufferstates[commandBuffer]->beginRenderPassesList.push_back(beginRenderPassState);

  if (Config::Get().IsRecorder() || captureRenderPasses() ||
      !Config::Get().player.captureVulkanSubmitsResources.empty()) {

    for (uint32_t i = 0; i < pRenderingInfo->colorAttachmentCount; i++) {
      if (pRenderingInfo->pColorAttachments[i].imageView != VK_NULL_HANDLE) {
        beginRenderPassState->imageViewStateStoreListKHR.push_back(
            SD()._imageviewstates[pRenderingInfo->pColorAttachments[i].imageView]);
        beginRenderPassState->imageLoadOp.push_back(pRenderingInfo->pColorAttachments[i].loadOp);
        beginRenderPassState->imageStoreOp.push_back(pRenderingInfo->pColorAttachments[i].storeOp);
      }
    }

    if ((pRenderingInfo->pDepthAttachment != NULL) &&
        (pRenderingInfo->pDepthAttachment->imageView != VK_NULL_HANDLE)) {
      beginRenderPassState->imageViewStateStoreListKHR.push_back(
          SD()._imageviewstates[pRenderingInfo->pDepthAttachment->imageView]);
      beginRenderPassState->imageLoadOp.push_back(pRenderingInfo->pDepthAttachment->loadOp);
      beginRenderPassState->imageStoreOp.push_back(pRenderingInfo->pDepthAttachment->storeOp);
    }

    if ((pRenderingInfo->pStencilAttachment != NULL) &&
        (pRenderingInfo->pStencilAttachment->imageView != VK_NULL_HANDLE)) {
      beginRenderPassState->imageViewStateStoreListKHR.push_back(
          SD()._imageviewstates[pRenderingInfo->pStencilAttachment->imageView]);
      beginRenderPassState->imageLoadOp.push_back(pRenderingInfo->pStencilAttachment->loadOp);
      beginRenderPassState->imageStoreOp.push_back(pRenderingInfo->pStencilAttachment->storeOp);
    }
  }

  if (Config::Get().IsRecorder()) {
    for (uint32_t i = 0; i < beginRenderPassState->imageViewStateStoreListKHR.size(); i++) {
      auto& imageState = beginRenderPassState->imageViewStateStoreListKHR[i]->imageStateStore;
      if ((TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED ==
           Config::Get().recorder.vulkan.utilities.memoryUpdateState) ||
          (isSubcaptureBeforeRestorationPhase())) {
        SD().bindingImages[commandBuffer].insert(imageState->imageHandle);
      }
      if ((Config::Get().recorder.vulkan.utilities.memorySegmentSize ||
           (Config::Get().recorder.vulkan.utilities.shadowMemory)) &&
          (imageState->binding)) {
        VkDeviceSize dstOffsetFinal = imageState->binding->memoryOffset;
        VkDeviceMemory dstDeviceMemory =
            imageState->binding->deviceMemoryStateStore->deviceMemoryHandle;
        VkMemoryRequirements memRequirements = {};
        drvVk.vkGetImageMemoryRequirements(imageState->deviceStateStore->deviceHandle,
                                           imageState->imageHandle, &memRequirements);
        //TODO : call vkGetImageSubresourceLayout when tiling is Linear
        SD().updatedMemoryInCmdBuffer[commandBuffer].AddToMap(dstDeviceMemory, dstOffsetFinal,
                                                              memRequirements.size);
      }
    }
  }
}

inline void vkCmdEndRendering_SD(VkCommandBuffer commandBuffer) {
  auto& commandBufferState = SD()._commandbufferstates[commandBuffer];
  vkEndRenderPass_setImageLayout(commandBufferState);
  vkEndRenderPass_updateNonDeterministicImages(commandBufferState);
}

namespace {

void BindVertexBuffers_SDHelper(VkCommandBuffer cmdBuffer,
                                uint32_t bindingCount,
                                const VkBuffer* pBuffers) {
  if (Config::Get().IsRecorder() && ((TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED ==
                                      Config::Get().recorder.vulkan.utilities.memoryUpdateState) ||
                                     isSubcaptureBeforeRestorationPhase())) {
    auto& bindingBuffers = SD().bindingBuffers[cmdBuffer];
    for (uint32_t i = 0; i < bindingCount; ++i) {
      if (pBuffers[i] != VK_NULL_HANDLE) {
        bindingBuffers.insert(pBuffers[i]);
      }
    }
  }
}

} // namespace

inline void vkCmdBindVertexBuffers_SD(VkCommandBuffer cmdBuffer,
                                      uint32_t startBinding,
                                      uint32_t bindingCount,
                                      const VkBuffer* pBuffers,
                                      const VkDeviceSize* pOffsets) {
  BindVertexBuffers_SDHelper(cmdBuffer, bindingCount, pBuffers);
}

inline void vkCmdBindVertexBuffers2_SD(VkCommandBuffer cmdBuffer,
                                       uint32_t firstBinding,
                                       uint32_t bindingCount,
                                       const VkBuffer* pBuffers,
                                       const VkDeviceSize* pOffsets,
                                       const VkDeviceSize* pSizes,
                                       const VkDeviceSize* pStrides) {
  BindVertexBuffers_SDHelper(cmdBuffer, bindingCount, pBuffers);
}

inline void vkCmdBindVertexBuffers2EXT_SD(VkCommandBuffer cmdBuffer,
                                          uint32_t firstBinding,
                                          uint32_t bindingCount,
                                          const VkBuffer* pBuffers,
                                          const VkDeviceSize* pOffsets,
                                          const VkDeviceSize* pSizes,
                                          const VkDeviceSize* pStrides) {
  BindVertexBuffers_SDHelper(cmdBuffer, bindingCount, pBuffers);
}

inline void vkCmdBindIndexBuffer_SD(VkCommandBuffer cmdBuffer,
                                    VkBuffer buffer,
                                    VkDeviceSize offset,
                                    VkIndexType indexType) {
  if (Config::Get().IsRecorder() && ((TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED ==
                                      Config::Get().recorder.vulkan.utilities.memoryUpdateState) ||
                                     isSubcaptureBeforeRestorationPhase())) {
    SD().bindingBuffers[cmdBuffer].insert(buffer);
  }
}

inline void vkCmdBindTransformFeedbackBuffersEXT_SD(VkCommandBuffer commandBuffer,
                                                    uint32_t firstBinding,
                                                    uint32_t bindingCount,
                                                    const VkBuffer* pBuffers,
                                                    const VkDeviceSize* pOffsets,
                                                    const VkDeviceSize* pSizes) {
  if (Config::Get().IsRecorder() && ((TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED ==
                                      Config::Get().recorder.vulkan.utilities.memoryUpdateState) ||
                                     isSubcaptureBeforeRestorationPhase())) {
    SD().bindingBuffers[commandBuffer].insert(pBuffers, pBuffers + bindingCount);
  }
}

inline void vkCmdBindPipeline_SD(VkCommandBuffer commandBuffer,
                                 VkPipelineBindPoint pipelineBindPoint,
                                 VkPipeline pipeline) {
  auto& commandBufferState = SD()._commandbufferstates[commandBuffer];
  commandBufferState->pipelineStateStoreList.emplace(pipeline, SD()._pipelinestates[pipeline]);
  commandBufferState->currentPipeline = pipeline;
}

inline void vkCmdBindDescriptorSets_SD(VkCommandBuffer commandBuffer,
                                       VkPipelineBindPoint pipelineBindPoint,
                                       VkPipelineLayout layout,
                                       uint32_t firstSet,
                                       uint32_t descriptorSetCount,
                                       const VkDescriptorSet* pDescriptorSets,
                                       uint32_t dynamicOffsetCount,
                                       const uint32_t* pDynamicOffsets) {
  if (Config::Get().IsRecorder() || !Config::Get().player.captureVulkanSubmitsResources.empty()) {
    for (unsigned int i = 0; i < descriptorSetCount; i++) {
      if (pDescriptorSets[i] == VK_NULL_HANDLE) {
        continue;
      }

      auto& commandBufferState = SD()._commandbufferstates[commandBuffer];
      if ((Config::Get().recorder.vulkan.utilities.memoryUpdateState ==
           TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED) ||
          isSubcaptureBeforeRestorationPhase()) {
        auto& descriptorSetState = SD()._descriptorsetstates[pDescriptorSets[i]];

        commandBufferState->descriptorSetStateStoreList.emplace(pDescriptorSets[i],
                                                                descriptorSetState);

        for (auto obj : descriptorSetState->descriptorBuffers) {
          SD().bindingBuffers[commandBuffer].insert(obj.second);
        }

        for (auto obj : descriptorSetState->descriptorImages) {
          SD().bindingImages[commandBuffer].insert(obj.second);
        }
      }
      for (auto& obj : SD()._descriptorsetstates[pDescriptorSets[i]]->descriptorWriteBuffers) {
        commandBufferState->touchedResources.emplace_back((uint64_t)obj.second.second, false);
        commandBufferState->resourceWriteBuffers[obj.second.second] = obj.second.first;
      }
      for (auto& obj : SD()._descriptorsetstates[pDescriptorSets[i]]->descriptorWriteImages) {
        commandBufferState->touchedResources.emplace_back((uint64_t)obj.second.second, true);
        commandBufferState->resourceWriteImages[obj.second.second] = obj.second.first;
      }
      if (Config::Get().recorder.vulkan.utilities.memorySegmentSize ||
          Config::Get().recorder.vulkan.utilities.shadowMemory) {
        for (auto& binding : SD()._descriptorsetstates[pDescriptorSets[i]]->descriptorMapMemory) {
          for (auto mem : binding.second) {
            for (auto obj : mem.second) {
              SD().updatedMemoryInCmdBuffer[commandBuffer].AddToMap(mem.first, obj.lower(),
                                                                    obj.upper() - obj.lower());
            }
          }
        }
      }
    }
  }
}

inline void vkCmdPushDescriptorSetKHR_SD(VkCommandBuffer commandBuffer,
                                         VkPipelineBindPoint pipelineBindPoint,
                                         VkPipelineLayout layout,
                                         uint32_t set,
                                         uint32_t descriptorWriteCount,
                                         const VkWriteDescriptorSet* pDescriptorWrites) {
  if (!Config::Get().IsRecorder()) {
    return;
  }
  if (!isSubcaptureBeforeRestorationPhase()) {
    return;
  }

  for (unsigned int i = 0; i < descriptorWriteCount; i++) {
    for (unsigned int j = 0; j < pDescriptorWrites[i].descriptorCount; j++) {
      const auto descriptorType = pDescriptorWrites[i].descriptorType;
      switch (descriptorType) {
      case VK_DESCRIPTOR_TYPE_SAMPLER:
      case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
      case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
      case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
      case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
        if ((nullptr != pDescriptorWrites[i].pImageInfo) &&
            (VK_NULL_HANDLE != pDescriptorWrites[i].pImageInfo[j].imageView)) {
          auto& imageState =
              SD()._imageviewstates[pDescriptorWrites[i].pImageInfo[j].imageView]->imageStateStore;

          if (VK_DESCRIPTOR_TYPE_STORAGE_IMAGE == descriptorType) {
            SD()._commandbufferstates[commandBuffer]->touchedResources.emplace_back(
                (uint64_t)imageState->imageHandle, true);
          }

          if (Config::Get().recorder.vulkan.utilities.memoryUpdateState ==
              TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED) {
            SD().bindingImages[commandBuffer].insert(imageState->imageHandle);

            if ((Config::Get().recorder.vulkan.utilities.memorySegmentSize ||
                 Config::Get().recorder.vulkan.utilities.shadowMemory) &&
                (VK_DESCRIPTOR_TYPE_STORAGE_IMAGE == descriptorType) && (imageState->binding)) {
              VkDeviceSize dstOffsetFinal = imageState->binding->memoryOffset;
              VkDeviceMemory dstDeviceMemory =
                  imageState->binding->deviceMemoryStateStore->deviceMemoryHandle;
              VkMemoryRequirements memRequirements = {};
              drvVk.vkGetImageMemoryRequirements(imageState->deviceStateStore->deviceHandle,
                                                 imageState->imageHandle, &memRequirements);
              //TODO : call vkGetImageSubresourceLayout when tiling is Linear
              SD().updatedMemoryInCmdBuffer[commandBuffer].AddToMap(dstDeviceMemory, dstOffsetFinal,
                                                                    memRequirements.size);
            }
          }
        }
        break;
      case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
      case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
        if (nullptr != pDescriptorWrites[i].pTexelBufferView) {
          auto& bufferState =
              SD()._bufferviewstates[pDescriptorWrites[i].pTexelBufferView[j]]->bufferStateStore;

          if (VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER == descriptorType) {
            SD()._commandbufferstates[commandBuffer]->touchedResources.emplace_back(
                (uint64_t)bufferState->bufferHandle, false);
          }

          if (Config::Get().recorder.vulkan.utilities.memoryUpdateState ==
              TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED) {
            SD().bindingBuffers[commandBuffer].insert(bufferState->bufferHandle);

            if ((Config::Get().recorder.vulkan.utilities.memorySegmentSize ||
                 Config::Get().recorder.vulkan.utilities.shadowMemory) &&
                (VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER == descriptorType) &&
                (bufferState->binding)) {
              auto& bufferViewState =
                  SD()._bufferviewstates[pDescriptorWrites[i].pTexelBufferView[j]];
              VkDeviceSize dstOffsetFinal =
                  bufferState->binding->memoryOffset +
                  bufferViewState->bufferViewCreateInfoData.Value()->offset;
              VkDeviceSize dstFinalSize;
              if (bufferViewState->bufferViewCreateInfoData.Value()->range == 0xFFFFFFFFFFFFFFFF) {
                dstFinalSize = bufferState->bufferCreateInfoData.Value()->size -
                               bufferViewState->bufferViewCreateInfoData.Value()->offset;
              } else {
                dstFinalSize = bufferViewState->bufferViewCreateInfoData.Value()->range;
              }

              SD().updatedMemoryInCmdBuffer[commandBuffer].AddToMap(
                  bufferState->binding->deviceMemoryStateStore->deviceMemoryHandle, dstOffsetFinal,
                  dstFinalSize);
            }
          }
        }
        break;
      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
      case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
      case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
        if ((nullptr != pDescriptorWrites[i].pBufferInfo) &&
            (VK_NULL_HANDLE != pDescriptorWrites[i].pBufferInfo[j].buffer)) {
          auto buffer = pDescriptorWrites[i].pBufferInfo[j].buffer;

          if ((VK_DESCRIPTOR_TYPE_STORAGE_BUFFER == descriptorType) ||
              (VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC == descriptorType)) {
            SD()._commandbufferstates[commandBuffer]->touchedResources.emplace_back(
                (uint64_t)buffer, false);
          }

          if (Config::Get().recorder.vulkan.utilities.memoryUpdateState ==
              TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED) {
            SD().bindingBuffers[commandBuffer].insert(buffer);

            if ((Config::Get().recorder.vulkan.utilities.memorySegmentSize ||
                 Config::Get().recorder.vulkan.utilities.shadowMemory) &&
                ((VK_DESCRIPTOR_TYPE_STORAGE_BUFFER == descriptorType) ||
                 (VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC == descriptorType))) {
              auto& bufferState = SD()._bufferstates[pDescriptorWrites[i].pBufferInfo[j].buffer];
              if (bufferState->binding) {
                VkDeviceSize dstOffsetFinal =
                    bufferState->binding->memoryOffset + pDescriptorWrites[i].pBufferInfo[j].offset;
                VkDeviceSize dstFinalSize;
                if (pDescriptorWrites[i].pBufferInfo[j].range == 0xFFFFFFFFFFFFFFFF) {
                  dstFinalSize = bufferState->bufferCreateInfoData.Value()->size -
                                 pDescriptorWrites[i].pBufferInfo[j].offset;
                } else {
                  dstFinalSize = pDescriptorWrites[i].pBufferInfo[j].range;
                }

                SD().updatedMemoryInCmdBuffer[commandBuffer].AddToMap(
                    bufferState->binding->deviceMemoryStateStore->deviceMemoryHandle,
                    dstOffsetFinal, dstFinalSize);
              }
            }
          }
        }
        break;
      default:
        Log(TRACE) << "Not handled VkDescriptorType enumeration: " + std::to_string(descriptorType);
        break;
      }
    }
  }
}

inline void vkCmdDraw_SD(VkCommandBuffer commandBuffer, uint32_t, uint32_t, uint32_t, uint32_t) {
  printShaderHashes(SD()._commandbufferstates[commandBuffer]->currentPipeline);
}

inline void vkCmdDrawIndexed_SD(
    VkCommandBuffer commandBuffer, uint32_t, uint32_t, uint32_t, int32_t, uint32_t) {
  printShaderHashes(SD()._commandbufferstates[commandBuffer]->currentPipeline);
}

inline void vkCmdDrawIndexedIndirect_SD(VkCommandBuffer commandBuffer,
                                        VkBuffer buffer,
                                        VkDeviceSize offset,
                                        uint32_t drawCount,
                                        uint32_t stride) {
  if (Config::Get().IsRecorder() && ((TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED ==
                                      Config::Get().recorder.vulkan.utilities.memoryUpdateState) ||
                                     isSubcaptureBeforeRestorationPhase())) {
    if (VK_NULL_HANDLE != buffer) {
      SD().bindingBuffers[commandBuffer].insert(buffer);
    }
  }
  printShaderHashes(SD()._commandbufferstates[commandBuffer]->currentPipeline);
}

inline void vkCmdDrawIndirect_SD(VkCommandBuffer commandBuffer,
                                 VkBuffer buffer,
                                 VkDeviceSize offset,
                                 uint32_t drawCount,
                                 uint32_t stride) {
  if (Config::Get().IsRecorder() && ((TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED ==
                                      Config::Get().recorder.vulkan.utilities.memoryUpdateState) ||
                                     isSubcaptureBeforeRestorationPhase())) {
    if (VK_NULL_HANDLE != buffer) {
      SD().bindingBuffers[commandBuffer].insert(buffer);
    }
  }
  printShaderHashes(SD()._commandbufferstates[commandBuffer]->currentPipeline);
}

inline void vkCmdDrawIndirectCountKHR_SD(VkCommandBuffer commandBuffer,
                                         VkBuffer buffer,
                                         VkDeviceSize offset,
                                         VkBuffer countBuffer,
                                         VkDeviceSize countBufferOffset,
                                         uint32_t maxDrawCount,
                                         uint32_t stride) {
  if (Config::Get().IsRecorder() && ((TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED ==
                                      Config::Get().recorder.vulkan.utilities.memoryUpdateState) ||
                                     isSubcaptureBeforeRestorationPhase())) {
    if (VK_NULL_HANDLE != buffer) {
      SD().bindingBuffers[commandBuffer].insert(buffer);
    }
    if (VK_NULL_HANDLE != countBuffer) {
      SD().bindingBuffers[commandBuffer].insert(countBuffer);
    }
  }
  printShaderHashes(SD()._commandbufferstates[commandBuffer]->currentPipeline);
}

inline void vkCmdDrawIndexedIndirectCount_SD(VkCommandBuffer commandBuffer,
                                             VkBuffer buffer,
                                             VkDeviceSize offset,
                                             VkBuffer countBuffer,
                                             VkDeviceSize countBufferOffset,
                                             uint32_t maxDrawCount,
                                             uint32_t stride) {
  if (Config::Get().IsRecorder() && ((TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED ==
                                      Config::Get().recorder.vulkan.utilities.memoryUpdateState) ||
                                     isSubcaptureBeforeRestorationPhase())) {
    if (VK_NULL_HANDLE != buffer) {
      SD().bindingBuffers[commandBuffer].insert(buffer);
    }
    if (VK_NULL_HANDLE != countBuffer) {
      SD().bindingBuffers[commandBuffer].insert(countBuffer);
    }
  }
  printShaderHashes(SD()._commandbufferstates[commandBuffer]->currentPipeline);
}

inline void vkCmdDrawIndexedIndirectCountKHR_SD(VkCommandBuffer commandBuffer,
                                                VkBuffer buffer,
                                                VkDeviceSize offset,
                                                VkBuffer countBuffer,
                                                VkDeviceSize countBufferOffset,
                                                uint32_t maxDrawCount,
                                                uint32_t stride) {
  if (Config::Get().IsRecorder() && ((TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED ==
                                      Config::Get().recorder.vulkan.utilities.memoryUpdateState) ||
                                     isSubcaptureBeforeRestorationPhase())) {
    if (VK_NULL_HANDLE != buffer) {
      SD().bindingBuffers[commandBuffer].insert(buffer);
    }
    if (VK_NULL_HANDLE != countBuffer) {
      SD().bindingBuffers[commandBuffer].insert(countBuffer);
    }
  }
  printShaderHashes(SD()._commandbufferstates[commandBuffer]->currentPipeline);
}

inline void vkCmdDrawMeshTasksEXT_SD(VkCommandBuffer commandBuffer,
                                     uint32_t groupCountX,
                                     uint32_t groupCountY,
                                     uint32_t groupCountZ) {
  printShaderHashes(SD()._commandbufferstates[commandBuffer]->currentPipeline);
}

inline void vkCmdDrawMeshTasksNV_SD(VkCommandBuffer commandBuffer,
                                    uint32_t taskCount,
                                    uint32_t firstTask) {
  printShaderHashes(SD()._commandbufferstates[commandBuffer]->currentPipeline);
}

inline void vkCmdDrawMeshTasksIndirectEXT_SD(VkCommandBuffer commandBuffer,
                                             VkBuffer buffer,
                                             VkDeviceSize offset,
                                             uint32_t drawCount,
                                             uint32_t stride) {
  if (isRecorder() && (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase())) {
    if (VK_NULL_HANDLE != buffer) {
      SD().bindingBuffers[commandBuffer].insert(buffer);
    }
  }
  printShaderHashes(SD()._commandbufferstates[commandBuffer]->currentPipeline);
}

inline void vkCmdDrawMeshTasksIndirectNV_SD(VkCommandBuffer commandBuffer,
                                            VkBuffer buffer,
                                            VkDeviceSize offset,
                                            uint32_t drawCount,
                                            uint32_t stride) {
  vkCmdDrawMeshTasksIndirectEXT_SD(commandBuffer, buffer, offset, drawCount, stride);
}

inline void vkCmdDrawMeshTasksIndirectCountEXT_SD(VkCommandBuffer commandBuffer,
                                                  VkBuffer buffer,
                                                  VkDeviceSize offset,
                                                  VkBuffer countBuffer,
                                                  VkDeviceSize countBufferOffset,
                                                  uint32_t maxDrawCount,
                                                  uint32_t stride) {
  if (isRecorder() && (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase())) {
    if (VK_NULL_HANDLE != buffer) {
      SD().bindingBuffers[commandBuffer].insert(buffer);
    }
    if (VK_NULL_HANDLE != countBuffer) {
      SD().bindingBuffers[commandBuffer].insert(countBuffer);
    }
  }
  printShaderHashes(SD()._commandbufferstates[commandBuffer]->currentPipeline);
}

inline void vkCmdDrawMeshTasksIndirectCountNV_SD(VkCommandBuffer commandBuffer,
                                                 VkBuffer buffer,
                                                 VkDeviceSize offset,
                                                 VkBuffer countBuffer,
                                                 VkDeviceSize countBufferOffset,
                                                 uint32_t maxDrawCount,
                                                 uint32_t stride) {
  vkCmdDrawMeshTasksIndirectCountEXT_SD(commandBuffer, buffer, offset, countBuffer,
                                        countBufferOffset, maxDrawCount, stride);
}

inline void vkCmdDispatch_SD(VkCommandBuffer commandBuffer, uint32_t, uint32_t, uint32_t) {
  printShaderHashes(SD()._commandbufferstates[commandBuffer]->currentPipeline);
}

inline void vkCmdDispatchIndirect_SD(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize) {
  if (Config::Get().IsRecorder() && ((TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED ==
                                      Config::Get().recorder.vulkan.utilities.memoryUpdateState) ||
                                     isSubcaptureBeforeRestorationPhase())) {
    if (VK_NULL_HANDLE != buffer) {
      SD().bindingBuffers[commandBuffer].insert(buffer);
    }
  }
  printShaderHashes(SD()._commandbufferstates[commandBuffer]->currentPipeline);
}

inline void vkCmdExecuteCommands_SD(VkCommandBuffer commandBuffer,
                                    uint32_t commandBufferCount,
                                    const VkCommandBuffer* pCommandBuffers) {
  if (Config::Get().IsRecorder() || (!Config::Get().player.captureVulkanSubmitsResources.empty() ||
                                     !Config::Get().player.captureVulkanRenderPasses.empty() ||
                                     Config::Get().player.execCmdBuffsBeforeQueueSubmit)) {
    for (unsigned int i = 0; i < commandBufferCount; i++) {
      auto& primaryCommandBufferState = SD()._commandbufferstates[commandBuffer];
      auto& secondaryCommandBufferState = SD()._commandbufferstates[pCommandBuffers[i]];

      if ((TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED ==
           Config::Get().recorder.vulkan.utilities.memoryUpdateState) ||
          (isSubcaptureBeforeRestorationPhase())) {
        auto& srcBindingImages = SD().bindingImages[pCommandBuffers[i]];
        SD().bindingImages[commandBuffer].insert(srcBindingImages.begin(), srcBindingImages.end());

        auto& srcBindingBuffers = SD().bindingBuffers[pCommandBuffers[i]];
        SD().bindingBuffers[commandBuffer].insert(srcBindingBuffers.begin(),
                                                  srcBindingBuffers.end());
      }

      primaryCommandBufferState->secondaryCommandBuffersStateStoreList[pCommandBuffers[i]] =
          secondaryCommandBufferState;

      // Track touched resources
      primaryCommandBufferState->touchedResources.insert(
          primaryCommandBufferState->touchedResources.end(),
          secondaryCommandBufferState->touchedResources.begin(),
          secondaryCommandBufferState->touchedResources.end());

      // Track query pool state
      for (auto& queryPoolState : secondaryCommandBufferState->resetQueriesAfterSubmit) {
        primaryCommandBufferState->resetQueriesAfterSubmit[queryPoolState.first].insert(
            queryPoolState.second.begin(), queryPoolState.second.end());
      }

      for (auto& queryPoolState : secondaryCommandBufferState->usedQueriesAfterSubmit) {
        primaryCommandBufferState->usedQueriesAfterSubmit[queryPoolState.first].insert(
            queryPoolState.second.begin(), queryPoolState.second.end());
      }

      // Track event state
      for (auto& eventState : secondaryCommandBufferState->eventStatesAfterSubmit) {
        primaryCommandBufferState->eventStatesAfterSubmit[eventState.first] = eventState.second;
      }

      // Track image layout state
      for (auto& imageLayoutAfterSubmitSecondaryCommandBuffer :
           secondaryCommandBufferState->imageLayoutAfterSubmit) {
        VkImage image = imageLayoutAfterSubmitSecondaryCommandBuffer.first;
        auto& imageLayout = SD()._imagestates[image]->currentLayout;
        if (!imageLayout.size()) {
          continue;
        }

        auto& imageLayoutAfterSubmitPrimaryCommandBuffer =
            primaryCommandBufferState->imageLayoutAfterSubmit[image];
        if (!imageLayoutAfterSubmitPrimaryCommandBuffer.size()) {
          imageLayoutAfterSubmitPrimaryCommandBuffer.resize(imageLayout.size());
          for (uint32_t l = 0; l < imageLayout.size(); ++l) {
            imageLayoutAfterSubmitPrimaryCommandBuffer[l].resize(imageLayout[l].size(),
                                                                 {(VkImageLayout)-1, 0});
          }
        }

        for (uint32_t l = 0; l < imageLayoutAfterSubmitSecondaryCommandBuffer.second.size(); ++l) {
          for (uint32_t m = 0; m < imageLayoutAfterSubmitSecondaryCommandBuffer.second[l].size();
               ++m) {
            if (imageLayoutAfterSubmitSecondaryCommandBuffer.second[l][m].Layout !=
                (VkImageLayout)-1) {
              imageLayoutAfterSubmitPrimaryCommandBuffer[l][m].Layout =
                  imageLayoutAfterSubmitSecondaryCommandBuffer.second[l][m].Layout;
              imageLayoutAfterSubmitPrimaryCommandBuffer[l][m].Access =
                  imageLayoutAfterSubmitSecondaryCommandBuffer.second[l][m].Access;
            }
            if (imageLayoutAfterSubmitSecondaryCommandBuffer.second[l][m].QueueFamilyIndex !=
                VK_QUEUE_FAMILY_IGNORED) {
              imageLayoutAfterSubmitPrimaryCommandBuffer[l][m].QueueFamilyIndex =
                  imageLayoutAfterSubmitSecondaryCommandBuffer.second[l][m].QueueFamilyIndex;
            }
          }
        }
      }
      if (!Config::Get().player.captureVulkanSubmitsResources.empty()) {
        for (auto obj : secondaryCommandBufferState->resourceWriteBuffers) {
          primaryCommandBufferState->resourceWriteBuffers[obj.first] = obj.second;
        }
        for (auto obj : secondaryCommandBufferState->resourceWriteImages) {
          primaryCommandBufferState->resourceWriteImages[obj.first] = obj.second;
        }
      }
      if (Config::Get().recorder.vulkan.utilities.memorySegmentSize ||
          Config::Get().recorder.vulkan.utilities.shadowMemory) {
        for (auto obj3 : SD().updatedMemoryInCmdBuffer[pCommandBuffers[i]].intervalMapMemory) {
          for (auto obj4 : obj3.second) {
            SD().updatedMemoryInCmdBuffer[commandBuffer].AddToMap(obj3.first, obj4.lower(),
                                                                  obj4.upper() - obj4.lower());
          }
        }
      }
    }
  }
}

inline void vkCmdPipelineBarrier_SD(VkCommandBuffer commandBuffer,
                                    VkPipelineStageFlags srcStageMask,
                                    VkPipelineStageFlags dstStageMask,
                                    VkDependencyFlags dependencyFlags,
                                    uint32_t memoryBarrierCount,
                                    const VkMemoryBarrier* pMemoryBarriers,
                                    uint32_t bufferMemoryBarrierCount,
                                    const VkBufferMemoryBarrier* pBufferMemoryBarriers,
                                    uint32_t imageMemoryBarrierCount,
                                    const VkImageMemoryBarrier* pImageMemoryBarriers) {
  if (captureRenderPasses() || captureVulkanSubmitsResources() ||
      (isRecorder() && ((updateOnlyUsedMemory()) || isSubcaptureBeforeRestorationPhase()))) {

    for (unsigned int i = 0; i < bufferMemoryBarrierCount; i++) {
      SD().bindingBuffers[commandBuffer].insert(pBufferMemoryBarriers[i].buffer);
    }

    for (unsigned int i = 0; i < imageMemoryBarrierCount; i++) {
      SD().bindingImages[commandBuffer].insert(pImageMemoryBarriers[i].image);

      if (!CGits::Instance().IsStateRestoration() &&
          pImageMemoryBarriers[i].oldLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
        SD().nonDeterministicImages.insert(pImageMemoryBarriers[i].image);
      }

      if (captureRenderPasses() || captureVulkanSubmitsResources() ||
          (isSubcaptureBeforeRestorationPhase() && crossPlatformStateRestoration())) {
        auto& imageState = SD()._imagestates[pImageMemoryBarriers[i].image];
        auto& imageLayout = imageState->currentLayout;
        if (!imageLayout.size()) {
          continue;
        }

        auto& imageLayoutAfterSubmit = SD()._commandbufferstates[commandBuffer]
                                           ->imageLayoutAfterSubmit[pImageMemoryBarriers[i].image];
        if (!imageLayoutAfterSubmit.size()) {
          imageLayoutAfterSubmit.resize(imageLayout.size());
          for (uint32_t l = 0; l < imageLayout.size(); ++l) {
            imageLayoutAfterSubmit[l].resize(imageLayout[l].size(),
                                             {(VkImageLayout)-1, 0, VK_QUEUE_FAMILY_IGNORED});
          }
        }

        auto& subresourceRange = pImageMemoryBarriers[i].subresourceRange;
        if (VK_IMAGE_TYPE_3D == imageState->imageType) {
          for (uint32_t m = subresourceRange.baseMipLevel;
               (m < subresourceRange.baseMipLevel + subresourceRange.levelCount) &&
               (m < imageLayoutAfterSubmit[0].size());
               ++m) {
            imageLayoutAfterSubmit[0][m] = {pImageMemoryBarriers[i].newLayout,
                                            pImageMemoryBarriers[i].dstAccessMask,
                                            pImageMemoryBarriers[i].dstQueueFamilyIndex};
          }
        } else {
          for (uint32_t l = subresourceRange.baseArrayLayer;
               (l < subresourceRange.baseArrayLayer + subresourceRange.layerCount) &&
               (l < imageLayoutAfterSubmit.size());
               ++l) {
            for (uint32_t m = subresourceRange.baseMipLevel;
                 (m < subresourceRange.baseMipLevel + subresourceRange.levelCount) &&
                 (m < imageLayoutAfterSubmit[l].size());
                 ++m) {
              imageLayoutAfterSubmit[l][m] = {pImageMemoryBarriers[i].newLayout,
                                              pImageMemoryBarriers[i].dstAccessMask,
                                              pImageMemoryBarriers[i].dstQueueFamilyIndex};
            }
          }
        }
      }
    }
  }
}

inline void vkCmdPipelineBarrier2UnifiedGITS_SD(VkCommandBuffer commandBuffer,
                                                const VkDependencyInfo* pDependencyInfo) {
  if (captureRenderPasses() || captureVulkanSubmitsResources() ||
      (isRecorder() && (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase()))) {

    for (unsigned int i = 0; i < pDependencyInfo->bufferMemoryBarrierCount; i++) {
      SD().bindingBuffers[commandBuffer].insert(pDependencyInfo->pBufferMemoryBarriers[i].buffer);
    }

    for (unsigned int i = 0; i < pDependencyInfo->imageMemoryBarrierCount; i++) {
      SD().bindingImages[commandBuffer].insert(pDependencyInfo->pImageMemoryBarriers[i].image);

      if (!CGits::Instance().IsStateRestoration() &&
          pDependencyInfo->pImageMemoryBarriers[i].oldLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
        SD().nonDeterministicImages.insert(pDependencyInfo->pImageMemoryBarriers[i].image);
      }

      if (captureRenderPasses() || captureVulkanSubmitsResources() ||
          (isSubcaptureBeforeRestorationPhase() && crossPlatformStateRestoration())) {
        auto& imageState = SD()._imagestates[pDependencyInfo->pImageMemoryBarriers[i].image];
        auto& imageLayout = imageState->currentLayout;
        if (!imageLayout.size()) {
          continue;
        }

        auto& imageLayoutAfterSubmit =
            SD()._commandbufferstates[commandBuffer]
                ->imageLayoutAfterSubmit[pDependencyInfo->pImageMemoryBarriers[i].image];
        if (!imageLayoutAfterSubmit.size()) {
          imageLayoutAfterSubmit.resize(imageLayout.size());
          for (uint32_t l = 0; l < imageLayout.size(); ++l) {
            imageLayoutAfterSubmit[l].resize(imageLayout[l].size(),
                                             {(VkImageLayout)-1, 0, VK_QUEUE_FAMILY_IGNORED});
          }
        }

        auto& subresourceRange = pDependencyInfo->pImageMemoryBarriers[i].subresourceRange;
        if (VK_IMAGE_TYPE_3D == imageState->imageType) {
          for (uint32_t m = subresourceRange.baseMipLevel;
               (m < subresourceRange.baseMipLevel + subresourceRange.levelCount) &&
               (m < imageLayoutAfterSubmit[0].size());
               ++m) {
            imageLayoutAfterSubmit[0][m] = {
                pDependencyInfo->pImageMemoryBarriers[i].newLayout,
                pDependencyInfo->pImageMemoryBarriers[i].dstAccessMask,
                pDependencyInfo->pImageMemoryBarriers[i].dstQueueFamilyIndex};
          }
        } else {
          for (uint32_t l = subresourceRange.baseArrayLayer;
               (l < subresourceRange.baseArrayLayer + subresourceRange.layerCount) &&
               (l < imageLayoutAfterSubmit.size());
               ++l) {
            for (uint32_t m = subresourceRange.baseMipLevel;
                 (m < subresourceRange.baseMipLevel + subresourceRange.levelCount) &&
                 (m < imageLayoutAfterSubmit[l].size());
                 ++m) {
              imageLayoutAfterSubmit[l][m] = {
                  pDependencyInfo->pImageMemoryBarriers[i].newLayout,
                  pDependencyInfo->pImageMemoryBarriers[i].dstAccessMask,
                  pDependencyInfo->pImageMemoryBarriers[i].dstQueueFamilyIndex};
            }
          }
        }
      }
    }
  }
}

inline void vkCmdPipelineBarrier2_SD(VkCommandBuffer commandBuffer,
                                     const VkDependencyInfo* pDependencyInfo) {
  vkCmdPipelineBarrier2UnifiedGITS_SD(commandBuffer, pDependencyInfo);
}

inline void vkCmdPipelineBarrier2KHR_SD(VkCommandBuffer commandBuffer,
                                        const VkDependencyInfo* pDependencyInfo) {
  vkCmdPipelineBarrier2UnifiedGITS_SD(commandBuffer, pDependencyInfo);
}

inline void vkCmdSetEvent_SD(VkCommandBuffer commandBuffer,
                             VkEvent event,
                             VkPipelineStageFlags stageMask) {
  SD()._commandbufferstates[commandBuffer]->eventStatesAfterSubmit[event] = true;
}

inline void vkCmdResetEvent_SD(VkCommandBuffer commandBuffer,
                               VkEvent event,
                               VkPipelineStageFlags stageMask) {
  SD()._commandbufferstates[commandBuffer]->eventStatesAfterSubmit[event] = false;
}

inline void vkCmdSetEvent2_SD(VkCommandBuffer commandBuffer,
                              VkEvent event,
                              const VkDependencyInfo* pDependencyInfo) {
  if (pDependencyInfo == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  SD()._commandbufferstates[commandBuffer]->eventStatesAfterSubmit[event] = true;

  // Not counting eventStatesAfterSubmit, a state tracking for the vkCmdPipelineBarrier2...()
  // functions is exactly the same as for the vkCmdSetEvent2() function. That's why, to
  // avoid code redundancy, a common vkCmdPipelineBarrier2UnifiedGITS_SD() is called.
  vkCmdPipelineBarrier2UnifiedGITS_SD(commandBuffer, pDependencyInfo);
}

inline void vkCmdSetEvent2KHR_SD(VkCommandBuffer commandBuffer,
                                 VkEvent event,
                                 const VkDependencyInfo* pDependencyInfo) {
  SD()._commandbufferstates[commandBuffer]->eventStatesAfterSubmit[event] = true;

  // Not counting eventStatesAfterSubmit, a state tracking for the vkCmdPipelineBarrier2...()
  // functions is exactly the same as for the vkCmdSetEvent2KHR() function. That's why, to
  // avoid code redundancy, a common vkCmdPipelineBarrier2UnifiedGITS_SD() is called.
  vkCmdPipelineBarrier2UnifiedGITS_SD(commandBuffer, pDependencyInfo);
}

inline void vkCmdResetQueryPool_SD(VkCommandBuffer commandBuffer,
                                   VkQueryPool queryPool,
                                   uint32_t firstQuery,
                                   uint32_t queryCount) {
  if (Config::Get().IsRecorder()) {
    auto& resetQueryAfterSubmit =
        SD()._commandbufferstates[commandBuffer]->resetQueriesAfterSubmit[queryPool];
    auto& usedQueryAfterSubmit =
        SD()._commandbufferstates[commandBuffer]->usedQueriesAfterSubmit[queryPool];
    for (uint32_t i = firstQuery; i < queryCount; ++i) {
      resetQueryAfterSubmit.insert(i);
      usedQueryAfterSubmit.erase(i);
    }
  }
}

inline void vkCmdWriteTimestamp_SD(VkCommandBuffer commandBuffer,
                                   VkPipelineStageFlagBits pipelineStage,
                                   VkQueryPool queryPool,
                                   uint32_t query) {
  if (Config::Get().IsRecorder()) {
    SD()._commandbufferstates[commandBuffer]->usedQueriesAfterSubmit[queryPool].insert(query);
  }
}

inline void vkCmdBeginQuery_SD(VkCommandBuffer commandBuffer,
                               VkQueryPool queryPool,
                               uint32_t query,
                               VkQueryControlFlags flags) {
  if (Config::Get().IsRecorder()) {
    SD()._commandbufferstates[commandBuffer]->usedQueriesAfterSubmit[queryPool].insert(query);
  }
}

inline void vkCmdCopyQueryPoolResults_SD(VkCommandBuffer commandBuffer,
                                         VkQueryPool queryPool,
                                         uint32_t firstQuery,
                                         uint32_t queryCount,
                                         VkBuffer dstBuffer,
                                         VkDeviceSize dstOffset,
                                         VkDeviceSize stride,
                                         VkQueryResultFlags flags) {
  if (Config::Get().IsRecorder()) {
    if ((TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED ==
         Config::Get().recorder.vulkan.utilities.memoryUpdateState) ||
        isSubcaptureBeforeRestorationPhase()) {
      SD().bindingBuffers[commandBuffer].insert(dstBuffer);
    }

    SD()._commandbufferstates[commandBuffer]->touchedResources.emplace_back((uint64_t)dstBuffer,
                                                                            false);

    if (Config::Get().recorder.vulkan.utilities.memorySegmentSize ||
        Config::Get().recorder.vulkan.utilities.shadowMemory) {
      auto& bufferState = SD()._bufferstates[dstBuffer];

      if (bufferState->binding) {
        VkDeviceSize dstOffsetFinal = bufferState->binding->memoryOffset + dstOffset;
        VkDeviceSize dstSize = bufferState->bufferCreateInfoData.Value()->size - dstOffset;
        VkDeviceMemory dstDeviceMemory =
            bufferState->binding->deviceMemoryStateStore->deviceMemoryHandle;
        SD().updatedMemoryInCmdBuffer[commandBuffer].AddToMap(dstDeviceMemory, dstOffsetFinal,
                                                              dstSize);
      }
    }
  }
}

inline void vkCmdUpdateBuffer_SD(VkCommandBuffer commandBuffer,
                                 VkBuffer dstBuffer,
                                 VkDeviceSize dstOffset,
                                 VkDeviceSize dataSize,
                                 const void* pData) {
  if (Config::Get().IsRecorder()) {
    if ((TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED ==
         Config::Get().recorder.vulkan.utilities.memoryUpdateState) ||
        isSubcaptureBeforeRestorationPhase()) {
      if (VK_NULL_HANDLE != dstBuffer) {
        SD().bindingBuffers[commandBuffer].insert(dstBuffer);
      }
    }

    SD()._commandbufferstates[commandBuffer]->touchedResources.emplace_back((uint64_t)dstBuffer,
                                                                            false);

    if (Config::Get().recorder.vulkan.utilities.memorySegmentSize ||
        Config::Get().recorder.vulkan.utilities.shadowMemory) {
      auto& bufferState = SD()._bufferstates[dstBuffer];

      if (bufferState->binding) {
        VkDeviceSize dstOffsetFinal = bufferState->binding->memoryOffset + dstOffset;
        VkDeviceSize dstSize = dataSize;
        VkDeviceMemory dstDeviceMemory =
            bufferState->binding->deviceMemoryStateStore->deviceMemoryHandle;
        SD().updatedMemoryInCmdBuffer[commandBuffer].AddToMap(dstDeviceMemory, dstOffsetFinal,
                                                              dstSize);
      }
    }
  }
}

inline void vkCmdFillBuffer_SD(VkCommandBuffer commandBuffer,
                               VkBuffer dstBuffer,
                               VkDeviceSize dstOffset,
                               VkDeviceSize size,
                               uint32_t data) {
  if (Config::Get().IsRecorder()) {
    if ((TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED ==
         Config::Get().recorder.vulkan.utilities.memoryUpdateState) ||
        isSubcaptureBeforeRestorationPhase()) {
      if (VK_NULL_HANDLE != dstBuffer) {
        SD().bindingBuffers[commandBuffer].insert(dstBuffer);
      }
    }

    SD()._commandbufferstates[commandBuffer]->touchedResources.emplace_back((uint64_t)dstBuffer,
                                                                            false);

    if (Config::Get().recorder.vulkan.utilities.memorySegmentSize ||
        Config::Get().recorder.vulkan.utilities.shadowMemory) {
      auto& bufferState = SD()._bufferstates[dstBuffer];

      if (bufferState->binding) {
        VkDeviceSize dstOffsetFinal = bufferState->binding->memoryOffset + dstOffset;
        VkDeviceSize dstSize = size;
        VkDeviceMemory dstDeviceMemory =
            bufferState->binding->deviceMemoryStateStore->deviceMemoryHandle;
        SD().updatedMemoryInCmdBuffer[commandBuffer].AddToMap(dstDeviceMemory, dstOffsetFinal,
                                                              dstSize);
      }
    }
  }
}

inline void vkCmdCopyBuffer_SD(VkCommandBuffer commandBuffer,
                               VkBuffer srcBuffer,
                               VkBuffer dstBuffer,
                               uint32_t regionCount,
                               const VkBufferCopy* pRegions) {
  if (Config::Get().IsPlayer() && !Config::Get().player.captureVulkanSubmitsResources.empty() &&
      (dstBuffer != NULL)) {
    SD()._commandbufferstates[commandBuffer]->resourceWriteBuffers[dstBuffer] =
        VULKAN_BLIT_DESTINATION_BUFFER;
  }
  if (Config::Get().IsRecorder()) {
    if ((TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED ==
         Config::Get().recorder.vulkan.utilities.memoryUpdateState) ||
        isSubcaptureBeforeRestorationPhase()) {
      if (srcBuffer != NULL) {
        SD().bindingBuffers[commandBuffer].insert(srcBuffer);
      }
      if (dstBuffer != NULL) {
        SD().bindingBuffers[commandBuffer].insert(dstBuffer);
      }
    }

    SD()._commandbufferstates[commandBuffer]->touchedResources.emplace_back((uint64_t)dstBuffer,
                                                                            false);

    if (Config::Get().recorder.vulkan.utilities.memorySegmentSize ||
        Config::Get().recorder.vulkan.utilities.shadowMemory) {
      auto& bufferState = SD()._bufferstates[dstBuffer];
      if (bufferState->binding) {
        for (uint32_t i = 0; i < regionCount; i++) {
          VkDeviceSize dstOffsetFinal = bufferState->binding->memoryOffset + pRegions[i].dstOffset;
          VkDeviceSize dstSize = pRegions[i].size;
          VkDeviceMemory dstDeviceMemory =
              bufferState->binding->deviceMemoryStateStore->deviceMemoryHandle;
          SD().updatedMemoryInCmdBuffer[commandBuffer].AddToMap(dstDeviceMemory, dstOffsetFinal,
                                                                dstSize);
        }
      }
    }
  }
}

inline void vkCmdCopyBuffer2_SD(VkCommandBuffer commandBuffer,
                                const VkCopyBufferInfo2* pCopyBufferInfo) {
  if (Config::Get().IsPlayer() && !Config::Get().player.captureVulkanSubmitsResources.empty() &&
      (pCopyBufferInfo->dstBuffer != NULL)) {
    SD()._commandbufferstates[commandBuffer]->resourceWriteBuffers[pCopyBufferInfo->dstBuffer] =
        VULKAN_BLIT_DESTINATION_BUFFER;
  }
  if (Config::Get().IsRecorder()) {
    if ((TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED ==
         Config::Get().recorder.vulkan.utilities.memoryUpdateState) ||
        isSubcaptureBeforeRestorationPhase()) {
      if (pCopyBufferInfo->srcBuffer != NULL) {
        SD().bindingBuffers[commandBuffer].insert(pCopyBufferInfo->srcBuffer);
      }
      if (pCopyBufferInfo->dstBuffer != NULL) {
        SD().bindingBuffers[commandBuffer].insert(pCopyBufferInfo->dstBuffer);
      }
    }

    SD()._commandbufferstates[commandBuffer]->touchedResources.emplace_back(
        (uint64_t)pCopyBufferInfo->dstBuffer, false);

    if (Config::Get().recorder.vulkan.utilities.memorySegmentSize ||
        Config::Get().recorder.vulkan.utilities.shadowMemory) {
      auto& bufferState = SD()._bufferstates[pCopyBufferInfo->dstBuffer];
      if (bufferState->binding) {
        for (uint32_t i = 0; i < pCopyBufferInfo->regionCount; i++) {
          VkDeviceSize dstOffsetFinal =
              bufferState->binding->memoryOffset + pCopyBufferInfo->pRegions[i].dstOffset;
          VkDeviceSize dstSize = pCopyBufferInfo->pRegions[i].size;
          VkDeviceMemory dstDeviceMemory =
              bufferState->binding->deviceMemoryStateStore->deviceMemoryHandle;
          SD().updatedMemoryInCmdBuffer[commandBuffer].AddToMap(dstDeviceMemory, dstOffsetFinal,
                                                                dstSize);
        }
      }
    }
  }
}

inline void vkCmdCopyBufferToImage_SD(VkCommandBuffer commandBuffer,
                                      VkBuffer srcBuffer,
                                      VkImage dstImage,
                                      VkImageLayout dstImageLayout,
                                      uint32_t regionCount,
                                      const VkBufferImageCopy* pRegions) {
  if (Config::Get().IsPlayer() && !Config::Get().player.captureVulkanSubmitsResources.empty() &&
      (dstImage != NULL)) {
    SD()._commandbufferstates[commandBuffer]->resourceWriteImages[dstImage] =
        VULKAN_BLIT_DESTINATION_IMAGE;
  }
  if (Config::Get().IsRecorder()) {
    if ((TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED ==
         Config::Get().recorder.vulkan.utilities.memoryUpdateState) ||
        isSubcaptureBeforeRestorationPhase()) {
      if (srcBuffer != NULL) {
        SD().bindingBuffers[commandBuffer].insert(srcBuffer);
      }
      if (dstImage != NULL) {
        SD().bindingImages[commandBuffer].insert(dstImage);
      }
    }

    SD()._commandbufferstates[commandBuffer]->touchedResources.emplace_back((uint64_t)dstImage,
                                                                            true);

    if (Config::Get().recorder.vulkan.utilities.memorySegmentSize ||
        Config::Get().recorder.vulkan.utilities.shadowMemory) {
      auto& imageState = SD()._imagestates[dstImage];

      if (imageState->binding) {
        if (imageState->imageCreateInfoData.Value() &&
            (imageState->imageCreateInfoData.Value()->tiling == VK_IMAGE_TILING_LINEAR)) {
          for (uint32_t i = 0; i < regionCount; i++) {
            VkImageSubresource imageSubresource;

            imageSubresource.aspectMask = pRegions[i].imageSubresource.aspectMask;
            imageSubresource.mipLevel = pRegions[i].imageSubresource.mipLevel;
            imageSubresource.arrayLayer = pRegions[i].imageSubresource.baseArrayLayer;

            VkSubresourceLayout subLayout;
            drvVk.vkGetImageSubresourceLayout(imageState->deviceStateStore->deviceHandle, dstImage,
                                              &imageSubresource, &subLayout);
            VkDeviceSize dstOffsetFinal = imageState->binding->memoryOffset + subLayout.offset;
            VkDeviceSize dstSize = subLayout.size;
            VkDeviceMemory dstDeviceMemory =
                imageState->binding->deviceMemoryStateStore->deviceMemoryHandle;
            SD().updatedMemoryInCmdBuffer[commandBuffer].AddToMap(dstDeviceMemory, dstOffsetFinal,
                                                                  dstSize);
          }
        } else {
          VkMemoryRequirements memRequirements = {};
          drvVk.vkGetImageMemoryRequirements(imageState->deviceStateStore->deviceHandle, dstImage,
                                             &memRequirements);

          VkDeviceSize dstOffsetFinal = imageState->binding->memoryOffset;
          VkDeviceSize dstSize = memRequirements.size;
          VkDeviceMemory dstDeviceMemory =
              imageState->binding->deviceMemoryStateStore->deviceMemoryHandle;
          SD().updatedMemoryInCmdBuffer[commandBuffer].AddToMap(dstDeviceMemory, dstOffsetFinal,
                                                                dstSize);
        }
      }
    }
  }
}

inline void vkCmdCopyBufferToImage2_SD(VkCommandBuffer commandBuffer,
                                       const VkCopyBufferToImageInfo2* pCopyBufferToImageInfo) {
  if (Config::Get().IsPlayer() && !Config::Get().player.captureVulkanSubmitsResources.empty() &&
      (pCopyBufferToImageInfo->dstImage != NULL)) {
    SD()._commandbufferstates[commandBuffer]
        ->resourceWriteImages[pCopyBufferToImageInfo->dstImage] = VULKAN_BLIT_DESTINATION_IMAGE;
  }
  if (Config::Get().IsRecorder()) {
    if ((TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED ==
         Config::Get().recorder.vulkan.utilities.memoryUpdateState) ||
        isSubcaptureBeforeRestorationPhase()) {
      if (pCopyBufferToImageInfo->srcBuffer != NULL) {
        SD().bindingBuffers[commandBuffer].insert(pCopyBufferToImageInfo->srcBuffer);
      }
      if (pCopyBufferToImageInfo->dstImage != NULL) {
        SD().bindingImages[commandBuffer].insert(pCopyBufferToImageInfo->dstImage);
      }
    }

    SD()._commandbufferstates[commandBuffer]->touchedResources.emplace_back(
        (uint64_t)pCopyBufferToImageInfo->dstImage, true);

    if (Config::Get().recorder.vulkan.utilities.memorySegmentSize ||
        Config::Get().recorder.vulkan.utilities.shadowMemory) {
      auto& imageState = SD()._imagestates[pCopyBufferToImageInfo->dstImage];

      if (imageState->binding) {
        if (imageState->imageCreateInfoData.Value() &&
            (imageState->imageCreateInfoData.Value()->tiling == VK_IMAGE_TILING_LINEAR)) {
          for (uint32_t i = 0; i < pCopyBufferToImageInfo->regionCount; i++) {
            VkImageSubresource imageSubresource;

            imageSubresource.aspectMask =
                pCopyBufferToImageInfo->pRegions[i].imageSubresource.aspectMask;
            imageSubresource.mipLevel =
                pCopyBufferToImageInfo->pRegions[i].imageSubresource.mipLevel;
            imageSubresource.arrayLayer =
                pCopyBufferToImageInfo->pRegions[i].imageSubresource.baseArrayLayer;

            VkSubresourceLayout subLayout;
            drvVk.vkGetImageSubresourceLayout(imageState->deviceStateStore->deviceHandle,
                                              pCopyBufferToImageInfo->dstImage, &imageSubresource,
                                              &subLayout);
            VkDeviceSize dstOffsetFinal = imageState->binding->memoryOffset + subLayout.offset;
            VkDeviceSize dstSize = subLayout.size;
            VkDeviceMemory dstDeviceMemory =
                imageState->binding->deviceMemoryStateStore->deviceMemoryHandle;
            SD().updatedMemoryInCmdBuffer[commandBuffer].AddToMap(dstDeviceMemory, dstOffsetFinal,
                                                                  dstSize);
          }
        } else {
          VkMemoryRequirements memRequirements = {};
          drvVk.vkGetImageMemoryRequirements(imageState->deviceStateStore->deviceHandle,
                                             pCopyBufferToImageInfo->dstImage, &memRequirements);

          VkDeviceSize dstOffsetFinal = imageState->binding->memoryOffset;
          VkDeviceSize dstSize = memRequirements.size;
          VkDeviceMemory dstDeviceMemory =
              imageState->binding->deviceMemoryStateStore->deviceMemoryHandle;
          SD().updatedMemoryInCmdBuffer[commandBuffer].AddToMap(dstDeviceMemory, dstOffsetFinal,
                                                                dstSize);
        }
      }
    }
  }
}

inline void vkCmdCopyImage_SD(VkCommandBuffer commandBuffer,
                              VkImage srcImage,
                              VkImageLayout srcImageLayout,
                              VkImage dstImage,
                              VkImageLayout dstImageLayout,
                              uint32_t regionCount,
                              const VkImageCopy* pRegions) {
  if (Config::Get().IsPlayer() && !Config::Get().player.captureVulkanSubmitsResources.empty() &&
      (dstImage != NULL)) {
    SD()._commandbufferstates[commandBuffer]->resourceWriteImages[dstImage] =
        VULKAN_BLIT_DESTINATION_IMAGE;
  }
  if (Config::Get().IsRecorder()) {
    if ((TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED ==
         Config::Get().recorder.vulkan.utilities.memoryUpdateState) ||
        isSubcaptureBeforeRestorationPhase()) {
      if (srcImage != NULL) {
        SD().bindingImages[commandBuffer].insert(srcImage);
      }
      if (dstImage != NULL) {
        SD().bindingImages[commandBuffer].insert(dstImage);
      }
    }

    SD()._commandbufferstates[commandBuffer]->touchedResources.emplace_back((uint64_t)dstImage,
                                                                            true);

    if (Config::Get().recorder.vulkan.utilities.memorySegmentSize ||
        Config::Get().recorder.vulkan.utilities.shadowMemory) {
      auto& imageState = SD()._imagestates[dstImage];
      if (imageState->binding) {
        if (imageState->imageCreateInfoData.Value() &&
            (imageState->imageCreateInfoData.Value()->tiling == VK_IMAGE_TILING_LINEAR)) {
          for (uint32_t i = 0; i < regionCount; i++) {
            VkImageSubresource imageSubresource;

            imageSubresource.aspectMask = pRegions[i].dstSubresource.aspectMask;
            imageSubresource.mipLevel = pRegions[i].dstSubresource.mipLevel;
            imageSubresource.arrayLayer = pRegions[i].dstSubresource.baseArrayLayer;

            VkSubresourceLayout subLayout;
            drvVk.vkGetImageSubresourceLayout(imageState->deviceStateStore->deviceHandle, dstImage,
                                              &imageSubresource, &subLayout);
            VkDeviceSize dstOffsetFinal = imageState->binding->memoryOffset + subLayout.offset;
            VkDeviceSize dstSize = subLayout.size;
            VkDeviceMemory dstDeviceMemory =
                imageState->binding->deviceMemoryStateStore->deviceMemoryHandle;
            SD().updatedMemoryInCmdBuffer[commandBuffer].AddToMap(dstDeviceMemory, dstOffsetFinal,
                                                                  dstSize);
          }
        } else {
          VkMemoryRequirements memRequirements = {};
          drvVk.vkGetImageMemoryRequirements(imageState->deviceStateStore->deviceHandle, dstImage,
                                             &memRequirements);

          VkDeviceSize dstOffsetFinal = imageState->binding->memoryOffset;
          VkDeviceSize dstSize = memRequirements.size;
          VkDeviceMemory dstDeviceMemory =
              imageState->binding->deviceMemoryStateStore->deviceMemoryHandle;
          SD().updatedMemoryInCmdBuffer[commandBuffer].AddToMap(dstDeviceMemory, dstOffsetFinal,
                                                                dstSize);
        }
      }
    }
  }
}

inline void vkCmdCopyImage2_SD(VkCommandBuffer commandBuffer,
                               const VkCopyImageInfo2* pCopyImageInfo) {
  if (Config::Get().IsPlayer() && !Config::Get().player.captureVulkanSubmitsResources.empty() &&
      (pCopyImageInfo->dstImage != NULL)) {
    SD()._commandbufferstates[commandBuffer]->resourceWriteImages[pCopyImageInfo->dstImage] =
        VULKAN_BLIT_DESTINATION_IMAGE;
  }
  if (Config::Get().IsRecorder()) {
    if ((TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED ==
         Config::Get().recorder.vulkan.utilities.memoryUpdateState) ||
        isSubcaptureBeforeRestorationPhase()) {
      if (pCopyImageInfo->srcImage != NULL) {
        SD().bindingImages[commandBuffer].insert(pCopyImageInfo->srcImage);
      }
      if (pCopyImageInfo->dstImage != NULL) {
        SD().bindingImages[commandBuffer].insert(pCopyImageInfo->dstImage);
      }
    }

    SD()._commandbufferstates[commandBuffer]->touchedResources.emplace_back(
        (uint64_t)pCopyImageInfo->dstImage, true);

    if (Config::Get().recorder.vulkan.utilities.memorySegmentSize ||
        Config::Get().recorder.vulkan.utilities.shadowMemory) {
      auto& imageState = SD()._imagestates[pCopyImageInfo->dstImage];
      if (imageState->binding) {
        if (imageState->imageCreateInfoData.Value() &&
            (imageState->imageCreateInfoData.Value()->tiling == VK_IMAGE_TILING_LINEAR)) {
          for (uint32_t i = 0; i < pCopyImageInfo->regionCount; i++) {
            VkImageSubresource imageSubresource;

            imageSubresource.aspectMask = pCopyImageInfo->pRegions[i].dstSubresource.aspectMask;
            imageSubresource.mipLevel = pCopyImageInfo->pRegions[i].dstSubresource.mipLevel;
            imageSubresource.arrayLayer = pCopyImageInfo->pRegions[i].dstSubresource.baseArrayLayer;

            VkSubresourceLayout subLayout;
            drvVk.vkGetImageSubresourceLayout(imageState->deviceStateStore->deviceHandle,
                                              pCopyImageInfo->dstImage, &imageSubresource,
                                              &subLayout);
            VkDeviceSize dstOffsetFinal = imageState->binding->memoryOffset + subLayout.offset;
            VkDeviceSize dstSize = subLayout.size;
            VkDeviceMemory dstDeviceMemory =
                imageState->binding->deviceMemoryStateStore->deviceMemoryHandle;
            SD().updatedMemoryInCmdBuffer[commandBuffer].AddToMap(dstDeviceMemory, dstOffsetFinal,
                                                                  dstSize);
          }
        } else {
          VkMemoryRequirements memRequirements = {};
          drvVk.vkGetImageMemoryRequirements(imageState->deviceStateStore->deviceHandle,
                                             pCopyImageInfo->dstImage, &memRequirements);

          VkDeviceSize dstOffsetFinal = imageState->binding->memoryOffset;
          VkDeviceSize dstSize = memRequirements.size;
          VkDeviceMemory dstDeviceMemory =
              imageState->binding->deviceMemoryStateStore->deviceMemoryHandle;
          SD().updatedMemoryInCmdBuffer[commandBuffer].AddToMap(dstDeviceMemory, dstOffsetFinal,
                                                                dstSize);
        }
      }
    }
  }
}

inline void vkCmdCopyImage2KHR_SD(VkCommandBuffer commandBuffer,
                                  const VkCopyImageInfo2* pCopyImageInfo) {
  vkCmdCopyImage2_SD(commandBuffer, pCopyImageInfo);
}

inline void vkCmdCopyImageToBuffer_SD(VkCommandBuffer commandBuffer,
                                      VkImage srcImage,
                                      VkImageLayout srcImageLayout,
                                      VkBuffer dstBuffer,
                                      uint32_t regionCount,
                                      const VkBufferImageCopy* pRegions) {
  if (Config::Get().IsPlayer() && !Config::Get().player.captureVulkanSubmitsResources.empty() &&
      (dstBuffer != NULL)) {
    SD()._commandbufferstates[commandBuffer]->resourceWriteBuffers[dstBuffer] =
        VULKAN_BLIT_DESTINATION_BUFFER;
  }
  if (Config::Get().IsRecorder()) {
    if ((TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED ==
         Config::Get().recorder.vulkan.utilities.memoryUpdateState) ||
        isSubcaptureBeforeRestorationPhase()) {
      if (srcImage != NULL) {
        SD().bindingImages[commandBuffer].insert(srcImage);
      }
      if (dstBuffer != NULL) {
        SD().bindingBuffers[commandBuffer].insert(dstBuffer);
      }
    }

    SD()._commandbufferstates[commandBuffer]->touchedResources.emplace_back((uint64_t)dstBuffer,
                                                                            false);

    if (Config::Get().recorder.vulkan.utilities.memorySegmentSize ||
        Config::Get().recorder.vulkan.utilities.shadowMemory) {
      auto& bufferState = SD()._bufferstates[dstBuffer];
      auto& imageState = SD()._imagestates[srcImage];
      if (bufferState->binding) {
        if (imageState->imageCreateInfoData.Value() &&
            (imageState->imageCreateInfoData.Value()->tiling == VK_IMAGE_TILING_LINEAR)) {
          for (uint32_t i = 0; i < regionCount; i++) {
            VkImageSubresource imageSubresource;

            imageSubresource.aspectMask = pRegions[i].imageSubresource.aspectMask;
            imageSubresource.mipLevel = pRegions[i].imageSubresource.mipLevel;
            imageSubresource.arrayLayer = pRegions[i].imageSubresource.baseArrayLayer;

            VkSubresourceLayout subLayout;
            drvVk.vkGetImageSubresourceLayout(bufferState->deviceStateStore->deviceHandle, srcImage,
                                              &imageSubresource, &subLayout);
            VkDeviceSize dstOffsetFinal =
                bufferState->binding->memoryOffset + pRegions[i].bufferOffset;
            VkDeviceSize dstSize = subLayout.size;
            VkDeviceMemory dstDeviceMemory =
                bufferState->binding->deviceMemoryStateStore->deviceMemoryHandle;
            SD().updatedMemoryInCmdBuffer[commandBuffer].AddToMap(dstDeviceMemory, dstOffsetFinal,
                                                                  dstSize);
          }
        } else {
          VkDeviceSize dstOffsetFinal = bufferState->binding->memoryOffset;
          VkDeviceSize dstSize = bufferState->bufferCreateInfoData.Value()->size;
          VkDeviceMemory dstDeviceMemory =
              bufferState->binding->deviceMemoryStateStore->deviceMemoryHandle;
          SD().updatedMemoryInCmdBuffer[commandBuffer].AddToMap(dstDeviceMemory, dstOffsetFinal,
                                                                dstSize);
        }
      }
    }
  }
}

inline void vkCmdCopyImageToBuffer2_SD(VkCommandBuffer commandBuffer,
                                       const VkCopyImageToBufferInfo2* pCopyImageToBufferInfo) {
  if (Config::Get().IsPlayer() && !Config::Get().player.captureVulkanSubmitsResources.empty() &&
      (pCopyImageToBufferInfo->dstBuffer != VK_NULL_HANDLE)) {
    SD()._commandbufferstates[commandBuffer]
        ->resourceWriteBuffers[pCopyImageToBufferInfo->dstBuffer] = VULKAN_BLIT_DESTINATION_BUFFER;
  }
  if (Config::Get().IsRecorder()) {
    if ((TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED ==
         Config::Get().recorder.vulkan.utilities.memoryUpdateState) ||
        isSubcaptureBeforeRestorationPhase()) {
      if (pCopyImageToBufferInfo->srcImage != NULL) {
        SD().bindingImages[commandBuffer].insert(pCopyImageToBufferInfo->srcImage);
      }
      if (pCopyImageToBufferInfo->dstBuffer != NULL) {
        SD().bindingBuffers[commandBuffer].insert(pCopyImageToBufferInfo->dstBuffer);
      }
    }

    SD()._commandbufferstates[commandBuffer]->touchedResources.emplace_back(
        (uint64_t)pCopyImageToBufferInfo->dstBuffer, false);

    if (Config::Get().recorder.vulkan.utilities.memorySegmentSize ||
        Config::Get().recorder.vulkan.utilities.shadowMemory) {
      auto& bufferState = SD()._bufferstates[pCopyImageToBufferInfo->dstBuffer];
      auto& imageState = SD()._imagestates[pCopyImageToBufferInfo->srcImage];
      if (bufferState->binding) {
        if (imageState->imageCreateInfoData.Value() &&
            (imageState->imageCreateInfoData.Value()->tiling == VK_IMAGE_TILING_LINEAR)) {
          for (uint32_t i = 0; i < pCopyImageToBufferInfo->regionCount; i++) {
            VkImageSubresource imageSubresource;

            imageSubresource.aspectMask =
                pCopyImageToBufferInfo->pRegions[i].imageSubresource.aspectMask;
            imageSubresource.mipLevel =
                pCopyImageToBufferInfo->pRegions[i].imageSubresource.mipLevel;
            imageSubresource.arrayLayer =
                pCopyImageToBufferInfo->pRegions[i].imageSubresource.baseArrayLayer;

            VkSubresourceLayout subLayout;
            drvVk.vkGetImageSubresourceLayout(bufferState->deviceStateStore->deviceHandle,
                                              pCopyImageToBufferInfo->srcImage, &imageSubresource,
                                              &subLayout);
            VkDeviceSize dstOffsetFinal = bufferState->binding->memoryOffset +
                                          pCopyImageToBufferInfo->pRegions[i].bufferOffset;
            VkDeviceSize dstSize = subLayout.size;
            VkDeviceMemory dstDeviceMemory =
                bufferState->binding->deviceMemoryStateStore->deviceMemoryHandle;
            SD().updatedMemoryInCmdBuffer[commandBuffer].AddToMap(dstDeviceMemory, dstOffsetFinal,
                                                                  dstSize);
          }
        } else {
          VkDeviceSize dstOffsetFinal = bufferState->binding->memoryOffset;
          VkDeviceSize dstSize = bufferState->bufferCreateInfoData.Value()->size;
          VkDeviceMemory dstDeviceMemory =
              bufferState->binding->deviceMemoryStateStore->deviceMemoryHandle;
          SD().updatedMemoryInCmdBuffer[commandBuffer].AddToMap(dstDeviceMemory, dstOffsetFinal,
                                                                dstSize);
        }
      }
    }
  }
}

inline void vkCmdCopyImageToBuffer2KHR_SD(VkCommandBuffer commandBuffer,
                                          const VkCopyImageToBufferInfo2* pCopyImageToBufferInfo) {
  vkCmdCopyImageToBuffer2_SD(commandBuffer, pCopyImageToBufferInfo);
}

inline void vkCmdBlitImage_SD(VkCommandBuffer commandBuffer,
                              VkImage srcImage,
                              VkImageLayout srcImageLayout,
                              VkImage dstImage,
                              VkImageLayout dstImageLayout,
                              uint32_t regionCount,
                              const VkImageBlit* pRegions,
                              VkFilter filter) {
  if (Config::Get().IsRecorder()) {
    if ((TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED ==
         Config::Get().recorder.vulkan.utilities.memoryUpdateState) ||
        isSubcaptureBeforeRestorationPhase()) {
      if (VK_NULL_HANDLE != srcImage) {
        SD().bindingImages[commandBuffer].insert(srcImage);
      }
      if (VK_NULL_HANDLE != dstImage) {
        SD().bindingImages[commandBuffer].insert(dstImage);
      }
    }

    SD()._commandbufferstates[commandBuffer]->touchedResources.emplace_back((uint64_t)dstImage,
                                                                            true);

    if (Config::Get().recorder.vulkan.utilities.memorySegmentSize ||
        Config::Get().recorder.vulkan.utilities.shadowMemory) {
      auto& imageState = SD()._imagestates[dstImage];
      if (imageState->binding) {
        if (imageState->imageCreateInfoData.Value()->tiling == VK_IMAGE_TILING_LINEAR) {
          for (uint32_t i = 0; i < regionCount; i++) {
            VkImageSubresource imageSubresource;

            imageSubresource.aspectMask = pRegions[i].dstSubresource.aspectMask;
            imageSubresource.mipLevel = pRegions[i].dstSubresource.mipLevel;
            imageSubresource.arrayLayer = pRegions[i].dstSubresource.baseArrayLayer;

            VkSubresourceLayout subLayout;
            drvVk.vkGetImageSubresourceLayout(imageState->deviceStateStore->deviceHandle, dstImage,
                                              &imageSubresource, &subLayout);
            VkDeviceSize dstOffsetFinal = imageState->binding->memoryOffset + subLayout.offset;
            VkDeviceSize dstSize = subLayout.size;
            VkDeviceMemory dstDeviceMemory =
                imageState->binding->deviceMemoryStateStore->deviceMemoryHandle;
            SD().updatedMemoryInCmdBuffer[commandBuffer].AddToMap(dstDeviceMemory, dstOffsetFinal,
                                                                  dstSize);
          }
        } else {
          VkMemoryRequirements memRequirements = {};
          drvVk.vkGetImageMemoryRequirements(imageState->deviceStateStore->deviceHandle, dstImage,
                                             &memRequirements);

          VkDeviceSize dstOffsetFinal = imageState->binding->memoryOffset;
          VkDeviceSize dstSize = memRequirements.size;
          VkDeviceMemory dstDeviceMemory =
              imageState->binding->deviceMemoryStateStore->deviceMemoryHandle;
          SD().updatedMemoryInCmdBuffer[commandBuffer].AddToMap(dstDeviceMemory, dstOffsetFinal,
                                                                dstSize);
        }
      }
    }
  }
}

inline void vkCmdBlitImage2_SD(VkCommandBuffer commandBuffer,
                               const VkBlitImageInfo2* pBlitInfoImage) {
  if (Config::Get().IsRecorder()) {
    if ((TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED ==
         Config::Get().recorder.vulkan.utilities.memoryUpdateState) ||
        isSubcaptureBeforeRestorationPhase()) {
      if (VK_NULL_HANDLE != pBlitInfoImage->srcImage) {
        SD().bindingImages[commandBuffer].insert(pBlitInfoImage->srcImage);
      }
      if (VK_NULL_HANDLE != pBlitInfoImage->dstImage) {
        SD().bindingImages[commandBuffer].insert(pBlitInfoImage->dstImage);
      }
    }

    SD()._commandbufferstates[commandBuffer]->touchedResources.emplace_back(
        (uint64_t)pBlitInfoImage->dstImage, true);

    if (Config::Get().recorder.vulkan.utilities.memorySegmentSize ||
        Config::Get().recorder.vulkan.utilities.shadowMemory) {
      auto& imageState = SD()._imagestates[pBlitInfoImage->dstImage];
      if (imageState->binding) {
        if (imageState->imageCreateInfoData.Value()->tiling == VK_IMAGE_TILING_LINEAR) {
          for (uint32_t i = 0; i < pBlitInfoImage->regionCount; i++) {
            VkImageSubresource imageSubresource;

            imageSubresource.aspectMask = pBlitInfoImage->pRegions[i].dstSubresource.aspectMask;
            imageSubresource.mipLevel = pBlitInfoImage->pRegions[i].dstSubresource.mipLevel;
            imageSubresource.arrayLayer = pBlitInfoImage->pRegions[i].dstSubresource.baseArrayLayer;

            VkSubresourceLayout subLayout;
            drvVk.vkGetImageSubresourceLayout(imageState->deviceStateStore->deviceHandle,
                                              pBlitInfoImage->dstImage, &imageSubresource,
                                              &subLayout);
            VkDeviceSize dstOffsetFinal = imageState->binding->memoryOffset + subLayout.offset;
            VkDeviceSize dstSize = subLayout.size;
            VkDeviceMemory dstDeviceMemory =
                imageState->binding->deviceMemoryStateStore->deviceMemoryHandle;
            SD().updatedMemoryInCmdBuffer[commandBuffer].AddToMap(dstDeviceMemory, dstOffsetFinal,
                                                                  dstSize);
          }
        } else {
          VkMemoryRequirements memRequirements = {};
          drvVk.vkGetImageMemoryRequirements(imageState->deviceStateStore->deviceHandle,
                                             pBlitInfoImage->dstImage, &memRequirements);

          VkDeviceSize dstOffsetFinal = imageState->binding->memoryOffset;
          VkDeviceSize dstSize = memRequirements.size;
          VkDeviceMemory dstDeviceMemory =
              imageState->binding->deviceMemoryStateStore->deviceMemoryHandle;
          SD().updatedMemoryInCmdBuffer[commandBuffer].AddToMap(dstDeviceMemory, dstOffsetFinal,
                                                                dstSize);
        }
      }
    }
  }
}

inline void vkCmdResolveImage_SD(VkCommandBuffer commandBuffer,
                                 VkImage srcImage,
                                 VkImageLayout srcImageLayout,
                                 VkImage dstImage,
                                 VkImageLayout dstImageLayout,
                                 uint32_t regionCount,
                                 const VkImageResolve* pRegions) {
  if (Config::Get().IsRecorder()) {
    if ((TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED ==
         Config::Get().recorder.vulkan.utilities.memoryUpdateState) ||
        isSubcaptureBeforeRestorationPhase()) {
      if (VK_NULL_HANDLE != srcImage) {
        SD().bindingImages[commandBuffer].insert(srcImage);
      }
      if (VK_NULL_HANDLE != dstImage) {
        SD().bindingImages[commandBuffer].insert(dstImage);
      }
    }

    SD()._commandbufferstates[commandBuffer]->touchedResources.emplace_back((uint64_t)dstImage,
                                                                            true);

    if (Config::Get().recorder.vulkan.utilities.memorySegmentSize ||
        Config::Get().recorder.vulkan.utilities.shadowMemory) {
      auto& imageState = SD()._imagestates[dstImage];

      if (imageState->binding) {
        if (imageState->imageCreateInfoData.Value()->tiling == VK_IMAGE_TILING_LINEAR) {
          for (uint32_t i = 0; i < regionCount; i++) {
            VkImageSubresource imageSubresource;

            imageSubresource.aspectMask = pRegions[i].dstSubresource.aspectMask;
            imageSubresource.mipLevel = pRegions[i].dstSubresource.mipLevel;
            imageSubresource.arrayLayer = pRegions[i].dstSubresource.baseArrayLayer;

            VkSubresourceLayout subLayout;
            drvVk.vkGetImageSubresourceLayout(imageState->deviceStateStore->deviceHandle, dstImage,
                                              &imageSubresource, &subLayout);
            VkDeviceSize dstOffsetFinal = imageState->binding->memoryOffset + subLayout.offset;
            VkDeviceSize dstSize = subLayout.size;
            VkDeviceMemory dstDeviceMemory =
                imageState->binding->deviceMemoryStateStore->deviceMemoryHandle;
            SD().updatedMemoryInCmdBuffer[commandBuffer].AddToMap(dstDeviceMemory, dstOffsetFinal,
                                                                  dstSize);
          }
        } else {
          VkMemoryRequirements memRequirements = {};
          drvVk.vkGetImageMemoryRequirements(imageState->deviceStateStore->deviceHandle, dstImage,
                                             &memRequirements);

          VkDeviceSize dstOffsetFinal = imageState->binding->memoryOffset;
          VkDeviceSize dstSize = memRequirements.size;
          VkDeviceMemory dstDeviceMemory =
              imageState->binding->deviceMemoryStateStore->deviceMemoryHandle;
          SD().updatedMemoryInCmdBuffer[commandBuffer].AddToMap(dstDeviceMemory, dstOffsetFinal,
                                                                dstSize);
        }
      }
    }
  }
}

inline void vkCmdResolveImage2_SD(VkCommandBuffer commandBuffer,
                                  const VkResolveImageInfo2* pResolveImageInfo) {
  if (Config::Get().IsRecorder()) {
    if ((TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED ==
         Config::Get().recorder.vulkan.utilities.memoryUpdateState) ||
        isSubcaptureBeforeRestorationPhase()) {
      if (VK_NULL_HANDLE != pResolveImageInfo->srcImage) {
        SD().bindingImages[commandBuffer].insert(pResolveImageInfo->srcImage);
      }
      if (VK_NULL_HANDLE != pResolveImageInfo->dstImage) {
        SD().bindingImages[commandBuffer].insert(pResolveImageInfo->dstImage);
      }
    }

    SD()._commandbufferstates[commandBuffer]->touchedResources.emplace_back(
        (uint64_t)pResolveImageInfo->dstImage, true);

    if (Config::Get().recorder.vulkan.utilities.memorySegmentSize ||
        Config::Get().recorder.vulkan.utilities.shadowMemory) {
      auto& imageState = SD()._imagestates[pResolveImageInfo->dstImage];

      if (imageState->binding) {
        if (imageState->imageCreateInfoData.Value()->tiling == VK_IMAGE_TILING_LINEAR) {
          for (uint32_t i = 0; i < pResolveImageInfo->regionCount; i++) {
            VkImageSubresource imageSubresource;

            imageSubresource.aspectMask = pResolveImageInfo->pRegions[i].dstSubresource.aspectMask;
            imageSubresource.mipLevel = pResolveImageInfo->pRegions[i].dstSubresource.mipLevel;
            imageSubresource.arrayLayer =
                pResolveImageInfo->pRegions[i].dstSubresource.baseArrayLayer;

            VkSubresourceLayout subLayout;
            drvVk.vkGetImageSubresourceLayout(imageState->deviceStateStore->deviceHandle,
                                              pResolveImageInfo->dstImage, &imageSubresource,
                                              &subLayout);
            VkDeviceSize dstOffsetFinal = imageState->binding->memoryOffset + subLayout.offset;
            VkDeviceSize dstSize = subLayout.size;
            VkDeviceMemory dstDeviceMemory =
                imageState->binding->deviceMemoryStateStore->deviceMemoryHandle;
            SD().updatedMemoryInCmdBuffer[commandBuffer].AddToMap(dstDeviceMemory, dstOffsetFinal,
                                                                  dstSize);
          }
        } else {
          VkMemoryRequirements memRequirements = {};
          drvVk.vkGetImageMemoryRequirements(imageState->deviceStateStore->deviceHandle,
                                             pResolveImageInfo->dstImage, &memRequirements);

          VkDeviceSize dstOffsetFinal = imageState->binding->memoryOffset;
          VkDeviceSize dstSize = memRequirements.size;
          VkDeviceMemory dstDeviceMemory =
              imageState->binding->deviceMemoryStateStore->deviceMemoryHandle;
          SD().updatedMemoryInCmdBuffer[commandBuffer].AddToMap(dstDeviceMemory, dstOffsetFinal,
                                                                dstSize);
        }
      }
    }
  }
}

inline void vkCmdClearColorImage_SD(VkCommandBuffer commandBuffer,
                                    VkImage image,
                                    VkImageLayout imageLayout,
                                    const VkClearColorValue* pColor,
                                    uint32_t rangeCount,
                                    const VkImageSubresourceRange* pRanges) {
  if (Config::Get().IsRecorder()) {
    if ((TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED ==
         Config::Get().recorder.vulkan.utilities.memoryUpdateState) ||
        isSubcaptureBeforeRestorationPhase()) {
      SD().bindingImages[commandBuffer].insert(image);
    }

    SD()._commandbufferstates[commandBuffer]->touchedResources.emplace_back((uint64_t)image, true);

    if (Config::Get().recorder.vulkan.utilities.memorySegmentSize ||
        Config::Get().recorder.vulkan.utilities.shadowMemory) {
      auto& imageState = SD()._imagestates[image];
      if (imageState->binding) {
        VkMemoryRequirements memRequirements = {};
        drvVk.vkGetImageMemoryRequirements(imageState->deviceStateStore->deviceHandle, image,
                                           &memRequirements);
        //TODO : call vkGetImageSubresourceLayout when tiling is Linear

        VkDeviceSize dstOffsetFinal = imageState->binding->memoryOffset;
        VkDeviceSize dstSize = memRequirements.size;
        VkDeviceMemory dstDeviceMemory =
            imageState->binding->deviceMemoryStateStore->deviceMemoryHandle;
        SD().updatedMemoryInCmdBuffer[commandBuffer].AddToMap(dstDeviceMemory, dstOffsetFinal,
                                                              dstSize);
      }
    }
  }
  if (captureRenderPasses()) {
    SD()._commandbufferstates[commandBuffer]->clearedImages.insert(image);
  }
}

inline void vkCmdClearDepthStencilImage_SD(VkCommandBuffer commandBuffer,
                                           VkImage image,
                                           VkImageLayout imageLayout,
                                           const VkClearDepthStencilValue* pDepthStencil,
                                           uint32_t rangeCount,
                                           const VkImageSubresourceRange* pRanges) {
  if (Config::Get().IsRecorder()) {
    if ((TMemoryUpdateStates::MEMORY_STATE_UPDATE_ONLY_USED ==
         Config::Get().recorder.vulkan.utilities.memoryUpdateState) ||
        isSubcaptureBeforeRestorationPhase()) {
      SD().bindingImages[commandBuffer].insert(image);
    }

    SD()._commandbufferstates[commandBuffer]->touchedResources.emplace_back((uint64_t)image, true);

    if (Config::Get().recorder.vulkan.utilities.memorySegmentSize ||
        Config::Get().recorder.vulkan.utilities.shadowMemory) {
      auto& imageState = SD()._imagestates[image];
      if (imageState->binding) {
        VkMemoryRequirements memRequirements = {};
        drvVk.vkGetImageMemoryRequirements(imageState->deviceStateStore->deviceHandle, image,
                                           &memRequirements);
        //TODO : call vkGetImageSubresourceLayout when tiling is Linear

        VkDeviceSize dstOffsetFinal = imageState->binding->memoryOffset;
        VkDeviceSize dstSize = memRequirements.size;
        VkDeviceMemory dstDeviceMemory =
            imageState->binding->deviceMemoryStateStore->deviceMemoryHandle;
        SD().updatedMemoryInCmdBuffer[commandBuffer].AddToMap(dstDeviceMemory, dstOffsetFinal,
                                                              dstSize);
      }
    }
  }
  if (captureRenderPasses()) {
    SD()._commandbufferstates[commandBuffer]->clearedImages.insert(image);
  }
}

inline void vkCmdClearAttachments_SD(VkCommandBuffer commandBuffer,
                                     uint32_t attachmentCount,
                                     const VkClearAttachment* pAttachments,
                                     uint32_t rectCount,
                                     const VkClearRect* pRects) {
  if (captureRenderPasses()) {
    auto& commandBufferState = SD()._commandbufferstates[commandBuffer];
    for (uint32_t i = 0; i < attachmentCount; ++i) {
      if (commandBufferState->beginRenderPassesList.size()) {
        auto& framebufferState =
            commandBufferState->beginRenderPassesList.back()->framebufferStateStore;
        uint32_t imageViewSize;
        if (framebufferState && !(framebufferState->framebufferCreateInfoData.Value()->flags &
                                  VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT)) {
          imageViewSize = (uint32_t)framebufferState->imageViewStateStoreList.size();
        } else {
          imageViewSize = (uint32_t)commandBufferState->beginRenderPassesList.back()
                              ->imageViewStateStoreListKHR.size();
        }
        uint32_t colorAttIndex = 0;
        for (uint32_t imageview = 0; imageview < imageViewSize; ++imageview) {
          std::string fileName;
          VkImage imageHandle;
          VkImageAspectFlags aspect;
          if (framebufferState && !(framebufferState->framebufferCreateInfoData.Value()->flags &
                                    VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT)) {
            imageHandle =
                framebufferState->imageViewStateStoreList[imageview]->imageStateStore->imageHandle;
            aspect = getFormatAspectFlags(
                framebufferState->imageViewStateStoreList[imageview]->imageStateStore->imageFormat);
          } else {
            imageHandle = commandBufferState->beginRenderPassesList.back()
                              ->imageViewStateStoreListKHR[imageview]
                              ->imageStateStore->imageHandle;
            aspect = getFormatAspectFlags(commandBufferState->beginRenderPassesList.back()
                                              ->imageViewStateStoreListKHR[imageview]
                                              ->imageStateStore->imageFormat);
          }
          if (pAttachments[i].aspectMask == aspect) {
            if (aspect == VK_IMAGE_ASPECT_COLOR_BIT) {
              if (pAttachments[i].colorAttachment == colorAttIndex) {
                SD()._commandbufferstates[commandBuffer]->clearedImages.insert(imageHandle);
              }
              colorAttIndex++;
            } else {
              SD()._commandbufferstates[commandBuffer]->clearedImages.insert(imageHandle);
            }
          }
        }
      }
    }
  }
}
} // namespace Vulkan
} // namespace gits

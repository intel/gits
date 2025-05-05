// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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

#include "vulkanTools.h"

namespace gits {

namespace Vulkan {

namespace {

inline bool isRecorder() {
  static bool isRecorder = Configurator::IsRecorder();
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
  static bool updateOnlyUsedMemory =
      TMemoryUpdateStates::ONLY_USED == Configurator::Get().vulkan.recorder.memoryUpdateState;
  return updateOnlyUsedMemory;
}

inline bool captureRenderPasses() {
  static bool captureRenderPasses =
      !Configurator::Get().vulkan.player.captureVulkanSubmits.empty() ||
      !Configurator::Get().vulkan.player.captureVulkanRenderPasses.empty() ||
      !Configurator::Get().vulkan.player.captureVulkanDraws.empty() ||
      !Configurator::Get().vulkan.recorder.dumpSubmits.empty();
  return captureRenderPasses;
}

inline bool captureRenderPassesResources() {
  static bool captureRenderPassesResources =
      !Configurator::Get().vulkan.player.captureVulkanSubmitsResources.empty() ||
      !Configurator::Get().vulkan.player.captureVulkanRenderPassesResources.empty() ||
      !Configurator::Get().vulkan.player.captureVulkanResources.empty();
  return captureRenderPassesResources;
}

inline bool crossPlatformStateRestoration() {
  static bool crossPlatformStateRestoration =
      Configurator::Get().vulkan.recorder.crossPlatformStateRestoration.images;
  return crossPlatformStateRestoration;
}
#ifdef GITS_PLATFORM_WINDOWS
inline bool usePresentSrcLayoutTransitionAsAFrameBoundary() {
  static bool usePresentSrcLayoutTransitionAsAFrameBoundary =
      Configurator::Get().vulkan.recorder.usePresentSrcLayoutTransitionAsAFrameBoundary;
  return usePresentSrcLayoutTransitionAsAFrameBoundary;
}
#endif

inline bool useCaptureReplayFeaturesForBuffersAndAccelerationStructures() {
  static bool useCaptureReplayFeaturesForBuffersAndAccelerationStructures =
      Configurator::Get()
          .vulkan.recorder.useCaptureReplayFeaturesForBuffersAndAccelerationStructures;
  return useCaptureReplayFeaturesForBuffersAndAccelerationStructures;
}

inline bool isUseExternalMemoryExtensionUsed() {
#ifdef GITS_PLATFORM_WINDOWS
  return Configurator::Get().vulkan.recorder.useExternalMemoryExtension;
#else
  return false;
#endif
}

inline bool isGitsRecorderAttached() {
  if (drvVk.GetGlobalDispatchTable().vkIAmRecorderGITS) {
    return true;
  }
  return false;
}

template <class STATE_CONTAINER, class KEY, class DST_CONTAINER>
inline void insertStateIfFound(STATE_CONTAINER& state, KEY key, DST_CONTAINER& dst) {
  const auto it = state.find(key);
  if (it != state.end()) {
    dst.insert(it->second);
  }
}

template <>
inline void insertStateIfFound(gits::Vulkan::CStateDynamic::TAccelerationStructureKHRStates& state,
                               VkAccelerationStructureKHR key,
                               std::unordered_set<std::shared_ptr<CBufferState>>& dst) {
  const auto it = state.find(key);
  if (it != state.end()) {
    dst.insert(it->second->bufferStateStore);
  }
}

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
    Configurator::GetMutable().vulkan.recorder.usePresentSrcLayoutTransitionAsAFrameBoundary = true;
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

    if (Configurator::Get().vulkan.player.skipNonDeterministicImages) {
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

    for (const auto& physicalDeviceState : SD()._physicaldevicestates) {
      if (physicalDeviceState.second->instanceStateStore->instanceHandle == instance) {
        physicalDevicesToRemove.push_back(physicalDeviceState.first);
      }
    }
    for (const auto& physicalDevice : physicalDevicesToRemove) {
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
  for (const auto& extension : supportedExtensions) {
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
    const auto& surfaceState = SD()._surfacekhrstates[surface];
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
    CAutoCaller autoCaller(drvVk.vkPauseRecordingGITS, drvVk.vkContinueRecordingGITS);

    auto physicalDeviceState = SD()._physicaldevicestates[physicalDevice];
    auto deviceState = std::make_shared<CDeviceState>(pDevice, pCreateInfo, physicalDeviceState);
    const auto& queueFamilies = physicalDeviceState->queueFamilyPropertiesCurrent;
    auto device = *pDevice;

    for (uint32_t qci = 0; qci < pCreateInfo->queueCreateInfoCount; ++qci) {
      auto flags = pCreateInfo->pQueueCreateInfos[qci].flags;

      for (uint32_t qc = 0; qc < pCreateInfo->pQueueCreateInfos[qci].queueCount; ++qc) {
        VkQueue queue = VK_NULL_HANDLE;
        uint32_t queueFamilyIndex = pCreateInfo->pQueueCreateInfos[qci].queueFamilyIndex;
        uint32_t queueIndex = qc;

        if (drvVk.GetDeviceDispatchTable(device).vkGetDeviceQueue2) {
          VkDeviceQueueInfo2 deviceQueueInfo2 = {
              VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2, // VkStructureType sType;
              nullptr,                               // const void* pNext;
              flags,                                 // VkDeviceQueueCreateFlags flags;
              queueFamilyIndex,                      // uint32_t queueFamilyIndex;
              queueIndex                             // uint32_t queueIndex;
          };
          drvVk.vkGetDeviceQueue2(device, &deviceQueueInfo2, &queue);
        } else {
          drvVk.vkGetDeviceQueue(device, queueFamilyIndex, queueIndex, &queue);
        }

        auto queueState =
            std::make_shared<CQueueState>(&queue, queueFamilyIndex, queueIndex,
                                          queueFamilies[queueFamilyIndex].queueFlags, deviceState);
        SD()._queuestates.emplace(queue, queueState);
        deviceState->queueStateStoreList.push_back(std::move(queueState));
      }
    }

    SD()._devicestates.emplace(device, deviceState);
    for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; ++i) {
      deviceState->enabledExtensions.emplace_back(pCreateInfo->ppEnabledExtensionNames[i]);
    }

    // Synchronization2 feature check
    {
      const auto* sync2Features =
          (const VkPhysicalDeviceSynchronization2Features*)getPNextStructure(
              pCreateInfo->pNext, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES);
      if (sync2Features && (sync2Features->synchronization2 == VK_TRUE)) {
        deviceState->synchronization2 = true;
      }
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

  if (Configurator::IsRecorder()) {
    for (uint32_t bic = 0; bic < bindInfoCount; ++bic) {
      const auto& bindInfo = pBindInfo[bic];

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
      CAutoCaller autoCaller(drvVk.vkPauseRecordingGITS, drvVk.vkContinueRecordingGITS);

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
        swapchainState->imageStateStoreList.push_back(std::move(imageState));
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
    swapchainState->imageStateStoreList.push_back(std::move(imageState));
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
  if (pImageIndex != nullptr) {
    SD()._swapchainkhrstates[swapchain]->acquiredImages.insert(*pImageIndex);
  }
}

inline void vkAcquireNextImage2KHR_SD(VkResult return_value,
                                      VkDevice device,
                                      const VkAcquireNextImageInfoKHR* pAcquireInfo,
                                      uint32_t* pImageIndex) {
  if (pAcquireInfo == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  if (VK_NULL_HANDLE != pAcquireInfo->semaphore) {
    SD()._semaphorestates[pAcquireInfo->semaphore]->semaphoreUsed = true;
  }
  if (VK_NULL_HANDLE != pAcquireInfo->fence) {
    SD()._fencestates[pAcquireInfo->fence]->fenceUsed = true;
  }
  if (pImageIndex != nullptr) {
    SD()._swapchainkhrstates[pAcquireInfo->swapchain]->acquiredImages.insert(*pImageIndex);
  }
}

inline void vkQueuePresentKHR_SD(VkResult return_value,
                                 VkQueue queue,
                                 const VkPresentInfoKHR* pPresentInfo) {
  if (pPresentInfo == nullptr ||
      (pPresentInfo->waitSemaphoreCount > 0 && pPresentInfo->pWaitSemaphores == nullptr) ||
      (pPresentInfo->swapchainCount > 0 && pPresentInfo->pSwapchains == nullptr)) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
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
  for (const auto& imageState : SD()._swapchainkhrstates[swapchain]->imageStateStoreList) {
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
  if (Configurator::IsRecorder() || captureRenderPassesResources()) {
    auto& descriptorSetStateList =
        SD()._descriptorpoolstates[descriptorPool]->descriptorSetStateStoreList;

    for (const auto& descriptorSetState : descriptorSetStateList) {
      SD()._descriptorsetstates.erase(descriptorSetState->descriptorSetHandle); // Stardust
    }
    descriptorSetStateList.clear();
  }
}

inline void vkDestroyDescriptorPool_SD(VkDevice device,
                                       VkDescriptorPool descriptorPool,
                                       const VkAllocationCallbacks* pAllocator) {
  if (descriptorPool != VK_NULL_HANDLE) {
    for (const auto& descriptorSetState :
         SD()._descriptorpoolstates[descriptorPool]->descriptorSetStateStoreList) {
      SD()._descriptorsetstates.erase(descriptorSetState->descriptorSetHandle); // Stardust
    }
    SD()._descriptorpoolstates.erase(descriptorPool);
  }
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
                                    VkCommandBuffer cmdBuf,
                                    VkCommandBufferResetFlags /* flags */,
                                    bool stateRestore = false);

inline void vkResetCommandPool_SD(VkResult return_value,
                                  VkDevice device,
                                  VkCommandPool commandPool,
                                  VkCommandPoolResetFlags flags) {
  for (const auto& commandBufferState :
       SD()._commandpoolstates[commandPool]->commandBufferStateStoreList) {
    vkResetCommandBuffer_SD(return_value, commandBufferState->commandBufferHandle, flags);
  }
}

inline void vkDestroyCommandPool_SD(VkDevice device,
                                    VkCommandPool commandPool,
                                    const VkAllocationCallbacks* pAllocator) {
  if (commandPool != VK_NULL_HANDLE) {
    // For the purpose of tracking ray tracing-related resources and data,
    // we create temporary resources associated with command buffers.
    // Those need to be destroyed (explicitly and manually) so additionally
    // we need to call vkResetCommandPool_SD(), which calls vkResetCommandBuffer_SD()
    vkResetCommandPool_SD(VK_SUCCESS, device, commandPool, 0);

    const auto& commandPoolState = SD()._commandpoolstates[commandPool];
    for (const auto& commandBufferState : commandPoolState->commandBufferStateStoreList) {
      SD()._commandbufferstates.erase(commandBufferState->commandBufferHandle);

      if (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase()) {
        SD().bindingBuffers.erase(commandBufferState->commandBufferHandle);
        SD().bindingImages.erase(commandBufferState->commandBufferHandle);
      }

      if (Configurator::Get().vulkan.recorder.memorySegmentSize ||
          Configurator::Get().vulkan.recorder.shadowMemory) {
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
  if (((Configurator::IsPlayer()) && (Configurator::Get().common.player.cleanResourcesOnExit)) ||
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
  if (((Configurator::IsPlayer()) && (Configurator::Get().common.player.cleanResourcesOnExit)) ||
      (isSubcaptureBeforeRestorationPhase())) {
    SD()._samplerstates.erase(sampler);
  }
}

// SamplerYcbcrConversion

inline void vkCreateSamplerYcbcrConversion_SD(VkResult return_value,
                                              VkDevice device,
                                              const VkSamplerYcbcrConversionCreateInfo* pCreateInfo,
                                              const VkAllocationCallbacks* pAllocator,
                                              VkSamplerYcbcrConversion* pYcbcrConversion) {
  if ((return_value == VK_SUCCESS) && (*pYcbcrConversion != VK_NULL_HANDLE)) {
    SD()._ycbcrConversiontates.emplace(
        *pYcbcrConversion, std::make_shared<CYcbcrConversionState>(pYcbcrConversion, pCreateInfo,
                                                                   SD()._devicestates[device]));
  }
}

inline void vkCreateSamplerYcbcrConversionKHR_SD(
    VkResult return_value,
    VkDevice device,
    const VkSamplerYcbcrConversionCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkSamplerYcbcrConversion* pYcbcrConversion) {
  if ((return_value == VK_SUCCESS) && (*pYcbcrConversion != VK_NULL_HANDLE)) {
    SD()._ycbcrConversiontates.emplace(
        *pYcbcrConversion, std::make_shared<CYcbcrConversionState>(pYcbcrConversion, pCreateInfo,
                                                                   SD()._devicestates[device]));
  }
}

inline void vkDestroySamplerYcbcrConversion_SD(VkDevice device,
                                               VkSamplerYcbcrConversion ycbcrConversion,
                                               const VkAllocationCallbacks* pAllocator) {
  if (ycbcrConversion != VK_NULL_HANDLE) {
    SD()._ycbcrConversiontates.erase(ycbcrConversion);
  }
}

inline void vkDestroySamplerYcbcrConversionKHR_SD(VkDevice device,
                                                  VkSamplerYcbcrConversion ycbcrConversion,
                                                  const VkAllocationCallbacks* pAllocator) {
  if (ycbcrConversion != VK_NULL_HANDLE) {
    SD()._ycbcrConversiontates.erase(ycbcrConversion);
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

  memoryState->mapping.reset(
      new CDeviceMemoryState::CMapping(offset, unmapSize, flags, (char*)*ppData));

  if (Configurator::Get().vulkan.player.printMemUsageVk) {
    SD().currentlyMappedMemory += unmapSize;
    Log(INFO) << "Currently Allocated Memory TOTAL: " << SD().currentlyAllocatedMemoryAll / 1000000
              << " MB; GPU_ONLY: " << SD().currentlyAllocatedMemoryGPU / 1000000
              << " MB; CPU_GPU_Shared: " << SD().currentlyAllocatedMemoryCPU_GPU / 1000000
              << " MB; Currently mapped memory: " << SD().currentlyMappedMemory / 1000000 << " MB";
  }

  if (Configurator::IsRecorder()) {
    if (isUseExternalMemoryExtensionUsed() ||
        Configurator::Get().vulkan.recorder.writeWatchDetection) {
      WriteWatchSniffer::ResetTouchedPages(*ppData, unmapSize);
    } else if (Configurator::Get().vulkan.recorder.memoryAccessDetection) {
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
        if (!Configurator::Get().vulkan.recorder.shadowMemory) {
          Log(ERR) << "Please try to record with enabled ShadowMemory option.";
        }
      }
    }
  }
}

inline void vkUnmapMemory_SD(VkDevice device, VkDeviceMemory memory) {
  auto& memoryState = SD()._devicememorystates[memory];

  if (memoryState->IsMapped()) {
    if (Configurator::IsRecorder() && Configurator::Get().vulkan.recorder.memoryAccessDetection) {
      MemorySniffer::Get().RemoveRegion(memoryState->mapping->sniffedRegionHandle);
    }

    if (Configurator::Get().vulkan.player.printMemUsageVk) {
      SD().currentlyMappedMemory -= memoryState->mapping->size;
    }

    memoryState->mapping.reset();
  }

  if (Configurator::IsRecorder() && Configurator::Get().vulkan.recorder.shadowMemory) {
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
    if (Configurator::Get().vulkan.player.printMemUsageVk) {
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
  if (memory == VK_NULL_HANDLE) {
    return;
  }
  vkUnmapMemory_SD(device, memory);

  if (Configurator::Get().vulkan.player.printMemUsageVk) {
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
  if (Configurator::IsRecorder() && isSubcaptureBeforeRestorationPhase()) {
    const auto it = SD()._imagestates.find(image);
    if ((it != SD()._imagestates.end()) && (it->second->binding != nullptr)) {
      it->second->binding->deviceMemoryStateStore->aliasingTracker.RemoveImage(
          it->second->binding->memoryOffset, it->second->binding->memorySizeRequirement, image);
    }
  }
  SD().nonDeterministicImages.erase(image);
  SD()._imagestates.erase(image); // Stardust ImageView
}

inline void vkGetImageMemoryRequirements_SD(VkDevice device,
                                            VkImage image,
                                            VkMemoryRequirements* pMemoryRequirements) {
  if (pMemoryRequirements == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
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

  if (Configurator::IsRecorder() && isSubcaptureBeforeRestorationPhase()) {
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

    // BUFFER DEVICE ADDRESS GROUP COMMENT TOKEN
    // Please, (un)comment all the areas with the above token together, at the same time
    //
    // if (Configurator::IsRecorder() &&
    //    ((pCreateInfo->usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) == VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)) {
    //   CBufferState::shaderDeviceAddressBuffers[*pBuffer] = newBufferState;
    // }
  }
}

inline void vkDestroyBuffer_SD(VkDevice device,
                               VkBuffer buffer,
                               const VkAllocationCallbacks* pAllocator) {
  if (buffer == VK_NULL_HANDLE) {
    return;
  }
  if (Configurator::IsRecorder()) {
    const auto it = SD()._bufferstates.find(buffer);
    if (it == SD()._bufferstates.end()) {
      Log(WARN) << "Unknown buffer destroyed: " << buffer;
      return;
    }

    const auto& bufferState = it->second;

    if (isSubcaptureBeforeRestorationPhase()) {
      if (bufferState->binding != nullptr) {
        bufferState->binding->deviceMemoryStateStore->aliasingTracker.RemoveBuffer(
            bufferState->binding->memoryOffset, bufferState->binding->memorySizeRequirement,
            buffer);
      }
    }

    {
      VkDeviceAddress deviceAddress = bufferState->deviceAddress;
      auto it =
          std::find_if(CBufferState::deviceAddresses.begin(), CBufferState::deviceAddresses.end(),
                       [&deviceAddress](auto const& element) {
                         return (deviceAddress >= element.start) && (deviceAddress < element.end);
                       });

      if (it != CBufferState::deviceAddresses.end()) {
        CBufferState::deviceAddresses.erase(it);
      }

      for (auto address : bufferState->deviceAddressesToErase) {
        CBufferState::deviceAddressesQuickLook.erase(address);
      }
    }

    // BUFFER DEVICE ADDRESS GROUP COMMENT TOKEN
    // Please, (un)comment all the areas with the above token together, at the same time
    //
    // CBufferState::shaderDeviceAddressBuffers.erase(buffer);
  }

  SD()._bufferstates.erase(buffer); //SDK
}

inline void vkGetBufferMemoryRequirements_SD(VkDevice device,
                                             VkBuffer buffer,
                                             VkMemoryRequirements* pMemoryRequirements) {
  if (pMemoryRequirements == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
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

  if (isBitSet(bufferState->bufferCreateInfoData.Value()->usage,
               VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)) {
    bufferState->deviceAddress =
        getBufferDeviceAddress(bufferState->deviceStateStore->deviceHandle, buffer);
  }

  if (Configurator::IsRecorder() && isSubcaptureBeforeRestorationPhase()) {
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

  const auto& bufferState = SD()._bufferstates[pInfo->buffer];
  bufferState->deviceAddress =
      return_value; // <- We need this data to properly replay streams (see CBufferDeviceAddressObject class)

  if (Configurator::IsRecorder()) {
    CBufferState::deviceAddresses.insert(
        {return_value, return_value + bufferState->bufferCreateInfoData.Value()->size,
         pInfo->buffer});
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
  if ((Configurator::IsRecorder() || captureRenderPassesResources()) &&
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
          const auto& layoutBindingData = layoutCreateInfo->pBindings[j];
          auto& descriptorSetBinding =
              descriptorSetState->descriptorSetBindings[layoutBindingData.binding];
          uint32_t descriptorCount = layoutBindingData.descriptorCount;

          // Check if the number of descriptors is modified by the VK_EXT_descriptor_indexing extension (VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT flag)
          if (variableDescriptorCount != nullptr) {
            // Flags are specified for each descriptor set / for each binding
            VkDescriptorBindingFlags bindingFlags =
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
  if (Configurator::IsRecorder() || captureRenderPassesResources()) {
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
            currentDescriptorData->accelerationStructureStateStore.reset();
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
              const auto& imageViewState =
                  SD()._imageviewstates[pDescriptorWrites[i].pImageInfo[j].imageView];
              const auto& imageState = imageViewState->imageStateStore;

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

              if (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase()) {
                descriptorSetState->descriptorImages[currentBinding] = imageState;
              }

              if (descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) {
                descriptorSetState->descriptorWriteImages[currentBinding] = {
                    VULKAN_STORAGE_IMAGE, imageState->imageHandle};

                if ((Configurator::Get().vulkan.recorder.memorySegmentSize ||
                     Configurator::Get().vulkan.recorder.shadowMemory) &&
                    (imageState->binding)) {
                  VkMemoryRequirements memRequirements = {};
                  drvVk.vkGetImageMemoryRequirements(imageState->deviceStateStore->deviceHandle,
                                                     imageState->imageHandle, &memRequirements);

                  //TODO : call vkGetImageSubresourceLayout when tiling is Linear

                  const auto memory =
                      imageState->binding->deviceMemoryStateStore->deviceMemoryHandle;
                  const auto offset = imageState->binding->memoryOffset;

                  descriptorSetState->descriptorMapMemory[currentBinding].clear();
                  descriptorSetState->descriptorMapMemory[currentBinding][memory].insert(
                      offset, memRequirements.size + offset);
                }
              }
            }
            break;
          case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
          case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
            if ((pDescriptorWrites[i].pTexelBufferView != nullptr) &&
                (pDescriptorWrites[i].pTexelBufferView[j]) != VK_NULL_HANDLE) {
              const auto& bufferViewState =
                  SD()._bufferviewstates[pDescriptorWrites[i].pTexelBufferView[j]];

              if (isSubcaptureBeforeRestorationPhase() && (currentDescriptorData != nullptr)) {
                currentDescriptorData->pTexelBufferView = std::make_shared<CVkBufferViewDataArray>(
                    1, &pDescriptorWrites[i].pTexelBufferView[j]);
                currentDescriptorData->bufferViewStateStore = bufferViewState;
              }

              const auto& bufferState = bufferViewState->bufferStateStore;

              if (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase()) {
                descriptorSetState->descriptorBuffers[currentBinding] = bufferState;
              }
              if (descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER) {
                descriptorSetState->descriptorWriteBuffers[currentBinding] = {
                    VULKAN_STORAGE_TEXEL_BUFFER, bufferState->bufferHandle};

                if ((Configurator::Get().vulkan.recorder.memorySegmentSize ||
                     Configurator::Get().vulkan.recorder.shadowMemory) &&
                    (bufferState->binding)) {
                  const auto* pViewCreateInfo = bufferViewState->bufferViewCreateInfoData.Value();
                  const auto memory =
                      bufferState->binding->deviceMemoryStateStore->deviceMemoryHandle;
                  const auto offset = bufferState->binding->memoryOffset + pViewCreateInfo->offset;
                  const auto size = (pViewCreateInfo->range == 0xFFFFFFFFFFFFFFFF)
                                        ? (bufferState->bufferCreateInfoData.Value()->size -
                                           pViewCreateInfo->offset)
                                        : (pViewCreateInfo->range);

                  descriptorSetState->descriptorMapMemory[currentBinding].clear();
                  descriptorSetState->descriptorMapMemory[currentBinding][memory].insert(
                      offset, size + offset);
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
              const auto& bufferInfo = pDescriptorWrites[i].pBufferInfo[j];
              const auto& bufferState = SD()._bufferstates[bufferInfo.buffer];

              if (isSubcaptureBeforeRestorationPhase() && (currentDescriptorData != nullptr)) {
                currentDescriptorData->pBufferInfo =
                    std::make_shared<CVkDescriptorBufferInfoData>(&bufferInfo);
                currentDescriptorData->bufferStateStore = bufferState;
              }

              if (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase()) {
                descriptorSetState->descriptorBuffers[currentBinding] = bufferState;
              }
              if ((descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) ||
                  (descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)) {
                VulkanResourceType resType = descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
                                                 ? VULKAN_STORAGE_BUFFER
                                                 : VULKAN_STORAGE_BUFFER_DYNAMIC;
                descriptorSetState->descriptorWriteBuffers[currentBinding] = {resType,
                                                                              bufferInfo.buffer};

                if ((Configurator::Get().vulkan.recorder.memorySegmentSize ||
                     Configurator::Get().vulkan.recorder.shadowMemory) &&
                    bufferState->binding) {
                  const auto memory =
                      bufferState->binding->deviceMemoryStateStore->deviceMemoryHandle;
                  const auto offset = bufferState->binding->memoryOffset + bufferInfo.offset;
                  const auto size =
                      (bufferInfo.range == 0xFFFFFFFFFFFFFFFF)
                          ? (bufferState->bufferCreateInfoData.Value()->size - bufferInfo.offset)
                          : (bufferInfo.range);

                  descriptorSetState->descriptorMapMemory[currentBinding].clear();
                  descriptorSetState->descriptorMapMemory[currentBinding][memory].insert(
                      offset, size + offset);
                }
              }
            }
            break;
          case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR: {
            auto* accelerationStructureWrite =
                static_cast<const VkWriteDescriptorSetAccelerationStructureKHR*>(getPNextStructure(
                    pDescriptorWrites[i].pNext,
                    VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR));
            if (accelerationStructureWrite && accelerationStructureWrite->pAccelerationStructures &&
                (accelerationStructureWrite->pAccelerationStructures[j] != VK_NULL_HANDLE)) {
              const auto& accelerationStructureState =
                  SD()._accelerationstructurekhrstates[accelerationStructureWrite
                                                           ->pAccelerationStructures[j]];
              if (isSubcaptureBeforeRestorationPhase()) {
                currentDescriptorData->accelerationStructureStateStore = accelerationStructureState;
              }

              if (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase()) {
                descriptorSetState->descriptorBuffers[currentBinding] =
                    accelerationStructureState->bufferStateStore;
              }
            }
          } break;
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
      const auto& srcDescriptorSetState = SD()._descriptorsetstates[pDescriptorCopies[i].srcSet];
      const auto& dstDescriptorSetState = SD()._descriptorsetstates[pDescriptorCopies[i].dstSet];

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

      if (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase()) {
        for (const auto& obj : srcDescriptorSetState->descriptorBuffers) {
          dstDescriptorSetState->descriptorBuffers[obj.first] = obj.second;
        }

        for (const auto& obj : srcDescriptorSetState->descriptorImages) {
          dstDescriptorSetState->descriptorImages[obj.first] = obj.second;
        }
      }
      if (Configurator::Get().vulkan.recorder.memorySegmentSize ||
          Configurator::Get().vulkan.recorder.shadowMemory) {
        for (const auto& binding : srcDescriptorSetState->descriptorMapMemory) {
          dstDescriptorSetState->descriptorMapMemory[binding.first].clear();
          for (const auto& obj : binding.second) {
            for (const auto& obj2 : obj.second.getIntervals()) {
              dstDescriptorSetState->descriptorMapMemory[binding.first][obj.first].insert(
                  obj2.first, obj2.second);
            }
          }
        }
      }

      for (const auto& obj : srcDescriptorSetState->descriptorWriteBuffers) {
        dstDescriptorSetState->descriptorWriteBuffers[obj.first] = obj.second;
      }
      for (const auto& obj : srcDescriptorSetState->descriptorWriteImages) {
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
  if (Configurator::IsRecorder() || captureRenderPassesResources()) {
    for (unsigned int i = 0; i < descriptorSetCount; i++) {
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
    if (Configurator::IsRecorder()) {
      for (uint32_t i = 0; i < pCreateInfo->setLayoutCount; i++) {
        if (pCreateInfo->pSetLayouts[i] == VK_NULL_HANDLE) {
          state->descriptorSetLayoutStates.push_back(0);
          continue;
        }

        const auto it = SD()._descriptorsetlayoutstates.find(pCreateInfo->pSetLayouts[i]);
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
  if (((Configurator::IsPlayer()) && (Configurator::Get().common.player.cleanResourcesOnExit ||
                                      captureRenderPassesResources())) ||
      (Configurator::IsRecorder())) {
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
  if (((Configurator::IsPlayer()) && (Configurator::Get().common.player.cleanResourcesOnExit ||
                                      captureRenderPassesResources())) ||
      (Configurator::IsRecorder())) {
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
  if (((Configurator::IsPlayer()) && (Configurator::Get().common.player.cleanResourcesOnExit)) ||
      (Configurator::IsRecorder())) {
    SD()._descriptorupdatetemplatestates.erase(descriptorUpdateTemplate);
  }
}

inline void vkDestroyDescriptorUpdateTemplateKHR_SD(
    VkDevice device,
    VkDescriptorUpdateTemplate descriptorUpdateTemplate,
    const VkAllocationCallbacks* pAllocator) {
  if (((Configurator::IsPlayer()) && (Configurator::Get().common.player.cleanResourcesOnExit)) ||
      (Configurator::IsRecorder())) {
    vkDestroyDescriptorUpdateTemplate_SD(device, descriptorUpdateTemplate, pAllocator);
  }
}

inline void vkUpdateDescriptorSetWithTemplate_SD(
    VkDevice device,
    VkDescriptorSet descriptorSet,
    VkDescriptorUpdateTemplate descriptorUpdateTemplate,
    const void* pData) {
  if (Configurator::IsRecorder() || captureRenderPassesResources()) {
    const auto& descriptorSetState = SD()._descriptorsetstates[descriptorSet];
    const auto& descriptorUpdateTemplateState =
        SD()._descriptorupdatetemplatestates[descriptorUpdateTemplate];
    const auto* descriptorUpdateTemplateCreateInfoData =
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
            const auto& imageViewState = SD()._imageviewstates[descriptorImageInfo->imageView];

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

            const auto& imageState = imageViewState->imageStateStore;

            if (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase()) {
              descriptorSetState->descriptorImages
                  [descriptorUpdateTemplateCreateInfoData->pDescriptorUpdateEntries[i].dstBinding] =
                  imageState;
            }
            if (descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) {
              descriptorSetState->descriptorWriteImages[descriptorUpdateTemplateCreateInfoData
                                                            ->pDescriptorUpdateEntries[i]
                                                            .dstBinding] = {
                  VULKAN_STORAGE_IMAGE, imageState->imageHandle};

              if ((Configurator::Get().vulkan.recorder.memorySegmentSize ||
                   Configurator::Get().vulkan.recorder.shadowMemory) &&
                  (imageState->binding)) {
                VkMemoryRequirements memRequirements = {};
                drvVk.vkGetImageMemoryRequirements(imageState->deviceStateStore->deviceHandle,
                                                   imageState->imageHandle, &memRequirements);

                //TODO : call vkGetImageSubresourceLayout when tiling is Linear

                const auto memory = imageState->binding->deviceMemoryStateStore->deviceMemoryHandle;
                const auto offset = imageState->binding->memoryOffset;

                descriptorSetState
                    ->descriptorMapMemory[descriptorUpdateTemplateCreateInfoData
                                              ->pDescriptorUpdateEntries[i]
                                              .dstBinding]
                    .clear();
                descriptorSetState
                    ->descriptorMapMemory[descriptorUpdateTemplateCreateInfoData
                                              ->pDescriptorUpdateEntries[i]
                                              .dstBinding][memory]
                    .insert(offset, memRequirements.size + offset);
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
            const auto& bufferViewState = SD()._bufferviewstates[*bufferView];

            if (isSubcaptureBeforeRestorationPhase() && (currentDescriptorData != nullptr)) {
              currentDescriptorData->pTexelBufferView =
                  std::make_shared<CVkBufferViewDataArray>(1, bufferView);
              currentDescriptorData->bufferViewStateStore = bufferViewState;
            }

            const auto& bufferState = bufferViewState->bufferStateStore;

            if (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase()) {
              descriptorSetState->descriptorBuffers
                  [descriptorUpdateTemplateCreateInfoData->pDescriptorUpdateEntries[i].dstBinding] =
                  bufferState;
            }
            if (descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER) {
              descriptorSetState->descriptorWriteBuffers[descriptorUpdateTemplateCreateInfoData
                                                             ->pDescriptorUpdateEntries[i]
                                                             .dstBinding] = {
                  VULKAN_STORAGE_TEXEL_BUFFER, bufferState->bufferHandle};

              if ((Configurator::Get().vulkan.recorder.memorySegmentSize ||
                   Configurator::Get().vulkan.recorder.shadowMemory) &&
                  (bufferState->binding)) {
                const auto* pViewCreateInfo = bufferViewState->bufferViewCreateInfoData.Value();
                const auto memory =
                    bufferState->binding->deviceMemoryStateStore->deviceMemoryHandle;
                const auto offset = bufferState->binding->memoryOffset + pViewCreateInfo->offset;
                const auto size = (pViewCreateInfo->range == 0xFFFFFFFFFFFFFFFF)
                                      ? (bufferState->bufferCreateInfoData.Value()->size -
                                         pViewCreateInfo->offset)
                                      : (pViewCreateInfo->range);

                descriptorSetState
                    ->descriptorMapMemory[descriptorUpdateTemplateCreateInfoData
                                              ->pDescriptorUpdateEntries[i]
                                              .dstBinding]
                    .clear();
                descriptorSetState
                    ->descriptorMapMemory[descriptorUpdateTemplateCreateInfoData
                                              ->pDescriptorUpdateEntries[i]
                                              .dstBinding][memory]
                    .insert(offset, size + offset);
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
            const auto& bufferState = SD()._bufferstates[descBufferInfo->buffer];

            if (isSubcaptureBeforeRestorationPhase() && (currentDescriptorData != nullptr)) {
              currentDescriptorData->pBufferInfo =
                  std::make_shared<CVkDescriptorBufferInfoData>(descBufferInfo);
              currentDescriptorData->bufferStateStore = bufferState;
            }

            if (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase()) {
              descriptorSetState->descriptorBuffers
                  [descriptorUpdateTemplateCreateInfoData->pDescriptorUpdateEntries[i].dstBinding] =
                  bufferState;
            }

            if ((descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) ||
                (descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)) {
              VulkanResourceType resType = (descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                                               ? VULKAN_STORAGE_BUFFER
                                               : VULKAN_STORAGE_BUFFER_DYNAMIC;
              descriptorSetState->descriptorWriteBuffers[descriptorUpdateTemplateCreateInfoData
                                                             ->pDescriptorUpdateEntries[i]
                                                             .dstBinding] = {
                  resType, descBufferInfo->buffer};

              if ((Configurator::Get().vulkan.recorder.memorySegmentSize ||
                   Configurator::Get().vulkan.recorder.shadowMemory) &&
                  bufferState->binding) {
                const auto memory =
                    bufferState->binding->deviceMemoryStateStore->deviceMemoryHandle;
                const auto offset = bufferState->binding->memoryOffset + descBufferInfo->offset;
                const auto size =
                    (descBufferInfo->range == 0xFFFFFFFFFFFFFFFF)
                        ? (bufferState->bufferCreateInfoData.Value()->size - descBufferInfo->offset)
                        : (descBufferInfo->range);

                descriptorSetState
                    ->descriptorMapMemory[descriptorUpdateTemplateCreateInfoData
                                              ->pDescriptorUpdateEntries[i]
                                              .dstBinding]
                    .clear();
                descriptorSetState
                    ->descriptorMapMemory[descriptorUpdateTemplateCreateInfoData
                                              ->pDescriptorUpdateEntries[i]
                                              .dstBinding][memory]
                    .insert(offset, size + offset);
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
  if (((Configurator::IsPlayer()) && (Configurator::Get().common.player.cleanResourcesOnExit)) ||
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
  if (((Configurator::IsPlayer()) && (Configurator::Get().common.player.cleanResourcesOnExit)) ||
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
    uint32_t hash = GetHash(pCreateInfo->codeSize, pCreateInfo->pCode);
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
      std::shared_ptr<CPipelineLayoutState> pipelineLayoutObj = NULL;
      if (createInfo->layout) {
        pipelineLayoutObj = SD()._pipelinelayoutstates[createInfo->layout];
      }
      auto pipelineState = std::make_shared<CPipelineState>(
          &pPipelines[i], createInfo, SD()._devicestates[device], pipelineLayoutObj, renderPassObj);

      for (unsigned int j = 0; j < createInfo->stageCount; j++) {
        const auto& stageInfo = createInfo->pStages[j];

        if (stageInfo.module != VK_NULL_HANDLE) {
          const auto& shaderModuleState = SD()._shadermodulestates[stageInfo.module];
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
        const auto& stageInfo = createInfo->stage;
        if (stageInfo.module != VK_NULL_HANDLE) {
          const auto& shaderModuleState = SD()._shadermodulestates[stageInfo.module];
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

inline void vkCreateRayTracingPipelinesKHR_SD(VkResult return_value,
                                              VkDevice device,
                                              VkDeferredOperationKHR deferredOperation,
                                              VkPipelineCache pipelineCache,
                                              uint32_t createInfoCount,
                                              const VkRayTracingPipelineCreateInfoKHR* pCreateInfos,
                                              const VkAllocationCallbacks* pAllocator,
                                              VkPipeline* pPipelines) {
  if (!pCreateInfos || (VK_SUCCESS != return_value)) {
    return;
  }

  CAutoCaller autoCaller(drvVk.vkPauseRecordingGITS, drvVk.vkContinueRecordingGITS);

  for (uint32_t i = 0; i < createInfoCount; i++) {
    if (pPipelines[i] == VK_NULL_HANDLE) {
      continue;
    }

    const auto* pCreateInfo = &pCreateInfos[i];
    auto pipelineState =
        std::make_shared<CPipelineState>(&pPipelines[i], pCreateInfo, SD()._devicestates[device],
                                         SD()._pipelinelayoutstates[pCreateInfo->layout]);
    pipelineState->isLibrary = isBitSet(pCreateInfo->flags, VK_PIPELINE_CREATE_LIBRARY_BIT_KHR);

    for (unsigned int j = 0; j < pCreateInfo->stageCount; j++) {
      const auto& stageInfo = pCreateInfo->pStages[j];
      const auto& shaderModuleState = SD()._shadermodulestates[stageInfo.module];
      pipelineState->shaderModuleStateStoreList.push_back(shaderModuleState);
      pipelineState->stageShaderHashMapping[stageInfo.stage] = shaderModuleState->shaderHash;
    }

    SD()._pipelinestates.emplace(pPipelines[i], pipelineState);

    // Shader group handles
    auto& shaderGroup = pipelineState->shaderGroupHandles;

    // Handles assigned on a current platform
    std::vector<uint8_t> currentHandles;
    {
      currentHandles.resize(shaderGroup.dataSize);
      drvVk.vkGetRayTracingShaderGroupHandlesKHR(device, pPipelines[i], 0, shaderGroup.count,
                                                 shaderGroup.dataSize, currentHandles.data());
    }

    // Handles assigned during stream recording (passed from a recorder)
    auto* pOriginalHandles = (VkOriginalShaderGroupHandlesGITS*)getPNextStructure(
        pCreateInfo->pNext, VK_STRUCTURE_TYPE_ORIGINAL_SHADER_GROUP_HANDLES_GITS);
    std::vector<uint8_t>& originalHandles = shaderGroup.originalHandles;
    {
      originalHandles.resize(shaderGroup.dataSize);
      if (pOriginalHandles) {
        memcpy(originalHandles.data(), pOriginalHandles->pData, shaderGroup.dataSize);
        shaderGroup.patchingRequired =
            Config::Get().vulkan.player.patchShaderGroupHandles &&
            (memcmp(originalHandles.data(), currentHandles.data(), shaderGroup.dataSize) != 0);
      } else {
        originalHandles = currentHandles;
        shaderGroup.patchingRequired = false;
      }
    }

    // Create a per RT-pipeline buffer to store shader group handles which are used later during
    // SBT patching. The buffer contains both original (recorder-side) and current (player-side)
    // handles, that's why its size is twice as large.
    if (shaderGroup.patchingRequired) {
      shaderGroup.memoryBufferPair = createTemporaryBuffer(
          device, 2 * shaderGroup.dataSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, nullptr,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

      auto memory = shaderGroup.memoryBufferPair.first->deviceMemoryHandle;
      auto buffer = shaderGroup.memoryBufferPair.second->bufferHandle;
      shaderGroup.deviceAddress = getBufferDeviceAddress(device, buffer);

      char* mappedMemoryPtr;
      drvVk.vkMapMemory(device, memory, 0, VK_WHOLE_SIZE, 0, (void**)&mappedMemoryPtr);

      // Buffer used to patch shader group handles contains original and current handles

      // Copy original shader group handles passed from a recorder
      memcpy(mappedMemoryPtr, originalHandles.data(), shaderGroup.dataSize);
      // Copy current shader group handles acquired from a driver
      memcpy(mappedMemoryPtr + shaderGroup.dataSize, currentHandles.data(), shaderGroup.dataSize);

      VkMappedMemoryRange range = {
          VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, // VkStructureType sType;
          nullptr,                               // const void* pNext;
          memory,                                // VkDeviceMemory memory;
          0,                                     // VkDeviceSize offset;
          shaderGroup.dataSize * 2               // VkDeviceSize size;
      };
      drvVk.vkFlushMappedMemoryRanges(device, 1, &range);
      drvVk.vkUnmapMemory(device, memory);
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
  if (Configurator::Get().vulkan.recorder.delayFenceChecksCount > 0) {
    for (auto& fenceState : SD()._fencestates) {
      if ((fenceState.second->deviceStateStore->deviceHandle == device) &&
          fenceState.second->fenceUsed) {
        fenceState.second->delayChecksCount =
            Configurator::Get().vulkan.recorder.delayFenceChecksCount;
      }
    }
  }
}

inline void vkQueueWaitIdle_SD(VkResult return_value, VkQueue queue) {
  if (Configurator::Get().vulkan.recorder.delayFenceChecksCount > 0) {
    for (auto& fenceState : SD()._fencestates) {
      if ((fenceState.second->deviceStateStore->deviceHandle ==
           SD()._queuestates[queue]->deviceStateStore->deviceHandle) &&
          fenceState.second->fenceUsed) {
        fenceState.second->delayChecksCount =
            Configurator::Get().vulkan.recorder.delayFenceChecksCount;
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
  if (((Configurator::IsPlayer()) && (Configurator::Get().common.player.cleanResourcesOnExit)) ||
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
  if (((Configurator::IsPlayer()) && (Configurator::Get().common.player.cleanResourcesOnExit)) ||
      (isSubcaptureBeforeRestorationPhase())) {
    SD()._querypoolstates.erase(queryPool);
  }
}

inline void vkResetQueryPool_SD(VkDevice device,
                                VkQueryPool queryPool,
                                uint32_t firstQuery,
                                uint32_t queryCount) {
  if (((Configurator::IsPlayer()) && (Configurator::Get().common.player.cleanResourcesOnExit)) ||
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
          std::move(commandBufferState));
    }
  }
}

inline void vkFreeCommandBuffers_SD(VkDevice device,
                                    VkCommandPool commandPool,
                                    uint32_t commandBufferCount,
                                    const VkCommandBuffer* pCommandBuffers) {
  for (uint32_t i = 0; i < commandBufferCount; i++) {
    // For the purpose of tracking ray tracing-related resources and data,
    // we create temporary resources associated with command buffers.
    // Those need to be destroyed (explicitly and manually),
    // so additionally we need to call vkResetCommandBuffer_SD()
    vkResetCommandBuffer_SD(VK_SUCCESS, pCommandBuffers[i], 0);

    SD()._commandpoolstates[commandPool]->commandBufferStateStoreList.erase(
        SD()._commandbufferstates[pCommandBuffers[i]]);
    SD()._commandbufferstates.erase(pCommandBuffers[i]);

    if (Configurator::IsRecorder()) {
      SD().bindingBuffers.erase(pCommandBuffers[i]);
      SD().bindingImages.erase(pCommandBuffers[i]);

      if (Configurator::Get().vulkan.recorder.memorySegmentSize ||
          Configurator::Get().vulkan.recorder.shadowMemory) {
        SD().updatedMemoryInCmdBuffer.erase(pCommandBuffers[i]);
      }
    }
  }
}

inline void vkResetCommandBuffer_SD(VkResult /* return_value */,
                                    VkCommandBuffer cmdBuf,
                                    VkCommandBufferResetFlags /* flags */,
                                    bool stateRestore) {
  auto& commandBufferState = SD()._commandbufferstates[cmdBuf];

  if (!stateRestore) {
    // In stateRestore, these are used for RenderPass mode preparation, so we can't reset them.
    commandBufferState->beginCommandBuffer.reset();
    commandBufferState->tokensBuffer.Clear();
    commandBufferState->ended = false;
  }
  commandBufferState->beginRenderPassesList.clear();
  commandBufferState->submitted = false;
  commandBufferState->restored = false;
  commandBufferState->currentPipeline = VK_NULL_HANDLE;
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
  commandBufferState->addressPatchers.clear();
  commandBufferState->queueSubmitEndMessageReceivers.clear();

  // Temporary buffers with bound device memory are created for the purpose of tracking
  // data which needs to be obtained from submitted command buffers and is available
  // after submission is finished. This type of data includes ray tracing-related resources
  // like for example list of bottom-level AS instances provided during building top-level
  // ASs.
  // These temporary buffers need to be destroyed (explicitly and manually).
  if (commandBufferState->temporaryBuffers.size() > 0) {
    CAutoCaller autoCaller(drvVk.vkPauseRecordingGITS, drvVk.vkContinueRecordingGITS);

    for (const auto& temporaryBuffer : commandBufferState->temporaryBuffers) {
      auto memoryState = temporaryBuffer.first;
      auto bufferState = temporaryBuffer.second;

      drvVk.vkDestroyBuffer(bufferState->deviceStateStore->deviceHandle, bufferState->bufferHandle,
                            nullptr);
      drvVk.vkFreeMemory(memoryState->deviceStateStore->deviceHandle,
                         memoryState->deviceMemoryHandle, nullptr);
    }
    commandBufferState->temporaryBuffers.clear();
  }

  if (commandBufferState->temporaryDescriptors.size() > 0) {
    CAutoCaller autoCaller(drvVk.vkPauseRecordingGITS, drvVk.vkContinueRecordingGITS);

    for (const auto& poolState : commandBufferState->temporaryDescriptors) {
      drvVk.vkDestroyDescriptorPool(poolState->deviceStateStore->deviceHandle,
                                    poolState->descriptorPoolHandle, nullptr);
    }
    commandBufferState->temporaryDescriptors.clear();
  }

  if (Configurator::IsRecorder()) {
    SD().bindingBuffers[cmdBuf].clear();
    SD().bindingImages[cmdBuf].clear();

    if (Configurator::Get().vulkan.recorder.memorySegmentSize ||
        Configurator::Get().vulkan.recorder.shadowMemory) {
      SD().updatedMemoryInCmdBuffer.erase(cmdBuf);
    }
  }
}

inline void vkBeginCommandBuffer_SD(VkResult /* return_value */,
                                    VkCommandBuffer cmdBuf,
                                    const VkCommandBufferBeginInfo* pBeginInfo) {
  vkResetCommandBuffer_SD(VK_SUCCESS, cmdBuf, 0);

  if (pBeginInfo == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  auto& commandBufferState = SD()._commandbufferstates[cmdBuf];

  commandBufferState->beginCommandBuffer.reset(
      new CCommandBufferState::CBeginCommandBuffer(pBeginInfo));

  if (Configurator::IsRecorder()) {
    if (nullptr != pBeginInfo->pInheritanceInfo) {
      if (VK_NULL_HANDLE != pBeginInfo->pInheritanceInfo->framebuffer) {
        for (const auto& imageViewState :
             SD()._framebufferstates[pBeginInfo->pInheritanceInfo->framebuffer]
                 ->imageViewStateStoreList) {
          const auto& imageState = imageViewState->imageStateStore;

          if (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase()) {
            SD().bindingImages[cmdBuf].insert(imageState);
          }

          if ((!Configurator::Get().vulkan.recorder.memorySegmentSize &&
               !Configurator::Get().vulkan.recorder.shadowMemory) ||
              !imageState->binding) {
            return;
          }

          VkMemoryRequirements memRequirements = {};
          drvVk.vkGetImageMemoryRequirements(imageState->deviceStateStore->deviceHandle,
                                             imageState->imageHandle, &memRequirements);

          //TODO : call vkGetImageSubresourceLayout when tiling is Linear

          const auto memory = imageState->binding->deviceMemoryStateStore->deviceMemoryHandle;
          const auto offset = imageState->binding->memoryOffset;
          SD().updatedMemoryInCmdBuffer[cmdBuf].AddToMap(memory, offset, memRequirements.size);
        }
      }
    }
  }
}

inline void vkEndCommandBuffer_SD(VkResult return_value, VkCommandBuffer cmdBuf) {
  SD()._commandbufferstates[cmdBuf]->ended = true;
}

// Deferred operation

inline void vkCreateDeferredOperationKHR_SD(VkResult return_value,
                                            VkDevice device,
                                            const VkAllocationCallbacks* pAllocator,
                                            VkDeferredOperationKHR* pDeferredOperation) {
  if ((return_value == VK_SUCCESS) && (*pDeferredOperation != VK_NULL_HANDLE)) {
    SD()._deferredoperationkhrstates.emplace(*pDeferredOperation,
                                             std::make_shared<CDeferredOperationKHRState>(
                                                 pDeferredOperation, SD()._devicestates[device]));
  }
}

inline void vkDestroyDeferredOperationKHR_SD(VkDevice device,
                                             VkDeferredOperationKHR operation,
                                             const VkAllocationCallbacks* pAllocator) {
  SD()._deferredoperationkhrstates.erase(operation);
}

// Acceleration structure

inline void vkCreateAccelerationStructureKHR_SD(
    VkResult return_value,
    VkDevice device,
    const VkAccelerationStructureCreateInfoKHR* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkAccelerationStructureKHR* pAccelerationStructure) {
  if ((return_value == VK_SUCCESS) && (*pAccelerationStructure != VK_NULL_HANDLE)) {
    auto accelerationStructureState = std::make_shared<CAccelerationStructureKHRState>(
        pAccelerationStructure, pCreateInfo, SD()._bufferstates[pCreateInfo->buffer]);
    accelerationStructureState->deviceAddress =
        getAccelerationStructureDeviceAddress(device, *pAccelerationStructure);

    SD()._accelerationstructurekhrstates.emplace(*pAccelerationStructure,
                                                 accelerationStructureState);
    CAccelerationStructureKHRState::deviceAddresses[accelerationStructureState->deviceAddress] =
        *pAccelerationStructure;

    // Required to properly track device addresses of buffers used for AS space
    if (Configurator::IsRecorder()) {
      VkBuffer buffer = accelerationStructureState->bufferStateStore->bufferHandle;
      VkDeviceAddress address = getBufferDeviceAddress(device, buffer);
      VkBufferDeviceAddressInfo info = {
          VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, // VkStructureType sType;
          nullptr,                                      // const void* pNext;
          buffer                                        // VkBuffer buffer;
      };
      vkGetBufferDeviceAddressUnifiedGITS_SD(address, device, &info);
    }
  }
}

inline void vkDestroyAccelerationStructureKHR_SD(VkDevice device,
                                                 VkAccelerationStructureKHR accelerationStructure,
                                                 const VkAllocationCallbacks* pAllocator) {
  if (accelerationStructure != VK_NULL_HANDLE) { // Doom Eternal fix
    CAccelerationStructureKHRState::deviceAddresses.erase(
        SD()._accelerationstructurekhrstates[accelerationStructure]->deviceAddress);
    SD()._accelerationstructurekhrstates.erase(accelerationStructure);
  }
}

inline void vkBuildAccelerationStructuresKHR_SD(
    VkResult return_value,
    VkDevice device,
    VkDeferredOperationKHR deferredOperation,
    uint32_t infoCount,
    const VkAccelerationStructureBuildGeometryInfoKHR* pInfos,
    const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos) {
  throw std::runtime_error("Ray tracing operations on host are not yet supported!");
}

inline void vkCmdBuildAccelerationStructuresKHR_SD(
    VkCommandBuffer cmdBuf,
    uint32_t infoCount,
    const VkAccelerationStructureBuildGeometryInfoKHR* pInfos,
    const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos) {
  CAccelerationStructureKHRState::globalAccelerationStructureBuildCommandIndex++;

  if (!Configurator::IsRecorder()) {
    return;
  }
  auto device =
      SD()._commandbufferstates[cmdBuf]->commandPoolStateStore->deviceStateStore->deviceHandle;

  for (uint32_t acc = 0; acc < infoCount; ++acc) {
    // Struct storage data is going to be injected into original structures via pNext
    // that's why a pointer to original, app-provided structures is needed here.
    auto* buildInfo = &pInfos[acc];
    auto* pRangeInfos = ppBuildRangeInfos[acc];
    auto& accelerationStructureState =
        SD()._accelerationstructurekhrstates[buildInfo->dstAccelerationStructure];

    if (isSubcaptureBeforeRestorationPhase()) {
      VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo = {
          VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR, // VkStructureType sType;
          nullptr,                                                       // const void* pNext;
          0, // VkDeviceSize accelerationStructureSize;
          0, // VkDeviceSize updateScratchSize;
          0  // VkDeviceSize buildScratchSize;
      };

      std::vector<uint32_t> primitivesCount(buildInfo->geometryCount);

      for (uint32_t g = 0; g < buildInfo->geometryCount; ++g) {
        primitivesCount[g] = pRangeInfos[g].primitiveCount;
      }

      drvVk.vkGetAccelerationStructureBuildSizesKHR(
          device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, buildInfo,
          primitivesCount.data(), &buildSizeInfo);
      accelerationStructureState->buildSizeInfo = buildSizeInfo;
    }

    if (buildInfo->mode == VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR) {
      // Full acceleration structure build
      accelerationStructureState->buildInfo.reset(new CAccelerationStructureKHRState::CBuildInfo(
          buildInfo, pRangeInfos, prepareAccelerationStructureControlData(cmdBuf)));
      accelerationStructureState->updateInfo.reset();
      accelerationStructureState->copyInfo.reset();
    } else if (buildInfo->mode == VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR) {
      // Acceleration structure update (full build info left untouched)
      const auto& srcAccelerationStructureState =
          SD()._accelerationstructurekhrstates[buildInfo->srcAccelerationStructure];
      accelerationStructureState->updateInfo.reset(new CAccelerationStructureKHRState::CBuildInfo(
          buildInfo, pRangeInfos, prepareAccelerationStructureControlData(cmdBuf),
          srcAccelerationStructureState));
    }
  }

  if (!(updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase())) {
    return;
  }

  auto& bindingBuffers = SD().bindingBuffers[cmdBuf];

  for (uint32_t acc = 0; acc < infoCount; ++acc) {
    auto buildInfo = &pInfos[acc];
    auto buildRangeInfos = ppBuildRangeInfos[acc];

    insertStateIfFound(SD()._accelerationstructurekhrstates, buildInfo->dstAccelerationStructure,
                       bindingBuffers);

    if (buildInfo->geometryCount == 0) {
      continue;
    }

    auto addBindingBuffer = [&bindingBuffers](VkDeviceAddress deviceAddress, uint64_t offset) {
      if (deviceAddress != 0) {
        auto buffer = findBufferFromDeviceAddress(deviceAddress + offset);
        insertStateIfFound(SD()._bufferstates, buffer, bindingBuffers);
      }
    };

    for (uint32_t geom = 0; geom < buildInfo->geometryCount; ++geom) {
      const VkAccelerationStructureGeometryKHR* pGeometry = (buildInfo->pGeometries != nullptr)
                                                                ? (&buildInfo->pGeometries[geom])
                                                                : (buildInfo->ppGeometries[geom]);
      const auto& buildRangeInfo = buildRangeInfos[geom];

      switch (pGeometry->geometryType) {
      case VK_GEOMETRY_TYPE_TRIANGLES_KHR: {
        const auto& trianglesData = pGeometry->geometry.triangles;

        if (trianglesData.indexType != VK_INDEX_TYPE_NONE_KHR) {
          // Index buffer
          addBindingBuffer(trianglesData.indexData.deviceAddress, buildRangeInfo.primitiveOffset);
          // Vertex buffer
          addBindingBuffer(trianglesData.vertexData.deviceAddress,
                           std::max(0u, trianglesData.maxVertex - 1) * trianglesData.vertexStride);
        } else {
          // Vertex buffer
          addBindingBuffer(trianglesData.vertexData.deviceAddress,
                           buildRangeInfo.primitiveOffset +
                               trianglesData.vertexStride * buildRangeInfo.firstVertex);
        }

        // Transform buffer
        addBindingBuffer(trianglesData.transformData.deviceAddress, buildRangeInfo.transformOffset);
      } break;

      case VK_GEOMETRY_TYPE_AABBS_KHR: {
        // AABBs data buffer
        addBindingBuffer(pGeometry->geometry.aabbs.data.deviceAddress,
                         buildRangeInfo.primitiveOffset);
      } break;

      case VK_GEOMETRY_TYPE_INSTANCES_KHR: {
        // Instance data buffer
        addBindingBuffer(pGeometry->geometry.instances.data.deviceAddress,
                         buildRangeInfo.primitiveOffset);
      } break;

      default:
        throw std::runtime_error("Unknown geometry type provided!");
        break;
      }
    }
  }
}

inline void vkCopyAccelerationStructureKHR_SD(VkResult return_value,
                                              VkDevice device,
                                              VkDeferredOperationKHR deferredOperation,
                                              const VkCopyAccelerationStructureInfoKHR* pInfo) {
  throw std::runtime_error("Ray tracing operations on host are not yet supported!");
}

inline void vkCmdCopyAccelerationStructureKHR_SD(VkCommandBuffer cmdBuf,
                                                 const VkCopyAccelerationStructureInfoKHR* pInfo) {
  if (Configurator::IsRecorder() && CGits::Instance().apis.Iface3D().CfgRec_IsSubcapture()) {
    const auto& srcAccelerationStructureState = SD()._accelerationstructurekhrstates[pInfo->src];
    auto& dstAccelerationStructureState = SD()._accelerationstructurekhrstates[pInfo->dst];

    dstAccelerationStructureState->buildInfo.reset();
    dstAccelerationStructureState->updateInfo.reset();
    dstAccelerationStructureState->copyInfo.reset(new CAccelerationStructureKHRState::CCopyInfo(
        pInfo, srcAccelerationStructureState, getCommandExecutionSide(cmdBuf)));
    dstAccelerationStructureState->buildSizeInfo = srcAccelerationStructureState->buildSizeInfo;
  }
}

inline void vkGetAccelerationStructureDeviceAddressUnifiedGITS_SD(
    VkDeviceAddress return_value,
    VkDevice device,
    const VkAccelerationStructureDeviceAddressInfoKHR* pInfo) {
  auto& accelerationStructureState =
      SD()._accelerationstructurekhrstates[pInfo->accelerationStructure];

  accelerationStructureState->deviceAddress = return_value;

  if (Configurator::IsRecorder()) {
    CAccelerationStructureKHRState::deviceAddresses[return_value] = pInfo->accelerationStructure;
  }
}

// Queue submit

namespace {
inline void vkQueueSubmit_setImageLayout(std::shared_ptr<CCommandBufferState>& commandBufferState,
                                         uint32_t queueFamilyIndex) {
  if ((isSubcaptureBeforeRestorationPhase() &&
       Configurator::Get().vulkan.recorder.crossPlatformStateRestoration.images) ||
      captureRenderPasses() || captureRenderPassesResources()) {

    for (const auto& imageLayoutAfterSubmit : commandBufferState->imageLayoutAfterSubmit) {
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
    for (const auto& resetQueryAfterSubmit : commandBufferState->resetQueriesAfterSubmit) {
      auto& queryPoolState = SD()._querypoolstates[resetQueryAfterSubmit.first];

      for (auto queryIndex : resetQueryAfterSubmit.second) {
        queryPoolState->resetQueries[queryIndex] = true;
        queryPoolState->usedQueries[queryIndex] = false;
      }
    }

    for (const auto& usedQueryAfterSubmit : commandBufferState->usedQueriesAfterSubmit) {
      auto& queryPoolState = SD()._querypoolstates[usedQueryAfterSubmit.first];

      for (auto queryIndex : usedQueryAfterSubmit.second) {
        queryPoolState->usedQueries[queryIndex] = true;
      }
    }
  }
}

inline void vkQueueSubmit_setTimestamps(std::shared_ptr<CCommandBufferState>& commandBufferState) {
  if (isSubcaptureBeforeRestorationPhase()) {
    for (uint32_t i = 0; i < commandBufferState->touchedResources.size(); ++i) {
      const auto& touchedResource = commandBufferState->touchedResources[i];
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
  if (Configurator::Get().vulkan.player.skipNonDeterministicImages) {
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
                             VkFence fence,
                             bool stateRestore = false) {
  if (Configurator::IsRecorder() || captureRenderPasses() || captureRenderPassesResources()) {
    if (pSubmits != NULL) {
      for (uint32_t s = 0; s < submitCount; ++s) {
        for (uint32_t c = 0; c < pSubmits[s].commandBufferCount; ++c) {
          auto& commandBufferState = SD()._commandbufferstates[pSubmits[s].pCommandBuffers[c]];

          commandBufferState->submitted = true;

          for (auto& secondaryCommandBufferState :
               commandBufferState->secondaryCommandBuffersStateStoreList) {
            secondaryCommandBufferState.second->submitted = true;
          }
          if (!stateRestore) {
            for (const auto& eventState : commandBufferState->eventStatesAfterSubmit) {
              SD()._eventstates[eventState.first]->eventUsed = eventState.second;
            }
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

        if (!stateRestore) {
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
            for (uint32_t t = 0; t < pSubmits[s].signalSemaphoreCount; ++t) {
              auto semaphore = pSubmits[s].pSignalSemaphores[t];

              auto it = SD()._semaphorestates.find(semaphore);
              if ((it != SD()._semaphorestates.end()) && (it->second->isTimeline)) {
                it->second->timelineSemaphoreValue =
                    timelineSemaphoreSubmitInfo->pSignalSemaphoreValues[t];
              }
            }
          }
        }
      }
    }
  }

  if (pSubmits != NULL && !stateRestore) {
    for (uint32_t s = 0; s < submitCount; ++s) {
      for (uint32_t c = 0; c < pSubmits[s].commandBufferCount; ++c) {
        const auto& cmdBufferState = SD()._commandbufferstates[pSubmits[s].pCommandBuffers[c]];

        for (const auto& eventState : cmdBufferState->eventStatesAfterSubmit) {
          SD()._eventstates[eventState.first]->eventUsed = eventState.second;
        }
      }
    }
  }

  if (VK_NULL_HANDLE != fence && !stateRestore) {
    SD()._fencestates[fence]->fenceUsed = true;
  }
}

inline void vkQueueSubmit2_SD(VkResult return_value,
                              VkQueue queue,
                              uint32_t submitCount,
                              const VkSubmitInfo2* pSubmits,
                              VkFence fence,
                              bool stateRestore = false) {
  if (Configurator::IsRecorder() || captureRenderPasses() || captureRenderPassesResources()) {
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
          if (!stateRestore) {
            for (const auto& eventState : commandBufferState->eventStatesAfterSubmit) {
              SD()._eventstates[eventState.first]->eventUsed = eventState.second;
            }
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

        if (!stateRestore) {
          // Semaphore state
          for (uint32_t i = 0; i < pSubmits[s].waitSemaphoreInfoCount; i++) {
            SD()._semaphorestates[pSubmits[s].pWaitSemaphoreInfos[i].semaphore]->semaphoreUsed =
                false;
          }

          for (uint32_t i = 0; i < pSubmits[s].signalSemaphoreInfoCount; i++) {
            const auto& signalInfo = pSubmits[s].pSignalSemaphoreInfos[i];
            auto& semaphoreState = SD()._semaphorestates[signalInfo.semaphore];

            semaphoreState->semaphoreUsed = true;
            if (semaphoreState->isTimeline) {
              semaphoreState->timelineSemaphoreValue = signalInfo.value;
            }
          }
        }
      }
    }
  }

  if (pSubmits != NULL && !stateRestore) {
    for (uint32_t s = 0; s < submitCount; ++s) {
      for (uint32_t c = 0; c < pSubmits[s].commandBufferInfoCount; ++c) {
        const auto& cmdBufferState =
            SD()._commandbufferstates[pSubmits[s].pCommandBufferInfos[c].commandBuffer];

        for (const auto& eventState : cmdBufferState->eventStatesAfterSubmit) {
          SD()._eventstates[eventState.first]->eventUsed = eventState.second;
        }
      }
    }
  }

  if (VK_NULL_HANDLE != fence && !stateRestore) {
    SD()._fencestates[fence]->fenceUsed = true;
  }
}

inline void vkQueueSubmit2KHR_SD(VkResult return_value,
                                 VkQueue queue,
                                 uint32_t submitCount,
                                 const VkSubmitInfo2* pSubmits,
                                 VkFence fence,
                                 bool stateRestore = false) {
  vkQueueSubmit2_SD(return_value, queue, submitCount, pSubmits, fence, stateRestore);
}

//RenderPass helper functions
namespace {
inline void vkEndRenderPass_setImageLayout(
    const std::shared_ptr<CCommandBufferState>& commandBufferState) {
  if (!Configurator::Get().vulkan.player.captureVulkanRenderPasses.empty() ||
      !Configurator::Get().vulkan.player.captureVulkanDraws.empty()) {
    for (const auto& imageLayoutAfterSubmit : commandBufferState->imageLayoutAfterSubmit) {
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
  if (Configurator::Get().vulkan.player.skipNonDeterministicImages &&
      commandBufferState->beginRenderPassesList.size()) {
    const auto& framebufferState =
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
    if (!Configurator::Get().vulkan.player.captureVulkanRenderPasses.empty() ||
        !Configurator::Get().vulkan.player.captureVulkanDraws.empty()) {
      for (const auto& obj : commandBufferState->clearedImages) {
        SD().nonDeterministicImages.erase(obj);
      }
    }
  }
}
} // namespace

// Command buffer recording commands

inline void vkCmdBeginRenderPass_SD(VkCommandBuffer cmdBuf,
                                    const VkRenderPassBeginInfo* pRenderPassBegin,
                                    VkSubpassContents contents) {
  if (pRenderPassBegin == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  auto beginRenderPassState = std::make_shared<CCommandBufferState::CBeginRenderPass>(
      pRenderPassBegin, &contents, SD()._renderpassstates[pRenderPassBegin->renderPass],
      SD()._framebufferstates[pRenderPassBegin->framebuffer]);
  SD()._commandbufferstates[cmdBuf]->beginRenderPassesList.push_back(beginRenderPassState);
  if (Configurator::IsRecorder() || captureRenderPasses() || captureRenderPassesResources()) {
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
        const auto& state = SD()._renderpassstates[pRenderPassBegin->renderPass];
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

        const auto& state = SD()._renderpassstates[pRenderPassBegin->renderPass];
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
  if (Configurator::IsRecorder()) {
    if (!(SD()._framebufferstates[pRenderPassBegin->framebuffer]
              ->framebufferCreateInfoData.Value()
              ->flags &
          VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT)) {
      for (const auto& imageViewState :
           SD()._framebufferstates[pRenderPassBegin->framebuffer]->imageViewStateStoreList) {
        const auto& imageState = imageViewState->imageStateStore;

        if (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase()) {
          SD().bindingImages[cmdBuf].insert(imageState);
        }

        if ((!Configurator::Get().vulkan.recorder.memorySegmentSize &&
             !Configurator::Get().vulkan.recorder.shadowMemory) ||
            !imageState->binding) {
          continue;
        }

        VkMemoryRequirements memRequirements = {};
        drvVk.vkGetImageMemoryRequirements(imageState->deviceStateStore->deviceHandle,
                                           imageState->imageHandle, &memRequirements);

        //TODO : call vkGetImageSubresourceLayout when tiling is Linear

        const auto memory = imageState->binding->deviceMemoryStateStore->deviceMemoryHandle;
        const auto offset = imageState->binding->memoryOffset;
        SD().updatedMemoryInCmdBuffer[cmdBuf].AddToMap(memory, offset, memRequirements.size);
      }
    } else {
      for (uint32_t i = 0; i < beginRenderPassState->imageViewStateStoreListKHR.size(); i++) {
        const auto& imageState =
            beginRenderPassState->imageViewStateStoreListKHR[i]->imageStateStore;

        if (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase()) {
          SD().bindingImages[cmdBuf].insert(imageState);
        }

        if ((Configurator::Get().vulkan.recorder.memorySegmentSize &&
             !Configurator::Get().vulkan.recorder.shadowMemory) ||
            !imageState->binding) {
          continue;
        }

        VkMemoryRequirements memRequirements = {};
        drvVk.vkGetImageMemoryRequirements(imageState->deviceStateStore->deviceHandle,
                                           imageState->imageHandle, &memRequirements);

        //TODO : call vkGetImageSubresourceLayout when tiling is Linear

        const auto memory = imageState->binding->deviceMemoryStateStore->deviceMemoryHandle;
        const auto offset = imageState->binding->memoryOffset;
        SD().updatedMemoryInCmdBuffer[cmdBuf].AddToMap(memory, offset, memRequirements.size);
      }
    }
  }
}

inline void vkCmdBeginRenderPass2_SD(VkCommandBuffer cmdBuf,
                                     const VkRenderPassBeginInfo* pRenderPassBegin,
                                     const VkSubpassBeginInfo* pSubpassBeginInfo) {
  if (pSubpassBeginInfo == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  vkCmdBeginRenderPass_SD(cmdBuf, pRenderPassBegin, pSubpassBeginInfo->contents);
}

inline void vkCmdBeginRenderPass2KHR_SD(VkCommandBuffer cmdBuf,
                                        const VkRenderPassBeginInfo* pRenderPassBegin,
                                        const VkSubpassBeginInfo* pSubpassBeginInfo) {
  if (pSubpassBeginInfo == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  vkCmdBeginRenderPass_SD(cmdBuf, pRenderPassBegin, pSubpassBeginInfo->contents);
}

inline void vkCmdEndRenderPass_SD(VkCommandBuffer cmdBuf) {
  if (((Configurator::Get().common.recorder.enabled) && (isSubcaptureBeforeRestorationPhase()) &&
       (Configurator::Get().vulkan.recorder.crossPlatformStateRestoration.images)) ||
      (captureRenderPasses() || captureRenderPassesResources())) {
    auto& commandBufferState = SD()._commandbufferstates[cmdBuf];

    if (commandBufferState->beginRenderPassesList.size()) {
      const auto& renderPassState =
          commandBufferState->beginRenderPassesList.back()->renderPassStateStore;
      const auto& framebufferState =
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
        const auto& imageLayoutData = imageState->currentLayout;
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

inline void vkCmdEndRenderPass2_SD(VkCommandBuffer cmdBuf,
                                   const VkSubpassEndInfo* pSubpassEndInfo) {
  vkCmdEndRenderPass_SD(cmdBuf);
}

inline void vkCmdEndRenderPass2KHR_SD(VkCommandBuffer cmdBuf,
                                      const VkSubpassEndInfo* pSubpassEndInfo) {
  vkCmdEndRenderPass_SD(cmdBuf);
}

inline void vkCmdBeginRendering_SD(VkCommandBuffer cmdBuf, const VkRenderingInfo* pRenderingInfo) {
  auto beginRenderPassState =
      std::make_shared<CCommandBufferState::CBeginRenderPass>(pRenderingInfo);
  SD()._commandbufferstates[cmdBuf]->beginRenderPassesList.push_back(beginRenderPassState);

  if (Configurator::IsRecorder() || captureRenderPasses() || captureRenderPassesResources()) {

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

  if (Configurator::IsRecorder()) {
    for (uint32_t i = 0; i < beginRenderPassState->imageViewStateStoreListKHR.size(); i++) {
      const auto& imageState = beginRenderPassState->imageViewStateStoreListKHR[i]->imageStateStore;

      if (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase()) {
        SD().bindingImages[cmdBuf].insert(imageState);
      }

      if ((!Configurator::Get().vulkan.recorder.memorySegmentSize &&
           !Configurator::Get().vulkan.recorder.shadowMemory) ||
          !imageState->binding) {
        continue;
      }

      VkMemoryRequirements memRequirements = {};
      drvVk.vkGetImageMemoryRequirements(imageState->deviceStateStore->deviceHandle,
                                         imageState->imageHandle, &memRequirements);

      //TODO : call vkGetImageSubresourceLayout when tiling is Linear

      const auto memory = imageState->binding->deviceMemoryStateStore->deviceMemoryHandle;
      const auto offset = imageState->binding->memoryOffset;
      SD().updatedMemoryInCmdBuffer[cmdBuf].AddToMap(memory, offset, memRequirements.size);
    }
  }
}

inline void vkCmdBeginRenderingKHR_SD(VkCommandBuffer cmdBuf,
                                      const VkRenderingInfo* pRenderingInfo) {
  vkCmdBeginRendering_SD(cmdBuf, pRenderingInfo);
}

inline void vkCmdEndRendering_SD(VkCommandBuffer cmdBuf) {
  auto& commandBufferState = SD()._commandbufferstates[cmdBuf];
  vkEndRenderPass_setImageLayout(commandBufferState);
  vkEndRenderPass_updateNonDeterministicImages(commandBufferState);
}

inline void vkCmdEndRenderingKHR_SD(VkCommandBuffer cmdBuf) {
  vkCmdEndRendering_SD(cmdBuf);
}

namespace {

void BindVertexBuffers_SDHelper(VkCommandBuffer cmdBuf,
                                uint32_t bindingCount,
                                const VkBuffer* pBuffers) {
  if (Configurator::IsRecorder() &&
      (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase())) {
    auto& bindingBuffers = SD().bindingBuffers[cmdBuf];

    for (uint32_t i = 0; i < bindingCount; ++i) {
      insertStateIfFound(SD()._bufferstates, pBuffers[i], bindingBuffers);
    }
  }
}

} // namespace

inline void vkCmdBindVertexBuffers_SD(VkCommandBuffer cmdBuf,
                                      uint32_t startBinding,
                                      uint32_t bindingCount,
                                      const VkBuffer* pBuffers,
                                      const VkDeviceSize* pOffsets) {
  BindVertexBuffers_SDHelper(cmdBuf, bindingCount, pBuffers);
}

inline void vkCmdBindVertexBuffers2_SD(VkCommandBuffer cmdBuf,
                                       uint32_t firstBinding,
                                       uint32_t bindingCount,
                                       const VkBuffer* pBuffers,
                                       const VkDeviceSize* pOffsets,
                                       const VkDeviceSize* pSizes,
                                       const VkDeviceSize* pStrides) {
  BindVertexBuffers_SDHelper(cmdBuf, bindingCount, pBuffers);
}

inline void vkCmdBindVertexBuffers2EXT_SD(VkCommandBuffer cmdBuf,
                                          uint32_t firstBinding,
                                          uint32_t bindingCount,
                                          const VkBuffer* pBuffers,
                                          const VkDeviceSize* pOffsets,
                                          const VkDeviceSize* pSizes,
                                          const VkDeviceSize* pStrides) {
  BindVertexBuffers_SDHelper(cmdBuf, bindingCount, pBuffers);
}

inline void vkCmdBindIndexBuffer_SD(VkCommandBuffer cmdBuf,
                                    VkBuffer buffer,
                                    VkDeviceSize offset,
                                    VkIndexType indexType) {
  if (Configurator::IsRecorder() &&
      (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase())) {
    insertStateIfFound(SD()._bufferstates, buffer, SD().bindingBuffers[cmdBuf]);
  }
}

inline void vkCmdBindTransformFeedbackBuffersEXT_SD(VkCommandBuffer cmdBuf,
                                                    uint32_t firstBinding,
                                                    uint32_t bindingCount,
                                                    const VkBuffer* pBuffers,
                                                    const VkDeviceSize* pOffsets,
                                                    const VkDeviceSize* pSizes) {
  if (Configurator::IsRecorder() &&
      (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase())) {
    for (uint32_t i = 0; i < bindingCount; ++i) {
      insertStateIfFound(SD()._bufferstates, pBuffers[i], SD().bindingBuffers[cmdBuf]);
    }
  }
}

inline void vkCmdBindPipeline_SD(VkCommandBuffer cmdBuf,
                                 VkPipelineBindPoint pipelineBindPoint,
                                 VkPipeline pipeline) {
  auto& commandBufferState = SD()._commandbufferstates[cmdBuf];
  commandBufferState->pipelineStateStoreList.emplace(pipeline, SD()._pipelinestates[pipeline]);
  commandBufferState->currentPipeline = pipeline;
  commandBufferState->currentPipelineBindPoint = pipelineBindPoint;
}

inline void vkCmdBindDescriptorSets_SD(VkCommandBuffer cmdBuf,
                                       VkPipelineBindPoint pipelineBindPoint,
                                       VkPipelineLayout layout,
                                       uint32_t firstSet,
                                       uint32_t descriptorSetCount,
                                       const VkDescriptorSet* pDescriptorSets,
                                       uint32_t dynamicOffsetCount,
                                       const uint32_t* pDynamicOffsets) {
  if (Configurator::IsRecorder() || captureRenderPassesResources()) {
    if (!pDescriptorSets) {
      return;
    }

    auto& commandBufferState = SD()._commandbufferstates[cmdBuf];
    for (unsigned int i = 0; i < descriptorSetCount; i++) {
      if (pDescriptorSets[i] == VK_NULL_HANDLE) {
        continue;
      }

      if (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase()) {
        const auto& descriptorSetState = SD()._descriptorsetstates[pDescriptorSets[i]];

        commandBufferState->descriptorSetStateStoreList.emplace(pDescriptorSets[i],
                                                                descriptorSetState);

        for (const auto& obj : descriptorSetState->descriptorBuffers) {
          SD().bindingBuffers[cmdBuf].insert(obj.second);
        }

        for (const auto& obj : descriptorSetState->descriptorImages) {
          SD().bindingImages[cmdBuf].insert(obj.second);
        }
      }
      for (const auto& obj :
           SD()._descriptorsetstates[pDescriptorSets[i]]->descriptorWriteBuffers) {
        commandBufferState->touchedResources.emplace_back((uint64_t)obj.second.second, false);
        commandBufferState->resourceWriteBuffers[obj.second.second] = obj.second.first;
      }
      for (const auto& obj : SD()._descriptorsetstates[pDescriptorSets[i]]->descriptorWriteImages) {
        commandBufferState->touchedResources.emplace_back((uint64_t)obj.second.second, true);
        commandBufferState->resourceWriteImages[obj.second.second] = obj.second.first;
      }
      if (Configurator::Get().vulkan.recorder.memorySegmentSize ||
          Configurator::Get().vulkan.recorder.shadowMemory) {
        for (const auto& binding :
             SD()._descriptorsetstates[pDescriptorSets[i]]->descriptorMapMemory) {
          for (const auto& mem : binding.second) {
            for (const auto& obj : mem.second.getIntervals()) {
              SD().updatedMemoryInCmdBuffer[cmdBuf].AddToMap(mem.first, obj.first,
                                                             obj.second - obj.first);
            }
          }
        }
      }
    }
  }
}

inline void vkCmdPushDescriptorSetKHR_SD(VkCommandBuffer cmdBuf,
                                         VkPipelineBindPoint pipelineBindPoint,
                                         VkPipelineLayout layout,
                                         uint32_t set,
                                         uint32_t descriptorWriteCount,
                                         const VkWriteDescriptorSet* pDescriptorWrites) {
  if (!Configurator::IsRecorder()) {
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
        break;
      case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
      case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
      case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
      case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: {
        if (!pDescriptorWrites[i].pImageInfo) {
          continue;
        }

        const auto it = SD()._imageviewstates.find(pDescriptorWrites[i].pImageInfo[j].imageView);
        if (it != SD()._imageviewstates.end()) {
          const auto& imageState = it->second->imageStateStore;

          if (VK_DESCRIPTOR_TYPE_STORAGE_IMAGE == descriptorType) {
            SD()._commandbufferstates[cmdBuf]->touchedResources.emplace_back(
                (uint64_t)imageState->imageHandle, true);
            SD()._commandbufferstates[cmdBuf]->resourceWriteImages[imageState->imageHandle] =
                VULKAN_STORAGE_IMAGE;
          }

          if (updateOnlyUsedMemory()) {
            SD().bindingImages[cmdBuf].insert(imageState);

            if ((Configurator::Get().vulkan.recorder.memorySegmentSize ||
                 Configurator::Get().vulkan.recorder.shadowMemory) &&
                (VK_DESCRIPTOR_TYPE_STORAGE_IMAGE == descriptorType) && (imageState->binding)) {
              VkMemoryRequirements memRequirements = {};
              drvVk.vkGetImageMemoryRequirements(imageState->deviceStateStore->deviceHandle,
                                                 imageState->imageHandle, &memRequirements);

              //TODO : call vkGetImageSubresourceLayout when tiling is Linear

              const auto memory = imageState->binding->deviceMemoryStateStore->deviceMemoryHandle;
              const auto offset = imageState->binding->memoryOffset;
              SD().updatedMemoryInCmdBuffer[cmdBuf].AddToMap(memory, offset, memRequirements.size);
            }
          }
        }
      } break;
      case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
      case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: {
        if (!pDescriptorWrites[i].pTexelBufferView) {
          continue;
        }

        const auto it = SD()._bufferviewstates.find(pDescriptorWrites[i].pTexelBufferView[j]);
        if (it != SD()._bufferviewstates.end()) {
          const auto& bufferState = it->second->bufferStateStore;

          if (VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER == descriptorType) {
            const auto cmdBufIt = SD()._commandbufferstates.find(cmdBuf);
            if (cmdBufIt != SD()._commandbufferstates.end()) {
              cmdBufIt->second->touchedResources.emplace_back((uint64_t)bufferState->bufferHandle,
                                                              false);
              cmdBufIt->second->resourceWriteBuffers[bufferState->bufferHandle] =
                  VULKAN_STORAGE_TEXEL_BUFFER;
            }
          }

          if (updateOnlyUsedMemory()) {
            SD().bindingBuffers[cmdBuf].insert(bufferState);

            if ((Configurator::Get().vulkan.recorder.memorySegmentSize ||
                 Configurator::Get().vulkan.recorder.shadowMemory) &&
                (VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER == descriptorType) &&
                (bufferState->binding)) {
              const auto* pViewCreateInfo = it->second->bufferViewCreateInfoData.Value();
              const auto memory = bufferState->binding->deviceMemoryStateStore->deviceMemoryHandle;
              const auto offset = bufferState->binding->memoryOffset + pViewCreateInfo->offset;
              const auto size =
                  (pViewCreateInfo->range == 0xFFFFFFFFFFFFFFFF)
                      ? (bufferState->bufferCreateInfoData.Value()->size - pViewCreateInfo->offset)
                      : (pViewCreateInfo->range);
              SD().updatedMemoryInCmdBuffer[cmdBuf].AddToMap(memory, offset, size);
            }
          }
        }
      } break;
      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
      case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
      case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: {
        if (!pDescriptorWrites[i].pBufferInfo) {
          continue;
        }

        const auto& bufferInfo = pDescriptorWrites[i].pBufferInfo[j];

        const auto it = SD()._bufferstates.find(bufferInfo.buffer);
        if (it != SD()._bufferstates.end()) {
          const auto& bufferState = it->second;

          if ((VK_DESCRIPTOR_TYPE_STORAGE_BUFFER == descriptorType) ||
              (VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC == descriptorType)) {
            SD()._commandbufferstates[cmdBuf]->touchedResources.emplace_back(
                (uint64_t)bufferState->bufferHandle, false);
            SD()._commandbufferstates[cmdBuf]->resourceWriteBuffers[bufferState->bufferHandle] =
                VULKAN_STORAGE_BUFFER_DYNAMIC;
          }

          if (updateOnlyUsedMemory()) {
            SD().bindingBuffers[cmdBuf].insert(bufferState);

            if ((Configurator::Get().vulkan.recorder.memorySegmentSize ||
                 Configurator::Get().vulkan.recorder.shadowMemory) &&
                ((VK_DESCRIPTOR_TYPE_STORAGE_BUFFER == descriptorType) ||
                 (VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC == descriptorType))) {
              if (bufferState->binding) {
                const auto memory =
                    bufferState->binding->deviceMemoryStateStore->deviceMemoryHandle;
                const auto offset = bufferState->binding->memoryOffset + bufferInfo.offset;
                const auto size =
                    (bufferInfo.range == 0xFFFFFFFFFFFFFFFF)
                        ? (bufferState->bufferCreateInfoData.Value()->size - bufferInfo.offset)
                        : (bufferInfo.range);
                SD().updatedMemoryInCmdBuffer[cmdBuf].AddToMap(memory, offset, size);
              }
            }
          }
        }
      } break;
      default:
        Log(TRACE) << "Not handled VkDescriptorType enumeration: " + std::to_string(descriptorType);
        break;
      }
    }
  }
}

inline void vkCmdDraw_SD(VkCommandBuffer cmdBuf, uint32_t, uint32_t, uint32_t, uint32_t) {
  printShaderHashes(SD()._commandbufferstates[cmdBuf]->currentPipeline);
}

inline void vkCmdDrawIndexed_SD(
    VkCommandBuffer cmdBuf, uint32_t, uint32_t, uint32_t, int32_t, uint32_t) {
  printShaderHashes(SD()._commandbufferstates[cmdBuf]->currentPipeline);
}

inline void vkCmdDrawIndexedIndirect_SD(VkCommandBuffer cmdBuf,
                                        VkBuffer buffer,
                                        VkDeviceSize offset,
                                        uint32_t drawCount,
                                        uint32_t stride) {
  if (Configurator::IsRecorder() &&
      (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase())) {
    insertStateIfFound(SD()._bufferstates, buffer, SD().bindingBuffers[cmdBuf]);
  }
  printShaderHashes(SD()._commandbufferstates[cmdBuf]->currentPipeline);
}

inline void vkCmdDrawIndirect_SD(VkCommandBuffer cmdBuf,
                                 VkBuffer buffer,
                                 VkDeviceSize offset,
                                 uint32_t drawCount,
                                 uint32_t stride) {
  if (Configurator::IsRecorder() &&
      (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase())) {
    insertStateIfFound(SD()._bufferstates, buffer, SD().bindingBuffers[cmdBuf]);
  }
  printShaderHashes(SD()._commandbufferstates[cmdBuf]->currentPipeline);
}

inline void vkCmdDrawIndirectCountKHR_SD(VkCommandBuffer cmdBuf,
                                         VkBuffer buffer,
                                         VkDeviceSize offset,
                                         VkBuffer countBuffer,
                                         VkDeviceSize countBufferOffset,
                                         uint32_t maxDrawCount,
                                         uint32_t stride) {
  if (Configurator::IsRecorder() &&
      (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase())) {
    auto& bindingBuffers = SD().bindingBuffers[cmdBuf];

    insertStateIfFound(SD()._bufferstates, buffer, bindingBuffers);
    insertStateIfFound(SD()._bufferstates, countBuffer, bindingBuffers);
  }
  printShaderHashes(SD()._commandbufferstates[cmdBuf]->currentPipeline);
}

inline void vkCmdDrawIndexedIndirectCount_SD(VkCommandBuffer cmdBuf,
                                             VkBuffer buffer,
                                             VkDeviceSize offset,
                                             VkBuffer countBuffer,
                                             VkDeviceSize countBufferOffset,
                                             uint32_t maxDrawCount,
                                             uint32_t stride) {
  if (Configurator::IsRecorder() &&
      (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase())) {
    auto& bindingBuffers = SD().bindingBuffers[cmdBuf];

    insertStateIfFound(SD()._bufferstates, buffer, bindingBuffers);
    insertStateIfFound(SD()._bufferstates, countBuffer, bindingBuffers);
  }
  printShaderHashes(SD()._commandbufferstates[cmdBuf]->currentPipeline);
}

inline void vkCmdDrawIndexedIndirectCountKHR_SD(VkCommandBuffer cmdBuf,
                                                VkBuffer buffer,
                                                VkDeviceSize offset,
                                                VkBuffer countBuffer,
                                                VkDeviceSize countBufferOffset,
                                                uint32_t maxDrawCount,
                                                uint32_t stride) {
  if (Configurator::IsRecorder() &&
      (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase())) {
    auto& bindingBuffers = SD().bindingBuffers[cmdBuf];

    insertStateIfFound(SD()._bufferstates, buffer, bindingBuffers);
    insertStateIfFound(SD()._bufferstates, countBuffer, bindingBuffers);
  }
  printShaderHashes(SD()._commandbufferstates[cmdBuf]->currentPipeline);
}

inline void vkCmdDrawMeshTasksEXT_SD(VkCommandBuffer cmdBuf,
                                     uint32_t groupCountX,
                                     uint32_t groupCountY,
                                     uint32_t groupCountZ) {
  printShaderHashes(SD()._commandbufferstates[cmdBuf]->currentPipeline);
}

inline void vkCmdDrawMeshTasksNV_SD(VkCommandBuffer cmdBuf,
                                    uint32_t taskCount,
                                    uint32_t firstTask) {
  printShaderHashes(SD()._commandbufferstates[cmdBuf]->currentPipeline);
}

inline void vkCmdDrawMeshTasksIndirectEXT_SD(VkCommandBuffer cmdBuf,
                                             VkBuffer buffer,
                                             VkDeviceSize offset,
                                             uint32_t drawCount,
                                             uint32_t stride) {
  if (isRecorder() && (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase())) {
    insertStateIfFound(SD()._bufferstates, buffer, SD().bindingBuffers[cmdBuf]);
  }
  printShaderHashes(SD()._commandbufferstates[cmdBuf]->currentPipeline);
}

inline void vkCmdDrawMeshTasksIndirectNV_SD(VkCommandBuffer cmdBuf,
                                            VkBuffer buffer,
                                            VkDeviceSize offset,
                                            uint32_t drawCount,
                                            uint32_t stride) {
  vkCmdDrawMeshTasksIndirectEXT_SD(cmdBuf, buffer, offset, drawCount, stride);
}

inline void vkCmdDrawMeshTasksIndirectCountEXT_SD(VkCommandBuffer cmdBuf,
                                                  VkBuffer buffer,
                                                  VkDeviceSize offset,
                                                  VkBuffer countBuffer,
                                                  VkDeviceSize countBufferOffset,
                                                  uint32_t maxDrawCount,
                                                  uint32_t stride) {
  if (isRecorder() && (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase())) {
    auto& bindingBuffers = SD().bindingBuffers[cmdBuf];

    insertStateIfFound(SD()._bufferstates, buffer, bindingBuffers);
    insertStateIfFound(SD()._bufferstates, countBuffer, bindingBuffers);
  }
  printShaderHashes(SD()._commandbufferstates[cmdBuf]->currentPipeline);
}

inline void vkCmdDrawMeshTasksIndirectCountNV_SD(VkCommandBuffer cmdBuf,
                                                 VkBuffer buffer,
                                                 VkDeviceSize offset,
                                                 VkBuffer countBuffer,
                                                 VkDeviceSize countBufferOffset,
                                                 uint32_t maxDrawCount,
                                                 uint32_t stride) {
  vkCmdDrawMeshTasksIndirectCountEXT_SD(cmdBuf, buffer, offset, countBuffer, countBufferOffset,
                                        maxDrawCount, stride);
}

inline void vkCmdDispatch_SD(VkCommandBuffer cmdBuf, uint32_t, uint32_t, uint32_t) {
  printShaderHashes(SD()._commandbufferstates[cmdBuf]->currentPipeline);
}

inline void vkCmdDispatchIndirect_SD(VkCommandBuffer cmdBuf, VkBuffer buffer, VkDeviceSize) {
  if (Configurator::IsRecorder() &&
      (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase())) {
    insertStateIfFound(SD()._bufferstates, buffer, SD().bindingBuffers[cmdBuf]);
  }
  printShaderHashes(SD()._commandbufferstates[cmdBuf]->currentPipeline);
}

inline void vkCmdTraceRaysIndirectKHR_SD(
    VkCommandBuffer cmdBuf,
    const VkStridedDeviceAddressRegionKHR* pRaygenShaderBindingTable,
    const VkStridedDeviceAddressRegionKHR* pMissShaderBindingTable,
    const VkStridedDeviceAddressRegionKHR* pHitShaderBindingTable,
    const VkStridedDeviceAddressRegionKHR* pCallableShaderBindingTable,
    VkDeviceAddress indirectDeviceAddress) {
  if (Configurator::IsRecorder() &&
      (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase())) {
    auto& bindingBuffers = SD().bindingBuffers[cmdBuf];

    auto addBindingBuffer = [&bindingBuffers](const VkStridedDeviceAddressRegionKHR* SBTtable) {
      if ((SBTtable != nullptr) && (SBTtable->deviceAddress != 0)) {
        auto buffer = findBufferFromDeviceAddress(SBTtable->deviceAddress);
        insertStateIfFound(SD()._bufferstates, buffer, bindingBuffers);
      }
    };

    addBindingBuffer(pRaygenShaderBindingTable);
    addBindingBuffer(pMissShaderBindingTable);
    addBindingBuffer(pHitShaderBindingTable);
    addBindingBuffer(pCallableShaderBindingTable);
    {
      auto buffer = findBufferFromDeviceAddress(indirectDeviceAddress);
      insertStateIfFound(SD()._bufferstates, buffer, bindingBuffers);
    }
  }

  printShaderHashes(SD()._commandbufferstates[cmdBuf]->currentPipeline);
}

inline void vkCmdTraceRaysKHR_SD(VkCommandBuffer cmdBuf,
                                 const VkStridedDeviceAddressRegionKHR* pRaygenShaderBindingTable,
                                 const VkStridedDeviceAddressRegionKHR* pMissShaderBindingTable,
                                 const VkStridedDeviceAddressRegionKHR* pHitShaderBindingTable,
                                 const VkStridedDeviceAddressRegionKHR* pCallableShaderBindingTable,
                                 uint32_t /* width */,
                                 uint32_t /* height */,
                                 uint32_t /* depth */) {
  if (Configurator::IsRecorder() &&
      (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase())) {
    auto& bindingBuffers = SD().bindingBuffers[cmdBuf];

    auto addBindingBuffer = [&bindingBuffers](const VkStridedDeviceAddressRegionKHR* SBTtable) {
      if ((SBTtable != nullptr) && (SBTtable->deviceAddress != 0)) {
        auto buffer = findBufferFromDeviceAddress(SBTtable->deviceAddress);
        insertStateIfFound(SD()._bufferstates, buffer, bindingBuffers);
      }
    };

    addBindingBuffer(pRaygenShaderBindingTable);
    addBindingBuffer(pMissShaderBindingTable);
    addBindingBuffer(pHitShaderBindingTable);
    addBindingBuffer(pCallableShaderBindingTable);
  }

  printShaderHashes(SD()._commandbufferstates[cmdBuf]->currentPipeline);
}

inline void vkCmdExecuteCommands_SD(VkCommandBuffer cmdBuf,
                                    uint32_t commandBufferCount,
                                    const VkCommandBuffer* pCommandBuffers) {
  if (Configurator::IsRecorder() ||
      (captureRenderPassesResources() ||
       !Configurator::Get().vulkan.player.captureVulkanRenderPasses.empty() ||
       Configurator::Get().vulkan.player.execCmdBuffsBeforeQueueSubmit)) {
    for (unsigned int i = 0; i < commandBufferCount; i++) {
      auto& primaryCommandBufferState = SD()._commandbufferstates[cmdBuf];
      const auto& secondaryCommandBufferState = SD()._commandbufferstates[pCommandBuffers[i]];

      if (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase()) {
        const auto& srcBindingImages = SD().bindingImages[pCommandBuffers[i]];
        SD().bindingImages[cmdBuf].insert(srcBindingImages.begin(), srcBindingImages.end());

        const auto& srcBindingBuffers = SD().bindingBuffers[pCommandBuffers[i]];
        SD().bindingBuffers[cmdBuf].insert(srcBindingBuffers.begin(), srcBindingBuffers.end());
      }

      primaryCommandBufferState->secondaryCommandBuffersStateStoreList[pCommandBuffers[i]] =
          secondaryCommandBufferState;

      // Track touched resources
      primaryCommandBufferState->touchedResources.insert(
          primaryCommandBufferState->touchedResources.end(),
          secondaryCommandBufferState->touchedResources.begin(),
          secondaryCommandBufferState->touchedResources.end());

      // Track query pool state
      for (const auto& queryPoolState : secondaryCommandBufferState->resetQueriesAfterSubmit) {
        primaryCommandBufferState->resetQueriesAfterSubmit[queryPoolState.first].insert(
            queryPoolState.second.begin(), queryPoolState.second.end());
      }

      for (const auto& queryPoolState : secondaryCommandBufferState->usedQueriesAfterSubmit) {
        primaryCommandBufferState->usedQueriesAfterSubmit[queryPoolState.first].insert(
            queryPoolState.second.begin(), queryPoolState.second.end());
      }

      // Track event state
      for (const auto& eventState : secondaryCommandBufferState->eventStatesAfterSubmit) {
        primaryCommandBufferState->eventStatesAfterSubmit[eventState.first] = eventState.second;
      }

      // Track image layout state
      for (const auto& imageLayoutAfterSubmitSecondaryCommandBuffer :
           secondaryCommandBufferState->imageLayoutAfterSubmit) {
        VkImage image = imageLayoutAfterSubmitSecondaryCommandBuffer.first;
        const auto& imageLayout = SD()._imagestates[image]->currentLayout;
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
      if (captureRenderPassesResources()) {
        for (const auto& obj : secondaryCommandBufferState->resourceWriteBuffers) {
          primaryCommandBufferState->resourceWriteBuffers[obj.first] = obj.second;
        }
        for (const auto& obj : secondaryCommandBufferState->resourceWriteImages) {
          primaryCommandBufferState->resourceWriteImages[obj.first] = obj.second;
        }
      }
      if (Configurator::Get().vulkan.recorder.memorySegmentSize ||
          Configurator::Get().vulkan.recorder.shadowMemory) {
        for (const auto& obj3 :
             SD().updatedMemoryInCmdBuffer[pCommandBuffers[i]].intervalMapMemory) {
          for (const auto& obj4 : obj3.second.getIntervals()) {
            SD().updatedMemoryInCmdBuffer[cmdBuf].AddToMap(obj3.first, obj4.first,
                                                           obj4.second - obj4.first);
          }
        }
      }
    }
  }
}

inline void vkCmdPipelineBarrier_SD(VkCommandBuffer cmdBuf,
                                    VkPipelineStageFlags srcStageMask,
                                    VkPipelineStageFlags dstStageMask,
                                    VkDependencyFlags dependencyFlags,
                                    uint32_t memoryBarrierCount,
                                    const VkMemoryBarrier* pMemoryBarriers,
                                    uint32_t bufferMemoryBarrierCount,
                                    const VkBufferMemoryBarrier* pBufferMemoryBarriers,
                                    uint32_t imageMemoryBarrierCount,
                                    const VkImageMemoryBarrier* pImageMemoryBarriers) {
  if (captureRenderPasses() || captureRenderPassesResources() ||
      (isRecorder() && ((updateOnlyUsedMemory()) || isSubcaptureBeforeRestorationPhase()))) {
    if (pBufferMemoryBarriers) {
      auto& bindingBuffers = SD().bindingBuffers[cmdBuf];
      for (unsigned int i = 0; i < bufferMemoryBarrierCount; i++) {
        insertStateIfFound(SD()._bufferstates, pBufferMemoryBarriers[i].buffer, bindingBuffers);
      }
    }

    if (!pImageMemoryBarriers) {
      return;
    }

    auto& bindingImages = SD().bindingImages[cmdBuf];
    for (unsigned int i = 0; i < imageMemoryBarrierCount; i++) {
      const auto it = SD()._imagestates.find(pImageMemoryBarriers[i].image);
      if (it == SD()._imagestates.end()) {
        continue;
      }

      const auto& imageState = it->second;

      bindingImages.insert(imageState);
      if (!CGits::Instance().IsStateRestoration() &&
          pImageMemoryBarriers[i].oldLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
        SD().nonDeterministicImages.insert(pImageMemoryBarriers[i].image);
      }

      if (captureRenderPasses() || captureRenderPassesResources() ||
          (isSubcaptureBeforeRestorationPhase() && crossPlatformStateRestoration())) {
        const auto& imageLayout = imageState->currentLayout;
        if (!imageLayout.size()) {
          continue;
        }

        auto& imageLayoutAfterSubmit = SD()._commandbufferstates[cmdBuf]
                                           ->imageLayoutAfterSubmit[pImageMemoryBarriers[i].image];
        if (!imageLayoutAfterSubmit.size()) {
          imageLayoutAfterSubmit.resize(imageLayout.size());
          for (uint32_t l = 0; l < imageLayout.size(); ++l) {
            imageLayoutAfterSubmit[l].resize(imageLayout[l].size(),
                                             {(VkImageLayout)-1, 0, VK_QUEUE_FAMILY_IGNORED});
          }
        }

        const auto& subresourceRange = pImageMemoryBarriers[i].subresourceRange;
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

inline void vkCmdPipelineBarrier2UnifiedGITS_SD(VkCommandBuffer cmdBuf,
                                                const VkDependencyInfo* pDependencyInfo) {
  if (!pDependencyInfo) {
    return;
  }

  if (captureRenderPasses() || captureRenderPassesResources() ||
      (isRecorder() && (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase()))) {
    if (pDependencyInfo->pBufferMemoryBarriers) {
      auto& bindingBuffers = SD().bindingBuffers[cmdBuf];
      for (unsigned int i = 0; i < pDependencyInfo->bufferMemoryBarrierCount; i++) {
        insertStateIfFound(SD()._bufferstates, pDependencyInfo->pBufferMemoryBarriers[i].buffer,
                           bindingBuffers);
      }
    }

    if (!pDependencyInfo->pImageMemoryBarriers) {
      return;
    }

    auto& bindingImages = SD().bindingImages[cmdBuf];
    for (unsigned int i = 0; i < pDependencyInfo->imageMemoryBarrierCount; i++) {
      const auto it = SD()._imagestates.find(pDependencyInfo->pImageMemoryBarriers[i].image);
      if (it == SD()._imagestates.end()) {
        continue;
      }

      const auto& imageState = it->second;
      bindingImages.insert(imageState);

      if (!CGits::Instance().IsStateRestoration() &&
          pDependencyInfo->pImageMemoryBarriers[i].oldLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
        SD().nonDeterministicImages.insert(pDependencyInfo->pImageMemoryBarriers[i].image);
      }

      if (captureRenderPasses() || captureRenderPassesResources() ||
          (isSubcaptureBeforeRestorationPhase() && crossPlatformStateRestoration())) {
        const auto& imageLayout = imageState->currentLayout;
        if (!imageLayout.size()) {
          continue;
        }

        auto& imageLayoutAfterSubmit =
            SD()._commandbufferstates[cmdBuf]
                ->imageLayoutAfterSubmit[pDependencyInfo->pImageMemoryBarriers[i].image];
        if (!imageLayoutAfterSubmit.size()) {
          imageLayoutAfterSubmit.resize(imageLayout.size());
          for (uint32_t l = 0; l < imageLayout.size(); ++l) {
            imageLayoutAfterSubmit[l].resize(imageLayout[l].size(),
                                             {(VkImageLayout)-1, 0, VK_QUEUE_FAMILY_IGNORED});
          }
        }

        const auto& subresourceRange = pDependencyInfo->pImageMemoryBarriers[i].subresourceRange;
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

inline void vkCmdPipelineBarrier2_SD(VkCommandBuffer cmdBuf,
                                     const VkDependencyInfo* pDependencyInfo) {
  vkCmdPipelineBarrier2UnifiedGITS_SD(cmdBuf, pDependencyInfo);
}

inline void vkCmdPipelineBarrier2KHR_SD(VkCommandBuffer cmdBuf,
                                        const VkDependencyInfo* pDependencyInfo) {
  vkCmdPipelineBarrier2UnifiedGITS_SD(cmdBuf, pDependencyInfo);
}

inline void vkCmdSetEvent_SD(VkCommandBuffer cmdBuf,
                             VkEvent event,
                             VkPipelineStageFlags stageMask) {
  SD()._commandbufferstates[cmdBuf]->eventStatesAfterSubmit[event] = true;
}

inline void vkCmdResetEvent_SD(VkCommandBuffer cmdBuf,
                               VkEvent event,
                               VkPipelineStageFlags stageMask) {
  SD()._commandbufferstates[cmdBuf]->eventStatesAfterSubmit[event] = false;
}

inline void vkCmdSetEvent2_SD(VkCommandBuffer cmdBuf,
                              VkEvent event,
                              const VkDependencyInfo* pDependencyInfo) {
  if (pDependencyInfo == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  SD()._commandbufferstates[cmdBuf]->eventStatesAfterSubmit[event] = true;

  // Not counting eventStatesAfterSubmit, a state tracking for the vkCmdPipelineBarrier2...()
  // functions is exactly the same as for the vkCmdSetEvent2() function. That's why, to
  // avoid code redundancy, a common vkCmdPipelineBarrier2UnifiedGITS_SD() is called.
  vkCmdPipelineBarrier2UnifiedGITS_SD(cmdBuf, pDependencyInfo);
}

inline void vkCmdSetEvent2KHR_SD(VkCommandBuffer cmdBuf,
                                 VkEvent event,
                                 const VkDependencyInfo* pDependencyInfo) {
  SD()._commandbufferstates[cmdBuf]->eventStatesAfterSubmit[event] = true;

  // Not counting eventStatesAfterSubmit, a state tracking for the vkCmdPipelineBarrier2...()
  // functions is exactly the same as for the vkCmdSetEvent2KHR() function. That's why, to
  // avoid code redundancy, a common vkCmdPipelineBarrier2UnifiedGITS_SD() is called.
  vkCmdPipelineBarrier2UnifiedGITS_SD(cmdBuf, pDependencyInfo);
}

inline void vkCmdResetQueryPool_SD(VkCommandBuffer cmdBuf,
                                   VkQueryPool queryPool,
                                   uint32_t firstQuery,
                                   uint32_t queryCount) {
  if (Configurator::IsRecorder()) {
    auto& resetQueryAfterSubmit =
        SD()._commandbufferstates[cmdBuf]->resetQueriesAfterSubmit[queryPool];
    auto& usedQueryAfterSubmit =
        SD()._commandbufferstates[cmdBuf]->usedQueriesAfterSubmit[queryPool];
    for (uint32_t i = firstQuery; i < queryCount; ++i) {
      resetQueryAfterSubmit.insert(i);
      usedQueryAfterSubmit.erase(i);
    }
  }
}

inline void vkCmdWriteTimestamp_SD(VkCommandBuffer cmdBuf,
                                   VkPipelineStageFlagBits pipelineStage,
                                   VkQueryPool queryPool,
                                   uint32_t query) {
  if (Configurator::IsRecorder()) {
    SD()._commandbufferstates[cmdBuf]->usedQueriesAfterSubmit[queryPool].insert(query);
  }
}

inline void vkCmdBeginQuery_SD(VkCommandBuffer cmdBuf,
                               VkQueryPool queryPool,
                               uint32_t query,
                               VkQueryControlFlags flags) {
  if (Configurator::IsRecorder()) {
    SD()._commandbufferstates[cmdBuf]->usedQueriesAfterSubmit[queryPool].insert(query);
  }
}

inline void vkCmdWriteAccelerationStructuresPropertiesKHR_SD(
    VkCommandBuffer cmdBuf,
    uint32_t accelerationStructureCount,
    const VkAccelerationStructureKHR* pAccelerationStructures,
    VkQueryType queryType,
    VkQueryPool queryPool,
    uint32_t firstQuery) {
  if (Configurator::IsRecorder()) {
    auto& usedQueries = SD()._commandbufferstates[cmdBuf]->usedQueriesAfterSubmit[queryPool];
    for (uint32_t i = firstQuery; i < accelerationStructureCount; ++i) {
      usedQueries.insert(i);
    }
  }
}

inline void vkCmdCopyQueryPoolResults_SD(VkCommandBuffer cmdBuf,
                                         VkQueryPool queryPool,
                                         uint32_t firstQuery,
                                         uint32_t queryCount,
                                         VkBuffer dstBuffer,
                                         VkDeviceSize dstOffset,
                                         VkDeviceSize stride,
                                         VkQueryResultFlags flags) {
  if (Configurator::IsRecorder()) {
    const auto it = SD()._bufferstates.find(dstBuffer);
    if (it == SD()._bufferstates.end()) {
      return;
    }

    const auto& bufferState = it->second;

    if (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase()) {
      SD().bindingBuffers[cmdBuf].insert(bufferState);
    }

    SD()._commandbufferstates[cmdBuf]->touchedResources.emplace_back((uint64_t)dstBuffer, false);

    if (Configurator::Get().vulkan.recorder.memorySegmentSize ||
        Configurator::Get().vulkan.recorder.shadowMemory) {

      if (bufferState->binding) {
        const auto memory = bufferState->binding->deviceMemoryStateStore->deviceMemoryHandle;
        const auto offset = bufferState->binding->memoryOffset + dstOffset;
        const auto size = bufferState->bufferCreateInfoData.Value()->size - dstOffset;
        SD().updatedMemoryInCmdBuffer[cmdBuf].AddToMap(memory, offset, size);
      }
    }
  }
}

inline void vkCmdUpdateBuffer_SD(VkCommandBuffer cmdBuf,
                                 VkBuffer dstBuffer,
                                 VkDeviceSize dstOffset,
                                 VkDeviceSize dataSize,
                                 const void* pData) {
  if (Configurator::IsPlayer() && captureRenderPassesResources() && (dstBuffer != NULL)) {
    SD()._commandbufferstates[cmdBuf]->resourceWriteBuffers[dstBuffer] =
        VULKAN_BLIT_DESTINATION_BUFFER;
  }
  if (Configurator::IsRecorder()) {
    const auto it = SD()._bufferstates.find(dstBuffer);
    if (it == SD()._bufferstates.end()) {
      return;
    }

    const auto& bufferState = it->second;

    if (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase()) {
      SD().bindingBuffers[cmdBuf].insert(bufferState);
    }

    SD()._commandbufferstates[cmdBuf]->touchedResources.emplace_back((uint64_t)dstBuffer, false);

    if (Configurator::Get().vulkan.recorder.memorySegmentSize ||
        Configurator::Get().vulkan.recorder.shadowMemory) {
      if (bufferState->binding) {
        const auto memory = bufferState->binding->deviceMemoryStateStore->deviceMemoryHandle;
        const auto offset = bufferState->binding->memoryOffset + dstOffset;
        const auto size = dataSize;
        SD().updatedMemoryInCmdBuffer[cmdBuf].AddToMap(memory, offset, size);
      }
    }
  }
}

inline void vkCmdFillBuffer_SD(VkCommandBuffer cmdBuf,
                               VkBuffer dstBuffer,
                               VkDeviceSize dstOffset,
                               VkDeviceSize dstSize,
                               uint32_t data) {
  if (Configurator::IsPlayer() && captureRenderPassesResources() && (dstBuffer != NULL)) {
    SD()._commandbufferstates[cmdBuf]->resourceWriteBuffers[dstBuffer] =
        VULKAN_BLIT_DESTINATION_BUFFER;
  }
  if (Configurator::IsRecorder()) {
    const auto it = SD()._bufferstates.find(dstBuffer);
    if (it == SD()._bufferstates.end()) {
      return;
    }

    const auto& bufferState = it->second;

    if (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase()) {
      SD().bindingBuffers[cmdBuf].insert(bufferState);
    }

    SD()._commandbufferstates[cmdBuf]->touchedResources.emplace_back((uint64_t)dstBuffer, false);

    if (Configurator::Get().vulkan.recorder.memorySegmentSize ||
        Configurator::Get().vulkan.recorder.shadowMemory) {
      if (bufferState->binding) {
        const auto memory = bufferState->binding->deviceMemoryStateStore->deviceMemoryHandle;
        const auto offset = bufferState->binding->memoryOffset + dstOffset;
        const auto size = (dstSize == VK_WHOLE_SIZE)
                              ? (bufferState->bufferCreateInfoData.Value()->size - dstOffset)
                              : dstSize;
        SD().updatedMemoryInCmdBuffer[cmdBuf].AddToMap(memory, offset, size);
      }
    }
  }
}

inline void vkCmdCopyBuffer_SD(VkCommandBuffer cmdBuf,
                               VkBuffer srcBuffer,
                               VkBuffer dstBuffer,
                               uint32_t regionCount,
                               const VkBufferCopy* pRegions) {
  if (Configurator::IsPlayer() && captureRenderPassesResources() && (dstBuffer != NULL)) {
    SD()._commandbufferstates[cmdBuf]->resourceWriteBuffers[dstBuffer] =
        VULKAN_BLIT_DESTINATION_BUFFER;
  }
  if (Configurator::IsRecorder()) {
    const auto dstIt = SD()._bufferstates.find(dstBuffer);

    if (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase()) {
      auto& bindingBuffers = SD().bindingBuffers[cmdBuf];

      // Src buffer
      insertStateIfFound(SD()._bufferstates, srcBuffer, bindingBuffers);

      // Dst buffer
      if (dstIt != SD()._bufferstates.end()) {
        bindingBuffers.insert(dstIt->second);
      }
    }

    if (dstIt == SD()._bufferstates.end()) {
      return;
    }

    SD()._commandbufferstates[cmdBuf]->touchedResources.emplace_back((uint64_t)dstBuffer, false);

    if (Configurator::Get().vulkan.recorder.memorySegmentSize ||
        Configurator::Get().vulkan.recorder.shadowMemory) {
      const auto& bufferBinding = dstIt->second->binding;
      if (bufferBinding) {
        for (uint32_t i = 0; i < regionCount; i++) {
          const auto memory = bufferBinding->deviceMemoryStateStore->deviceMemoryHandle;
          const auto offset = bufferBinding->memoryOffset + pRegions[i].dstOffset;
          const auto size = pRegions[i].size;
          SD().updatedMemoryInCmdBuffer[cmdBuf].AddToMap(memory, offset, size);
        }
      }
    }
  }
}

inline void vkCmdCopyBuffer2_SD(VkCommandBuffer cmdBuf, const VkCopyBufferInfo2* pCopyBufferInfo) {
  if (Configurator::IsPlayer() && captureRenderPassesResources() &&
      (pCopyBufferInfo->dstBuffer != NULL)) {
    SD()._commandbufferstates[cmdBuf]->resourceWriteBuffers[pCopyBufferInfo->dstBuffer] =
        VULKAN_BLIT_DESTINATION_BUFFER;
  }

  if (Configurator::IsRecorder() && pCopyBufferInfo) {
    const auto dstIt = SD()._bufferstates.find(pCopyBufferInfo->dstBuffer);

    if (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase()) {
      auto& bindingBuffers = SD().bindingBuffers[cmdBuf];

      // Src buffer
      insertStateIfFound(SD()._bufferstates, pCopyBufferInfo->srcBuffer, bindingBuffers);

      // Dst buffer
      if (dstIt != SD()._bufferstates.end()) {
        bindingBuffers.insert(dstIt->second);
      }
    }

    if (dstIt == SD()._bufferstates.end()) {
      return;
    }

    SD()._commandbufferstates[cmdBuf]->touchedResources.emplace_back(
        (uint64_t)pCopyBufferInfo->dstBuffer, false);

    if (Configurator::Get().vulkan.recorder.memorySegmentSize ||
        Configurator::Get().vulkan.recorder.shadowMemory) {
      const auto& bufferBinding = dstIt->second->binding;

      if (bufferBinding) {
        for (uint32_t i = 0; i < pCopyBufferInfo->regionCount; i++) {
          const auto memory = bufferBinding->deviceMemoryStateStore->deviceMemoryHandle;
          const auto offset = bufferBinding->memoryOffset + pCopyBufferInfo->pRegions[i].dstOffset;
          const auto size = pCopyBufferInfo->pRegions[i].size;
          SD().updatedMemoryInCmdBuffer[cmdBuf].AddToMap(memory, offset, size);
        }
      }
    }
  }
}

namespace {

VkImageSubresource getImageSubresource(const VkBufferImageCopy& region) {
  return VkImageSubresource{
      region.imageSubresource.aspectMask,    // VkImageAspectFlags aspectMask;
      region.imageSubresource.mipLevel,      // uint32_t mipLevel;
      region.imageSubresource.baseArrayLayer // uint32_t arrayLayer;
  };
}

VkImageSubresource getImageSubresource(const VkBufferImageCopy2& region) {
  return VkImageSubresource{
      region.imageSubresource.aspectMask,    // VkImageAspectFlags aspectMask;
      region.imageSubresource.mipLevel,      // uint32_t mipLevel;
      region.imageSubresource.baseArrayLayer // uint32_t arrayLayer;
  };
}

VkImageSubresource getImageSubresource(const VkImageCopy& region) {
  return VkImageSubresource{
      region.dstSubresource.aspectMask,    // VkImageAspectFlags aspectMask;
      region.dstSubresource.mipLevel,      // uint32_t mipLevel;
      region.dstSubresource.baseArrayLayer // uint32_t arrayLayer;
  };
}

VkImageSubresource getImageSubresource(const VkImageCopy2& region) {
  return VkImageSubresource{
      region.dstSubresource.aspectMask,    // VkImageAspectFlags aspectMask;
      region.dstSubresource.mipLevel,      // uint32_t mipLevel;
      region.dstSubresource.baseArrayLayer // uint32_t arrayLayer;
  };
}

VkImageSubresource getImageSubresource(const VkImageBlit& region) {
  return VkImageSubresource{
      region.dstSubresource.aspectMask,    // VkImageAspectFlags aspectMask;
      region.dstSubresource.mipLevel,      // uint32_t mipLevel;
      region.dstSubresource.baseArrayLayer // uint32_t arrayLayer;
  };
}

VkImageSubresource getImageSubresource(const VkImageBlit2& region) {
  return VkImageSubresource{
      region.dstSubresource.aspectMask,    // VkImageAspectFlags aspectMask;
      region.dstSubresource.mipLevel,      // uint32_t mipLevel;
      region.dstSubresource.baseArrayLayer // uint32_t arrayLayer;
  };
}

VkImageSubresource getImageSubresource(const VkImageResolve& region) {
  return VkImageSubresource{
      region.dstSubresource.aspectMask,    // VkImageAspectFlags aspectMask;
      region.dstSubresource.mipLevel,      // uint32_t mipLevel;
      region.dstSubresource.baseArrayLayer // uint32_t arrayLayer;
  };
}

VkImageSubresource getImageSubresource(const VkImageResolve2& region) {
  return VkImageSubresource{
      region.dstSubresource.aspectMask,    // VkImageAspectFlags aspectMask;
      region.dstSubresource.mipLevel,      // uint32_t mipLevel;
      region.dstSubresource.baseArrayLayer // uint32_t arrayLayer;
  };
}

template <typename REGION_TYPE>
void ProcessDestinationImage(std::shared_ptr<CImageState>& imageState,
                             uint32_t regionCount,
                             const REGION_TYPE* pRegions,
                             CMemoryUpdateState& updatedMemoryInCmdBuffer) {
  auto device = imageState->deviceStateStore->deviceHandle;
  VkDeviceMemory memory = imageState->binding->deviceMemoryStateStore->deviceMemoryHandle;

  if (imageState->imageCreateInfoData.Value() &&
      (imageState->imageCreateInfoData.Value()->tiling == VK_IMAGE_TILING_LINEAR)) {
    for (uint32_t i = 0; i < regionCount; i++) {
      const auto imageSubresource = getImageSubresource(pRegions[i]);

      VkSubresourceLayout subLayout;
      drvVk.vkGetImageSubresourceLayout(device, imageState->imageHandle, &imageSubresource,
                                        &subLayout);

      const auto offset = imageState->binding->memoryOffset + subLayout.offset;
      const auto size = subLayout.size;
      updatedMemoryInCmdBuffer.AddToMap(memory, offset, size);
    }
  } else {
    VkMemoryRequirements memRequirements = {};
    drvVk.vkGetImageMemoryRequirements(device, imageState->imageHandle, &memRequirements);

    const auto offset = imageState->binding->memoryOffset;
    const auto size = memRequirements.size;
    updatedMemoryInCmdBuffer.AddToMap(memory, offset, size);
  }
}
} // namespace

inline void vkCmdCopyBufferToImage_SD(VkCommandBuffer cmdBuf,
                                      VkBuffer srcBuffer,
                                      VkImage dstImage,
                                      VkImageLayout dstImageLayout,
                                      uint32_t regionCount,
                                      const VkBufferImageCopy* pRegions) {
  if (Configurator::IsPlayer() && captureRenderPassesResources() && (dstImage != NULL)) {
    SD()._commandbufferstates[cmdBuf]->resourceWriteImages[dstImage] =
        VULKAN_BLIT_DESTINATION_IMAGE;
  }

  if (Configurator::IsRecorder()) {
    const auto dstIt = SD()._imagestates.find(dstImage);

    if (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase()) {
      // Src buffer
      insertStateIfFound(SD()._bufferstates, srcBuffer, SD().bindingBuffers[cmdBuf]);

      // Dst image
      if (dstIt != SD()._imagestates.end()) {
        SD().bindingImages[cmdBuf].insert(dstIt->second);
      }
    }

    if (dstIt == SD()._imagestates.end()) {
      return;
    }

    SD()._commandbufferstates[cmdBuf]->touchedResources.emplace_back((uint64_t)dstImage, true);

    if ((!Configurator::Get().vulkan.recorder.memorySegmentSize &&
         !Configurator::Get().vulkan.recorder.shadowMemory) ||
        !dstIt->second->binding) {
      return;
    }

    ProcessDestinationImage(dstIt->second, regionCount, pRegions,
                            SD().updatedMemoryInCmdBuffer[cmdBuf]);
  }
}

inline void vkCmdCopyBufferToImage2_SD(VkCommandBuffer cmdBuf,
                                       const VkCopyBufferToImageInfo2* pCopyBufferToImageInfo) {
  if (Configurator::IsPlayer() && captureRenderPassesResources() &&
      (pCopyBufferToImageInfo->dstImage != NULL)) {
    SD()._commandbufferstates[cmdBuf]->resourceWriteImages[pCopyBufferToImageInfo->dstImage] =
        VULKAN_BLIT_DESTINATION_IMAGE;
  }

  if (Configurator::IsRecorder() && pCopyBufferToImageInfo) {
    const auto dstIt = SD()._imagestates.find(pCopyBufferToImageInfo->dstImage);

    if (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase()) {
      // Src buffer
      insertStateIfFound(SD()._bufferstates, pCopyBufferToImageInfo->srcBuffer,
                         SD().bindingBuffers[cmdBuf]);

      // Dst image
      if (dstIt != SD()._imagestates.end()) {
        SD().bindingImages[cmdBuf].insert(dstIt->second);
      }
    }

    if (dstIt == SD()._imagestates.end()) {
      return;
    }

    SD()._commandbufferstates[cmdBuf]->touchedResources.emplace_back(
        (uint64_t)pCopyBufferToImageInfo->dstImage, true);

    if ((!Configurator::Get().vulkan.recorder.memorySegmentSize &&
         !Configurator::Get().vulkan.recorder.shadowMemory) ||
        !dstIt->second->binding) {
      return;
    }

    ProcessDestinationImage(dstIt->second, pCopyBufferToImageInfo->regionCount,
                            pCopyBufferToImageInfo->pRegions,
                            SD().updatedMemoryInCmdBuffer[cmdBuf]);
  }
}

inline void vkCmdCopyImage_SD(VkCommandBuffer cmdBuf,
                              VkImage srcImage,
                              VkImageLayout srcImageLayout,
                              VkImage dstImage,
                              VkImageLayout dstImageLayout,
                              uint32_t regionCount,
                              const VkImageCopy* pRegions) {
  if (Configurator::IsPlayer() && captureRenderPassesResources() && (dstImage != NULL)) {
    SD()._commandbufferstates[cmdBuf]->resourceWriteImages[dstImage] =
        VULKAN_BLIT_DESTINATION_IMAGE;
  }

  if (Configurator::IsRecorder()) {
    const auto dstIt = SD()._imagestates.find(dstImage);

    if (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase()) {
      auto& bindingImages = SD().bindingImages[cmdBuf];

      // Src image
      insertStateIfFound(SD()._imagestates, srcImage, bindingImages);

      // Dst image
      {
        if (dstIt != SD()._imagestates.end()) {
          bindingImages.insert(dstIt->second);
        }
      }
    }

    if (dstIt == SD()._imagestates.end()) {
      return;
    }

    SD()._commandbufferstates[cmdBuf]->touchedResources.emplace_back((uint64_t)dstImage, true);

    if ((!Configurator::Get().vulkan.recorder.memorySegmentSize &&
         !Configurator::Get().vulkan.recorder.shadowMemory) ||
        !dstIt->second->binding) {
      return;
    }

    ProcessDestinationImage(dstIt->second, regionCount, pRegions,
                            SD().updatedMemoryInCmdBuffer[cmdBuf]);
  }
}

inline void vkCmdCopyImage2_SD(VkCommandBuffer cmdBuf, const VkCopyImageInfo2* pCopyImageInfo) {
  if (Configurator::IsPlayer() && captureRenderPassesResources() &&
      (pCopyImageInfo->dstImage != NULL)) {
    SD()._commandbufferstates[cmdBuf]->resourceWriteImages[pCopyImageInfo->dstImage] =
        VULKAN_BLIT_DESTINATION_IMAGE;
  }
  if (Configurator::IsRecorder()) {
    const auto dstIt = SD()._imagestates.find(pCopyImageInfo->dstImage);

    if (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase()) {
      auto& bindingImages = SD().bindingImages[cmdBuf];

      // Src image
      insertStateIfFound(SD()._imagestates, pCopyImageInfo->srcImage, bindingImages);

      // Dst image
      if (dstIt != SD()._imagestates.end()) {
        bindingImages.insert(dstIt->second);
      }
    }

    if (dstIt == SD()._imagestates.end()) {
      return;
    }

    SD()._commandbufferstates[cmdBuf]->touchedResources.emplace_back(
        (uint64_t)pCopyImageInfo->dstImage, true);

    if ((!Configurator::Get().vulkan.recorder.memorySegmentSize &&
         !Configurator::Get().vulkan.recorder.shadowMemory) ||
        !dstIt->second->binding) {
      return;
    }

    ProcessDestinationImage(dstIt->second, pCopyImageInfo->regionCount, pCopyImageInfo->pRegions,
                            SD().updatedMemoryInCmdBuffer[cmdBuf]);
  }
}

inline void vkCmdCopyImage2KHR_SD(VkCommandBuffer cmdBuf, const VkCopyImageInfo2* pCopyImageInfo) {
  vkCmdCopyImage2_SD(cmdBuf, pCopyImageInfo);
}

namespace {

template <typename REGION_TYPE>
void CopyImageToBufferHelper(VkCommandBuffer cmdBuf,
                             VkImage srcImage,
                             VkBuffer dstBuffer,
                             uint32_t regionCount,
                             const REGION_TYPE* pRegions) {
  if (Configurator::IsPlayer() && captureRenderPassesResources() && (dstBuffer != VK_NULL_HANDLE)) {
    SD()._commandbufferstates[cmdBuf]->resourceWriteBuffers[dstBuffer] =
        VULKAN_BLIT_DESTINATION_BUFFER;
  }
  if (Configurator::IsRecorder()) {
    const auto srcIt = SD()._imagestates.find(srcImage);
    const auto dstIt = SD()._bufferstates.find(dstBuffer);

    if (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase()) {
      // Src image
      if (srcIt != SD()._imagestates.end()) {
        SD().bindingImages[cmdBuf].insert(srcIt->second);
      }
      // Dst buffer
      if (dstIt != SD()._bufferstates.end()) {
        SD().bindingBuffers[cmdBuf].insert(dstIt->second);
      }
    }

    if (dstIt == SD()._bufferstates.end()) {
      return;
    }

    SD()._commandbufferstates[cmdBuf]->touchedResources.emplace_back((uint64_t)dstBuffer, false);

    if (Configurator::Get().vulkan.recorder.memorySegmentSize ||
        Configurator::Get().vulkan.recorder.shadowMemory) {
      const auto& bufferBinding = dstIt->second->binding;

      if (bufferBinding) {
        const auto& bufferState = dstIt->second;
        const auto memory = bufferBinding->deviceMemoryStateStore->deviceMemoryHandle;

        if ((srcIt != SD()._imagestates.end()) && srcIt->second->imageCreateInfoData.Value() &&
            (srcIt->second->imageCreateInfoData.Value()->tiling == VK_IMAGE_TILING_LINEAR)) {
          for (uint32_t i = 0; i < regionCount; i++) {
            const auto imageSubresource = getImageSubresource(pRegions[i]);

            VkSubresourceLayout subLayout;
            drvVk.vkGetImageSubresourceLayout(bufferState->deviceStateStore->deviceHandle, srcImage,
                                              &imageSubresource, &subLayout);

            const auto offset = bufferBinding->memoryOffset + pRegions[i].bufferOffset;
            const auto size = subLayout.size;
            SD().updatedMemoryInCmdBuffer[cmdBuf].AddToMap(memory, offset, size);
          }
        } else {
          const auto offset = bufferBinding->memoryOffset;
          const auto size = bufferState->bufferCreateInfoData.Value()->size;
          SD().updatedMemoryInCmdBuffer[cmdBuf].AddToMap(memory, offset, size);
        }
      }
    }
  }
}

} // namespace

inline void vkCmdCopyImageToBuffer_SD(VkCommandBuffer cmdBuf,
                                      VkImage srcImage,
                                      VkImageLayout srcImageLayout,
                                      VkBuffer dstBuffer,
                                      uint32_t regionCount,
                                      const VkBufferImageCopy* pRegions) {
  CopyImageToBufferHelper(cmdBuf, srcImage, dstBuffer, regionCount, pRegions);
}

inline void vkCmdCopyImageToBuffer2_SD(VkCommandBuffer cmdBuf,
                                       const VkCopyImageToBufferInfo2* pCopyImageToBufferInfo) {
  if (!pCopyImageToBufferInfo) {
    return;
  }

  CopyImageToBufferHelper(cmdBuf, pCopyImageToBufferInfo->srcImage,
                          pCopyImageToBufferInfo->dstBuffer, pCopyImageToBufferInfo->regionCount,
                          pCopyImageToBufferInfo->pRegions);
}

inline void vkCmdCopyImageToBuffer2KHR_SD(VkCommandBuffer cmdBuf,
                                          const VkCopyImageToBufferInfo2* pCopyImageToBufferInfo) {
  if (!pCopyImageToBufferInfo) {
    return;
  }

  CopyImageToBufferHelper(cmdBuf, pCopyImageToBufferInfo->srcImage,
                          pCopyImageToBufferInfo->dstBuffer, pCopyImageToBufferInfo->regionCount,
                          pCopyImageToBufferInfo->pRegions);
}

namespace {

template <typename REGION_TYPE>
void BlitOrResolveImageHelper(VkCommandBuffer cmdBuf,
                              VkImage srcImage,
                              VkImage dstImage,
                              uint32_t regionCount,
                              const REGION_TYPE* pRegions) {
  if (Configurator::IsPlayer() && captureRenderPassesResources() && (dstImage != VK_NULL_HANDLE)) {
    SD()._commandbufferstates[cmdBuf]->resourceWriteImages[dstImage] =
        VULKAN_BLIT_DESTINATION_IMAGE;
  }

  if (Configurator::IsRecorder()) {
    const auto dstIt = SD()._imagestates.find(dstImage);

    if (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase()) {
      auto& bindingImages = SD().bindingImages[cmdBuf];

      // Src image
      insertStateIfFound(SD()._imagestates, srcImage, bindingImages);

      // Dst image
      if (dstIt != SD()._imagestates.end()) {
        bindingImages.insert(dstIt->second);
      }
    }

    if (dstIt == SD()._imagestates.end()) {
      return;
    }

    SD()._commandbufferstates[cmdBuf]->touchedResources.emplace_back((uint64_t)dstImage, true);

    if ((!Configurator::Get().vulkan.recorder.memorySegmentSize &&
         !Configurator::Get().vulkan.recorder.shadowMemory) ||
        !dstIt->second->binding) {
      return;
    }

    ProcessDestinationImage(dstIt->second, regionCount, pRegions,
                            SD().updatedMemoryInCmdBuffer[cmdBuf]);
  }
}

} // namespace

inline void vkCmdBlitImage_SD(VkCommandBuffer cmdBuf,
                              VkImage srcImage,
                              VkImageLayout srcImageLayout,
                              VkImage dstImage,
                              VkImageLayout dstImageLayout,
                              uint32_t regionCount,
                              const VkImageBlit* pRegions,
                              VkFilter filter) {
  BlitOrResolveImageHelper(cmdBuf, srcImage, dstImage, regionCount, pRegions);
}

inline void vkCmdBlitImage2_SD(VkCommandBuffer cmdBuf, const VkBlitImageInfo2* pBlitInfoImage) {
  if (!pBlitInfoImage) {
    return;
  }

  BlitOrResolveImageHelper(cmdBuf, pBlitInfoImage->srcImage, pBlitInfoImage->dstImage,
                           pBlitInfoImage->regionCount, pBlitInfoImage->pRegions);
}

inline void vkCmdResolveImage_SD(VkCommandBuffer cmdBuf,
                                 VkImage srcImage,
                                 VkImageLayout srcImageLayout,
                                 VkImage dstImage,
                                 VkImageLayout dstImageLayout,
                                 uint32_t regionCount,
                                 const VkImageResolve* pRegions) {
  BlitOrResolveImageHelper(cmdBuf, srcImage, dstImage, regionCount, pRegions);
}

inline void vkCmdResolveImage2_SD(VkCommandBuffer cmdBuf,
                                  const VkResolveImageInfo2* pResolveImageInfo) {
  if (!pResolveImageInfo) {
    return;
  }

  BlitOrResolveImageHelper(cmdBuf, pResolveImageInfo->srcImage, pResolveImageInfo->dstImage,
                           pResolveImageInfo->regionCount, pResolveImageInfo->pRegions);
}

inline void vkCmdClearColorImage_SD(VkCommandBuffer cmdBuf,
                                    VkImage image,
                                    VkImageLayout imageLayout,
                                    const VkClearColorValue* pColor,
                                    uint32_t rangeCount,
                                    const VkImageSubresourceRange* pRanges) {
  const auto dstIt = SD()._imagestates.find(image);
  if (dstIt == SD()._imagestates.end()) {
    return;
  }

  if (captureRenderPasses()) {
    SD()._commandbufferstates[cmdBuf]->clearedImages.insert(image);
  }

  if (Configurator::IsRecorder()) {
    const auto& imageState = dstIt->second;

    if (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase()) {
      SD().bindingImages[cmdBuf].insert(imageState);
    }

    SD()._commandbufferstates[cmdBuf]->touchedResources.emplace_back((uint64_t)image, true);

    if ((!Configurator::Get().vulkan.recorder.memorySegmentSize &&
         !Configurator::Get().vulkan.recorder.shadowMemory) ||
        !imageState->binding) {
      return;
    }

    VkMemoryRequirements memRequirements = {};
    drvVk.vkGetImageMemoryRequirements(imageState->deviceStateStore->deviceHandle, image,
                                       &memRequirements);

    //TODO : call vkGetImageSubresourceLayout when tiling is Linear

    const auto memory = imageState->binding->deviceMemoryStateStore->deviceMemoryHandle;
    const auto offset = imageState->binding->memoryOffset;
    const auto size = memRequirements.size;
    SD().updatedMemoryInCmdBuffer[cmdBuf].AddToMap(memory, offset, size);
  }
}

inline void vkCmdClearDepthStencilImage_SD(VkCommandBuffer cmdBuf,
                                           VkImage image,
                                           VkImageLayout imageLayout,
                                           const VkClearDepthStencilValue* pDepthStencil,
                                           uint32_t rangeCount,
                                           const VkImageSubresourceRange* pRanges) {
  const auto dstIt = SD()._imagestates.find(image);
  if (dstIt == SD()._imagestates.end()) {
    return;
  }

  if (captureRenderPasses()) {
    SD()._commandbufferstates[cmdBuf]->clearedImages.insert(image);
  }

  if (Configurator::IsRecorder()) {
    const auto& imageState = dstIt->second;

    if (updateOnlyUsedMemory() || isSubcaptureBeforeRestorationPhase()) {
      SD().bindingImages[cmdBuf].insert(imageState);
    }

    SD()._commandbufferstates[cmdBuf]->touchedResources.emplace_back((uint64_t)image, true);

    if ((!Configurator::Get().vulkan.recorder.memorySegmentSize &&
         !Configurator::Get().vulkan.recorder.shadowMemory) ||
        !imageState->binding) {
      return;
    }

    VkMemoryRequirements memRequirements = {};
    drvVk.vkGetImageMemoryRequirements(imageState->deviceStateStore->deviceHandle, image,
                                       &memRequirements);

    //TODO : call vkGetImageSubresourceLayout when tiling is Linear

    const auto memory = imageState->binding->deviceMemoryStateStore->deviceMemoryHandle;
    const auto offset = imageState->binding->memoryOffset;
    const auto size = memRequirements.size;
    SD().updatedMemoryInCmdBuffer[cmdBuf].AddToMap(memory, offset, size);
  }
}

inline void vkCmdClearAttachments_SD(VkCommandBuffer cmdBuf,
                                     uint32_t attachmentCount,
                                     const VkClearAttachment* pAttachments,
                                     uint32_t rectCount,
                                     const VkClearRect* pRects) {
  if (captureRenderPasses()) {
    const auto& commandBufferState = SD()._commandbufferstates[cmdBuf];
    for (uint32_t i = 0; i < attachmentCount; ++i) {
      if (commandBufferState->beginRenderPassesList.size()) {
        const auto& framebufferState =
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
                SD()._commandbufferstates[cmdBuf]->clearedImages.insert(imageHandle);
              }
              colorAttIndex++;
            } else {
              SD()._commandbufferstates[cmdBuf]->clearedImages.insert(imageHandle);
            }
          }
        }
      }
    }
  }
}

inline void vkCmdPushConstants_SD(VkCommandBuffer cmdBuf,
                                  VkPipelineLayout layout,
                                  VkShaderStageFlags stageFlags,
                                  uint32_t offset,
                                  uint32_t size,
                                  const void* pValues) {
  auto& commandBufferState = SD()._commandbufferstates[cmdBuf];

  commandBufferState->pushContantsData = {
      layout,     // VkPipelineLayout layout;
      stageFlags, // VkShaderStageFlags stageFlags;
      offset,     // uint32_t offset;
      {}          // std::vector<uint8_t> data;
  };

  commandBufferState->pushContantsData.data.resize(size);
  memcpy(commandBufferState->pushContantsData.data.data(), pValues, size);
}

} // namespace Vulkan
} // namespace gits

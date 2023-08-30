// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "vulkanStateTracking.h"
#include "vulkan_apis_iface.h"
#include "vulkanLog.h"
#include <thread>

#if defined(GITS_PLATFORM_WINDOWS)
#include "renderDocUtil.h"
#endif

namespace gits {
namespace Vulkan {
namespace {

inline VkResult AcquireFakeSwapchainImageIndex(uint32_t recorderIndex,
                                               uint32_t* imageIndex,
                                               VkDevice device,
                                               VkQueue queue,
                                               VkSemaphore semaphore,
                                               VkFence fence) {
  *imageIndex = recorderIndex;

  // Signal semaphore and/or fence
  VkSubmitInfo submitInfo = {
      VK_STRUCTURE_TYPE_SUBMIT_INFO,           // VkStructureType sType;
      nullptr,                                 // const void* pNext;
      0,                                       // uint32_t waitSemaphoreCount;
      nullptr,                                 // const VkSemaphore* pWaitSemaphores;
      nullptr,                                 // const VkPipelineStageFlags* pWaitDstStageMask;
      0,                                       // uint32_t commandBufferCount;
      nullptr,                                 // const VkCommandBuffer* pCommandBuffers;
      (semaphore != VK_NULL_HANDLE) ? 1u : 0u, // uint32_t signalSemaphoreCount;
      &semaphore                               // const VkSemaphore* pSignalSemaphores;
  };
  VkResult result = drvVk.vkQueueSubmit(queue, 1, &submitInfo, fence);

  return result;
}

inline void RewindSwapchainImageIndex(VkDevice device,
                                      VkQueue queue,
                                      VkFence fence,
                                      VkSemaphore semaphore,
                                      VkSwapchainKHR swapchain,
                                      uint32_t* imageIndex) {
  VkPresentInfoKHR presentInfo = {
      VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, // VkStructureType sType;
      nullptr,                            // const void* pNext;
      0,                                  // uint32_t waitSemaphoreCount;
      nullptr,                            // const VkSemaphore* pWaitSemaphores;
      1,                                  // uint32_t swapchainCount;
      &swapchain,                         // const VkSwapchainKHR* pSwapchains;
      imageIndex,                         // const uint32_t* pImageIndices;
      nullptr                             // VkResult* pResults;
  };

  if (VK_NULL_HANDLE != semaphore) {
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &semaphore;
  }
  if (VK_NULL_HANDLE != fence) {
    drvVk.vkWaitForFences(device, 1, &fence, VK_FALSE, 2000000000);
    drvVk.vkResetFences(device, 1, &fence);
  }

  // Send message to benchmarkGPU VkShims or to GITS recorder
  drvVk.vkGetDeviceProcAddr(device, VK_UNWIND_QUEUE_PRESENT_GITS_FUNCTION_NAME);

  drvVk.vkQueuePresentKHR(queue, &presentInfo);
  drvVk.vkQueueWaitIdle(queue);
}

} // namespace

inline void vkAcquireNextImage2KHR_WRAPRUN(CVkResult& return_value,
                                           CVkDevice& device,
                                           CVkAcquireNextImageInfoKHR& pAcquireInfo,
                                           Cuint32_t::CSArray& pImageIndex) {
  auto indexPtr = *pImageIndex;
  if (indexPtr == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  uint32_t recorderIndex = *indexPtr;
  VkResult recorderReturnValue = *return_value;
  VkAcquireNextImageInfoKHR* acquireInfo = *pAcquireInfo;
  if (acquireInfo == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  // Offscreen rendering
  if (Config::Get().player.renderOffscreen) {
    return_value.Assign(AcquireFakeSwapchainImageIndex(
        recorderIndex, indexPtr, *device,
        SD()._devicestates[*device]->queueStateStoreList[0]->queueHandle, acquireInfo->semaphore,
        acquireInfo->fence));
  }
  // Normal rendering
  else {
    return_value.Assign(drvVk.vkAcquireNextImage2KHR(*device, acquireInfo, indexPtr));

    if (recorderIndex != *indexPtr &&
        (recorderReturnValue == VK_SUCCESS || recorderReturnValue == VK_SUBOPTIMAL_KHR)) {
      Log(TRACE) << "vkAcquireNextImage2KHR restore section begin.";
      uint32_t maxAllowedVkSwapchainRewinds = Config::Get().player.maxAllowedVkSwapchainRewinds;
      uint32_t rewindCount = 0;
      while (recorderIndex != *indexPtr) {
        if (++rewindCount > maxAllowedVkSwapchainRewinds) {
          Log(ERR) << "Maximum swapchain rewind limit (" << maxAllowedVkSwapchainRewinds
                   << ") exceeded.";
          throw std::runtime_error(EXCEPTION_MESSAGE);
        }

        RewindSwapchainImageIndex(
            *device, SD()._devicestates[*device]->queueStateStoreList[0]->queueHandle,
            acquireInfo->fence, acquireInfo->semaphore, acquireInfo->swapchain, indexPtr);

        return_value.Assign(drvVk.vkAcquireNextImage2KHR(*device, acquireInfo, indexPtr));
      }
      Log(TRACE) << "vkAcquireNextImage2KHR restore section end.";
    }
  }

  vkAcquireNextImage2KHR_SD(*return_value, *device, acquireInfo, indexPtr);
}

inline void vkAcquireNextImageKHR_WRAPRUN(CVkResult& return_value,
                                          CVkDevice& device,
                                          CVkSwapchainKHR& swapchain,
                                          Cuint64_t& timeout,
                                          CVkSemaphore& semaphore,
                                          CVkFence& fence,
                                          Cuint32_t::CSArray& pImageIndex) {
  auto indexPtr = *pImageIndex;
  if (indexPtr == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  uint32_t recorderIndex = *indexPtr;
  VkResult recorderReturnValue = *return_value;

  // Offscreen rendering
  if (Config::Get().player.renderOffscreen) {
    return_value.Assign(AcquireFakeSwapchainImageIndex(
        recorderIndex, indexPtr, *device,
        SD()._devicestates[*device]->queueStateStoreList[0]->queueHandle, *semaphore, *fence));
  }
  // Normal rendering
  else {
    return_value.Assign(
        drvVk.vkAcquireNextImageKHR(*device, *swapchain, *timeout, *semaphore, *fence, indexPtr));

    if (recorderIndex != *indexPtr &&
        (recorderReturnValue == VK_SUCCESS || recorderReturnValue == VK_SUBOPTIMAL_KHR)) {
      Log(TRACE) << "vkAcquireNextImageKHR restore section begin.";
      uint32_t maxAllowedVkSwapchainRewinds = Config::Get().player.maxAllowedVkSwapchainRewinds;
      uint32_t rewindCount = 0;
      while (recorderIndex != *indexPtr) {
        if (++rewindCount > maxAllowedVkSwapchainRewinds) {
          Log(ERR) << "Maximum swapchain rewind limit (" << maxAllowedVkSwapchainRewinds
                   << ") exceeded.";
          throw std::runtime_error(EXCEPTION_MESSAGE);
        }
        RewindSwapchainImageIndex(*device,
                                  SD()._devicestates[*device]->queueStateStoreList[0]->queueHandle,
                                  *fence, *semaphore, *swapchain, indexPtr);

        return_value.Assign(drvVk.vkAcquireNextImageKHR(*device, *swapchain, *timeout, *semaphore,
                                                        *fence, indexPtr));
      }
      Log(TRACE) << "vkAcquireNextImageKHR restore section end.";
    }
  }

  vkAcquireNextImageKHR_SD(*return_value, *device, *swapchain, *timeout, *semaphore, *fence,
                           indexPtr);
}

namespace {

std::vector<VkPhysicalDevice> GetPhysicalDevicesFromGroupProperties(
    uint32_t groupCount, VkPhysicalDeviceGroupProperties* groupProperties) {
  std::vector<VkPhysicalDevice> physicalDevices;
  for (uint32_t g = 0; g < groupCount; ++g) {
    for (uint32_t d = 0; d < groupProperties[g].physicalDeviceCount; ++d) {
      physicalDevices.push_back(groupProperties[g].physicalDevices[d]);
    }
  }
  return physicalDevices;
}

void HandlePhysicalDeviceMapping(std::vector<VkPhysicalDevice> const& recorderSideDevices,
                                 std::vector<VkPhysicalDevice> const& playerSideDevices) {
  uint32_t selectedPhysicalDeviceIndex = Config::Get().player.vulkanForcedPhysicalDeviceIndex;

  std::vector<VkPhysicalDeviceProperties> playerSideDevicesProperties(playerSideDevices.size());
  for (uint32_t i = 0; i < playerSideDevices.size(); ++i) {
    drvVk.vkGetPhysicalDeviceProperties(playerSideDevices[i], &playerSideDevicesProperties[i]);
  }

  // Select a device with a provided name
  if (Config::Get().player.vulkanForcedPhysicalDeviceName.size() > 0) {
    for (uint32_t i = 0; i < playerSideDevices.size(); ++i) {
      auto deviceName = ToLowerCopy(playerSideDevicesProperties[i].deviceName);
      auto requestedName = ToLowerCopy(Config::Get().player.vulkanForcedPhysicalDeviceName);
      if (strstr(deviceName.c_str(), requestedName.c_str()) != nullptr) {
        selectedPhysicalDeviceIndex = i;
        break;
      }
    }
  }
  // Select a device of a provided type
  if (Config::Get().player.vulkanForcedPhysicalDeviceType != TDeviceType::DEVICE_TYPE_ANY) {
    for (uint32_t i = 0; i < playerSideDevices.size(); ++i) {
      if (((Config::Get().player.vulkanForcedPhysicalDeviceType ==
            TDeviceType::DEVICE_TYPE_INTEGRATED) &&
           (playerSideDevicesProperties[i].deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)) ||
          ((Config::Get().player.vulkanForcedPhysicalDeviceType ==
            TDeviceType::DEVICE_TYPE_DISCRETE) &&
           (playerSideDevicesProperties[i].deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU))) {
        selectedPhysicalDeviceIndex = i;
        break;
      }
    }
  }

  if (selectedPhysicalDeviceIndex >= playerSideDevices.size()) {
    Log(WARN) << "Selected physical device index is greater than the number of enumerated physical "
                 "devices. Defaulting to 0.";
    selectedPhysicalDeviceIndex = 0;
  }

  VkPhysicalDevice selectedPhysicalDevice = playerSideDevices[selectedPhysicalDeviceIndex];
  VkPhysicalDeviceProperties& physicalDeviceProperties =
      playerSideDevicesProperties[selectedPhysicalDeviceIndex];

  CALL_ONCE[physicalDeviceProperties] {
    Log(INFO) << "Playing stream on a"
              << ((physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
                      ? ("n integrated")
                      : (" discrete"))
              << " device named \"" << physicalDeviceProperties.deviceName << "\".";
  };
  if (playerSideDevices.size() > 1) {
    CALL_ONCE[] {
      Log(INFO, NO_PREFIX) << "      (To change device, use the following option: "
                              "--useVKPhysicalDeviceIndex <index>)";
    };
  }

  for (auto recorderSidePhysicalDevice : recorderSideDevices) {
    CVkPhysicalDevice::AddMapping(recorderSidePhysicalDevice, selectedPhysicalDevice);
  }
}

} // namespace

inline void vkEnumeratePhysicalDeviceGroups_WRAPRUN(
    CVkResult& recorderSideReturnValue,
    CVkInstance& instance,
    Cuint32_t::CSArray& pPhysicalDeviceGroupCount,
    CVkPhysicalDeviceGroupPropertiesArray& pPhysicalDeviceGroupProperties) {
  uint32_t playerSideGroupCount;
  VkResult playerSideReturnValue =
      drvVk.vkEnumeratePhysicalDeviceGroups(*instance, &playerSideGroupCount, nullptr);

  if (pPhysicalDeviceGroupProperties.Original()) {
    auto groupCountPtr = *pPhysicalDeviceGroupCount;
    if (groupCountPtr == nullptr) {
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }

    std::vector<VkPhysicalDevice> recorderSidePhysicalDevices =
        GetPhysicalDevicesFromGroupProperties(*groupCountPtr,
                                              pPhysicalDeviceGroupProperties.Original());

    std::vector<VkPhysicalDeviceGroupProperties> playerSideGroupProperties(playerSideGroupCount);
    playerSideReturnValue = drvVk.vkEnumeratePhysicalDeviceGroups(*instance, &playerSideGroupCount,
                                                                  playerSideGroupProperties.data());

    std::vector<VkPhysicalDevice> playerSidePhysicalDevices = GetPhysicalDevicesFromGroupProperties(
        playerSideGroupCount, playerSideGroupProperties.data());

    HandlePhysicalDeviceMapping(recorderSidePhysicalDevices, playerSidePhysicalDevices);
    vkEnumeratePhysicalDeviceGroups_SD(playerSideReturnValue, *instance, &playerSideGroupCount,
                                       playerSideGroupProperties.data());
  }

  checkReturnValue(playerSideReturnValue, recorderSideReturnValue,
                   "vkEnumeratePhysicalDeviceGroups");
  recorderSideReturnValue.Assign(playerSideReturnValue);
}

inline void vkEnumeratePhysicalDeviceGroupsKHR_WRAPRUN(
    CVkResult& recorderSideReturnValue,
    CVkInstance& instance,
    Cuint32_t::CSArray& pPhysicalDeviceGroupCount,
    CVkPhysicalDeviceGroupPropertiesArray& pPhysicalDeviceGroupProperties) {
  uint32_t playerSideGroupCount;
  VkResult playerSideReturnValue =
      drvVk.vkEnumeratePhysicalDeviceGroupsKHR(*instance, &playerSideGroupCount, nullptr);

  if (pPhysicalDeviceGroupProperties.Original()) {
    auto groupCountPtr = *pPhysicalDeviceGroupCount;
    if (groupCountPtr == nullptr) {
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }

    std::vector<VkPhysicalDevice> recorderSidePhysicalDevices =
        GetPhysicalDevicesFromGroupProperties(*groupCountPtr,
                                              pPhysicalDeviceGroupProperties.Original());

    std::vector<VkPhysicalDeviceGroupProperties> playerSideGroupProperties(playerSideGroupCount);
    playerSideReturnValue = drvVk.vkEnumeratePhysicalDeviceGroupsKHR(
        *instance, &playerSideGroupCount, playerSideGroupProperties.data());

    std::vector<VkPhysicalDevice> playerSidePhysicalDevices = GetPhysicalDevicesFromGroupProperties(
        playerSideGroupCount, playerSideGroupProperties.data());

    HandlePhysicalDeviceMapping(recorderSidePhysicalDevices, playerSidePhysicalDevices);
    vkEnumeratePhysicalDeviceGroupsKHR_SD(playerSideReturnValue, *instance, &playerSideGroupCount,
                                          playerSideGroupProperties.data());
  }

  checkReturnValue(playerSideReturnValue, recorderSideReturnValue,
                   "vkEnumeratePhysicalDeviceGroupsKHR");
  recorderSideReturnValue.Assign(playerSideReturnValue);
}

inline void vkEnumeratePhysicalDevices_WRAPRUN(CVkResult& recorderSideReturnValue,
                                               CVkInstance& instance,
                                               Cuint32_t::CSArray& pPhysicalDeviceCount,
                                               CVkPhysicalDevice::CSMapArray& pPhysicalDevices) {
  uint32_t playerSideDevicesCount;
  VkResult playerSideReturnValue =
      drvVk.vkEnumeratePhysicalDevices(*instance, &playerSideDevicesCount, nullptr);
  auto physicalDevicesOriginal = pPhysicalDevices.Original();
  if (physicalDevicesOriginal != nullptr) {
    auto physDevCountPtr = *pPhysicalDeviceCount;
    if (physDevCountPtr == nullptr) {
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }
    uint32_t recorderSideDevicesCount = *physDevCountPtr;
    std::vector<VkPhysicalDevice> recorderSidePhysicalDevices;
    for (uint32_t i = 0; i < recorderSideDevicesCount; ++i) {
      recorderSidePhysicalDevices.push_back(physicalDevicesOriginal[i]);
    }

    std::vector<VkPhysicalDevice> playerSidePhysicalDevices(playerSideDevicesCount);
    playerSideReturnValue = drvVk.vkEnumeratePhysicalDevices(*instance, &playerSideDevicesCount,
                                                             playerSidePhysicalDevices.data());

    HandlePhysicalDeviceMapping(recorderSidePhysicalDevices, playerSidePhysicalDevices);
    vkEnumeratePhysicalDevices_SD(playerSideReturnValue, *instance, &playerSideDevicesCount,
                                  playerSidePhysicalDevices.data());
  }

  checkReturnValue(playerSideReturnValue, recorderSideReturnValue, "vkEnumeratePhysicalDevices");
  recorderSideReturnValue.Assign(playerSideReturnValue);
}

inline void vkGetSwapchainImagesKHR_WRAPRUN(CVkResult& recorderSideReturnValue,
                                            CVkDevice& device,
                                            CVkSwapchainKHR& swapChain,
                                            Cuint32_t::CSArray& pDataSize,
                                            CVkImage::CSMapArray& pData) {
  VkResult playerSideReturnValue = VK_SUCCESS;
  auto dataSizePtr = *pDataSize;
  if (dataSizePtr == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  if (pData.Size() != 0) {
    uint64_t recorderDataSize = *dataSizePtr;
    drvVk.vkGetSwapchainImagesKHR(*device, *swapChain, dataSizePtr, NULL);

    if (recorderDataSize != *dataSizePtr) {
      throw ENotSupported(EXCEPTION_MESSAGE);
    }

    playerSideReturnValue = drvVk.vkGetSwapchainImagesKHR(*device, *swapChain, dataSizePtr, *pData);
  } else {
    playerSideReturnValue = drvVk.vkGetSwapchainImagesKHR(*device, *swapChain, dataSizePtr, NULL);
  }

  checkReturnValue(playerSideReturnValue, recorderSideReturnValue, "vkGetSwapchainImagesKHR");
  recorderSideReturnValue.Assign(playerSideReturnValue);
}

inline void vkCreateInstance_WRAPRUN(CVkResult& recorderSideReturnValue,
                                     CVkInstanceCreateInfo& pCreateInfo,
                                     CNullWrapper& pAllocator,
                                     CVkInstance::CSMapArray& pInstance) {
  gits::CGits::Instance().apis.UseApi3dIface(
      std::shared_ptr<gits::ApisIface::Api3d>(new gits::Vulkan::VulkanApi()));

  if (CGits::Instance().IsStateRestoration()) {
    CGits::Instance().apis.Iface3D().Play_StateRestoreBegin();
  }

  VkInstanceCreateInfo* createInfoPtrOrig = *pCreateInfo;
  if (createInfoPtrOrig == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  VkInstanceCreateInfo createInfo = *createInfoPtrOrig;

  std::vector<const char*> requestedExtensions(createInfo.ppEnabledExtensionNames,
                                               createInfo.ppEnabledExtensionNames +
                                                   createInfo.enabledExtensionCount);
  std::vector<const char*> requestedLayers(createInfo.ppEnabledLayerNames,
                                           createInfo.ppEnabledLayerNames +
                                               createInfo.enabledLayerCount);

  suppressRequestedNames(requestedExtensions, Config::Get().player.suppressVKExtensions,
                         createInfo.enabledExtensionCount, createInfo.ppEnabledExtensionNames);
  suppressRequestedNames(requestedLayers, Config::Get().player.suppressVKLayers,
                         createInfo.enabledLayerCount, createInfo.ppEnabledLayerNames);

  bool allSupported = true;
  Log(TRACE, NO_PREFIX) << "";
  Log(TRACE) << " ------------------ ";
  Log(TRACE) << "Instance compatibility check section begin.\n";

  allSupported &= checkForSupportForInstanceExtensions(createInfo.enabledExtensionCount,
                                                       createInfo.ppEnabledExtensionNames);
  allSupported &= checkForSupportForInstanceLayers(createInfo.enabledLayerCount,
                                                   createInfo.ppEnabledLayerNames);
  Log(TRACE, NO_PREFIX) << "";

  if (!allSupported && !Config::Get().player.ignoreVKCrossPlatformIncompatibilitiesWA) {
    throw std::runtime_error("Error - stream uses instance extensions and/or layers which are not "
                             "supported on a current platform. Exiting!");
  }

  Log(TRACE) << "Instance compatibility check section end"
             << (allSupported ? "." : " - please read warning messages!!");
  Log(TRACE) << " ------------------ \n";

  VkResult playerSideReturnValue = drvVk.vkCreateInstance(&createInfo, *pAllocator, *pInstance);
  checkReturnValue(playerSideReturnValue, recorderSideReturnValue, "vkCreateInstance");
  recorderSideReturnValue.Assign(playerSideReturnValue);
  vkCreateInstance_SD(playerSideReturnValue, createInfoPtrOrig, *pAllocator, *pInstance);
}

inline void vkQueuePresentKHR_WRAPRUN(CVkResult& recorderSideReturnValue,
                                      CVkQueue& queue,
                                      CVkPresentInfoKHR& pPresentInfo) {
  if (*pPresentInfo == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  VkPresentInfoKHR presentInfo = *pPresentInfo;

  if (Config::Get().player.captureFrames[CGits::Instance().CurrentFrame()] &&
      !Config::Get().player.captureScreenshot) {
    writeScreenshot(*queue, presentInfo);
  }

  VkResult playerSideReturnValue = VK_SUCCESS;

  // Offscreen rendering
  if (Config::Get().player.renderOffscreen) {
    // Unsignal semaphores (perform fake waiting on all semaphores)
    if ((presentInfo.waitSemaphoreCount > 0) && (presentInfo.pWaitSemaphores)) {
      std::vector<VkPipelineStageFlags> waitStages(presentInfo.waitSemaphoreCount,
                                                   VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
      VkSubmitInfo submitInfo = {
          VK_STRUCTURE_TYPE_SUBMIT_INFO,  // VkStructureType sType;
          nullptr,                        // const void* pNext;
          presentInfo.waitSemaphoreCount, // uint32_t waitSemaphoreCount;
          presentInfo.pWaitSemaphores,    // const VkSemaphore* pWaitSemaphores;
          waitStages.data(),              // const VkPipelineStageFlags* pWaitDstStageMask;
          0,                              // uint32_t commandBufferCount;
          nullptr,                        // const VkCommandBuffer* pCommandBuffers;
          0,                              // uint32_t signalSemaphoreCount;
          nullptr                         // const VkSemaphore* pSignalSemaphores;
      };
      playerSideReturnValue = drvVk.vkQueueSubmit(*queue, 1, &submitInfo, VK_NULL_HANDLE);
    }
    // Set fake results
    if ((presentInfo.swapchainCount > 0) && (presentInfo.pResults)) {
      for (uint32_t i = 0; i < presentInfo.swapchainCount; ++i) {
        presentInfo.pResults[i] = playerSideReturnValue;
      }
    }

    // Send message to benchmarkGPU VkShims
    drvVk.vkGetDeviceProcAddr(SD()._queuestates[*queue]->deviceStateStore->deviceHandle,
                              VK_FAKE_QUEUE_PRESENT_GITS_FUNCTION_NAME);
  }
  // Normal rendering
  else {
    playerSideReturnValue = drvVk.vkQueuePresentKHR(*queue, &presentInfo);
    if (Config::Get().player.captureFrames[CGits::Instance().CurrentFrame()] &&
        Config::Get().player.captureScreenshot) {
      sleep_millisec(1000);
      writeScreenshot(*queue, presentInfo);
    }
  }
  checkReturnValue(playerSideReturnValue, recorderSideReturnValue, "vkQueuePresentKHR");
  recorderSideReturnValue.Assign(playerSideReturnValue);
  vkQueuePresentKHR_SD(playerSideReturnValue, *queue, &presentInfo);
}
namespace {
inline void HandleQueueSubmitRenderDocStart() {
#if defined(GITS_PLATFORM_WINDOWS)
  if (Config::Get().player.renderDoc.queuesubmitRecEnabled &&
      Config::Get()
          .player.renderDoc.captureRange[CGits::Instance().vkCounters.CurrentQueueSubmitCount()]) {
    RenderDocUtil::GetInstance().StartRecording();
  }
#endif
}

inline void HandleQueueSubmitRenderDocStop() {
#if defined(GITS_PLATFORM_WINDOWS)
  if (Config::Get().player.renderDoc.queuesubmitRecEnabled &&
      Config::Get()
          .player.renderDoc.captureRange[CGits::Instance().vkCounters.CurrentQueueSubmitCount()]) {
    bool isLast = Config::Get()
                      .player.renderDoc
                      .captureRange[CGits::Instance().vkCounters.CurrentQueueSubmitCount()] &&
                  !Config::Get()
                       .player.renderDoc
                       .captureRange[CGits::Instance().vkCounters.CurrentQueueSubmitCount() + 1];
    if (!Config::Get().player.renderDoc.continuousCapture || isLast) {
      RenderDocUtil::GetInstance().StopRecording();
    }
    if (Config::Get().player.renderDoc.enableUI && isLast) {
      RenderDocUtil::GetInstance().LaunchRenderDocUI();
    }
  }
#endif
}

inline void ExecCmdBuffer(VkCommandBuffer commandBuffer,
                          bool secondary,
                          uint32_t cmdBuffBatchNumber = 0,
                          uint32_t cmdBuffNumber = 0) {
  auto& commandBuffState = SD()._commandbufferstates[commandBuffer];
  if (commandBuffState->beginCommandBuffer) {
    if (commandBuffState->restored) {
      drvVk.vkResetCommandBuffer(commandBuffer, 0);
    }
    drvVk.vkBeginCommandBuffer(
        commandBuffer, commandBuffState->beginCommandBuffer->commandBufferBeginInfoData.Value());
    if (secondary) { // for now we didn't support image dumping in secondary cmdbuffer
      commandBuffState->tokensBuffer.ExecAndStateTrack();
    } else {
      commandBuffState->tokensBuffer.ExecAndDump(
          commandBuffer, CGits::Instance().vkCounters.CurrentQueueSubmitCount(), cmdBuffBatchNumber,
          cmdBuffNumber);
    }

    if (commandBuffState->ended) {
      drvVk.vkEndCommandBuffer(commandBuffer);
    }
    commandBuffState->restored = true;
    if (commandBuffState->beginCommandBuffer->oneTimeSubmit) {
      commandBuffState->tokensBuffer.Clear();
    }
  }
}
} // namespace

inline void vkQueueSubmit_WRAPRUN(CVkResult& return_value,
                                  CVkQueue& queue,
                                  Cuint32_t& submitCount,
                                  CVkSubmitInfoArray& pSubmits,
                                  CVkFence& fence) {
  if ((*submitCount > 0) && (!Config::Get().player.captureVulkanSubmits.empty() ||
                             !Config::Get().player.captureVulkanSubmitsResources.empty() ||
                             !Config::Get().player.captureVulkanRenderPasses.empty() ||
                             !Config::Get().player.captureVulkanRenderPassesResources.empty())) {
    auto pSubmitInfoArray = *pSubmits;
    if (pSubmitInfoArray == nullptr) {
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }
    for (uint32_t i = 0; i < *submitCount; i++) {
      VkFence fenceNew = VK_NULL_HANDLE;
      VkSubmitInfo submitInfoOrig = pSubmitInfoArray[i];
      if (submitInfoOrig.commandBufferCount == 0) {
        if (i == (*submitCount - 1)) { //last submit in QueueSubmit (restoring original fence)
          fenceNew = *fence;
        }
        TODO("Adjust behavior of vkQueueSubmit to other functions which use checkReturnValue().")
        return_value.Assign(drvVk.vkQueueSubmit(*queue, 1, &submitInfoOrig, fenceNew));
        if (*return_value != VK_SUCCESS) {
          Log(WARN) << "vkQueueSubmit failed.";
          if (Config::Get().player.exitOnVkQueueSubmitFail) {
            std::ostringstream error;
            error << "vkQueueSubmit function returned the " << *return_value << " error!\n";
            error << "Exiting!\n";
            throw std::runtime_error(error.str());
          }
        }
        vkQueueSubmit_SD(*return_value, *queue, 1, &submitInfoOrig, fenceNew);
      }
      for (uint32_t cmdBufIndex = 0; cmdBufIndex < submitInfoOrig.commandBufferCount;
           cmdBufIndex++) {
        const VkCommandBuffer& cmdbuffer = submitInfoOrig.pCommandBuffers[cmdBufIndex];
        if (Config::Get().player.execCmdBuffsBeforeQueueSubmit) {
          auto& commandBuffState = SD()._commandbufferstates[cmdbuffer];
          for (auto secondaryCmdBuffer : commandBuffState->secondaryCommandBuffers) {
            ExecCmdBuffer(secondaryCmdBuffer, true);
          }
          ExecCmdBuffer(cmdbuffer, false, i, cmdBufIndex);
        }
        VkSubmitInfo submitInfoNew;
        if (cmdBufIndex ==
            submitInfoOrig.commandBufferCount -
                1) //last command buffer in queue submit (restoring original settings)
        {
          if (i == (*submitCount - 1)) { //last submit in QueueSubmit (restoring original fence)
            fenceNew = *fence;
          }
          submitInfoNew = {VK_STRUCTURE_TYPE_SUBMIT_INFO,
                           submitInfoOrig.pNext,
                           submitInfoOrig.waitSemaphoreCount,
                           submitInfoOrig.pWaitSemaphores,
                           submitInfoOrig.pWaitDstStageMask,
                           1,
                           &cmdbuffer,
                           submitInfoOrig.signalSemaphoreCount,
                           submitInfoOrig.pSignalSemaphores};
        } else {
          submitInfoNew = {
              VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr, 0, nullptr, 0, 1, &cmdbuffer, 0, nullptr};
        }
        return_value.Assign(drvVk.vkQueueSubmit(*queue, 1, &submitInfoNew, fenceNew));
        if (*return_value != VK_SUCCESS) {
          Log(WARN) << "vkQueueSubmit failed.";
          if (Config::Get().player.exitOnVkQueueSubmitFail) {
            std::ostringstream error;
            error << "vkQueueSubmit function returned the " << *return_value << " error!\n";
            error << "Exiting!\n";
            throw std::runtime_error(error.str());
          }
        }
        vkQueueSubmit_SD(*return_value, *queue, 1, &submitInfoNew, fenceNew);
        bool captureVulkanSubmitsCheck =
            Config::Get().player.captureVulkanSubmits[(size_t)CGits::Instance()
                                                          .vkCounters.CurrentQueueSubmitCount()];
        bool captureVulkanSubmitsResourcesCheck =
            Config::Get().player.captureVulkanSubmitsResources
                [(size_t)CGits::Instance().vkCounters.CurrentQueueSubmitCount()];

        bool captureVulkanRenderPassesCheck =
            !Config::Get().player.captureVulkanRenderPasses.empty() &&
            !SD()._commandbufferstates[cmdbuffer]->renderPassImages.empty();

        bool captureVulkanRenderPassesResourcesCheck =
            !Config::Get().player.captureVulkanRenderPassesResources.empty() &&
            (!SD()._commandbufferstates[cmdbuffer]->renderPassResourceImages.empty() ||
             !SD()._commandbufferstates[cmdbuffer]->renderPassResourceBuffers.empty());

        if (captureVulkanSubmitsCheck || captureVulkanSubmitsResourcesCheck ||
            captureVulkanRenderPassesCheck || captureVulkanRenderPassesResourcesCheck ||
            Config::Get().player.waitAfterQueueSubmitWA) {
          drvVk.vkQueueWaitIdle(*queue);
        }
        if (captureVulkanSubmitsCheck) {
          writeScreenshot(*queue, cmdbuffer, i, cmdBufIndex);
        }
        if (captureVulkanSubmitsResourcesCheck) {
          writeResources(*queue, cmdbuffer, i, cmdBufIndex);
        }
        if (captureVulkanRenderPassesCheck) {
          vulkanDumpRenderPasses(cmdbuffer);
        }
        if (captureVulkanRenderPassesResourcesCheck) {
          vulkanDumpRenderPassResources(cmdbuffer);
        }
      }
    }
  } else {
    if ((*submitCount > 0) && Config::Get().player.execCmdBuffsBeforeQueueSubmit) {
      for (uint32_t i = 0; i < *submitCount; i++) {
        auto pSubmitInfoArray = *pSubmits;
        if (pSubmitInfoArray == nullptr) {
          throw std::runtime_error(EXCEPTION_MESSAGE);
        }
        for (uint32_t j = 0; j < pSubmitInfoArray[i].commandBufferCount; j++) {
          auto& commandBuffState =
              SD()._commandbufferstates[pSubmitInfoArray[i].pCommandBuffers[j]];
          for (auto secondaryCmdBuffer : commandBuffState->secondaryCommandBuffers) {
            ExecCmdBuffer(secondaryCmdBuffer, true);
          }
          ExecCmdBuffer(pSubmitInfoArray[i].pCommandBuffers[j], false, i, j);
        }
      }
    }
    HandleQueueSubmitRenderDocStart();
    return_value.Assign(drvVk.vkQueueSubmit(*queue, *submitCount, *pSubmits, *fence));
    if (*return_value != VK_SUCCESS) {
      Log(WARN) << "vkQueueSubmit failed.";
      if (Config::Get().player.exitOnVkQueueSubmitFail) {
        std::ostringstream error;
        error << "vkQueueSubmit function returned the " << *return_value << " error!\n";
        error << "Exiting!\n";
        throw std::runtime_error(error.str());
      }
    }
    if (Config::Get().player.waitAfterQueueSubmitWA) {
      drvVk.vkQueueWaitIdle(*queue);
    }
    vkQueueSubmit_SD(*return_value, *queue, *submitCount, *pSubmits, *fence);
    HandleQueueSubmitRenderDocStop();
  }
}

inline void vkQueueSubmit2_WRAPRUN(CVkResult& return_value,
                                   CVkQueue& queue,
                                   Cuint32_t& submitCount,
                                   CVkSubmitInfo2Array& pSubmits,
                                   CVkFence& fence) {
  if ((*submitCount > 0) && (!Config::Get().player.captureVulkanSubmits.empty() ||
                             !Config::Get().player.captureVulkanSubmitsResources.empty() ||
                             !Config::Get().player.captureVulkanRenderPasses.empty() ||
                             !Config::Get().player.captureVulkanRenderPassesResources.empty())) {
    auto pSubmitInfo2Array = *pSubmits;
    if (pSubmitInfo2Array == nullptr) {
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }
    for (uint32_t i = 0; i < *submitCount; i++) {
      VkFence fenceNew = VK_NULL_HANDLE;
      VkSubmitInfo2 submitInfoOrig = pSubmitInfo2Array[i];
      if (submitInfoOrig.commandBufferInfoCount == 0) {
        if (i == (*submitCount - 1)) { //last submit in QueueSubmit (restoring original fence)
          fenceNew = *fence;
        }
        TODO("Adjust behavior of vkQueueSubmit2 to other functions which use checkReturnValue().")
        return_value.Assign(drvVk.vkQueueSubmit2(*queue, 1, &submitInfoOrig, fenceNew));
        if (*return_value != VK_SUCCESS) {
          Log(WARN) << "vkQueueSubmit2 failed.";
          if (Config::Get().player.exitOnVkQueueSubmitFail) {
            std::ostringstream error;
            error << "vkQueueSubmit2 function returned the " << *return_value << " error!\n";
            error << "Exiting!\n";
            throw std::runtime_error(error.str());
          }
        }
        vkQueueSubmit2_SD(*return_value, *queue, 1, &submitInfoOrig, fenceNew);
      }
      for (uint32_t cmdBufIndex = 0; cmdBufIndex < submitInfoOrig.commandBufferInfoCount;
           cmdBufIndex++) {
        const VkCommandBufferSubmitInfo& cmdbufferSubmitInfo =
            submitInfoOrig.pCommandBufferInfos[cmdBufIndex];
        if (Config::Get().player.execCmdBuffsBeforeQueueSubmit) {
          auto& commandBuffState = SD()._commandbufferstates[cmdbufferSubmitInfo.commandBuffer];
          for (auto secondaryCmdBuffer : commandBuffState->secondaryCommandBuffers) {
            ExecCmdBuffer(secondaryCmdBuffer, true);
          }
          ExecCmdBuffer(cmdbufferSubmitInfo.commandBuffer, false, i, cmdBufIndex);
        }
        VkSubmitInfo2 submitInfoNew;
        if (cmdBufIndex ==
            submitInfoOrig.commandBufferInfoCount -
                1) //last command buffer in queue submit (restoring original settings)
        {
          if (i == (*submitCount - 1)) { //last submit in QueueSubmit (restoring original fence)
            fenceNew = *fence;
          }
          submitInfoNew = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                           submitInfoOrig.pNext,
                           submitInfoOrig.flags,
                           submitInfoOrig.waitSemaphoreInfoCount,
                           submitInfoOrig.pWaitSemaphoreInfos,
                           1,
                           &cmdbufferSubmitInfo,
                           submitInfoOrig.signalSemaphoreInfoCount,
                           submitInfoOrig.pSignalSemaphoreInfos};
        } else {
          submitInfoNew = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                           nullptr,
                           0,
                           0,
                           nullptr,
                           1,
                           &cmdbufferSubmitInfo,
                           0,
                           nullptr};
        }
        return_value.Assign(drvVk.vkQueueSubmit2(*queue, 1, &submitInfoNew, fenceNew));
        if (*return_value != VK_SUCCESS) {
          Log(WARN) << "vkQueueSubmit2 failed.";
          if (Config::Get().player.exitOnVkQueueSubmitFail) {
            std::ostringstream error;
            error << "vkQueueSubmit2 function returned the " << *return_value << " error!\n";
            error << "Exiting!\n";
            throw std::runtime_error(error.str());
          }
        }
        vkQueueSubmit2_SD(*return_value, *queue, 1, &submitInfoNew, fenceNew);
        bool captureVulkanSubmitsCheck =
            Config::Get().player.captureVulkanSubmits[(size_t)CGits::Instance()
                                                          .vkCounters.CurrentQueueSubmitCount()];
        bool captureVulkanSubmitsResourcesCheck =
            Config::Get().player.captureVulkanSubmitsResources
                [(size_t)CGits::Instance().vkCounters.CurrentQueueSubmitCount()];

        bool captureVulkanRenderPassesCheck =
            !Config::Get().player.captureVulkanRenderPasses.empty() &&
            !SD()._commandbufferstates[cmdbufferSubmitInfo.commandBuffer]->renderPassImages.empty();

        bool captureVulkanRenderPassesResourcesCheck =
            !Config::Get().player.captureVulkanRenderPassesResources.empty() &&
            (!SD()._commandbufferstates[cmdbufferSubmitInfo.commandBuffer]
                  ->renderPassResourceImages.empty() ||
             !SD()._commandbufferstates[cmdbufferSubmitInfo.commandBuffer]
                  ->renderPassResourceBuffers.empty());

        if (captureVulkanSubmitsCheck || captureVulkanSubmitsResourcesCheck ||
            captureVulkanRenderPassesCheck || captureVulkanRenderPassesResourcesCheck ||
            Config::Get().player.waitAfterQueueSubmitWA) {
          drvVk.vkQueueWaitIdle(*queue);
        }
        if (captureVulkanSubmitsCheck) {
          writeScreenshot(*queue, cmdbufferSubmitInfo.commandBuffer, i, cmdBufIndex);
        }
        if (captureVulkanSubmitsResourcesCheck) {
          writeResources(*queue, cmdbufferSubmitInfo.commandBuffer, i, cmdBufIndex);
        }
        if (captureVulkanRenderPassesCheck) {
          vulkanDumpRenderPasses(cmdbufferSubmitInfo.commandBuffer);
        }
        if (captureVulkanRenderPassesResourcesCheck) {
          vulkanDumpRenderPassResources(cmdbufferSubmitInfo.commandBuffer);
        }
      }
    }
  } else {
    if ((*submitCount > 0) && Config::Get().player.execCmdBuffsBeforeQueueSubmit) {
      for (uint32_t i = 0; i < *submitCount; i++) {
        auto pSubmitInfoArray = *pSubmits;
        if (pSubmitInfoArray == nullptr) {
          throw std::runtime_error(EXCEPTION_MESSAGE);
        }
        for (uint32_t j = 0; j < pSubmitInfoArray[i].commandBufferInfoCount; j++) {
          auto& commandBuffState =
              SD()._commandbufferstates[pSubmitInfoArray[i].pCommandBufferInfos[j].commandBuffer];
          for (auto secondaryCmdBuffer : commandBuffState->secondaryCommandBuffers) {
            ExecCmdBuffer(secondaryCmdBuffer, true);
          }
          ExecCmdBuffer(pSubmitInfoArray[i].pCommandBufferInfos[j].commandBuffer, false, i, j);
        }
      }
    }
    HandleQueueSubmitRenderDocStart();
    return_value.Assign(drvVk.vkQueueSubmit2(*queue, *submitCount, *pSubmits, *fence));
    if (*return_value != VK_SUCCESS) {
      Log(WARN) << "vkQueueSubmit2 failed.";
      if (Config::Get().player.exitOnVkQueueSubmitFail) {
        std::ostringstream error;
        error << "vkQueueSubmit2 function returned the " << *return_value << " error!\n";
        error << "Exiting!\n";
        throw std::runtime_error(error.str());
      }
    }
    if (Config::Get().player.waitAfterQueueSubmitWA) {
      drvVk.vkQueueWaitIdle(*queue);
    }
    vkQueueSubmit2_SD(*return_value, *queue, *submitCount, *pSubmits, *fence);

    HandleQueueSubmitRenderDocStop();
  }
}

inline void vkMapMemory_WRAPRUN(CVkResult& recorderSideReturnValue,
                                CVkDevice& device,
                                CVkDeviceMemory& mem,
                                Cuint64_t& offset,
                                Cuint64_t& size,
                                Cuint32_t& flags,
                                CVoidPtr& ppData) {
  checkMemoryMappingFeasibility(*device, *mem);

  void* pData;
  VkResult playerSideReturnValue = drvVk.vkMapMemory(*device, *mem, *offset, *size, *flags, &pData);
  checkReturnValue(playerSideReturnValue, recorderSideReturnValue, "vkMapMemory");
  recorderSideReturnValue.Assign(playerSideReturnValue);
  vkMapMemory_SD(playerSideReturnValue, *device, *mem, *offset, *size, *flags, &pData);
}

inline void vkGetFenceStatus_WRAPRUN(CVkResult& return_value, CVkDevice& device, CVkFence& fence) {
  VkResult recRetVal = *return_value;
  return_value.Assign(drvVk.vkGetFenceStatus(*device, *fence));

  if (*return_value != VK_SUCCESS && (*return_value != recRetVal) &&
      (SD()._fencestates[*fence]->fenceUsed)) {
    VkFence fenceCopy = *fence;
    drvVk.vkWaitForFences(*device, 1, &fenceCopy, VK_TRUE, 0xFFFFFFFFFFFFFFFF);
    return_value.Assign(drvVk.vkGetFenceStatus(*device, *fence));
  }
}

inline void vkGetEventStatus_WRAPRUN(CVkResult& return_value, CVkDevice& device, CVkEvent& event) {
  VkResult recRetVal = *return_value;
  return_value.Assign(drvVk.vkGetEventStatus(*device, *event));

  while ((*return_value != VK_EVENT_SET) && (*return_value != recRetVal) &&
         (SD()._eventstates[*event]->eventUsed)) {
    sleep_millisec(5);
    return_value.Assign(drvVk.vkGetEventStatus(*device, *event));
  }
}

inline void vkGetSemaphoreCounterValue_WRAPRUN(CVkResult& return_value,
                                               CVkDevice& device,
                                               CVkSemaphore& semaphore,
                                               Cuint64_t::CSArray& pValue) {
  VkResult recRetVal = *return_value;
  auto valuePtr = *pValue;
  if (valuePtr == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  uint64_t recValue = *valuePtr;
  uint64_t currentValue = 0;
  VkSemaphore semaphoreHandle = *semaphore;
  return_value.Assign(drvVk.vkGetSemaphoreCounterValue(*device, *semaphore, &currentValue));
  if ((*return_value == VK_SUCCESS) && (recRetVal == VK_SUCCESS) && (currentValue < recValue)) {
    VkSemaphoreWaitInfo waitInfo = {
        VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO, // VkStructureType sType;
        nullptr,                               // const void * pNext;
        0,                                     // VkSemaphoreWaitFlags flags;
        1,                                     // uint32_t semaphoreCount;
        &semaphoreHandle,                      // const VkSemaphore * pSemaphores;
        &recValue                              // const uint64_t* pValues;
    };
    drvVk.vkWaitSemaphores(*device, &waitInfo, 0xFFFFFFFFFFFFFFFF);
  }
}

inline void vkGetSemaphoreCounterValueKHR_WRAPRUN(CVkResult& return_value,
                                                  CVkDevice& device,
                                                  CVkSemaphore& semaphore,
                                                  Cuint64_t::CSArray& pValue) {
  VkResult recRetVal = *return_value;
  auto valuePtr = *pValue;
  if (valuePtr == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  uint64_t recValue = *valuePtr;
  uint64_t currentValue = 0;
  VkSemaphore semaphoreHandle = *semaphore;
  return_value.Assign(drvVk.vkGetSemaphoreCounterValueKHR(*device, *semaphore, &currentValue));
  if ((*return_value == VK_SUCCESS) && (recRetVal == VK_SUCCESS) && (currentValue < recValue)) {
    VkSemaphoreWaitInfo waitInfo = {
        VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO, // VkStructureType sType;
        nullptr,                               // const void * pNext;
        0,                                     // VkSemaphoreWaitFlags flags;
        1,                                     // uint32_t semaphoreCount;
        &semaphoreHandle,                      // const VkSemaphore * pSemaphores;
        &recValue                              // const uint64_t* pValues;
    };
    drvVk.vkWaitSemaphoresKHR(*device, &waitInfo, 0xFFFFFFFFFFFFFFFF);
  }
}

inline void vkAllocateMemory_WRAPRUN(CVkResult& recorderSideReturnValue,
                                     CVkDevice& deviceRef,
                                     CVkMemoryAllocateInfo& pAllocateInfo,
                                     CNullWrapper& pAllocator,
                                     CVkDeviceMemory::CSMapArray& pMemory) {
  auto allocateInfoPtr = pAllocateInfo.Value();
  if (allocateInfoPtr == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  auto device = *deviceRef;

  auto dedicatedAllocation = (VkMemoryDedicatedAllocateInfo*)getPNextStructure(
      allocateInfoPtr->pNext, VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO);
  if (dedicatedAllocation) {
    if (dedicatedAllocation->image != VK_NULL_HANDLE) {
      auto& imageState = SD()._imagestates[dedicatedAllocation->image];

      // Get memory requirements for an image if there are none
      if (imageState->memoryRequirements.size == 0) {
        VkMemoryRequirements requirements = {};
        drvVk.vkGetImageMemoryRequirements(device, dedicatedAllocation->image, &requirements);
        vkGetImageMemoryRequirements_SD(device, dedicatedAllocation->image, &requirements);
      }

      // Adjust memory object's allocation size
      if (imageState->memoryRequirements.size > allocateInfoPtr->allocationSize) {
        allocateInfoPtr->allocationSize = imageState->memoryRequirements.size;
      }

      // Adjust memory type from which memory object is allocated
      if (!isBitSet(imageState->memoryRequirements.memoryTypeBits,
                    1 << allocateInfoPtr->memoryTypeIndex)) {
        allocateInfoPtr->memoryTypeIndex = findCompatibleMemoryTypeIndex(
            imageState->deviceStateStore->physicalDeviceStateStore->physicalDeviceHandle,
            allocateInfoPtr->memoryTypeIndex, imageState->memoryRequirements.memoryTypeBits);
      }
    }

    if (dedicatedAllocation->buffer != VK_NULL_HANDLE) {
      auto& bufferState = SD()._bufferstates[dedicatedAllocation->buffer];

      // Get memory requirements for a buffer if there are none
      if (bufferState->memoryRequirements.size == 0) {
        VkMemoryRequirements requirements = {};
        drvVk.vkGetBufferMemoryRequirements(device, dedicatedAllocation->buffer, &requirements);
        vkGetBufferMemoryRequirements_SD(device, dedicatedAllocation->buffer, &requirements);
      }

      // Adjust memory object's allocation size
      if (bufferState->memoryRequirements.size > allocateInfoPtr->allocationSize) {
        allocateInfoPtr->allocationSize = bufferState->memoryRequirements.size;
      }

      // Adjust memory type from which memory object is allocated
      if (!isBitSet(bufferState->memoryRequirements.memoryTypeBits,
                    1 << allocateInfoPtr->memoryTypeIndex)) {
        allocateInfoPtr->memoryTypeIndex = findCompatibleMemoryTypeIndex(
            bufferState->deviceStateStore->physicalDeviceStateStore->physicalDeviceHandle,
            allocateInfoPtr->memoryTypeIndex, bufferState->memoryRequirements.memoryTypeBits);
      }
    }
  }

  VkResult playerSideReturnValue =
      drvVk.vkAllocateMemory(device, allocateInfoPtr, *pAllocator, *pMemory);
  checkReturnValue(playerSideReturnValue, recorderSideReturnValue, "vkAllocateMemory");
  recorderSideReturnValue.Assign(playerSideReturnValue);
  vkAllocateMemory_SD(playerSideReturnValue, device, allocateInfoPtr, *pAllocator, *pMemory);

  VkDeviceMemory* memoryPtr = *pMemory;
  if (memoryPtr == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  if (checkMemoryMappingFeasibility(device, allocateInfoPtr->memoryTypeIndex, false)) {
    //clearMemory
    void* ptr = nullptr;

    VkResult map_return_value = drvVk.vkMapMemory(device, *memoryPtr, 0, VK_WHOLE_SIZE, 0, &ptr);
    if (map_return_value == VK_SUCCESS) {
      memset(ptr, 0, (size_t)allocateInfoPtr->allocationSize);
      drvVk.vkUnmapMemory(device, *memoryPtr);
    } else {
      Log(WARN) << "vkMapMemory() was used to clear allocated memory but failed with the code: "
                << map_return_value << ". It can cause rendering errors!";
    }
  }
}

inline void vkGetImageSubresourceLayout_WRAPRUN(CVkDevice& device,
                                                CVkImage& image,
                                                CVkImageSubresource& pSubresource,
                                                CVkSubresourceLayout& pLayout) {
  VkSubresourceLayout layout;
  if (*pLayout == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  VkSubresourceLayout layout_original = *pLayout;

  drvVk.vkGetImageSubresourceLayout(*device, *image, *pSubresource, &layout);

  if (layout.offset != layout_original.offset) {
    CALL_ONCE[] {
      Log(WARN) << "vkGetImageSubresourceLayout() returned different offset than in gits Recorder. "
                   "Stream was probably recorded on a different HW platform. "
                   "It can cause a crash and/or corruptions.";
    };
  }

  if (layout.rowPitch != layout_original.rowPitch) {
    CALL_ONCE[] {
      Log(WARN)
          << "vkGetImageSubresourceLayout() returned different rowPitch than in gits Recorder. "
             "Stream was probably recorded on a different HW platform. "
             "It can cause a crash and/or corruptions.";
    };
  }
}

namespace {
void GetBufferMemoryRequirementsHelper(VkDeviceSize originalAlignment,
                                       VkDeviceSize currentAlignment) {
  if ((originalAlignment % currentAlignment) != 0) {
    CALL_ONCE[&] {
      Log(WARN) << "Stream recorded on a platform with alignment: " << originalAlignment
                << " Current alignment: " << currentAlignment
                << " It can cause a crash or corruptions.";
    };
  }
}
} // namespace

inline void vkGetBufferMemoryRequirements_WRAPRUN(CVkDevice& device,
                                                  CVkBuffer& buffer,
                                                  CVkMemoryRequirements& pMemoryRequirements) {
  if (*pMemoryRequirements == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  VkMemoryRequirements memRequirements_original = *pMemoryRequirements;
  VkMemoryRequirements memRequirements = {};

  drvVk.vkGetBufferMemoryRequirements(*device, *buffer, &memRequirements);
  GetBufferMemoryRequirementsHelper(memRequirements_original.alignment, memRequirements.alignment);
  vkGetBufferMemoryRequirements_SD(*device, *buffer, &memRequirements);
}

inline void vkGetBufferMemoryRequirements2_WRAPRUN(CVkDevice& device,
                                                   CVkBufferMemoryRequirementsInfo2& pInfo,
                                                   CVkMemoryRequirements2& pMemoryRequirements) {
  if (*pMemoryRequirements == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  VkMemoryRequirements2 memRequirements_original = *pMemoryRequirements;
  VkMemoryDedicatedRequirements dedicatedRequirements = {
      VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS, // VkStructureType  sType
      nullptr,                                         // void*            pNext
      VK_FALSE, // VkBool32         prefersDedicatedAllocation
      VK_FALSE  // VK_VkBool32      requiresDedicatedAllocation
  };
  VkMemoryRequirements2 memRequirements = {
      VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2, // VkStructureType sType;
      nullptr,                                 // void*           pNext
      {}                                       // VkMemoryRequirements memoryRequirements;
  };
  if (memRequirements_original.pNext &&
      (VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS ==
       *static_cast<VkStructureType*>(memRequirements_original.pNext))) {
    memRequirements.pNext = &dedicatedRequirements;
  }

  drvVk.vkGetBufferMemoryRequirements2(*device, *pInfo, &memRequirements);
  GetBufferMemoryRequirementsHelper(memRequirements_original.memoryRequirements.alignment,
                                    memRequirements.memoryRequirements.alignment);
  vkGetBufferMemoryRequirements2_SD(*device, *pInfo, &memRequirements);
}

inline void vkGetBufferMemoryRequirements2KHR_WRAPRUN(CVkDevice& device,
                                                      CVkBufferMemoryRequirementsInfo2& pInfo,
                                                      CVkMemoryRequirements2& pMemoryRequirements) {
  if (*pMemoryRequirements == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  VkMemoryRequirements2 memRequirements_original = *pMemoryRequirements;
  VkMemoryDedicatedRequirements dedicatedRequirements = {
      VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS, // VkStructureType  sType
      nullptr,                                         // void*            pNext
      VK_FALSE, // VkBool32         prefersDedicatedAllocation
      VK_FALSE  // VK_VkBool32      requiresDedicatedAllocation
  };
  VkMemoryRequirements2 memRequirements = {
      VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2, // VkStructureType sType;
      nullptr,                                 // void*           pNext
      {}                                       // VkMemoryRequirements memoryRequirements;
  };
  if (memRequirements_original.pNext &&
      (VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS ==
       *static_cast<VkStructureType*>(memRequirements_original.pNext))) {
    memRequirements.pNext = &dedicatedRequirements;
  }

  drvVk.vkGetBufferMemoryRequirements2KHR(*device, *pInfo, &memRequirements);
  GetBufferMemoryRequirementsHelper(memRequirements_original.memoryRequirements.alignment,
                                    memRequirements.memoryRequirements.alignment);
  vkGetBufferMemoryRequirements2KHR_SD(*device, *pInfo, &memRequirements);
}

namespace {
void GetImageMemoryRequirementsHelper(VkDeviceSize originalAlignment,
                                      VkDeviceSize currentAlignment) {
  if ((originalAlignment % currentAlignment) != 0) {
    CALL_ONCE[&] {
      Log(WARN) << "Stream recorded on a platform with alignment: " << originalAlignment
                << " Current alignment: " << currentAlignment
                << " It can cause a crash or corruptions.";
    };
  }
}
} // namespace

inline void vkGetImageMemoryRequirements_WRAPRUN(CVkDevice& device,
                                                 CVkImage& image,
                                                 CVkMemoryRequirements& pMemoryRequirements) {
  if (*pMemoryRequirements == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  VkMemoryRequirements memRequirements_original = *pMemoryRequirements;
  VkMemoryRequirements memRequirements = {};

  drvVk.vkGetImageMemoryRequirements(*device, *image, &memRequirements);
  GetImageMemoryRequirementsHelper(memRequirements_original.alignment, memRequirements.alignment);
  vkGetImageMemoryRequirements_SD(*device, *image, &memRequirements);
}

inline void vkGetImageMemoryRequirements2_WRAPRUN(CVkDevice& device,
                                                  CVkImageMemoryRequirementsInfo2& pInfo,
                                                  CVkMemoryRequirements2& pMemoryRequirements) {
  if (*pMemoryRequirements == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  VkMemoryRequirements2 memRequirements_original = *pMemoryRequirements;
  VkMemoryDedicatedRequirements dedicatedRequirements = {
      VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS, // VkStructureType  sType
      nullptr,                                         // void*            pNext
      VK_FALSE, // VkBool32         prefersDedicatedAllocation
      VK_FALSE  // VK_VkBool32      requiresDedicatedAllocation
  };
  VkMemoryRequirements2 memRequirements = {
      VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2, // VkStructureType sType;
      nullptr,                                 // void*           pNext
      {}                                       // VkMemoryRequirements memoryRequirements;
  };
  if (memRequirements_original.pNext &&
      (VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS ==
       *static_cast<VkStructureType*>(memRequirements_original.pNext))) {
    memRequirements.pNext = &dedicatedRequirements;
  }

  drvVk.vkGetImageMemoryRequirements2(*device, *pInfo, &memRequirements);
  GetImageMemoryRequirementsHelper(memRequirements_original.memoryRequirements.alignment,
                                   memRequirements.memoryRequirements.alignment);
  vkGetImageMemoryRequirements2_SD(*device, *pInfo, &memRequirements);
}

inline void vkGetImageMemoryRequirements2KHR_WRAPRUN(CVkDevice& device,
                                                     CVkImageMemoryRequirementsInfo2& pInfo,
                                                     CVkMemoryRequirements2& pMemoryRequirements) {
  if (*pMemoryRequirements == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  VkMemoryRequirements2 memRequirements_original = *pMemoryRequirements;
  VkMemoryDedicatedRequirements dedicatedRequirements = {
      VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS, // VkStructureType  sType
      nullptr,                                         // void*            pNext
      VK_FALSE, // VkBool32         prefersDedicatedAllocation
      VK_FALSE  // VK_VkBool32      requiresDedicatedAllocation
  };
  VkMemoryRequirements2 memRequirements = {
      VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2, // VkStructureType sType;
      nullptr,                                 // void*           pNext
      {}                                       // VkMemoryRequirements memoryRequirements;
  };
  if (memRequirements_original.pNext &&
      (VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS ==
       *static_cast<VkStructureType*>(memRequirements_original.pNext))) {
    memRequirements.pNext = &dedicatedRequirements;
  }

  drvVk.vkGetImageMemoryRequirements2KHR(*device, *pInfo, &memRequirements);
  GetImageMemoryRequirementsHelper(memRequirements_original.memoryRequirements.alignment,
                                   memRequirements.memoryRequirements.alignment);
  vkGetImageMemoryRequirements2KHR_SD(*device, *pInfo, &memRequirements);
}

inline void vkGetQueryPoolResults_WRAPRUN(CVkResult& return_value,
                                          CVkDevice& device,
                                          CVkQueryPool& queryPool,
                                          Cuint32_t& firstQuery,
                                          Cuint32_t& queryCount,
                                          Csize_t& dataSize,
                                          Cuint8_t::CSArray& pData,
                                          Cuint64_t& stride,
                                          Cuint32_t& flags) {
  VkResult recRetVal = *return_value;
  return_value.Assign(drvVk.vkGetQueryPoolResults(*device, *queryPool, *firstQuery, *queryCount,
                                                  *dataSize, *pData, *stride, *flags));

  if (*return_value != VK_SUCCESS && (*return_value != recRetVal)) {
    drvVk.vkDeviceWaitIdle(*device);
  }
}

inline void vkWaitForFences_WRAPRUN(CVkResult& return_value,
                                    CVkDevice& device,
                                    Cuint32_t& fenceCount,
                                    CVkFence::CSArray& pFences,
                                    Cuint32_t& waitAll,
                                    Cuint64_t& timeout) {
  VkResult recRetVal = *return_value;

  VkFence* fencesArrayPtr = *pFences;
  if (fencesArrayPtr == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  std::vector<VkFence> fences;
  for (unsigned i = 0; i < *fenceCount; i++) {
    if (SD()._fencestates[fencesArrayPtr[i]]->fenceUsed) {
      fences.push_back(fencesArrayPtr[i]);
    }
  }
  if (fences.size() > 0) {
    return_value.Assign(
        drvVk.vkWaitForFences(*device, (uint32_t)fences.size(), &fences[0], *waitAll, *timeout));

    if (*return_value != VK_SUCCESS && (*return_value != recRetVal)) {
      return_value.Assign(drvVk.vkWaitForFences(*device, (uint32_t)fences.size(), &fences[0],
                                                VK_TRUE, 0xFFFFFFFFFFFFFFFF));
    }
  }
}

inline void vkWaitSemaphores_WRAPRUN(CVkResult& return_value,
                                     CVkDevice& device,
                                     CVkSemaphoreWaitInfo& pWaitInfo,
                                     Cuint64_t& timeout) {
  VkResult recRetVal = *return_value;

  return_value.Assign(drvVk.vkWaitSemaphores(*device, *pWaitInfo, *timeout));

  if ((*return_value != VK_SUCCESS) && (*return_value != recRetVal)) {
    return_value.Assign(drvVk.vkWaitSemaphores(*device, *pWaitInfo, 0xFFFFFFFFFFFFFFFF));
  }
}

inline void vkWaitSemaphoresKHR_WRAPRUN(CVkResult& return_value,
                                        CVkDevice& device,
                                        CVkSemaphoreWaitInfo& pWaitInfo,
                                        Cuint64_t& timeout) {
  VkResult recRetVal = *return_value;

  return_value.Assign(drvVk.vkWaitSemaphoresKHR(*device, *pWaitInfo, *timeout));

  if ((*return_value != VK_SUCCESS) && (*return_value != recRetVal)) {
    return_value.Assign(drvVk.vkWaitSemaphoresKHR(*device, *pWaitInfo, 0xFFFFFFFFFFFFFFFF));
  }
}

namespace {
void BindBufferMemory_WRAPRUNHelper(VkDevice device,
                                    VkBuffer buffer,
                                    VkDeviceMemory memory,
                                    uint64_t memoryOffset) {
  VkMemoryRequirements memRequirements;
  drvVk.vkGetBufferMemoryRequirements(device, buffer, &memRequirements);
  auto memAllocateInfo = SD()._devicememorystates[memory]->memoryAllocateInfoData.Value();
  bool incompatibilityError = false;

  if ((memRequirements.alignment > 0) && ((memoryOffset % memRequirements.alignment) != 0)) {
    Log(ERR) << "Offset of a memory bound to a buffer is not divisible by the required alignment. "
                "It can cause crash or corruptions!!";
    Log(ERR, NO_PREFIX) << "    Memory offset used: " << memoryOffset;
    Log(ERR, NO_PREFIX) << "    Required alignment: " << memRequirements.alignment;
    incompatibilityError = true;
  }

  if (((1 << memAllocateInfo->memoryTypeIndex) & memRequirements.memoryTypeBits) == 0) {
    Log(ERR) << "Memory object bound to a buffer is allocated from an incompatible memory type "
                "index. It can cause crash or corruptions!!";
    incompatibilityError = true;
  }

  if ((memAllocateInfo->allocationSize - (memoryOffset)) < memRequirements.size) {
    Log(ERR) << "Too small memory block bound to a buffer. It can cause crash or corruptions!!";
    Log(ERR, NO_PREFIX) << "    Memory object's allocation size: "
                        << memAllocateInfo->allocationSize;
    Log(ERR, NO_PREFIX) << "    Provided offset: " << memoryOffset;
    Log(ERR, NO_PREFIX) << "    Required size: " << memRequirements.size;
    incompatibilityError = true;
  }

  if (incompatibilityError && !Config::Get().player.ignoreVKCrossPlatformIncompatibilitiesWA) {
    throw std::runtime_error("Properties of a memory object bound to a buffer are incompatible "
                             "with current platform. Exiting!!");
  }
}
} // namespace

inline void vkBindBufferMemory_WRAPRUN(CVkResult& recorderSideReturnValue,
                                       CVkDevice& device,
                                       CVkBuffer& buffer,
                                       CVkDeviceMemory& memory,
                                       Cuint64_t& memoryOffset) {
  BindBufferMemory_WRAPRUNHelper(*device, *buffer, *memory, *memoryOffset);

  VkResult playerSideReturnValue =
      drvVk.vkBindBufferMemory(*device, *buffer, *memory, *memoryOffset);
  checkReturnValue(playerSideReturnValue, recorderSideReturnValue, "vkBindBufferMemory");
  recorderSideReturnValue.Assign(playerSideReturnValue);
  vkBindBufferMemory_SD(playerSideReturnValue, *device, *buffer, *memory, *memoryOffset);
}

inline void vkBindBufferMemory2_WRAPRUN(CVkResult& recorderSideReturnValue,
                                        CVkDevice& device,
                                        Cuint32_t& bindInfoCount,
                                        CVkBindBufferMemoryInfoArray& pBindInfos) {
  auto bindInfosPtr = *pBindInfos;
  if (bindInfosPtr == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  for (uint32_t i = 0; i < *bindInfoCount; ++i) {
    auto& bindBufferMemoryInfo = bindInfosPtr[i];
    BindBufferMemory_WRAPRUNHelper(*device, bindBufferMemoryInfo.buffer,
                                   bindBufferMemoryInfo.memory, bindBufferMemoryInfo.memoryOffset);
  }

  VkResult playerSideReturnValue = drvVk.vkBindBufferMemory2(*device, *bindInfoCount, bindInfosPtr);
  checkReturnValue(playerSideReturnValue, recorderSideReturnValue, "vkBindBufferMemory2");
  recorderSideReturnValue.Assign(playerSideReturnValue);
  vkBindBufferMemory2_SD(playerSideReturnValue, *device, *bindInfoCount, bindInfosPtr);
}

inline void vkBindBufferMemory2KHR_WRAPRUN(CVkResult& recorderSideReturnValue,
                                           CVkDevice& device,
                                           Cuint32_t& bindInfoCount,
                                           CVkBindBufferMemoryInfoArray& pBindInfos) {
  auto bindInfosPtr = *pBindInfos;
  if (bindInfosPtr == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  for (uint32_t i = 0; i < *bindInfoCount; ++i) {
    auto& bindBufferMemoryInfo = bindInfosPtr[i];
    BindBufferMemory_WRAPRUNHelper(*device, bindBufferMemoryInfo.buffer,
                                   bindBufferMemoryInfo.memory, bindBufferMemoryInfo.memoryOffset);
  }

  VkResult playerSideReturnValue =
      drvVk.vkBindBufferMemory2KHR(*device, *bindInfoCount, bindInfosPtr);
  checkReturnValue(playerSideReturnValue, recorderSideReturnValue, "vkBindBufferMemory2KHR");
  recorderSideReturnValue.Assign(playerSideReturnValue);
  vkBindBufferMemory2KHR_SD(playerSideReturnValue, *device, *bindInfoCount, bindInfosPtr);
}

namespace {
void BindImageMemory_WRAPRUNHelper(VkDevice device,
                                   VkImage image,
                                   VkDeviceMemory memory,
                                   uint64_t memoryOffset) {
  VkMemoryRequirements memRequirements;
  drvVk.vkGetImageMemoryRequirements(device, image, &memRequirements);
  auto memAllocateInfo = SD()._devicememorystates[memory]->memoryAllocateInfoData.Value();
  bool incompatibilityError = false;

  if ((memRequirements.alignment > 0) && ((memoryOffset % memRequirements.alignment) != 0)) {
    Log(ERR) << "Offset of a memory bound to an image is not divisible by the required alignment. "
                "It can cause crash or corruptions!!";
    Log(ERR, NO_PREFIX) << "    Memory offset used: " << memoryOffset;
    Log(ERR, NO_PREFIX) << "    Required alignment: " << memRequirements.alignment;
    incompatibilityError = true;
  }

  if (((1 << memAllocateInfo->memoryTypeIndex) & memRequirements.memoryTypeBits) == 0) {
    Log(ERR) << "Memory object bound to an image is allocated from an incompatible memory type "
                "index. It can cause crash or corruptions!!";
    incompatibilityError = true;
  }

  if ((memAllocateInfo->allocationSize - (memoryOffset)) < memRequirements.size) {
    Log(ERR) << "Too small memory block bound to an image. It can cause crash or corruptions!!";
    Log(ERR, NO_PREFIX) << "    Memory object's allocation size: "
                        << memAllocateInfo->allocationSize;
    Log(ERR, NO_PREFIX) << "    Provided offset: " << memoryOffset;
    Log(ERR, NO_PREFIX) << "    Required size: " << memRequirements.size;
    incompatibilityError = true;
  }

  if (incompatibilityError && !Config::Get().player.ignoreVKCrossPlatformIncompatibilitiesWA) {
    throw std::runtime_error("Properties of a memory object bound to an image are incompatible "
                             "with current platform. Exiting!!");
  }
}
} // namespace

inline void vkBindImageMemory_WRAPRUN(CVkResult& recorderSideReturnValue,
                                      CVkDevice& device,
                                      CVkImage& image,
                                      CVkDeviceMemory& memory,
                                      Cuint64_t& memoryOffset) {
  BindImageMemory_WRAPRUNHelper(*device, *image, *memory, *memoryOffset);

  VkResult playerSideReturnValue = drvVk.vkBindImageMemory(*device, *image, *memory, *memoryOffset);
  checkReturnValue(playerSideReturnValue, recorderSideReturnValue, "vkBindImageMemory");
  recorderSideReturnValue.Assign(playerSideReturnValue);
  vkBindImageMemory_SD(playerSideReturnValue, *device, *image, *memory, *memoryOffset);
}

inline void vkBindImageMemory2_WRAPRUN(CVkResult& recorderSideReturnValue,
                                       CVkDevice& device,
                                       Cuint32_t& bindInfoCount,
                                       CVkBindImageMemoryInfoArray& pBindInfos) {
  auto bindInfosPtr = *pBindInfos;
  if (bindInfosPtr == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  for (uint32_t i = 0; i < *bindInfoCount; ++i) {
    auto& bindImageMemoryInfo = bindInfosPtr[i];
    BindImageMemory_WRAPRUNHelper(*device, bindImageMemoryInfo.image, bindImageMemoryInfo.memory,
                                  bindImageMemoryInfo.memoryOffset);
  }

  VkResult playerSideReturnValue = drvVk.vkBindImageMemory2(*device, *bindInfoCount, bindInfosPtr);
  checkReturnValue(playerSideReturnValue, recorderSideReturnValue, "vkBindImageMemory2");
  recorderSideReturnValue.Assign(playerSideReturnValue);
  vkBindImageMemory2_SD(playerSideReturnValue, *device, *bindInfoCount, bindInfosPtr);
}

inline void vkBindImageMemory2KHR_WRAPRUN(CVkResult& recorderSideReturnValue,
                                          CVkDevice& device,
                                          Cuint32_t& bindInfoCount,
                                          CVkBindImageMemoryInfoArray& pBindInfos) {
  auto bindInfosPtr = *pBindInfos;
  if (bindInfosPtr == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  for (uint32_t i = 0; i < *bindInfoCount; ++i) {
    auto& bindImageMemoryInfo = bindInfosPtr[i];
    BindImageMemory_WRAPRUNHelper(*device, bindImageMemoryInfo.image, bindImageMemoryInfo.memory,
                                  bindImageMemoryInfo.memoryOffset);
  }

  VkResult playerSideReturnValue =
      drvVk.vkBindImageMemory2KHR(*device, *bindInfoCount, bindInfosPtr);
  checkReturnValue(playerSideReturnValue, recorderSideReturnValue, "vkBindImageMemory2KHR");
  recorderSideReturnValue.Assign(playerSideReturnValue);
  vkBindImageMemory2KHR_SD(playerSideReturnValue, *device, *bindInfoCount, bindInfosPtr);
}

inline void vkCreateDevice_WRAPRUN(CVkResult& recorderSideReturnValue,
                                   CVkPhysicalDevice& physicalDevice,
                                   CVkDeviceCreateInfo& pCreateInfo,
                                   CNullWrapper& pAllocator,
                                   CVkDevice::CSMapArray& pDevice) {
  bool allSupported = true;
  if (*pCreateInfo == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  VkDeviceCreateInfo createInfo = *pCreateInfo;
  std::vector<const char*> requestedExtensions(createInfo.ppEnabledExtensionNames,
                                               createInfo.ppEnabledExtensionNames +
                                                   createInfo.enabledExtensionCount);
  std::vector<const char*> requestedLayers(createInfo.ppEnabledLayerNames,
                                           createInfo.ppEnabledLayerNames +
                                               createInfo.enabledLayerCount);

  suppressPhysicalDeviceFeatures(
      Config::Get().player.suppressVKDeviceFeatures,
      const_cast<VkPhysicalDeviceFeatures*>(createInfo.pEnabledFeatures));
  suppressRequestedNames(requestedExtensions, Config::Get().player.suppressVKExtensions,
                         createInfo.enabledExtensionCount, createInfo.ppEnabledExtensionNames);
  suppressRequestedNames(requestedLayers, Config::Get().player.suppressVKLayers,
                         createInfo.enabledLayerCount, createInfo.ppEnabledLayerNames);

  Log(TRACE, NO_PREFIX) << "";
  Log(TRACE) << " ------------------ ";
  Log(TRACE) << "Device compatibility check section begin.\n";
  // Trace memory properties from the original platform the stream was recorded on
  VkLog(TRACE) << "Memory properties of the platform the (original) stream was recorded on: "
                  "VkPhysicalDevice physicalDevice="
               << *physicalDevice << ", VkPhysicalDeviceMemoryProperties* pMemoryProperties="
               << &SD()._physicaldevicestates[*physicalDevice]->memoryProperties;

  allSupported &= checkForSupportForPhysicalDeviceFeatures(
      *physicalDevice, const_cast<VkPhysicalDeviceFeatures*>(createInfo.pEnabledFeatures));
  allSupported &= areDeviceExtensionsSupported(*physicalDevice, createInfo.enabledExtensionCount,
                                               createInfo.ppEnabledExtensionNames, true);
  allSupported &= checkForSupportForQueues(*physicalDevice, createInfo.queueCreateInfoCount,
                                           createInfo.pQueueCreateInfos);

  Log(TRACE, NO_PREFIX) << "";

  if (!allSupported && !Config::Get().player.ignoreVKCrossPlatformIncompatibilitiesWA) {
    throw std::runtime_error("Error - stream is incompatible with the current platform. Exiting!");
  }

  Log(TRACE) << "Device compatibility check section end"
             << (allSupported ? "." : " - please read warning messages!!");
  Log(TRACE) << " ------------------ \n";

  VkResult playerSideReturnValue =
      drvVk.vkCreateDevice(*physicalDevice, &createInfo, *pAllocator, *pDevice);
  checkReturnValue(playerSideReturnValue, recorderSideReturnValue, "vkCreateDevice");
  recorderSideReturnValue.Assign(playerSideReturnValue);
  vkCreateDevice_SD(playerSideReturnValue, *physicalDevice, *pCreateInfo, *pAllocator, *pDevice);

#if defined(GITS_PLATFORM_WINDOWS)
  if (Config::Get().player.renderDoc.frameRecEnabled ||
      Config::Get().player.renderDoc.queuesubmitRecEnabled) {
    auto vkInstance =
        SD()._physicaldevicestates[*physicalDevice]->instanceStateStore->instanceHandle;
    RenderDocUtil::GetInstance().SetRenderDocDevice(vkInstance);
  }
  if (Config::Get().player.renderDoc.frameRecEnabled &&
      Config::Get().player.renderDoc.captureRange[CGits::Instance().CurrentFrame()] &&
      CGits::Instance().CurrentFrame() == 1) {
    RenderDocUtil::GetInstance().StartRecording();
  }
#endif

  if (!Config::Get().player.overrideVKPipelineCache.empty()) {
    auto initialData = GetBinaryFileContents(Config::Get().player.overrideVKPipelineCache.string());

    VkPipelineCacheCreateInfo cacheCreateInfo = {
        VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO, // VkStructureType sType;
        nullptr,                                      // const void* pNext;
        0,                                            // VkPipelineCacheCreateFlags flags;
        initialData.size(),                           // size_t initialDataSize;
        initialData.data()                            // const void* pInitialData;
    };
    VkPipelineCache pipelineCache = VK_NULL_HANDLE;
    VkDevice* devicePtr = *pDevice;
    if (devicePtr == nullptr) {
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }
    drvVk.vkCreatePipelineCache(*devicePtr, &cacheCreateInfo, nullptr, &pipelineCache);
    if (pipelineCache != VK_NULL_HANDLE) {
      SD().internalResources.pipelineCacheHandles[*devicePtr] = pipelineCache;
    }
  }
}

inline void vkCreateSwapchainKHR_WRAPRUN(CVkResult& recorderSideReturnValue,
                                         CVkDevice& device,
                                         CVkSwapchainCreateInfoKHR& pCreateInfo,
                                         CNullWrapper& pAllocator,
                                         CVkSwapchainKHR::CSMapArray& pSwapchain) {
  if (*pCreateInfo == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  VkSwapchainCreateInfoKHR createInfo = *pCreateInfo;
  if (!Config::Get().player.captureFrames.empty() ||
      !Config::Get().player.captureVulkanSubmits.empty() ||
      !Config::Get().player.captureVulkanRenderPasses.empty() ||
      !Config::Get().player.captureVulkanRenderPassesResources.empty()) {
    createInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    Log(TRACE) << "Modifying swapchain usage for frames/render targets capturing!!";
  }
  if (Config::Get().player.forceWindowSize) {
    createInfo.imageExtent.width = Config::Get().player.windowSize.first;
    createInfo.imageExtent.height = Config::Get().player.windowSize.second;
  }
  VkResult playerSideReturnValue =
      drvVk.vkCreateSwapchainKHR(*device, &createInfo, *pAllocator, *pSwapchain);
  checkReturnValue(playerSideReturnValue, recorderSideReturnValue, "vkCreateSwapchainKHR");
  recorderSideReturnValue.Assign(playerSideReturnValue);
  vkCreateSwapchainKHR_SD(playerSideReturnValue, *device, *pCreateInfo, *pAllocator, *pSwapchain);

  VkSwapchainKHR* swapchainPtr = *pSwapchain;

  if (Config::Get().player.renderOffscreen && (swapchainPtr != nullptr)) {
    auto& swapchainState = SD()._swapchainkhrstates[*swapchainPtr];
    for (uint32_t i = 0; i < swapchainState->imageStateStoreList.size(); ++i) {
      uint32_t imageIndex;
      drvVk.vkAcquireNextImageKHR(*device, *swapchainPtr, 5000000000, VK_NULL_HANDLE,
                                  VK_NULL_HANDLE, &imageIndex);
    }
  }
}

namespace {
void ForceScissor_Helper(VkRect2D* originalScissorRect) {
  if (originalScissorRect) {
    const std::vector<int>& rect = Config::Get().player.scissorCoords;

    auto& offset = originalScissorRect->offset;
    auto& extent = originalScissorRect->extent;

    originalScissorRect->offset.x = std::max(rect[0], offset.x);
    originalScissorRect->offset.y = std::max(rect[1], offset.y);
    originalScissorRect->extent.width = std::min(static_cast<uint32_t>(rect[2]), extent.width);
    originalScissorRect->extent.height = std::min(static_cast<uint32_t>(rect[3]), extent.height);
  }
}

template <class CREATE_INFO>
inline VkResult CreatePipelines_Helper(
    VkResult(VKAPI_CALL* callCreatePipelines)(VkDevice,
                                              VkPipelineCache,
                                              uint32_t,
                                              const CREATE_INFO*,
                                              const VkAllocationCallbacks*,
                                              VkPipeline*),
    VkDevice device,
    VkPipelineCache pipelineCache,
    uint32_t createInfoCount,
    CREATE_INFO* pCreateInfos,
    const VkAllocationCallbacks* pAllocator,
    VkPipeline* pPipelines) {
  auto return_value = VK_SUCCESS;

  VkPipelineCache cacheToUse = pipelineCache;
  if (!Config::Get().player.overrideVKPipelineCache.empty() &&
      SD().internalResources.pipelineCacheHandles[device] != VK_NULL_HANDLE) {
    cacheToUse = SD().internalResources.pipelineCacheHandles[device];
  }

  CREATE_INFO* createInfosToUse = pCreateInfos;

  for (uint32_t i = 0; i < createInfoCount; ++i) {
    createInfosToUse[i].flags =
        createInfosToUse[i].flags & (~VK_PIPELINE_CREATE_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT);
    createInfosToUse[i].basePipelineHandle = VK_NULL_HANDLE;
    createInfosToUse[i].basePipelineIndex = VK_NULL_HANDLE;
  }

  if (Config::Get().player.forceMultithreadedPipelineCompilation && createInfoCount > 1) {
    VkPipeline* pipelinesToCreate = pPipelines;

    // Calculate number of pipelines per thread
    uint32_t numThreads = std::min(createInfoCount, std::thread::hardware_concurrency());
    uint32_t pipelinesPerThread = createInfoCount / numThreads;
    uint32_t remainingPipelines = createInfoCount % numThreads;
    std::vector<std::thread> threads(numThreads);

    for (size_t i = 0; i < numThreads; ++i) {
      uint32_t numPipelinesInThread = pipelinesPerThread + std::min(1u, remainingPipelines);
      threads[i] = std::thread(callCreatePipelines, device, cacheToUse, numPipelinesInThread,
                               createInfosToUse, pAllocator, pipelinesToCreate);

      createInfosToUse = &createInfosToUse[numPipelinesInThread];
      pipelinesToCreate = &pipelinesToCreate[numPipelinesInThread];
      remainingPipelines = (remainingPipelines > 0) ? remainingPipelines - 1 : 0;
    }

    for (size_t i = 0; i < numThreads; ++i) {
      threads[i].join();
    }

    for (uint32_t i = 0; i < createInfoCount; ++i) {
      if (pPipelines[i] == VK_NULL_HANDLE) {
        return_value = VK_ERROR_INITIALIZATION_FAILED;
        break;
      }
    }
  } else {
    return_value = callCreatePipelines(device, cacheToUse, createInfoCount, createInfosToUse,
                                       pAllocator, pPipelines);
  }

  return return_value;
}
} // namespace

inline void vkCreateGraphicsPipelines_WRAPRUN(CVkResult& recorderSideReturnValue,
                                              CVkDevice& device,
                                              CVkPipelineCache& pipelineCache,
                                              Cuint32_t& createInfoCount,
                                              CVkGraphicsPipelineCreateInfoArray& pCreateInfos,
                                              CNullWrapper& pAllocator,
                                              CVkPipeline::CSMapArray& pPipelines) {
  VkGraphicsPipelineCreateInfo* modifiedCreateInfos = *pCreateInfos;
  if (modifiedCreateInfos == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  if (Config::Get().player.forceScissor) {
    for (uint32_t i = 0; i < *createInfoCount; ++i) {
      ForceScissor_Helper(const_cast<VkRect2D*>(modifiedCreateInfos[i].pViewportState->pScissors));
    }
  }

  VkResult playerSideReturnValue =
      CreatePipelines_Helper(drvVk.vkCreateGraphicsPipelines, *device, *pipelineCache,
                             *createInfoCount, modifiedCreateInfos, *pAllocator, *pPipelines);
  checkReturnValue(playerSideReturnValue, recorderSideReturnValue, "vkCreateGraphicsPipelines");
  recorderSideReturnValue.Assign(playerSideReturnValue);
  vkCreateGraphicsPipelines_SD(playerSideReturnValue, *device, *pipelineCache, *createInfoCount,
                               modifiedCreateInfos, *pAllocator, *pPipelines);
}

inline void vkCreateComputePipelines_WRAPRUN(CVkResult& recorderSideReturnValue,
                                             CVkDevice& device,
                                             CVkPipelineCache& pipelineCache,
                                             Cuint32_t& createInfoCount,
                                             CVkComputePipelineCreateInfoArray& pCreateInfos,
                                             CNullWrapper& pAllocator,
                                             CVkPipeline::CSMapArray& pPipelines) {
  VkResult playerSideReturnValue =
      CreatePipelines_Helper(drvVk.vkCreateComputePipelines, *device, *pipelineCache,
                             *createInfoCount, *pCreateInfos, *pAllocator, *pPipelines);
  checkReturnValue(playerSideReturnValue, recorderSideReturnValue, "vkCreateComputePipelines");
  recorderSideReturnValue.Assign(playerSideReturnValue);
  vkCreateComputePipelines_SD(playerSideReturnValue, *device, *pipelineCache, *createInfoCount,
                              *pCreateInfos, *pAllocator, *pPipelines);
}

inline void vkCmdSetScissor_WRAPRUN(CVkCommandBuffer& commandBuffer,
                                    Cuint32_t& firstScissor,
                                    Cuint32_t& scissorCount,
                                    CVkRect2DArray& pScissors) {
  VkRect2D* scissors = *pScissors;

  if (Config::Get().player.forceScissor) {
    for (uint32_t i = 0; i < *scissorCount; ++i) {
      ForceScissor_Helper(&scissors[i]);
    }
  }
  if (Config::Get().player.execCmdBuffsBeforeQueueSubmit) {
    SD()._commandbufferstates[*commandBuffer]->tokensBuffer.Add(
        new CvkCmdSetScissor(commandBuffer.Original(), firstScissor.Original(),
                             scissorCount.Original(), pScissors.Original()));
  } else {
    drvVk.vkCmdSetScissor(*commandBuffer, *firstScissor, *scissorCount, scissors);
  }
}

inline void vkCreateImage_WRAPRUN(CVkResult& recorderSideReturnValue,
                                  CVkDevice& device,
                                  CVkImageCreateInfo& pCreateInfo,
                                  CNullWrapper& pAllocator,
                                  CVkImage::CSMapArray& image) {
  if (*pCreateInfo == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  VkImageCreateInfo createInfo = *pCreateInfo;
  if (!Config::Get().player.captureVulkanSubmits.empty() ||
      !Config::Get().player.captureVulkanSubmitsResources.empty() ||
      !Config::Get().player.captureVulkanRenderPasses.empty() ||
      !Config::Get().player.captureVulkanRenderPassesResources.empty()) {
    createInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    Log(TRACE) << "Modifying image usage for render targets capturing!!";
  }
  VkResult playerSideReturnValue = drvVk.vkCreateImage(*device, &createInfo, *pAllocator, *image);
  checkReturnValue(playerSideReturnValue, recorderSideReturnValue, "vkCreateImage");
  recorderSideReturnValue.Assign(playerSideReturnValue);
  vkCreateImage_SD(playerSideReturnValue, *device, *pCreateInfo, *pAllocator, *image);
}

inline void vkCreateBuffer_WRAPRUN(CVkResult& recorderSideReturnValue,
                                   CVkDevice& device,
                                   CVkBufferCreateInfo& pCreateInfo,
                                   CNullWrapper& pAllocator,
                                   CVkBuffer::CSMapArray& pBuffer) {
  if (*pCreateInfo == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  VkBufferCreateInfo createInfo = *pCreateInfo;
  if (!Config::Get().player.captureVulkanSubmitsResources.empty()) {
    createInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    Log(TRACE) << "Modifying buffer usage for resource capturing!!";
  }
  VkResult playerSideReturnValue =
      drvVk.vkCreateBuffer(*device, &createInfo, *pAllocator, *pBuffer);
  checkReturnValue(playerSideReturnValue, recorderSideReturnValue, "vkCreateBuffer");
  recorderSideReturnValue.Assign(playerSideReturnValue);
  vkCreateBuffer_SD(playerSideReturnValue, *device, *pCreateInfo, *pAllocator, *pBuffer);
}

inline void vkPassPhysicalDeviceMemoryPropertiesGITS_WRAPRUN(
    CVkPhysicalDevice& physicalDevice, CVkPhysicalDeviceMemoryProperties& pMemoryProperties) {
  // If the function is available, it means GITS Recorder is attached and we need to pass memory properties from the original platform the stream was recorded on
  // If the function is not available, internal GITS mechanism will prevent null-ptr function call
  drvVk.vkPassPhysicalDeviceMemoryPropertiesGITS(*physicalDevice, *pMemoryProperties);

  vkPassPhysicalDeviceMemoryPropertiesGITS_SD(*physicalDevice, *pMemoryProperties);
}

inline void vkDestroyDevice_WRAPRUN(CVkDevice& device, CNullWrapper& pAllocator) {
  // If vkDestroyDevice() function is recorded and replayed, then we need to destroy all the device-level resources before the device destruction
  destroyDeviceLevelResources(*device);

  drvVk.vkDestroyDevice(*device, *pAllocator);
  vkDestroyDevice_SD(*device, *pAllocator);
  device.RemoveMapping();
}

inline void vkDestroyInstance_WRAPRUN(CVkInstance& instance, CNullWrapper& pAllocator) {
  // If vkDestroyInstance() function is recorded and replayed, then we need to destroy all the instance-level resources before the instance destruction
  destroyInstanceLevelResources(*instance);

  drvVk.vkDestroyInstance(*instance, *pAllocator);
  vkDestroyInstance_SD(*instance, *pAllocator);
  instance.RemoveMapping();
}

inline void vkCreateCommandPool_WRAPRUN(CVkResult& recorderSideReturnValue,
                                        CVkDevice& device,
                                        CVkCommandPoolCreateInfo& pCreateInfo,
                                        CNullWrapper& pAllocator,
                                        CVkCommandPool::CSMapArray& pCommandPool) {
  if (*pCreateInfo == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  VkCommandPoolCreateInfo createInfo = *pCreateInfo;
  if (Config::Get().player.execCmdBuffsBeforeQueueSubmit) {
    createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  }
  VkResult playerSideReturnValue =
      drvVk.vkCreateCommandPool(*device, &createInfo, *pAllocator, *pCommandPool);
  checkReturnValue(playerSideReturnValue, recorderSideReturnValue, "vkCreateCommandPool");
  recorderSideReturnValue.Assign(playerSideReturnValue);
  vkCreateCommandPool_SD(playerSideReturnValue, *device, *pCreateInfo, *pAllocator, *pCommandPool);
}

inline void vkGetBufferDeviceAddressUnifiedGITS_WRAPRUN(Cuint64_t& _return_value,
                                                        CVkDevice& _device,
                                                        CVkBufferDeviceAddressInfo& _pInfo) {
  auto bufferDeviceAddress = drvVk.vkGetBufferDeviceAddressUnifiedGITS(*_device, *_pInfo);
  vkGetBufferDeviceAddressUnifiedGITS_SD(bufferDeviceAddress, *_device, *_pInfo);
}

inline void vkBeginCommandBuffer_WRAPRUN(CVkResult& return_value,
                                         CVkCommandBuffer& commandBuffer,
                                         CVkCommandBufferBeginInfo& pBeginInfo) {
  if (!Config::Get().player.execCmdBuffsBeforeQueueSubmit) {
    return_value.Assign(drvVk.vkBeginCommandBuffer(*commandBuffer, *pBeginInfo));
  }
  // State Tracking is necessary also when using execCmdBuffsBeforeQueueSubmit or captureVulkanRenderPasses because we will call vkBeginCommandBuffer manually later.
  vkBeginCommandBuffer_SD(*return_value, *commandBuffer, *pBeginInfo);
}

inline void vkEndCommandBuffer_WRAPRUN(CVkResult& return_value, CVkCommandBuffer& commandBuffer) {
  if (!Config::Get().player.execCmdBuffsBeforeQueueSubmit) {
    return_value.Assign(drvVk.vkEndCommandBuffer(*commandBuffer));
  }
  // State Tracking is necessary also when using execCmdBuffsBeforeQueueSubmit or captureVulkanRenderPasses because we will call vkEndCommandBuffer manually later.
  vkEndCommandBuffer_SD(*return_value, *commandBuffer);
}

inline void vkCmdExecuteCommands_WRAPRUN(CVkCommandBuffer& commandBuffer,
                                         Cuint32_t& commandBufferCount,
                                         CVkCommandBuffer::CSArray& pCommandBuffers) {
  if (Config::Get().player.execCmdBuffsBeforeQueueSubmit) {
    SD()._commandbufferstates[*commandBuffer]->tokensBuffer.Add(new CvkCmdExecuteCommands(
        commandBuffer.Original(), commandBufferCount.Original(), pCommandBuffers.Original()));
    if (*commandBufferCount > 0) {
      auto commandBuffersVector = *pCommandBuffers;
      if (commandBuffersVector == nullptr) {
        throw std::runtime_error(EXCEPTION_MESSAGE);
      }
      for (unsigned i = 0; i < *commandBufferCount; i++) {
        SD()._commandbufferstates[*commandBuffer]->secondaryCommandBuffers.push_back(
            commandBuffersVector[i]);
      }
    }
  } else {
    drvVk.vkCmdExecuteCommands(*commandBuffer, *commandBufferCount, *pCommandBuffers);
    vkCmdExecuteCommands_SD(*commandBuffer, *commandBufferCount, *pCommandBuffers);
  }
}
} // namespace Vulkan
} // namespace gits

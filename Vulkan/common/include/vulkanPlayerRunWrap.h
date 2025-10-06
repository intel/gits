// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "vulkanStateTracking.h"
#include "vulkan_apis_iface.h"
#include "vulkanDrivers.h"
#include "vulkanLog.h"

#include <thread>

#if defined(GITS_PLATFORM_WINDOWS)
#include "vulkanRenderDocUtil.h"
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
  if (Configurator::Get().common.player.renderOffscreen) {
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
      LOG_TRACE << "vkAcquireNextImage2KHR restore section begin.";
      uint32_t maxAllowedVkSwapchainRewinds =
          Configurator::Get().vulkan.player.maxAllowedVkSwapchainRewinds;
      uint32_t rewindCount = 0;
      while (recorderIndex != *indexPtr) {
        if (++rewindCount > maxAllowedVkSwapchainRewinds) {
          LOG_ERROR << "Maximum swapchain rewind limit (" << maxAllowedVkSwapchainRewinds
                    << ") exceeded.";
          throw std::runtime_error(EXCEPTION_MESSAGE);
        }

        RewindSwapchainImageIndex(
            *device, SD()._devicestates[*device]->queueStateStoreList[0]->queueHandle,
            acquireInfo->fence, acquireInfo->semaphore, acquireInfo->swapchain, indexPtr);

        return_value.Assign(drvVk.vkAcquireNextImage2KHR(*device, acquireInfo, indexPtr));
      }
      LOG_TRACE << "vkAcquireNextImage2KHR restore section end.";
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
  if (Configurator::Get().common.player.renderOffscreen) {
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
      LOG_TRACE << "vkAcquireNextImageKHR restore section begin.";
      uint32_t maxAllowedVkSwapchainRewinds =
          Configurator::Get().vulkan.player.maxAllowedVkSwapchainRewinds;
      uint32_t rewindCount = 0;
      while (recorderIndex != *indexPtr) {
        if (++rewindCount > maxAllowedVkSwapchainRewinds) {
          LOG_ERROR << "Maximum swapchain rewind limit (" << maxAllowedVkSwapchainRewinds
                    << ") exceeded.";
          throw std::runtime_error(EXCEPTION_MESSAGE);
        }
        RewindSwapchainImageIndex(*device,
                                  SD()._devicestates[*device]->queueStateStoreList[0]->queueHandle,
                                  *fence, *semaphore, *swapchain, indexPtr);

        return_value.Assign(drvVk.vkAcquireNextImageKHR(*device, *swapchain, *timeout, *semaphore,
                                                        *fence, indexPtr));
      }
      LOG_TRACE << "vkAcquireNextImageKHR restore section end.";
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
  CAutoCaller autoCaller(drvVk.vkPauseRecordingGITS, drvVk.vkContinueRecordingGITS);

  uint32_t selectedPhysicalDeviceIndex =
      Configurator::Get().vulkan.player.vulkanForcedPhysicalDeviceIndex;

  std::vector<VkPhysicalDeviceProperties> playerSideDevicesProperties(playerSideDevices.size());
  for (uint32_t i = 0; i < playerSideDevices.size(); ++i) {
    drvVk.vkGetPhysicalDeviceProperties(playerSideDevices[i], &playerSideDevicesProperties[i]);
  }

  // Select a device with a provided name
  if (Configurator::Get().vulkan.player.vulkanForcedPhysicalDeviceName.size() > 0) {
    for (uint32_t i = 0; i < playerSideDevices.size(); ++i) {
      auto deviceName = ToLowerCopy(playerSideDevicesProperties[i].deviceName);
      auto requestedName =
          ToLowerCopy(Configurator::Get().vulkan.player.vulkanForcedPhysicalDeviceName);
      if (strstr(deviceName.c_str(), requestedName.c_str()) != nullptr) {
        selectedPhysicalDeviceIndex = i;
        break;
      }
    }
  }
  // Select a device of a provided type
  if (Configurator::Get().vulkan.player.vulkanForcedPhysicalDeviceType != TDeviceType::ANY) {
    for (uint32_t i = 0; i < playerSideDevices.size(); ++i) {
      if (((Configurator::Get().vulkan.player.vulkanForcedPhysicalDeviceType ==
            TDeviceType::INTEGRATED) &&
           (playerSideDevicesProperties[i].deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)) ||
          ((Configurator::Get().vulkan.player.vulkanForcedPhysicalDeviceType ==
            TDeviceType::DISCRETE) &&
           (playerSideDevicesProperties[i].deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU))) {
        selectedPhysicalDeviceIndex = i;
        break;
      }
    }
  }

  if (selectedPhysicalDeviceIndex >= playerSideDevices.size()) {
    LOG_WARNING
        << "Selected physical device index is greater than the number of enumerated physical "
           "devices. Defaulting to 0.";
    selectedPhysicalDeviceIndex = 0;
  }

  VkPhysicalDevice selectedPhysicalDevice = playerSideDevices[selectedPhysicalDeviceIndex];
  VkPhysicalDeviceProperties& physicalDeviceProperties =
      playerSideDevicesProperties[selectedPhysicalDeviceIndex];

  CALL_ONCE[&physicalDeviceProperties] {
    LOG_INFO << "Playing stream on a"
             << ((physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
                     ? ("n integrated")
                     : (" discrete"))
             << " device named \"" << physicalDeviceProperties.deviceName << "\".";
  };
  if (playerSideDevices.size() > 1) {
    CALL_ONCE[] {
      LOG_INFO << "      (To change device, use the following option: "
               << "--useVKPhysicalDeviceIndex <index>)";
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

  drvVk.Initialize();

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

  suppressRequestedNames(requestedExtensions, Configurator::Get().vulkan.shared.suppressExtensions,
                         createInfo.enabledExtensionCount, createInfo.ppEnabledExtensionNames);
  suppressRequestedNames(requestedLayers, Configurator::Get().vulkan.shared.suppressLayers,
                         createInfo.enabledLayerCount, createInfo.ppEnabledLayerNames);

  bool allSupported = true;

  LOG_TRACE << " ------------------ ";
  LOG_TRACE << "Instance compatibility check section begin.";

  allSupported &= checkForSupportForInstanceExtensions(createInfo.enabledExtensionCount,
                                                       createInfo.ppEnabledExtensionNames);
  allSupported &= checkForSupportForInstanceLayers(createInfo.enabledLayerCount,
                                                   createInfo.ppEnabledLayerNames);
  if (!allSupported &&
      !Configurator::Get().vulkan.player.ignoreVKCrossPlatformIncompatibilitiesWA) {
    throw std::runtime_error("Error - stream uses instance extensions and/or layers which are not "
                             "supported on a current platform. Exiting!");
  }

  LOG_TRACE << "Instance compatibility check section end"
            << (allSupported ? "." : " - please read warning messages!!");
  LOG_TRACE << " ------------------ ";

  VkResult playerSideReturnValue = drvVk.vkCreateInstance(&createInfo, *pAllocator, *pInstance);
  checkReturnValue(playerSideReturnValue, recorderSideReturnValue, "vkCreateInstance");
  recorderSideReturnValue.Assign(playerSideReturnValue);
  vkCreateInstance_SD(playerSideReturnValue, createInfoPtrOrig, *pAllocator, *pInstance);

#if defined(GITS_PLATFORM_WINDOWS)
  if (Configurator::Get().vulkan.player.renderDoc.mode == TVkRenderDocCaptureMode::FRAMES &&
      Configurator::Get().vulkan.player.renderDoc.captureRange[CGits::Instance().CurrentFrame()]) {
    RenderDocUtil::GetInstance().AddCapturer(**pInstance);
  }
#endif

  if (isGitsRecorderAttached() && Configurator::Get().vulkan.player.patchShaderGroupHandles) {
    // Currently, shader group handle patching in SBT cannot be used during
    // substream recording. Inform user about this problem.
    auto& cfg = Configurator::GetMutable();
    cfg.vulkan.player.patchShaderGroupHandles = false;
    LOG_WARNING << "Shader group handles patching cannot be used during substream recording. GITS "
                   "will disable it for this replay.";
  }
}

inline void vkQueuePresentKHR_WRAPRUN(CVkResult& recorderSideReturnValue,
                                      CVkQueue& queue,
                                      CVkPresentInfoKHR& pPresentInfo) {
  if (*pPresentInfo == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  VkPresentInfoKHR presentInfo = *pPresentInfo;

  if (Configurator::Get().common.player.captureFrames[CGits::Instance().CurrentFrame()] &&
      !Configurator::Get().common.player.captureScreenshot) {
    writeScreenshot(*queue, presentInfo);
  }

  VkResult playerSideReturnValue = VK_SUCCESS;

  // Offscreen rendering
  if (Configurator::Get().common.player.renderOffscreen) {
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
    if (Configurator::Get().common.player.captureFrames[CGits::Instance().CurrentFrame()] &&
        Configurator::Get().common.player.captureScreenshot) {
      sleep_millisec(1000);
      writeScreenshot(*queue, presentInfo);
    }
  }
  checkReturnValue(playerSideReturnValue, recorderSideReturnValue, "vkQueuePresentKHR");
  recorderSideReturnValue.Assign(playerSideReturnValue);
  vkQueuePresentKHR_SD(playerSideReturnValue, *queue, &presentInfo);

  LOG_FORMAT_RAW
  LOG_TRACE << "End of frame #" << CGits::Instance().CurrentFrame();
}
namespace {
inline void HandleQueueSubmitRenderDocStart() {
#if defined(GITS_PLATFORM_WINDOWS)
  if (Configurator::Get().vulkan.player.renderDoc.mode == TVkRenderDocCaptureMode::QUEUE_SUBMIT &&
      Configurator::Get()
          .vulkan.player.renderDoc
          .captureRange[CGits::Instance().vkCounters.CurrentQueueSubmitCount()]) {
    RenderDocUtil::GetInstance().StartRecording();
  }
#endif
}

inline void HandleQueueSubmitRenderDocStop() {
#if defined(GITS_PLATFORM_WINDOWS)
  if (Configurator::Get().vulkan.player.renderDoc.mode == TVkRenderDocCaptureMode::QUEUE_SUBMIT &&
      Configurator::Get()
          .vulkan.player.renderDoc
          .captureRange[CGits::Instance().vkCounters.CurrentQueueSubmitCount()]) {
    bool isLast = Configurator::Get()
                      .vulkan.player.renderDoc
                      .captureRange[CGits::Instance().vkCounters.CurrentQueueSubmitCount()] &&
                  !Configurator::Get()
                       .vulkan.player.renderDoc
                       .captureRange[CGits::Instance().vkCounters.CurrentQueueSubmitCount() + 1];
    if (!Configurator::Get().vulkan.player.renderDoc.continuousCapture || isLast) {
      RenderDocUtil::GetInstance().StopRecording();
    }
    if (Configurator::Get().vulkan.player.renderDoc.enableUI && isLast) {
      RenderDocUtil::GetInstance().LaunchRenderDocUI();
    }
  }
#endif
}

inline void ExecCmdBuffer(VkCommandBuffer commandBuffer,
                          bool secondary,
                          uint32_t cmdBuffBatchNumber = 0,
                          uint32_t cmdBuffNumber = 0) {
  auto commandBuffState = SD()._commandbufferstates[commandBuffer];
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
          CGits::Instance().vkCounters.CurrentQueueSubmitCount(), cmdBuffBatchNumber, cmdBuffNumber,
          commandBuffer);
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
  if ((*submitCount > 0) &&
      (!Configurator::Get().vulkan.player.captureVulkanSubmits.empty() ||
       !Configurator::Get().vulkan.player.captureVulkanSubmitsResources.empty() ||
       Configurator::Get().vulkan.player.oneVulkanDrawPerCommandBuffer ||
       Configurator::Get().vulkan.player.oneVulkanRenderPassPerCommandBuffer)) {
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
          LOG_WARNING << "vkQueueSubmit failed.";
          if (Configurator::Get().vulkan.player.exitOnVkQueueSubmitFail) {
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
        VkCommandBuffer cmdbuffer = submitInfoOrig.pCommandBuffers[cmdBufIndex];
        if (Configurator::Get().vulkan.player.execCmdBuffsBeforeQueueSubmit) {
          auto& commandBuffState = SD()._commandbufferstates[cmdbuffer];
          for (auto secondaryCmdBuffer : commandBuffState->secondaryCommandBuffers) {
            ExecCmdBuffer(secondaryCmdBuffer, true);
          }
          ExecCmdBuffer(cmdbuffer, false, i, cmdBufIndex);
        }
        VkSubmitInfo submitInfoNew;
        // VkCommandBuffer can be recreated inside method ExecCmdBuffer(), so we want to get actual pointer below
        cmdbuffer =
            CVkCommandBuffer::GetMapping(pSubmits.Original()[i].pCommandBuffers[cmdBufIndex]);
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
          LOG_WARNING << "vkQueueSubmit failed.";
          if (Configurator::Get().vulkan.player.exitOnVkQueueSubmitFail) {
            std::ostringstream error;
            error << "vkQueueSubmit function returned the " << *return_value << " error!\n";
            error << "Exiting!\n";
            throw std::runtime_error(error.str());
          }
        }
        vkQueueSubmit_SD(*return_value, *queue, 1, &submitInfoNew, fenceNew);
        bool captureVulkanSubmitsCheck =
            Configurator::Get().vulkan.player.captureVulkanSubmits
                [(size_t)CGits::Instance().vkCounters.CurrentQueueSubmitCount()];
        bool captureVulkanSubmitsResourcesCheck =
            Configurator::Get().vulkan.player.captureVulkanSubmitsResources
                [(size_t)CGits::Instance().vkCounters.CurrentQueueSubmitCount()];

        bool capturesResourcesCheck =
            !Configurator::Get().vulkan.player.captureVulkanResources.empty() &&
            (!SD()._commandbufferstates[cmdbuffer]->renderPassResourceImages.empty() ||
             !SD()._commandbufferstates[cmdbuffer]->renderPassResourceBuffers.empty());

        if (captureVulkanSubmitsCheck || captureVulkanSubmitsResourcesCheck ||
            capturesResourcesCheck || Configurator::Get().vulkan.player.waitAfterQueueSubmitWA) {
          drvVk.vkQueueWaitIdle(*queue);
        }
        if (captureVulkanSubmitsCheck) {
          writeScreenshot(*queue, cmdbuffer, i, cmdBufIndex);
        }
        if (captureVulkanSubmitsResourcesCheck) {
          writeResources(*queue, cmdbuffer, i, cmdBufIndex);
        }
        if (capturesResourcesCheck) {
          vulkanDumpRenderPassResources(cmdbuffer);
        }
      }
    }
  } else {
    if ((*submitCount > 0) && Configurator::Get().vulkan.player.execCmdBuffsBeforeQueueSubmit) {
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
      LOG_WARNING << "vkQueueSubmit failed.";
      if (Configurator::Get().vulkan.player.exitOnVkQueueSubmitFail) {
        std::ostringstream error;
        error << "vkQueueSubmit function returned the " << *return_value << " error!\n";
        error << "Exiting!\n";
        throw std::runtime_error(error.str());
      }
    }
    if (Configurator::Get().vulkan.player.waitAfterQueueSubmitWA) {
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
                                   CVkFence& fence,
                                   bool isKHR = false) {
  if ((*submitCount > 0) &&
      (!Configurator::Get().vulkan.player.captureVulkanSubmits.empty() ||
       !Configurator::Get().vulkan.player.captureVulkanSubmitsResources.empty() ||
       Configurator::Get().vulkan.player.oneVulkanDrawPerCommandBuffer ||
       Configurator::Get().vulkan.player.oneVulkanRenderPassPerCommandBuffer)) {
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
        if (isKHR) {
          return_value.Assign(drvVk.vkQueueSubmit2KHR(*queue, 1, &submitInfoOrig, fenceNew));
        } else {
          return_value.Assign(drvVk.vkQueueSubmit2(*queue, 1, &submitInfoOrig, fenceNew));
        }
        if (*return_value != VK_SUCCESS) {
          LOG_WARNING << "vkQueueSubmit2 failed.";
          if (Configurator::Get().vulkan.player.exitOnVkQueueSubmitFail) {
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
        VkCommandBufferSubmitInfo cmdbufferSubmitInfo =
            submitInfoOrig.pCommandBufferInfos[cmdBufIndex];
        if (Configurator::Get().vulkan.player.execCmdBuffsBeforeQueueSubmit) {
          auto& commandBuffState = SD()._commandbufferstates[cmdbufferSubmitInfo.commandBuffer];
          for (auto secondaryCmdBuffer : commandBuffState->secondaryCommandBuffers) {
            ExecCmdBuffer(secondaryCmdBuffer, true);
          }
          ExecCmdBuffer(cmdbufferSubmitInfo.commandBuffer, false, i, cmdBufIndex);
        }
        VkSubmitInfo2 submitInfoNew;
        // VkCommandBuffer can be recreated inside method ExecCmdBuffer(), so we want to get actual pointer below
        cmdbufferSubmitInfo.commandBuffer = CVkCommandBuffer::GetMapping(
            pSubmits.Original()[i].pCommandBufferInfos[cmdBufIndex].commandBuffer);
        if (cmdBufIndex == submitInfoOrig.commandBufferInfoCount - 1) {
          // Last command buffer in queue submit (restoring original settings).
          submitInfoNew = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                           submitInfoOrig.pNext,
                           submitInfoOrig.flags,
                           submitInfoOrig.waitSemaphoreInfoCount,
                           submitInfoOrig.pWaitSemaphoreInfos,
                           1,
                           &cmdbufferSubmitInfo,
                           submitInfoOrig.signalSemaphoreInfoCount,
                           submitInfoOrig.pSignalSemaphoreInfos};

          if (i == (*submitCount - 1)) { // Last submit in QueueSubmit (restoring original fence).
            fenceNew = *fence;
          }
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
        if (isKHR) {
          return_value.Assign(drvVk.vkQueueSubmit2KHR(*queue, 1, &submitInfoNew, fenceNew));
        } else {
          return_value.Assign(drvVk.vkQueueSubmit2(*queue, 1, &submitInfoNew, fenceNew));
        }
        if (*return_value != VK_SUCCESS) {
          LOG_WARNING << "vkQueueSubmit2 failed.";
          if (Configurator::Get().vulkan.player.exitOnVkQueueSubmitFail) {
            std::ostringstream error;
            error << "vkQueueSubmit2 function returned the " << *return_value << " error!\n";
            error << "Exiting!\n";
            throw std::runtime_error(error.str());
          }
        }
        vkQueueSubmit2_SD(*return_value, *queue, 1, &submitInfoNew, fenceNew);
        bool captureVulkanSubmitsCheck =
            Configurator::Get().vulkan.player.captureVulkanSubmits
                [(size_t)CGits::Instance().vkCounters.CurrentQueueSubmitCount()];
        bool captureVulkanSubmitsResourcesCheck =
            Configurator::Get().vulkan.player.captureVulkanSubmitsResources
                [(size_t)CGits::Instance().vkCounters.CurrentQueueSubmitCount()];

        bool capturesResourcesCheck =
            !Configurator::Get().vulkan.player.captureVulkanResources.empty() &&
            (!SD()._commandbufferstates[cmdbufferSubmitInfo.commandBuffer]
                  ->renderPassResourceImages.empty() ||
             !SD()._commandbufferstates[cmdbufferSubmitInfo.commandBuffer]
                  ->renderPassResourceBuffers.empty());

        if (captureVulkanSubmitsCheck || captureVulkanSubmitsResourcesCheck ||
            capturesResourcesCheck || Configurator::Get().vulkan.player.waitAfterQueueSubmitWA) {
          drvVk.vkQueueWaitIdle(*queue);
        }
        if (captureVulkanSubmitsCheck) {
          writeScreenshot(*queue, cmdbufferSubmitInfo.commandBuffer, i, cmdBufIndex);
        }
        if (captureVulkanSubmitsResourcesCheck) {
          writeResources(*queue, cmdbufferSubmitInfo.commandBuffer, i, cmdBufIndex);
        }
        if (capturesResourcesCheck) {
          vulkanDumpRenderPassResources(cmdbufferSubmitInfo.commandBuffer);
        }
      }
    }
  } else {
    if ((*submitCount > 0) && Configurator::Get().vulkan.player.execCmdBuffsBeforeQueueSubmit) {
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
    if (isKHR) {
      return_value.Assign(drvVk.vkQueueSubmit2KHR(*queue, *submitCount, *pSubmits, *fence));
    } else {
      return_value.Assign(drvVk.vkQueueSubmit2(*queue, *submitCount, *pSubmits, *fence));
    }

    if (*return_value != VK_SUCCESS) {
      LOG_WARNING << "vkQueueSubmit2 failed.";
      if (Configurator::Get().vulkan.player.exitOnVkQueueSubmitFail) {
        std::ostringstream error;
        error << "vkQueueSubmit2 function returned the " << *return_value << " error!\n";
        error << "Exiting!\n";
        throw std::runtime_error(error.str());
      }
    }
    if (Configurator::Get().vulkan.player.waitAfterQueueSubmitWA) {
      drvVk.vkQueueWaitIdle(*queue);
    }
    vkQueueSubmit2_SD(*return_value, *queue, *submitCount, *pSubmits, *fence);

    HandleQueueSubmitRenderDocStop();
  }
}

inline void vkQueueSubmit2KHR_WRAPRUN(CVkResult& return_value,
                                      CVkQueue& queue,
                                      Cuint32_t& submitCount,
                                      CVkSubmitInfo2Array& pSubmits,
                                      CVkFence& fence) {
  vkQueueSubmit2_WRAPRUN(return_value, queue, submitCount, pSubmits, fence, true);
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

inline void vkGetSemaphoreCounterValueUnifiedGITS_WRAPRUN(CVkResult& return_value,
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
  return_value.Assign(
      drvVk.vkGetSemaphoreCounterValueUnifiedGITS(*device, *semaphore, &currentValue));
  if ((*return_value == VK_SUCCESS) && (recRetVal == VK_SUCCESS) && (currentValue < recValue)) {
    VkSemaphoreWaitInfo waitInfo = {
        VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO, // VkStructureType sType;
        nullptr,                               // const void * pNext;
        0,                                     // VkSemaphoreWaitFlags flags;
        1,                                     // uint32_t semaphoreCount;
        &semaphoreHandle,                      // const VkSemaphore * pSemaphores;
        &recValue                              // const uint64_t* pValues;
    };
    drvVk.vkWaitSemaphoresUnifiedGITS(*device, &waitInfo, 0xFFFFFFFFFFFFFFFF);
  }
}

inline void vkGetSemaphoreCounterValue_WRAPRUN(CVkResult& return_value,
                                               CVkDevice& device,
                                               CVkSemaphore& semaphore,
                                               Cuint64_t::CSArray& pValue) {
  vkGetSemaphoreCounterValueUnifiedGITS_WRAPRUN(return_value, device, semaphore, pValue);
}

inline void vkGetSemaphoreCounterValueKHR_WRAPRUN(CVkResult& return_value,
                                                  CVkDevice& device,
                                                  CVkSemaphore& semaphore,
                                                  Cuint64_t::CSArray& pValue) {
  vkGetSemaphoreCounterValueUnifiedGITS_WRAPRUN(return_value, device, semaphore, pValue);
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
  auto originalMemoryTypeIndex = allocateInfoPtr->memoryTypeIndex;
  // Memory type index matching between recorded and current platform based on VkMemoryPropertyFlags
  allocateInfoPtr->memoryTypeIndex =
      getMappedMemoryTypeIndex(device, allocateInfoPtr->memoryTypeIndex);

  auto dedicatedAllocation = (VkMemoryDedicatedAllocateInfo*)getPNextStructure(
      allocateInfoPtr->pNext, VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO);
  if (dedicatedAllocation) {
    CAutoCaller autoCaller(drvVk.vkPauseRecordingGITS, drvVk.vkContinueRecordingGITS);

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
        auto physicalDevice =
            imageState->deviceStateStore->physicalDeviceStateStore->physicalDeviceHandle;
        auto originalMemoryTypePropertyFlags =
            SD()._physicaldevicestates[physicalDevice]
                ->memoryPropertiesOriginal.memoryTypes[originalMemoryTypeIndex]
                .propertyFlags;
        auto currentPlatformProperties =
            SD()._physicaldevicestates[physicalDevice]->memoryPropertiesCurrent;
        allocateInfoPtr->memoryTypeIndex = findCompatibleMemoryTypeIndex(
            originalMemoryTypePropertyFlags, currentPlatformProperties,
            imageState->memoryRequirements.memoryTypeBits);
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
        auto physicalDevice =
            bufferState->deviceStateStore->physicalDeviceStateStore->physicalDeviceHandle;
        auto originalMemoryTypePropertyFlags =
            SD()._physicaldevicestates[physicalDevice]
                ->memoryPropertiesOriginal.memoryTypes[originalMemoryTypeIndex]
                .propertyFlags;
        auto currentPlatformProperties =
            SD()._physicaldevicestates[physicalDevice]->memoryPropertiesCurrent;
        allocateInfoPtr->memoryTypeIndex = findCompatibleMemoryTypeIndex(
            originalMemoryTypePropertyFlags, currentPlatformProperties,
            bufferState->memoryRequirements.memoryTypeBits);
      }
    }
  }

  VkResult playerSideReturnValue =
      drvVk.vkAllocateMemory(device, allocateInfoPtr, *pAllocator, *pMemory);
  checkReturnValue(playerSideReturnValue, recorderSideReturnValue, "vkAllocateMemory");
  recorderSideReturnValue.Assign(playerSideReturnValue);
  vkAllocateMemory_SD(playerSideReturnValue, device, allocateInfoPtr, *pAllocator, *pMemory);

  if (*pMemory == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  VkDeviceMemory memory = **pMemory;

  if ((playerSideReturnValue == VK_SUCCESS) && (memory != VK_NULL_HANDLE) &&
      checkMemoryMappingFeasibility(device, allocateInfoPtr->memoryTypeIndex, false)) {
    CAutoCaller autoCaller(drvVk.vkPauseRecordingGITS, drvVk.vkContinueRecordingGITS);

    //clearMemory
    void* ptr = nullptr;

    VkResult map_return_value = drvVk.vkMapMemory(device, memory, 0, VK_WHOLE_SIZE, 0, &ptr);
    if (map_return_value == VK_SUCCESS) {
      memset(ptr, 0, (size_t)allocateInfoPtr->allocationSize);
      drvVk.vkUnmapMemory(device, memory);
    } else {
      LOG_WARNING << "vkMapMemory() was used to clear allocated memory but failed with the code: "
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
      LOG_WARNING
          << "vkGetImageSubresourceLayout() returned different offset than in gits Recorder. "
             "Stream was probably recorded on a different HW platform. "
             "It can cause a crash and/or corruptions.";
    };
  }

  if (layout.rowPitch != layout_original.rowPitch) {
    CALL_ONCE[] {
      LOG_WARNING
          << "vkGetImageSubresourceLayout() returned different rowPitch than in gits Recorder. "
             "Stream was probably recorded on a different HW platform. "
             "It can cause a crash and/or corruptions.";
    };
  }
}

namespace {
void GetBufferMemoryRequirementsHelper(VkDeviceSize originalAlignment,
                                       VkDeviceSize currentAlignment) {
  if ((currentAlignment != 0) && (originalAlignment % currentAlignment) != 0) {
    CALL_ONCE[&] {
      LOG_WARNING << "Stream recorded on a platform with alignment: " << originalAlignment
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
  if ((currentAlignment != 0) && (originalAlignment % currentAlignment) != 0) {
    CALL_ONCE[&] {
      LOG_WARNING << "Stream recorded on a platform with alignment: " << originalAlignment
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

inline void vkWaitSemaphoresUnifiedGITS_WRAPRUN(CVkResult& return_value,
                                                CVkDevice& device,
                                                CVkSemaphoreWaitInfo& pWaitInfo,
                                                Cuint64_t& timeout) {
  VkResult recRetVal = *return_value;

  return_value.Assign(drvVk.vkWaitSemaphoresUnifiedGITS(*device, *pWaitInfo, *timeout));

  if ((*return_value != VK_SUCCESS) && (*return_value != recRetVal)) {
    return_value.Assign(drvVk.vkWaitSemaphoresUnifiedGITS(*device, *pWaitInfo, 0xFFFFFFFFFFFFFFFF));
  }
}

inline void vkWaitSemaphores_WRAPRUN(CVkResult& return_value,
                                     CVkDevice& device,
                                     CVkSemaphoreWaitInfo& pWaitInfo,
                                     Cuint64_t& timeout) {
  vkWaitSemaphoresUnifiedGITS_WRAPRUN(return_value, device, pWaitInfo, timeout);
}

inline void vkWaitSemaphoresKHR_WRAPRUN(CVkResult& return_value,
                                        CVkDevice& device,
                                        CVkSemaphoreWaitInfo& pWaitInfo,
                                        Cuint64_t& timeout) {
  vkWaitSemaphoresUnifiedGITS_WRAPRUN(return_value, device, pWaitInfo, timeout);
}

namespace {
void BindBufferMemory_WRAPRUNHelper(VkDevice device,
                                    VkBuffer buffer,
                                    VkDeviceMemory memory,
                                    uint64_t memoryOffset) {
  CAutoCaller autoCaller(drvVk.vkPauseRecordingGITS, drvVk.vkContinueRecordingGITS);

  VkMemoryRequirements memRequirements;
  drvVk.vkGetBufferMemoryRequirements(device, buffer, &memRequirements);
  auto memAllocateInfo = SD()._devicememorystates[memory]->memoryAllocateInfoData.Value();
  bool incompatibilityError = false;

  if ((memRequirements.alignment > 0) && ((memoryOffset % memRequirements.alignment) != 0)) {
    LOG_ERROR << "Offset of a memory bound to a buffer is not divisible by the required alignment. "
                 "It can cause crash or corruptions!!";
    LOG_ERROR << "    Memory offset used: " << memoryOffset;
    LOG_ERROR << "    Required alignment: " << memRequirements.alignment;
    incompatibilityError = true;
  }

  if (((1 << memAllocateInfo->memoryTypeIndex) & memRequirements.memoryTypeBits) == 0) {
    LOG_ERROR << "Memory object bound to a buffer is allocated from an incompatible memory type "
                 "index. It can cause crash or corruptions!!";
    incompatibilityError = true;
  }

  if ((memAllocateInfo->allocationSize - (memoryOffset)) < memRequirements.size) {
    LOG_ERROR << "Too small memory block bound to a buffer. It can cause crash or corruptions!!";
    LOG_ERROR << "    Memory object's allocation size: " << memAllocateInfo->allocationSize;
    LOG_ERROR << "    Provided offset: " << memoryOffset;
    LOG_ERROR << "    Required size: " << memRequirements.size;
    incompatibilityError = true;
  }

  if (incompatibilityError &&
      !Configurator::Get().vulkan.player.ignoreVKCrossPlatformIncompatibilitiesWA) {
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
  CAutoCaller autoCaller(drvVk.vkPauseRecordingGITS, drvVk.vkContinueRecordingGITS);

  VkMemoryRequirements memRequirements;
  drvVk.vkGetImageMemoryRequirements(device, image, &memRequirements);
  auto memAllocateInfo = SD()._devicememorystates[memory]->memoryAllocateInfoData.Value();
  bool incompatibilityError = false;

  if ((memRequirements.alignment > 0) && ((memoryOffset % memRequirements.alignment) != 0)) {
    LOG_ERROR << "Offset of a memory bound to an image is not divisible by the required alignment. "
                 "It can cause crash or corruptions!!";
    LOG_ERROR << "    Memory offset used: " << memoryOffset;
    LOG_ERROR << "    Required alignment: " << memRequirements.alignment;
    incompatibilityError = true;
  }

  if (((1 << memAllocateInfo->memoryTypeIndex) & memRequirements.memoryTypeBits) == 0) {
    LOG_ERROR << "Memory object bound to an image is allocated from an incompatible memory type "
                 "index. It can cause crash or corruptions!!";
    incompatibilityError = true;
  }

  if ((memAllocateInfo->allocationSize - (memoryOffset)) < memRequirements.size) {
    LOG_ERROR << "Too small memory block bound to an image. It can cause crash or corruptions!!";
    LOG_ERROR << "    Memory object's allocation size: " << memAllocateInfo->allocationSize;
    LOG_ERROR << "    Provided offset: " << memoryOffset;
    LOG_ERROR << "    Required size: " << memRequirements.size;
    incompatibilityError = true;
  }

  if (incompatibilityError &&
      !Configurator::Get().vulkan.player.ignoreVKCrossPlatformIncompatibilitiesWA) {
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

  auto& vkConfig = Configurator::GetMutable().vulkan;

  VkDeviceCreateInfo createInfo = *pCreateInfo;
  std::vector<const char*> requestedExtensions(createInfo.ppEnabledExtensionNames,
                                               createInfo.ppEnabledExtensionNames +
                                                   createInfo.enabledExtensionCount);
  std::vector<const char*> requestedLayers(createInfo.ppEnabledLayerNames,
                                           createInfo.ppEnabledLayerNames +
                                               createInfo.enabledLayerCount);

  suppressPhysicalDeviceFeatures(
      Configurator::Get().vulkan.shared.suppressPhysicalDeviceFeatures,
      const_cast<VkPhysicalDeviceFeatures*>(createInfo.pEnabledFeatures));
  suppressRequestedNames(requestedExtensions, Configurator::Get().vulkan.shared.suppressExtensions,
                         createInfo.enabledExtensionCount, createInfo.ppEnabledExtensionNames);
  suppressRequestedNames(requestedLayers, Configurator::Get().vulkan.shared.suppressLayers,
                         createInfo.enabledLayerCount, createInfo.ppEnabledLayerNames);

  // Don't use capture/replay features (if available) together with shader group handles patching
  if (vkConfig.player.patchShaderGroupHandles) {
    auto* rayTracingPipelineFeatures =
        (VkPhysicalDeviceRayTracingPipelineFeaturesKHR*)getPNextStructure(
            createInfo.pNext, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR);

    if (rayTracingPipelineFeatures != nullptr) {
      rayTracingPipelineFeatures->rayTracingPipelineShaderGroupHandleCaptureReplay = VK_FALSE;
    }
  }

  LOG_TRACE << " ------------------ ";
  LOG_TRACE << "Device compatibility check section begin.";
  // Trace memory properties from the original platform the stream was recorded on
  LOG_TRACE << "Memory properties of the platform the (original) stream was recorded on: "
               "VkPhysicalDevice physicalDevice="
            << *physicalDevice << ", VkPhysicalDeviceMemoryProperties* pMemoryProperties="
            << &SD()._physicaldevicestates[*physicalDevice]->memoryPropertiesOriginal;

  allSupported &= checkForSupportForPhysicalDeviceFeatures(
      *physicalDevice, const_cast<VkPhysicalDeviceFeatures*>(createInfo.pEnabledFeatures));
  allSupported &= areDeviceExtensionsSupported(*physicalDevice, createInfo.enabledExtensionCount,
                                               createInfo.ppEnabledExtensionNames, true);
  allSupported &= checkForSupportForQueues(*physicalDevice, createInfo.queueCreateInfoCount,
                                           createInfo.pQueueCreateInfos) ||
                  Configurator::Get().vulkan.player.ignoreMissingQueuesWA;

  if (!allSupported &&
      !Configurator::Get().vulkan.player.ignoreVKCrossPlatformIncompatibilitiesWA) {
    throw std::runtime_error("Error - stream is incompatible with the current platform. Exiting!");
  }

  LOG_TRACE << "Device compatibility check section end"
            << (allSupported ? "." : " - please read warning messages!!");
  LOG_TRACE << " ------------------ ";

  VkResult playerSideReturnValue =
      drvVk.vkCreateDevice(*physicalDevice, &createInfo, *pAllocator, *pDevice);
  checkReturnValue(playerSideReturnValue, recorderSideReturnValue, "vkCreateDevice");
  recorderSideReturnValue.Assign(playerSideReturnValue);
  vkCreateDevice_SD(playerSideReturnValue, *physicalDevice, *pCreateInfo, *pAllocator, *pDevice);

#if defined(GITS_PLATFORM_WINDOWS)
  if (Configurator::Get().vulkan.player.renderDoc.mode == TVkRenderDocCaptureMode::FRAMES &&
      Configurator::Get().vulkan.player.renderDoc.captureRange[CGits::Instance().CurrentFrame()]) {
    RenderDocUtil::GetInstance().StartRecording();
  }
#endif

  if (!Configurator::Get().vulkan.player.overrideVKPipelineCache.empty()) {
    CAutoCaller autoCaller(drvVk.vkPauseRecordingGITS, drvVk.vkContinueRecordingGITS);

    auto initialData =
        GetBinaryFileContents(Configurator::Get().vulkan.player.overrideVKPipelineCache.string());

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

inline void vkGetDeviceQueue_WRAPRUN(CVkDevice& device,
                                     Cuint32_t& queueFamilyIndex,
                                     Cuint32_t& queueIndex,
                                     CVkQueue::CSMapArray& pQueue) {
  const auto& queueFamilies =
      SD()._devicestates[*device]->physicalDeviceStateStore->queueFamilyPropertiesCurrent;

  if (*queueFamilyIndex >= queueFamilies.size()) {
    LOG_ERROR << "Ignoring vkGetDeviceQueue() call because requested family index is not supported "
                 "on the current hardware.";
  } else {
    drvVk.vkGetDeviceQueue(*device, *queueFamilyIndex, *queueIndex, *pQueue);
  }
}

inline void vkGetDeviceQueue2_WRAPRUN(CVkDevice& device,
                                      CVkDeviceQueueInfo2& pQueueInfo,
                                      CVkQueue::CSMapArray& pQueue) {
  const auto& queueFamilies =
      SD()._devicestates[*device]->physicalDeviceStateStore->queueFamilyPropertiesCurrent;

  auto* queueInfo = pQueueInfo.Value();

  if (queueInfo->queueFamilyIndex >= queueFamilies.size()) {
    LOG_ERROR
        << "Ignoring vkGetDeviceQueue2() call because requested family index is not supported "
           "on the current hardware.";
  } else {
    drvVk.vkGetDeviceQueue2(*device, queueInfo, *pQueue);
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
  if (!Configurator::Get().common.player.captureFrames.empty() ||
      !Configurator::Get().vulkan.player.captureVulkanSubmits.empty() ||
      !Configurator::Get().vulkan.player.captureVulkanRenderPasses.empty() ||
      !Configurator::Get().vulkan.player.captureVulkanRenderPassesResources.empty() ||
      !Configurator::Get().vulkan.player.captureVulkanDraws.empty() ||
      !Configurator::Get().vulkan.player.captureVulkanResources.empty()) {
    createInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    LOG_TRACE << "Modifying swapchain usage for frames/render targets capturing!!";
  }
  if (Configurator::Get().common.player.forceWindowSize.enabled) {
    createInfo.imageExtent.width = Configurator::Get().common.player.forceWindowSize.width;
    createInfo.imageExtent.height = Configurator::Get().common.player.forceWindowSize.height;
  }
  VkResult playerSideReturnValue =
      drvVk.vkCreateSwapchainKHR(*device, &createInfo, *pAllocator, *pSwapchain);
  checkReturnValue(playerSideReturnValue, recorderSideReturnValue, "vkCreateSwapchainKHR");
  recorderSideReturnValue.Assign(playerSideReturnValue);
  vkCreateSwapchainKHR_SD(playerSideReturnValue, *device, *pCreateInfo, *pAllocator, *pSwapchain);

  VkSwapchainKHR* swapchainPtr = *pSwapchain;

  if (Configurator::Get().common.player.renderOffscreen && (swapchainPtr != nullptr)) {
    CAutoCaller autoCaller(drvVk.vkPauseRecordingGITS, drvVk.vkContinueRecordingGITS);

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
    auto& rect = Configurator::Get().common.player.forceScissor;

    auto& offset = originalScissorRect->offset;
    auto& extent = originalScissorRect->extent;

    originalScissorRect->offset.x = std::max(static_cast<int32_t>(rect.x), offset.x);
    originalScissorRect->offset.y = std::max(static_cast<int32_t>(rect.y), offset.y);
    originalScissorRect->extent.width = std::min(static_cast<uint32_t>(rect.width), extent.width);
    originalScissorRect->extent.height =
        std::min(static_cast<uint32_t>(rect.height), extent.height);
  }
}

// Helper function used to reduce the number of arguments passed to a driver.
// This is necessary for the CreatePipelines_Helper template to accept the call.
// Besides, for now GITS doesn't support deferred operations so we need to
// pass VK_NULL_HANDLE anyway.
VkResult STDCALL
CreateRayTracingPipelinesArgumentWrapper(VkDevice device,
                                         VkPipelineCache pipelineCache,
                                         uint32_t createInfoCount,
                                         const VkRayTracingPipelineCreateInfoKHR* pCreateInfos,
                                         const VkAllocationCallbacks* pAllocator,
                                         VkPipeline* pPipelines) {
  // Don't use capture/replay features (if available) together with shader group handles patching
  if (Configurator::Get().vulkan.player.patchShaderGroupHandles) {
    for (uint32_t ci = 0; ci < createInfoCount; ci++) {
      auto* pCreateInfo = const_cast<VkRayTracingPipelineCreateInfoKHR*>(&pCreateInfos[ci]);
      pCreateInfo->flags &=
          ~VK_PIPELINE_CREATE_RAY_TRACING_SHADER_GROUP_HANDLE_CAPTURE_REPLAY_BIT_KHR;

      for (uint32_t g = 0; g < pCreateInfo->groupCount; ++g) {
        auto* pGroup = const_cast<VkRayTracingShaderGroupCreateInfoKHR*>(&pCreateInfo->pGroups[g]);
        pGroup->pShaderGroupCaptureReplayHandle = nullptr;
      }
    }
  }
  return drvVk.vkCreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE /* deferredOperation */,
                                              pipelineCache, createInfoCount, pCreateInfos,
                                              pAllocator, pPipelines);
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
  if (!Configurator::Get().vulkan.player.overrideVKPipelineCache.empty() &&
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

  if (Configurator::Get().vulkan.player.forceMultithreadedPipelineCompilation &&
      createInfoCount > 1) {
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

  if (Configurator::Get().common.player.forceScissor.enabled) {
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

inline void vkCreateRayTracingPipelinesKHR_WRAPRUN(
    CVkResult& recorderSideReturnValue,
    CVkDevice& device,
    CVkDeferredOperationKHR& deferredOperation,
    CVkPipelineCache& pipelineCache,
    Cuint32_t& createInfoCount,
    CVkRayTracingPipelineCreateInfoKHRArray& pCreateInfos,
    CNullWrapper& pAllocator,
    CVkPipeline::CSMapArray& pPipelines) {
  VkResult playerSideReturnValue =
      CreatePipelines_Helper(CreateRayTracingPipelinesArgumentWrapper, *device, *pipelineCache,
                             *createInfoCount, *pCreateInfos, *pAllocator, *pPipelines);
  checkReturnValue(playerSideReturnValue, recorderSideReturnValue,
                   "vkCreateRayTracingPipelinesKHR");
  recorderSideReturnValue.Assign(playerSideReturnValue);
  vkCreateRayTracingPipelinesKHR_SD(playerSideReturnValue, *device, *deferredOperation,
                                    *pipelineCache, *createInfoCount, *pCreateInfos, *pAllocator,
                                    *pPipelines);
}

inline void vkCmdSetScissor_WRAPRUN(CVkCommandBuffer& commandBuffer,
                                    Cuint32_t& firstScissor,
                                    Cuint32_t& scissorCount,
                                    CVkRect2DArray& pScissors) {
  VkRect2D* scissors = *pScissors;

  if (Configurator::Get().common.player.forceScissor.enabled) {
    for (uint32_t i = 0; i < *scissorCount; ++i) {
      ForceScissor_Helper(&scissors[i]);
    }
  }
  if (Configurator::Get().vulkan.player.execCmdBuffsBeforeQueueSubmit) {
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
  if (!Configurator::Get().vulkan.player.captureVulkanSubmits.empty() ||
      !Configurator::Get().vulkan.player.captureVulkanSubmitsResources.empty() ||
      !Configurator::Get().vulkan.player.captureVulkanRenderPasses.empty() ||
      !Configurator::Get().vulkan.player.captureVulkanRenderPassesResources.empty() ||
      !Configurator::Get().vulkan.player.captureVulkanDraws.empty() ||
      !Configurator::Get().vulkan.player.captureVulkanResources.empty()) {
    createInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    LOG_TRACE << "Modifying image usage for render targets capturing!!";
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
  if (!Configurator::Get().vulkan.player.captureVulkanSubmitsResources.empty() ||
      !Configurator::Get().vulkan.player.captureVulkanRenderPassesResources.empty() ||
      !Configurator::Get().vulkan.player.captureVulkanResources.empty()) {
    createInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    LOG_TRACE << "Modifying buffer usage for resource capturing!!";
  }
  VkResult playerSideReturnValue =
      drvVk.vkCreateBuffer(*device, &createInfo, *pAllocator, *pBuffer);
  checkReturnValue(playerSideReturnValue, recorderSideReturnValue, "vkCreateBuffer");
  recorderSideReturnValue.Assign(playerSideReturnValue);
  vkCreateBuffer_SD(playerSideReturnValue, *device, *pCreateInfo, *pAllocator, *pBuffer);
}

inline void vkPassPhysicalDeviceMemoryPropertiesGITS_WRAPRUN(
    CVkPhysicalDevice& physicalDevice, CVkPhysicalDeviceMemoryProperties& pMemoryProperties) {
  if (*pMemoryProperties == nullptr) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  SD()._physicaldevicestates[*physicalDevice]->memoryPropertiesOriginal = *pMemoryProperties;
  SD()._physicaldevicestates[*physicalDevice]->correspondingMemoryTypeIndexes =
      matchCorrespondingMemoryTypeIndexes(*physicalDevice);
}

inline void vkDestroyDevice_WRAPRUN(CVkDevice& device, CNullWrapper& pAllocator) {
  // If vkDestroyDevice() function is recorded and replayed, then we need to destroy all the device-level resources before the device destruction
  destroyDeviceLevelResources(*device);

  drvVk.vkDestroyDevice(*device, *pAllocator);
  vkDestroyDevice_SD(*device, *pAllocator);
  device.RemoveMapping();
}

inline void vkDestroyInstance_WRAPRUN(CVkInstance& instance, CNullWrapper& pAllocator) {
#ifdef GITS_PLATFORM_WINDOWS
  if (Configurator::Get().vulkan.player.renderDoc.mode != TVkRenderDocCaptureMode::NONE) {
    RenderDocUtil::GetInstance().DeleteCapturer(*instance);
  }
#endif
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
  if (Configurator::Get().vulkan.player.execCmdBuffsBeforeQueueSubmit) {
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
  auto deviceAddress = drvVk.vkGetBufferDeviceAddressUnifiedGITS(*_device, *_pInfo);
  vkGetBufferDeviceAddressUnifiedGITS_SD(deviceAddress, *_device, *_pInfo);
}

inline void vkBeginCommandBuffer_WRAPRUN(CVkResult& return_value,
                                         CVkCommandBuffer& commandBuffer,
                                         CVkCommandBufferBeginInfo& pBeginInfo) {
  if (!Configurator::Get().vulkan.player.execCmdBuffsBeforeQueueSubmit) {
    return_value.Assign(drvVk.vkBeginCommandBuffer(*commandBuffer, *pBeginInfo));
  }
  // State Tracking is necessary also when using execCmdBuffsBeforeQueueSubmit or captureVulkanRenderPasses because we will call vkBeginCommandBuffer manually later.
  vkBeginCommandBuffer_SD(*return_value, *commandBuffer, *pBeginInfo);
}

inline void vkEndCommandBuffer_WRAPRUN(CVkResult& return_value, CVkCommandBuffer& commandBuffer) {
  if (!Configurator::Get().vulkan.player.execCmdBuffsBeforeQueueSubmit) {
    return_value.Assign(drvVk.vkEndCommandBuffer(*commandBuffer));
  }
  // State Tracking is necessary also when using execCmdBuffsBeforeQueueSubmit or captureVulkanRenderPasses because we will call vkEndCommandBuffer manually later.
  vkEndCommandBuffer_SD(*return_value, *commandBuffer);
}

inline void vkCmdExecuteCommands_WRAPRUN(CVkCommandBuffer& commandBuffer,
                                         Cuint32_t& commandBufferCount,
                                         CVkCommandBuffer::CSArray& pCommandBuffers) {
  if (Configurator::Get().vulkan.player.execCmdBuffsBeforeQueueSubmit) {
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

inline void vkCreateRenderPass_WRAPRUN(CVkResult& return_value,
                                       CVkDevice& device,
                                       CVkRenderPassCreateInfo& pCreateInfo,
                                       CNullWrapper& pAllocator,
                                       CVkRenderPass::CSMapArray& pRenderPass) {
  return_value.Assign(drvVk.vkCreateRenderPass(*device, *pCreateInfo, *pAllocator, *pRenderPass));
  vkCreateRenderPass_SD(*return_value, *device, *pCreateInfo, *pAllocator, *pRenderPass);
  CreateRenderPasses_helper<VkRenderPassCreateInfo, VkAttachmentDescription>(
      *device, **pRenderPass, *pCreateInfo, SD()._renderpassstates[**pRenderPass]->createdWith);
}

inline void vkCreateRenderPass2_WRAPRUN(CVkResult& return_value,
                                        CVkDevice& device,
                                        CVkRenderPassCreateInfo2& pCreateInfo,
                                        CNullWrapper& pAllocator,
                                        CVkRenderPass::CSMapArray& pRenderPass) {
  return_value.Assign(drvVk.vkCreateRenderPass2(*device, *pCreateInfo, *pAllocator, *pRenderPass));
  vkCreateRenderPass2_SD(*return_value, *device, *pCreateInfo, *pAllocator, *pRenderPass);
  CreateRenderPasses_helper<VkRenderPassCreateInfo2, VkAttachmentDescription2>(
      *device, **pRenderPass, *pCreateInfo, SD()._renderpassstates[**pRenderPass]->createdWith);
}

inline void vkCreateRenderPass2KHR_WRAPRUN(CVkResult& return_value,
                                           CVkDevice& device,
                                           CVkRenderPassCreateInfo2& pCreateInfo,
                                           CNullWrapper& pAllocator,
                                           CVkRenderPass::CSMapArray& pRenderPass) {
  return_value.Assign(
      drvVk.vkCreateRenderPass2KHR(*device, *pCreateInfo, *pAllocator, *pRenderPass));
  vkCreateRenderPass2KHR_SD(*return_value, *device, *pCreateInfo, *pAllocator, *pRenderPass);
  CreateRenderPasses_helper<VkRenderPassCreateInfo2, VkAttachmentDescription2>(
      *device, **pRenderPass, *pCreateInfo, SD()._renderpassstates[**pRenderPass]->createdWith);
}

inline void vkDestroyRenderPass_WRAPRUN(CVkDevice& device,
                                        CVkRenderPass& renderPass,
                                        CNullWrapper& pAllocator) {
  drvVk.vkDestroyRenderPass(*device, *renderPass, *pAllocator);
  if (Configurator::Get().vulkan.player.oneVulkanDrawPerCommandBuffer) {
    if (SD()._renderpassstates[*renderPass]->loadAndStoreRenderPassHandle != *renderPass) {
      drvVk.vkDestroyRenderPass(*device,
                                SD()._renderpassstates[*renderPass]->loadAndStoreRenderPassHandle,
                                VK_NULL_HANDLE);
    }
    if (SD()._renderpassstates[*renderPass]->restoreRenderPassHandle != *renderPass) {
      drvVk.vkDestroyRenderPass(
          *device, SD()._renderpassstates[*renderPass]->restoreRenderPassHandle, VK_NULL_HANDLE);
    }
    if (SD()._renderpassstates[*renderPass]->storeNoLoadRenderPassHandle != *renderPass) {
      drvVk.vkDestroyRenderPass(*device,
                                SD()._renderpassstates[*renderPass]->storeNoLoadRenderPassHandle,
                                VK_NULL_HANDLE);
    }
  }
  vkDestroyRenderPass_SD(*device, *renderPass, *pAllocator);
  renderPass.RemoveMapping();
}

inline void vkGetAccelerationStructureDeviceAddressUnifiedGITS_WRAPRUN(
    Cuint64_t& _return_value,
    CVkDevice& _device,
    CVkAccelerationStructureDeviceAddressInfoKHR& _pInfo) {
  auto deviceAddress = drvVk.vkGetAccelerationStructureDeviceAddressUnifiedGITS(*_device, *_pInfo);
  vkGetAccelerationStructureDeviceAddressUnifiedGITS_SD(deviceAddress, *_device, *_pInfo);
}

inline void vkBuildAccelerationStructuresKHR_WRAPRUN(
    CVkResult& return_value,
    CVkDevice& device,
    CVkDeferredOperationKHR& deferredOperation,
    Cuint32_t& infoCount,
    CVkAccelerationStructureBuildGeometryInfoKHRArray& pInfos,
    CVkAccelerationStructureBuildRangeInfoKHRArrayOfArrays& ppBuildRangeInfos) {
  auto buildInfos = *pInfos;
  auto buildRangeInfos = *ppBuildRangeInfos;
  std::vector<std::vector<char>> scratchSpace(*infoCount);

  {
    CAutoCaller autoCaller(drvVk.vkPauseRecordingGITS, drvVk.vkContinueRecordingGITS);

    for (uint32_t i = 0; i < *infoCount; ++i) {
      // Prepare scratch space if stream doesn't contain it
      if (buildInfos[i].scratchData.hostAddress != nullptr) {
        continue;
      }

      std::vector<uint32_t> primitivesCount(buildInfos[i].geometryCount);

      for (uint32_t g = 0; g < buildInfos[i].geometryCount; ++g) {
        primitivesCount[g] = buildRangeInfos[i][g].primitiveCount;
      }

      VkAccelerationStructureBuildSizesInfoKHR buildSizesInfoKHR = {
          VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR, // VkStructureType    sType;
          nullptr, // const void       * pNext;
          0,       // VkDeviceSize       accelerationStructureSize;
          0,       // VkDeviceSize       updateScratchSize;
          0        // VkDeviceSize       buildScratchSize;
      };
      drvVk.vkGetAccelerationStructureBuildSizesKHR(
          *device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_HOST_KHR, &buildInfos[i],
          primitivesCount.data(), &buildSizesInfoKHR);

      if (buildInfos[i].mode == VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR) {
        scratchSpace[i].resize(buildSizesInfoKHR.buildScratchSize);
      } else if (buildInfos[i].mode == VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR) {
        scratchSpace[i].resize(buildSizesInfoKHR.updateScratchSize);
      }

      buildInfos[i].scratchData.hostAddress = scratchSpace[i].data();
    }
  }

  return_value.Assign(drvVk.vkBuildAccelerationStructuresKHR(
      *device, *deferredOperation, *infoCount, buildInfos, buildRangeInfos));
}

inline void vkCmdBuildAccelerationStructuresKHR_WRAPRUN(
    CVkCommandBuffer& commandBuffer,
    Cuint32_t& infoCount,
    CVkAccelerationStructureBuildGeometryInfoKHRArray& pInfos,
    CVkAccelerationStructureBuildRangeInfoKHRArrayOfArrays& ppBuildRangeInfos) {
  auto buildInfos = *pInfos;
  auto buildRangeInfos = *ppBuildRangeInfos;
  {
    CAutoCaller autoCaller(drvVk.vkPauseRecordingGITS, drvVk.vkContinueRecordingGITS);

    auto commandBufferState = SD()._commandbufferstates[*commandBuffer];
    VkDevice device = commandBufferState->commandPoolStateStore->deviceStateStore->deviceHandle;

    // Prepare scratch space if stream doesn't contain it
    for (uint32_t i = 0; i < *infoCount; ++i) {
      if (buildInfos[i].scratchData.deviceAddress != 0) {
        continue;
      }

      VkDeviceSize size = 0;
      {
        std::vector<uint32_t> primitivesCount(buildInfos[i].geometryCount);

        for (uint32_t g = 0; g < buildInfos[i].geometryCount; ++g) {
          primitivesCount[g] = buildRangeInfos[i][g].primitiveCount;
        }

        VkAccelerationStructureBuildSizesInfoKHR buildSizesInfoKHR = {
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR, // VkStructureType    sType;
            nullptr, // const void       * pNext;
            0,       // VkDeviceSize       accelerationStructureSize;
            0,       // VkDeviceSize       updateScratchSize;
            0        // VkDeviceSize       buildScratchSize;
        };
        drvVk.vkGetAccelerationStructureBuildSizesKHR(
            device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfos[i],
            primitivesCount.data(), &buildSizesInfoKHR);

        if (buildInfos[i].mode == VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR) {
          size = buildSizesInfoKHR.buildScratchSize;
        } else if (buildInfos[i].mode == VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR) {
          size = buildSizesInfoKHR.updateScratchSize;
        }
      }

      auto memoryBufferPair = createTemporaryBuffer(device, size,
                                                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                                        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                    commandBufferState.get());
      buildInfos[i].scratchData.deviceAddress =
          getBufferDeviceAddress(device, memoryBufferPair.second->bufferHandle);
    }
  }

  drvVk.vkCmdBuildAccelerationStructuresKHR(*commandBuffer, *infoCount, buildInfos,
                                            buildRangeInfos);
}

inline void vkGetPhysicalDeviceQueueFamilyProperties_WRAPRUN(
    CVkPhysicalDevice& _physicalDevice,
    Cuint32_t::CSArray& _pQueueFamilyPropertyCount,
    CVkQueueFamilyPropertiesArray& _pQueueFamilyProperties) {
  if (*_pQueueFamilyProperties != nullptr) {
    auto& physicalDeviceState = SD()._physicaldevicestates[*_physicalDevice];
    auto originalProperties = _pQueueFamilyProperties.Original();
    auto queueFamilyPropertyCount = **_pQueueFamilyPropertyCount;

    for (uint32_t i = 0; i < queueFamilyPropertyCount; i++) {
      physicalDeviceState->queueFamilyPropertiesOriginal.push_back(originalProperties[i]);
    }
  }

  drvVk.vkGetPhysicalDeviceQueueFamilyProperties(*_physicalDevice, *_pQueueFamilyPropertyCount,
                                                 *_pQueueFamilyProperties);
}

inline void vkGetPhysicalDeviceQueueFamilyProperties2_WRAPRUN(
    CVkPhysicalDevice& _physicalDevice,
    Cuint32_t::CSArray& _pQueueFamilyPropertyCount,
    CVkQueueFamilyProperties2Array& _pQueueFamilyProperties) {
  if (*_pQueueFamilyProperties != nullptr) {
    auto& physicalDeviceState = SD()._physicaldevicestates[*_physicalDevice];
    auto originalProperties = _pQueueFamilyProperties.Original();
    auto queueFamilyPropertyCount = **_pQueueFamilyPropertyCount;

    for (uint32_t i = 0; i < queueFamilyPropertyCount; i++) {
      physicalDeviceState->queueFamilyPropertiesOriginal.push_back(
          originalProperties[i].queueFamilyProperties);
    }
  }

  drvVk.vkGetPhysicalDeviceQueueFamilyProperties2(*_physicalDevice, *_pQueueFamilyPropertyCount,
                                                  *_pQueueFamilyProperties);
}

inline void vkGetPhysicalDeviceQueueFamilyProperties2KHR_WRAPRUN(
    CVkPhysicalDevice& _physicalDevice,
    Cuint32_t::CSArray& _pQueueFamilyPropertyCount,
    CVkQueueFamilyProperties2Array& _pQueueFamilyProperties) {
  if (*_pQueueFamilyProperties != nullptr) {
    const auto& physicalDeviceState = SD()._physicaldevicestates[*_physicalDevice];
    auto originalProperties = _pQueueFamilyProperties.Original();
    auto queueFamilyPropertyCount = **_pQueueFamilyPropertyCount;

    for (uint32_t i = 0; i < queueFamilyPropertyCount; i++) {
      physicalDeviceState->queueFamilyPropertiesOriginal.push_back(
          originalProperties[i].queueFamilyProperties);
    }
  }

  drvVk.vkGetPhysicalDeviceQueueFamilyProperties2KHR(*_physicalDevice, *_pQueueFamilyPropertyCount,
                                                     *_pQueueFamilyProperties);
}

namespace {

void PatchSBTHelper(const CCommandBufferState& cmdBufState,
                    const VkStridedDeviceAddressRegionKHR* pRaygenSBT,
                    const VkStridedDeviceAddressRegionKHR* pMissSBT,
                    const VkStridedDeviceAddressRegionKHR* pHitSBT,
                    const VkStridedDeviceAddressRegionKHR* pCallableSBT,
                    VkDeviceAddress oldHandlesMap,
                    VkDeviceAddress newHandlesMap,
                    uint32_t handlesMapEntriesCount) {
  // Patch shader group handles in a SBT before tracing rays
  CAutoCaller autoCaller(drvVk.vkPauseRecordingGITS, drvVk.vkContinueRecordingGITS);

  const auto device = cmdBufState.commandPoolStateStore->deviceStateStore->deviceHandle;
  const auto cmdBuf = cmdBufState.commandBufferHandle;

  // Inject compute - patch shader group handles in SBT with current values
  {
    VkMemoryBarrier barrierPre = {
        VK_STRUCTURE_TYPE_MEMORY_BARRIER,                      // VkStructureType sType;
        nullptr,                                               // const void* pNext;
        VK_ACCESS_MEMORY_WRITE_BIT,                            // VkAccessFlags srcAccessMask;
        VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT // VkAccessFlags dstAccessMask;
    };
    drvVk.vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                               VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &barrierPre, 0, nullptr,
                               0, nullptr);

    auto& gitsPipelines = SD().internalResources.internalPipelines[device];
    drvVk.vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE,
                            gitsPipelines.getPatchShaderGroupHandlesInSBT());

    auto dispatchComputeShader = [&](const VkStridedDeviceAddressRegionKHR* pSBT) {
      if (pSBT && pSBT->deviceAddress && pSBT->size) {
        auto stride = pSBT->stride > 0 ? pSBT->stride : pSBT->size;
        struct PushConstantsData {
          VkDeviceAddress OldHandlesMap;
          VkDeviceAddress NewHandlesMap;
          VkDeviceAddress SBTBaseAddress;
          uint32_t Stride;
          uint32_t Size;
        } pushConstants = {
            oldHandlesMap,       // VkDeviceAddress OldHandlesMap;
            newHandlesMap,       // VkDeviceAddress NewHandlesMap;
            pSBT->deviceAddress, // VkDeviceAddress SBTBaseAddress;
            (uint32_t)stride,    // uint32_t Stride;
            (uint32_t)pSBT->size // uint32_t Size;
        };
        // 32 is a performance optimization - 32 local invocations of a compute shader are dispatched
        uint32_t divisor = stride * 32;
        uint32_t invocationsCount = (pSBT->size / divisor) + ((pSBT->size % divisor > 0) ? 1 : 0);

        drvVk.vkCmdPushConstants(cmdBuf, gitsPipelines.getLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0,
                                 sizeof(PushConstantsData), &pushConstants);
        drvVk.vkCmdDispatch(cmdBuf, invocationsCount, handlesMapEntriesCount, 1);
      }
    };

    dispatchComputeShader(pRaygenSBT);
    dispatchComputeShader(pMissSBT);
    dispatchComputeShader(pHitSBT);
    dispatchComputeShader(pCallableSBT);

    VkMemoryBarrier barrierPost = {
        VK_STRUCTURE_TYPE_MEMORY_BARRIER,                       // VkStructureType sType;
        nullptr,                                                // const void* pNext;
        VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, // VkAccessFlags srcAccessMask;
        VK_ACCESS_SHADER_READ_BIT                               // VkAccessFlags dstAccessMask;
    };
    drvVk.vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                               VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, 0, 1, &barrierPost, 0,
                               nullptr, 0, nullptr);
  }

  // Restore original push constants data
  if (cmdBufState.currentPipeline != VK_NULL_HANDLE) {
    drvVk.vkCmdBindPipeline(cmdBuf, cmdBufState.currentPipelineBindPoint,
                            cmdBufState.currentPipeline);
    if (cmdBufState.pushContantsData.data.size()) {
      auto& pushContants = cmdBufState.pushContantsData;
      drvVk.vkCmdPushConstants(cmdBuf, pushContants.layout, pushContants.stageFlags,
                               pushContants.offset, pushContants.data.size(),
                               pushContants.data.data());
    }
  }
}

} // namespace

inline void vkCmdTraceRaysKHR_WRAPRUN(
    CVkCommandBuffer& _commandBuffer,
    CVkStridedDeviceAddressRegionKHR& _pRaygenShaderBindingTable,
    CVkStridedDeviceAddressRegionKHR& _pMissShaderBindingTable,
    CVkStridedDeviceAddressRegionKHR& _pHitShaderBindingTable,
    CVkStridedDeviceAddressRegionKHR& _pCallableShaderBindingTable,
    Cuint32_t& _width,
    Cuint32_t& _height,
    Cuint32_t& _depth) {
  const VkStridedDeviceAddressRegionKHR* pRaygenSBT = *_pRaygenShaderBindingTable;
  const VkStridedDeviceAddressRegionKHR* pMissSBT = *_pMissShaderBindingTable;
  const VkStridedDeviceAddressRegionKHR* pHitSBT = *_pHitShaderBindingTable;
  const VkStridedDeviceAddressRegionKHR* pCallableSBT = *_pCallableShaderBindingTable;

  const auto cmdBuf = *_commandBuffer;
  const auto& cmdBufState = *SD()._commandbufferstates[cmdBuf];
  const auto& groupHandles = SD()._pipelinestates[cmdBufState.currentPipeline]->shaderGroupHandles;
  const auto originalHandlesAddress = groupHandles.deviceAddress;
  const auto newHandlesAddress = groupHandles.deviceAddress + groupHandles.dataSize;

  if (groupHandles.patchingRequired) {
    PatchSBTHelper(cmdBufState, pRaygenSBT, pMissSBT, pHitSBT, pCallableSBT, originalHandlesAddress,
                   newHandlesAddress, groupHandles.count);
  }

  drvVk.vkCmdTraceRaysKHR(cmdBuf, pRaygenSBT, pMissSBT, pHitSBT, pCallableSBT, *_width, *_height,
                          *_depth);
  vkCmdTraceRaysKHR_SD(cmdBuf, pRaygenSBT, pMissSBT, pHitSBT, pCallableSBT, *_width, *_height,
                       *_depth);

  if (groupHandles.patchingRequired) {
    PatchSBTHelper(cmdBufState, pRaygenSBT, pMissSBT, pHitSBT, pCallableSBT, newHandlesAddress,
                   originalHandlesAddress, groupHandles.count);
  }
}

inline void vkCmdTraceRaysIndirectKHR_WRAPRUN(
    CVkCommandBuffer& _commandBuffer,
    CVkStridedDeviceAddressRegionKHR& _pRaygenShaderBindingTable,
    CVkStridedDeviceAddressRegionKHR& _pMissShaderBindingTable,
    CVkStridedDeviceAddressRegionKHR& _pHitShaderBindingTable,
    CVkStridedDeviceAddressRegionKHR& _pCallableShaderBindingTable,
    Cuint64_t& _indirectDeviceAddress) {
  const VkStridedDeviceAddressRegionKHR* pRaygenSBT = *_pRaygenShaderBindingTable;
  const VkStridedDeviceAddressRegionKHR* pMissSBT = *_pMissShaderBindingTable;
  const VkStridedDeviceAddressRegionKHR* pHitSBT = *_pHitShaderBindingTable;
  const VkStridedDeviceAddressRegionKHR* pCallableSBT = *_pCallableShaderBindingTable;

  const auto cmdBuf = *_commandBuffer;
  const auto& cmdBufState = *SD()._commandbufferstates[cmdBuf];
  const auto& groupHandles = SD()._pipelinestates[cmdBufState.currentPipeline]->shaderGroupHandles;
  const auto originalHandlesAddress = groupHandles.deviceAddress;
  const auto newHandlesAddress = groupHandles.deviceAddress + groupHandles.dataSize;

  if (groupHandles.patchingRequired) {
    PatchSBTHelper(cmdBufState, pRaygenSBT, pMissSBT, pHitSBT, pCallableSBT, originalHandlesAddress,
                   newHandlesAddress, groupHandles.count);
  }

  drvVk.vkCmdTraceRaysIndirectKHR(cmdBuf, pRaygenSBT, pMissSBT, pHitSBT, pCallableSBT,
                                  *_indirectDeviceAddress);
  vkCmdTraceRaysIndirectKHR_SD(cmdBuf, pRaygenSBT, pMissSBT, pHitSBT, pCallableSBT,
                               *_indirectDeviceAddress);

  if (groupHandles.patchingRequired) {
    PatchSBTHelper(cmdBufState, pRaygenSBT, pMissSBT, pHitSBT, pCallableSBT, newHandlesAddress,
                   originalHandlesAddress, groupHandles.count);
  }
}

} // namespace Vulkan
} // namespace gits

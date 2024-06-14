// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "helperVk.h"
#include "vulkanTools.h"
#include "vkWindowing.h"
#include "gits.h"
#include "platform.h"
#include "helper.h"
#if defined GITS_PLATFORM_WINDOWS
#include <windows.h>
#endif

// ### Global state ###

GlobalState globalState;

ImageState::ImageState(VkDevice device, const VkImageCreateInfo* imageCreateInfo)
    : width(imageCreateInfo->extent.width),
      height(imageCreateInfo->extent.height),
      depth(imageCreateInfo->extent.depth),
      imageFormat(imageCreateInfo->format),
      imageType(imageCreateInfo->imageType),
      imageCreateInfo(std::make_unique<const VkImageCreateInfo>(*imageCreateInfo)) {
  currentLayout.resize(imageCreateInfo->arrayLayers);
  for (auto& layer : currentLayout) {
    layer.resize(imageCreateInfo->mipLevels, {imageCreateInfo->initialLayout, 0});
  }
}
ImageState::ImageState(VkSwapchainKHR swapchainKHR,
                       const VkSwapchainCreateInfoKHR* swapchainCreateInfo)
    : width(swapchainCreateInfo->imageExtent.width),
      height(swapchainCreateInfo->imageExtent.height),
      depth(1),
      imageFormat(swapchainCreateInfo->imageFormat),
      imageType(VK_IMAGE_TYPE_2D),
      swapchainCreateInfo(std::make_unique<const VkSwapchainCreateInfoKHR>(*swapchainCreateInfo)) {
  currentLayout.resize(swapchainCreateInfo->imageArrayLayers);
  for (auto& layer : currentLayout) {
    layer.resize(1, {VK_IMAGE_LAYOUT_UNDEFINED, 0});
  }
}

// ### CCode wrap functions ###

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

VkPhysicalDevice GetPhysicalDeviceForReplay(
    std::vector<VkPhysicalDevice> const& playerSideDevices) {
  uint32_t selectedPhysicalDeviceIndex =
      gits::Config::Get().vulkan.player.vulkanForcedPhysicalDeviceIndex;
  if (selectedPhysicalDeviceIndex >= playerSideDevices.size()) {
    Log(WARN) << "Selected physical device index is greater than the number of enumerated physical "
                 "devices. Defaulting to 0.";
    selectedPhysicalDeviceIndex = 0;
  }

  VkPhysicalDevice selectedPhysicalDevice = playerSideDevices[selectedPhysicalDeviceIndex];

  VkPhysicalDeviceProperties physicalDeviceProperties = {};
  vkGetPhysicalDeviceProperties(selectedPhysicalDevice, &physicalDeviceProperties);

  Log(INFO) << "Playing stream on a"
            << ((physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
                    ? ("n integrated")
                    : (" discrete"))
            << " device named \"" << physicalDeviceProperties.deviceName << "\".";
  if (playerSideDevices.size() > 1) {
    Log(INFO, NO_PREFIX)
        << "      (To change device, use the following option: --useVKPhysicalDeviceIndex <index>)";
  }

  return selectedPhysicalDevice;
}
} // namespace

namespace {

VkResult vkEnumeratePhysicalDeviceGroups_Helper(
    PFN_vkEnumeratePhysicalDeviceGroups callEnumeratePhysicalDeviceGroups,
    VkInstance instance,
    uint32_t* pPhysicalDeviceGroupCount,
    VkPhysicalDeviceGroupProperties* pPhysicalDeviceGroupProperties) {
  uint32_t playerSideGroupCount = 0;
  VkResult playerSideReturnValue =
      callEnumeratePhysicalDeviceGroups(instance, &playerSideGroupCount, nullptr);

  if (pPhysicalDeviceGroupCount && pPhysicalDeviceGroupProperties) {
    std::vector<VkPhysicalDeviceGroupProperties> playerSideGroupProperties(playerSideGroupCount);
    playerSideReturnValue = callEnumeratePhysicalDeviceGroups(instance, &playerSideGroupCount,
                                                              playerSideGroupProperties.data());

    VkPhysicalDevice selectedPhysicalDevice =
        GetPhysicalDeviceForReplay(GetPhysicalDevicesFromGroupProperties(
            playerSideGroupCount, playerSideGroupProperties.data()));
    for (uint32_t g = 0; g < *pPhysicalDeviceGroupCount; ++g) {
      for (uint32_t d = 0; d < pPhysicalDeviceGroupProperties[g].physicalDeviceCount; ++d) {
        AddMapping(pPhysicalDeviceGroupProperties[g].physicalDevices[d], selectedPhysicalDevice);
      }
    }
  }

  return playerSideReturnValue;
}

} // namespace

VkResult vkEnumeratePhysicalDeviceGroups_CCODEWRAP(
    VkInstance instance,
    uint32_t* pPhysicalDeviceGroupCount,
    VkPhysicalDeviceGroupProperties* pPhysicalDeviceGroupProperties) {
  return vkEnumeratePhysicalDeviceGroups_Helper(vkEnumeratePhysicalDeviceGroups, instance,
                                                pPhysicalDeviceGroupCount,
                                                pPhysicalDeviceGroupProperties);
}

VkResult vkEnumeratePhysicalDeviceGroupsKHR_CCODEWRAP(
    VkInstance instance,
    uint32_t* pPhysicalDeviceGroupCount,
    VkPhysicalDeviceGroupProperties* pPhysicalDeviceGroupProperties) {
  return vkEnumeratePhysicalDeviceGroups_Helper(vkEnumeratePhysicalDeviceGroupsKHR, instance,
                                                pPhysicalDeviceGroupCount,
                                                pPhysicalDeviceGroupProperties);
}

VkResult vkEnumeratePhysicalDevices_CCODEWRAP(VkInstance instance,
                                              uint32_t* pPhysicalDeviceCountOrig,
                                              VkPhysicalDevice* pPhysicalDevicesOrig) {
  uint32_t playerSideDevicesCount = 0;
  VkResult playerSideReturnValue =
      vkEnumeratePhysicalDevices(instance, &playerSideDevicesCount, nullptr);

  if (pPhysicalDeviceCountOrig && pPhysicalDevicesOrig) {
    std::vector<VkPhysicalDevice> playerSidePhysicalDevices(playerSideDevicesCount);
    playerSideReturnValue = vkEnumeratePhysicalDevices(instance, &playerSideDevicesCount,
                                                       playerSidePhysicalDevices.data());

    VkPhysicalDevice selectedPhysicalDevice = GetPhysicalDeviceForReplay(playerSidePhysicalDevices);
    for (uint32_t i = 0; i < *pPhysicalDeviceCountOrig; ++i) {
      AddMapping(pPhysicalDevicesOrig[i], selectedPhysicalDevice);
    }
  }

  return playerSideReturnValue;
}

VkResult vkMapMemory_CCODEWRAP(VkDevice device,
                               VkDeviceMemory memory,
                               VkDeviceSize offset,
                               VkDeviceSize size,
                               VkMemoryMapFlags flags) {
  void* mappedMemPtr;
  VkResult result = vkMapMemory(device, memory, offset, size, flags, &mappedMemPtr);

  if (result == VK_SUCCESS) {
    globalState.mappedMemPtrs[memory] = mappedMemPtr;
  } else {
    Log(WARN) << "Failure (" << result << ") returned by vkMapMemory(" << gits::hex(device) << ", "
              << gits::hex(memory) << ", " << offset << ", " << size << ", " << flags << ", "
              << gits::hex(&mappedMemPtr) << ")";
  }

  return result;
}

void vkUnmapMemory_CCODEWRAP(VkDevice device, VkDeviceMemory memory) {
  vkUnmapMemory(device, memory);

  globalState.mappedMemPtrs.erase(memory);
}

namespace {
// Helper function for AcquireNextImage wraps.
inline void RewindSwapchainImageIndex(VkDevice device,
                                      VkQueue queue,
                                      VkFence fence,
                                      VkSemaphore semaphore,
                                      VkSwapchainKHR swapchain,
                                      uint32_t* imageIndex) {
  VkPresentInfoKHR presentInfo;
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.pNext = nullptr;
  presentInfo.waitSemaphoreCount = 0;
  presentInfo.pWaitSemaphores = nullptr;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &swapchain;
  presentInfo.pImageIndices = imageIndex;
  presentInfo.pResults = nullptr;

  if (semaphore != VK_NULL_HANDLE) {
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &semaphore;
  }
  if (fence != VK_NULL_HANDLE) {
    vkWaitForFences(device, 1, &fence, VK_FALSE, 2000000000);
    vkResetFences(device, 1, &fence);
  }

  // Send message to benchmarkGPU VkShims.
  vkGetDeviceProcAddr(device, "vkUnwindQueuePresentGITS");

  vkQueuePresentKHR(queue, &presentInfo);
  vkQueueWaitIdle(queue);
}
} // namespace

VkResult vkAcquireNextImageKHR_CCODEWRAP(VkDevice device,
                                         VkSwapchainKHR swapchain,
                                         uint64_t timeout,
                                         VkSemaphore semaphore,
                                         VkFence fence,
                                         uint32_t* pImageIndex) {
  const uint32_t recorderIndex = *pImageIndex;
  VkResult retval =
      vkAcquireNextImageKHR(device, swapchain, timeout, semaphore, fence, pImageIndex);

  if (recorderIndex != *pImageIndex) {
    Log(TRACE) << "vkAcquireNextImageKHR restore section begin.";
    while (recorderIndex != *pImageIndex) {
      RewindSwapchainImageIndex(device, globalState.deviceStates.at(device).queueList[0], fence,
                                semaphore, swapchain, pImageIndex);

      retval = vkAcquireNextImageKHR(device, swapchain, timeout, semaphore, fence, pImageIndex);
    }
    Log(TRACE) << "vkAcquireNextImageKHR restore section end.";
  }
  if (VK_NULL_HANDLE != fence) {
    globalState.fenceStates[fence].fenceUsed = true;
  }

  return retval;
}

VkResult vkAcquireNextImage2KHR_CCODEWRAP(VkDevice device,
                                          const VkAcquireNextImageInfoKHR* pAcquireInfo,
                                          uint32_t* pImageIndex) {
  const uint32_t recorderIndex = *pImageIndex;
  const VkAcquireNextImageInfoKHR* acquireInfo = pAcquireInfo;
  VkResult retval = vkAcquireNextImage2KHR(device, acquireInfo, pImageIndex);

  if (recorderIndex != *pImageIndex) {
    Log(TRACE) << "vkAcquireNextImage2KHR restore section begin.";
    while (recorderIndex != *pImageIndex) {
      RewindSwapchainImageIndex(device, globalState.deviceStates.at(device).queueList[0],
                                acquireInfo->fence, acquireInfo->semaphore, acquireInfo->swapchain,
                                pImageIndex);

      retval = vkAcquireNextImage2KHR(device, acquireInfo, pImageIndex);
    }
    Log(TRACE) << "vkAcquireNextImage2KHR restore section end.";
  }
  if (VK_NULL_HANDLE != pAcquireInfo->fence) {
    globalState.fenceStates[pAcquireInfo->fence].fenceUsed = true;
  }

  return retval;
}

VkResult vkCreateDevice_CCODEWRAP(VkPhysicalDevice physicalDevice,
                                  const VkDeviceCreateInfo* pCreateInfo,
                                  const VkAllocationCallbacks* pAllocator,
                                  VkDevice* pDevice) {
  VkResult result = vkCreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);

  globalState.deviceStates[*pDevice].physicalDevice = physicalDevice;
  globalState.deviceStates[*pDevice].deviceCreateInfo = *pCreateInfo;

  return result;
}

void vkDestroyDevice_CCODEWRAP(VkDevice device, const VkAllocationCallbacks* pAllocator) {
  vkDestroyDevice(device, pAllocator);
  globalState.deviceStates.erase(device);
}

VkResult vkCreateImage_CCODEWRAP(VkDevice device,
                                 const VkImageCreateInfo* pCreateInfo,
                                 const VkAllocationCallbacks* pAllocator,
                                 VkImage* pImage) {
  VkResult result = vkCreateImage(device, pCreateInfo, pAllocator, pImage);

  globalState.imageStates.emplace(*pImage, std::make_unique<ImageState>(device, pCreateInfo));

  return result;
}

VkResult vkCreateSwapchainKHR_CCODEWRAP(VkDevice device,
                                        const VkSwapchainCreateInfoKHR* pCreateInfo,
                                        const VkAllocationCallbacks* pAllocator,
                                        VkSwapchainKHR* pSwapChain) {
  VkResult result = vkCreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapChain);

  uint32_t imageCount;
  vkGetSwapchainImagesKHR(device, *pSwapChain, &imageCount, NULL);
  std::vector<VkImage> images;
  images.resize(imageCount);
  vkGetSwapchainImagesKHR(device, *pSwapChain, &imageCount, images.data());
  globalState.swapchainStates[*pSwapChain].swapchainImages = images;

  for (VkImage image : images) {
    globalState.imageStates.emplace(image, std::make_unique<ImageState>(*pSwapChain, pCreateInfo));
  }

  return result;
}

void vkDestroySwapchainKHR_CCODEWRAP(VkDevice device,
                                     VkSwapchainKHR swapchain,
                                     const VkAllocationCallbacks* pAllocator) {
  vkDestroySwapchainKHR(device, swapchain, pAllocator);
  for (auto& image : globalState.swapchainStates.at(swapchain).swapchainImages) {
    globalState.imageStates.erase(image);
  }
  globalState.swapchainStates.erase(swapchain);
}

VkResult vkQueuePresentKHR_CCODEWRAP(VkQueue queue, const VkPresentInfoKHR* pPresentInfo) {
  OnFrameEnd();

  if (gits::Config::Get().common.player.captureFrames[CGits::Instance().CurrentFrame()]) {
    gits::Vulkan::writeCCodeScreenshot(
        queue, *pPresentInfo, [&](VkSwapchainKHR swapchain, unsigned int imageIndex) -> VkImage {
          return globalState.swapchainStates.at(swapchain).swapchainImages[imageIndex];
        });
  }

  return vkQueuePresentKHR(queue, pPresentInfo);
}

void vkGetDeviceQueue_CCODEWRAP(VkDevice device,
                                uint32_t queueFamilyIndex,
                                uint32_t queueIndex,
                                VkQueue* pQueue) {
  vkGetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);
  globalState.deviceStates[device].queueList.push_back(*pQueue);
  globalState.queueStates[*pQueue].device = device;
  globalState.queueStates[*pQueue].deviceQueueList.push_back({queueFamilyIndex, queueIndex});
}

void vkGetDeviceQueue2_CCODEWRAP(VkDevice device,
                                 const VkDeviceQueueInfo2* pQueueInfo,
                                 VkQueue* pQueue) {
  vkGetDeviceQueue2(device, pQueueInfo, pQueue);
  globalState.deviceStates[device].queueList.push_back(*pQueue);
  globalState.queueStates[*pQueue].device = device;
  globalState.queueStates[*pQueue].deviceQueueList.push_back(
      {pQueueInfo->queueFamilyIndex, pQueueInfo->queueIndex});
}

void vkGetFenceStatus_CCODEWRAP(VkResult recRetVal, VkDevice device, VkFence fence) {
  VkResult retValue = vkGetFenceStatus(device, fence);

  if (retValue != VK_SUCCESS && (retValue != recRetVal) &&
      globalState.fenceStates[fence].fenceUsed) {
    VkFence fenceCopy = fence;
    vkWaitForFences(device, 1, &fenceCopy, VK_TRUE, 0xFFFFFFFFFFFFFFFF);
    retValue = vkGetFenceStatus(device, fence);
  }
}

VkResult vkCreateFence_CCODEWRAP(VkDevice device,
                                 const VkFenceCreateInfo* pCreateInfo,
                                 const VkAllocationCallbacks* pAllocator,
                                 VkFence* pFence) {
  VkResult return_value = vkCreateFence(device, pCreateInfo, pAllocator, pFence);
  if (*pFence != VK_NULL_HANDLE && return_value == VK_SUCCESS) {
    globalState.fenceStates[*pFence].fenceUsed = pCreateInfo->flags & VK_FENCE_CREATE_SIGNALED_BIT;
  }
  return return_value;
}

VkResult vkWaitForFences_CCODEWRAP(VkResult recRetVal,
                                   VkDevice device,
                                   uint32_t fenceCount,
                                   const VkFence* pFences,
                                   VkBool32 waitAll,
                                   uint64_t timeout) {
  std::vector<VkFence> fences;
  for (unsigned i = 0; i < fenceCount; i++) {
    if (globalState.fenceStates[pFences[i]].fenceUsed) {
      fences.push_back(pFences[i]);
    }
  }
  VkResult return_value = VK_SUCCESS;
  if (fences.size() > 0) {
    return_value = vkWaitForFences(device, (uint32_t)fences.size(), &fences[0], waitAll, timeout);
    if (return_value != VK_SUCCESS && (return_value != recRetVal)) {
      return_value =
          vkWaitForFences(device, (uint32_t)fences.size(), &fences[0], VK_TRUE, 0xFFFFFFFFFFFFFFFF);
    }
  }
  return return_value;
}

VkResult vkQueueBindSparse_CCODEWRAP(VkQueue queue,
                                     uint32_t bindInfoCount,
                                     const VkBindSparseInfo* pBindInfo,
                                     VkFence fence) {
  VkResult return_value = vkQueueBindSparse(queue, bindInfoCount, pBindInfo, fence);
  if (VK_NULL_HANDLE != fence) {
    globalState.fenceStates[fence].fenceUsed = true;
  }
  return return_value;
}

VkResult vkQueueSubmit_CCODEWRAP(VkQueue queue,
                                 uint32_t submitCount,
                                 const VkSubmitInfo* pSubmits,
                                 VkFence fence) {
  VkResult return_value = vkQueueSubmit(queue, submitCount, pSubmits, fence);
  if (VK_NULL_HANDLE != fence) {
    globalState.fenceStates[fence].fenceUsed = true;
  }
  return return_value;
}

VkResult vkQueueSubmit2_CCODEWRAP(VkQueue queue,
                                  uint32_t submitCount,
                                  const VkSubmitInfo2* pSubmits,
                                  VkFence fence) {
  VkResult return_value = vkQueueSubmit2(queue, submitCount, pSubmits, fence);
  if (VK_NULL_HANDLE != fence) {
    globalState.fenceStates[fence].fenceUsed = true;
  }
  return return_value;
}

VkResult vkResetFences_CCODEWRAP(VkDevice device, uint32_t fenceCount, const VkFence* pFences) {
  VkResult return_value = vkResetFences(device, fenceCount, pFences);
  for (uint32_t i = 0; i < fenceCount; i++) {
    globalState.fenceStates[pFences[i]].fenceUsed = false;
  }
  return return_value;
}

// ### CGits* functions ###

void CGitsVkMemoryUpdate(VkDevice device,
                         VkDeviceMemory mem,
                         const uint64_t offset,
                         const uint64_t length,
                         const void* resource) {
  if (resource == nullptr) {
    Log(ERR) << "Resource can't be a nullptr.";
    throw std::invalid_argument(EXCEPTION_MESSAGE);
  }

  void* pointer;
  try {
    pointer = globalState.mappedMemPtrs.at(mem);
  } catch (const std::out_of_range&) {
    Log(ERR) << "VkDeviceMemory " << gits::hex(mem) << " should be mapped but it isn't.";
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  char* pointerToData = (char*)pointer + offset;
  std::memcpy(pointerToData, resource, (size_t)length);
}

void CGitsVkMemoryUpdate2(VkDeviceMemory mem,
                          uint64_t size,
                          const uint64_t* offsets,
                          const uint64_t* lengths,
                          const void** resources) {
  if (size == 0) {
    return; // Nothing to do.
  }

  if (mem == 0) {
    Log(ERR) << "Mem can't be a nullptr.";
    throw std::invalid_argument(EXCEPTION_MESSAGE);
  }

  if (offsets == nullptr) {
    Log(ERR) << "Offsets can't be a nullptr.";
    throw std::invalid_argument(EXCEPTION_MESSAGE);
  }
  if (lengths == nullptr) {
    Log(ERR) << "Lengths can't be a nullptr.";
    throw std::invalid_argument(EXCEPTION_MESSAGE);
  }
  if (resources == nullptr) {
    Log(ERR) << "Resources can't be a nullptr.";
    throw std::invalid_argument(EXCEPTION_MESSAGE);
  }

  for (uint64_t i = 0; i < size; ++i) {
    const void* resourcePtr = resources[i];
    if (resourcePtr) {
      void* pointer;
      try {
        pointer = globalState.mappedMemPtrs.at(mem);
      } catch (const std::out_of_range&) {
        Log(ERR) << "VkDeviceMemory " << gits::hex(mem) << " should be mapped but it isn't.";
        throw std::runtime_error(EXCEPTION_MESSAGE);
      }
      char* pointerToData = (char*)pointer + offsets[i];
      std::memcpy(pointerToData, resourcePtr, (size_t)lengths[i]);
    }
  }
}

void CGitsVkMemoryRestore(
    VkDevice device, VkDeviceMemory mem, uint64_t length, uint64_t offset, const void* resource) {
  if (resource) {
    void* pointer = nullptr;
    vkMapMemory(device, mem, offset, length, 0, &pointer);
    std::memcpy(pointer, resource, static_cast<size_t>(length));
    vkUnmapMemory(device, mem);
  }
}

void CGitsVkMemoryReset(VkDevice device, VkDeviceMemory mem, uint64_t length) {
  if (length == 0) {
    return;
  }

  void* pointer = nullptr;
  vkMapMemory(device, mem, 0, length, 0, &pointer);
  memset(pointer, 0, (size_t)length);
  vkUnmapMemory(device, mem);
}

void CGitsVkCreateNativeWindow(
    int x, int y, int w, int h, bool visible, vk_win_handle_t wh, connection_t connection) {
#if defined GITS_PLATFORM_WINDOWS
  gits::Vulkan::Window_* win = new gits::Vulkan::Window_(w, h, x, y, visible);
  globalState.windowStates[win->handle()].window = win;
  AddMapping(wh, (vk_win_handle_t)win->handle());
  AddMapping(connection, (HINSTANCE)GetModuleHandle(nullptr));
#elif defined GITS_PLATFORM_X11
  gits::Vulkan::Window_* win =
      new gits::Vulkan::Window_(w, h, x, y, visible, gits::Vulkan::DisplayProtocol::XCB);
  globalState.windowStates[win->handle()].window = win;
  AddMapping((Window)wh, (Window)win->handle());
  AddMapping(connection, (xcb_connection_t*)win->native_display());
#else
  Log(ERR) << "Vulkan window creation not implemented on this platform.";
  throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
}

void CGitsVkCreateXlibWindow(
    int x, int y, int w, int h, bool visible, vk_win_handle_t wh, connection_t connection) {
#if defined GITS_PLATFORM_X11
  gits::Vulkan::Window_* win =
      new gits::Vulkan::Window_(w, h, x, y, visible, gits::Vulkan::DisplayProtocol::XLIB);
  globalState.windowStates[win->handle()].window = win;
  AddMapping((Window)wh, (Window)win->handle());
  AddMapping((Display*)connection, (Display*)win->native_display());
#else
  Log(ERR) << "Vulkan XLIB window creation not implemented on this platform.";
  throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
}

void CGitsVkUpdateNativeWindow(int x, int y, int w, int h, bool visible, vk_win_handle_t wh) {
  gits::Vulkan::Window_* win = globalState.windowStates[wh].window;
  win->set_size(w, h);
  win->set_position(x, y);
  win->set_visibility(visible);
}

void CGitsDestroyVulkanDescriptorSets(size_t size, VkDescriptorSet* descSetsArray) {
  RemoveMapping(descSetsArray, size);
}

void CGitsDestroyVulkanCommandBuffers(size_t size, VkCommandBuffer* cmdbufArray) {
  RemoveMapping(cmdbufArray, size);
}

#if defined(GITS_PLATFORM_WINDOWS)

namespace {
BOOL CALLBACK Monitorenumproc(HMONITOR Arg1, HDC Arg2, LPRECT Arg3, LPARAM Arg4) {
  (*(HMONITOR*)Arg4) = Arg1;
  return FALSE;
}
} // namespace

void CGitsVkEnumerateDisplayMonitors(HMONITOR* hmonitor) {
  EnumDisplayMonitors(NULL, NULL, Monitorenumproc, (LPARAM)hmonitor);
}

#endif

// ### Helpers ###

void InitVk() {}

void ReleaseVk() {
  // Due to driver problems, all swapchains need to be always deleted
  for (auto& swapchainState : globalState.swapchainStates) {
    vkDestroySwapchainKHR(globalState.deviceStates.begin()->first, swapchainState.first, nullptr);
  }
  globalState.swapchainStates.clear();

  for (auto& deviceState : globalState.deviceStates) {
    vkDestroyDevice(deviceState.first, nullptr);
  }
  globalState.deviceStates.clear();
}

VkClearColorValue MakeVkClearColorValue(uint32_t red,
                                        uint32_t green,
                                        uint32_t blue,
                                        uint32_t alpha) {
  VkClearColorValue val;
  val.uint32[0] = red;
  val.uint32[1] = green;
  val.uint32[2] = blue;
  val.uint32[3] = alpha;
  return val;
}

// ### API calls ###

namespace api {

#define VK_GLOBAL_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call)   \
  return_type(STDCALL*& function_name) function_arguments = gits::Vulkan::drvVk.function_name;
#define VK_INSTANCE_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call, \
                                   first_argument_name)                                            \
  return_type(STDCALL*& function_name) function_arguments = gits::Vulkan::drvVk.function_name;
#define VK_DEVICE_LEVEL_FUNCTION(return_type, function_name, function_arguments, arguments_call,   \
                                 first_argument_name)                                              \
  return_type(STDCALL*& function_name) function_arguments = gits::Vulkan::drvVk.function_name;

#include "vulkanDriversAuto.inl"

} // namespace api

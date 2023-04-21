// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   vulkanRecorderSubWrappers.h
*
* @brief Automatically generated declarations of Vulkan library simple function call wrappers.
*
*/

#pragma once

#include "recorder.h"
#include "vulkanFunctions.h"
#include "vulkanPreToken.h"
#include "vulkanStateTracking.h"

namespace gits {

namespace Vulkan {

void vkQueuePresentKHR_RECWRAP(VkResult return_value,
                               VkQueue queue,
                               const VkPresentInfoKHR* pPresentInfo,
                               CRecorder& recorder);

namespace {

VkSemaphore createSemaphore(VkDevice device, CRecorder& recorder) {
  VkSemaphoreCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, // VkStructureType sType;
      nullptr,                                 // const void* pNext;
      0                                        // VkSemaphoreCreateFlags flags;
  };

  VkSemaphore semaphore;
  if (drvVk.vkCreateSemaphore(device, &createInfo, nullptr, &semaphore) != VK_SUCCESS) {
    throw std::runtime_error("Could not create a semaphore.");
  }
  vkCreateSemaphore_SD(VK_SUCCESS, device, &createInfo, nullptr, &semaphore);

  recorder.Schedule(new CvkCreateSemaphore(VK_SUCCESS, device, &createInfo, nullptr, &semaphore));
  return semaphore;
}

VkFence createFence(VkDevice device, bool signaled, CRecorder& recorder) {
  VkFenceCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,                          // VkStructureType sType;
      nullptr,                                                      // const void* pNext;
      (signaled) ? (VkFenceCreateFlags)VK_FENCE_CREATE_SIGNALED_BIT // VkFenceCreateFlags flags;
                 : (VkFenceCreateFlags)0};

  VkFence fence;
  if (drvVk.vkCreateFence(device, &createInfo, nullptr, &fence) != VK_SUCCESS) {
    throw std::runtime_error("Could not create a fence.");
  }
  vkCreateFence_SD(VK_SUCCESS, device, &createInfo, nullptr, &fence);

  recorder.Schedule(new CvkCreateFence(VK_SUCCESS, device, &createInfo, nullptr, &fence));
  return fence;
}

VkCommandPool createCommandPool(VkDevice device, uint32_t queueFamilyIndex, CRecorder& recorder) {
  VkCommandPoolCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,       // VkStructureType sType;
      nullptr,                                          // const void* pNext;
      VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | // VkCommandPoolCreateFlags flags;
          VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
      queueFamilyIndex // uint32_t queueFamilyIndex;
  };

  VkCommandPool commandPool;
  if (drvVk.vkCreateCommandPool(device, &createInfo, nullptr, &commandPool) != VK_SUCCESS) {
    throw std::runtime_error("Could not create a command pool.");
  }
  vkCreateCommandPool_SD(VK_SUCCESS, device, &createInfo, nullptr, &commandPool);

  recorder.Schedule(
      new CvkCreateCommandPool(VK_SUCCESS, device, &createInfo, nullptr, &commandPool));
  return commandPool;
}

VkCommandBuffer allocateCommandBuffer(VkDevice device,
                                      VkCommandPool commandPool,
                                      CRecorder& recorder) {
  VkCommandBufferAllocateInfo allocateInfo = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, // VkStructureType sType;
      nullptr,                                        // const void* pNext;
      commandPool,                                    // VkCommandPool commandPool;
      VK_COMMAND_BUFFER_LEVEL_PRIMARY,                // VkCommandBufferLevel level;
      1                                               // uint32_t commandBufferCount;
  };

  VkCommandBuffer commandBuffer;
  if (drvVk.vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("Could not allocate a command buffer.");
  }
  vkAllocateCommandBuffers_SD(VK_SUCCESS, device, &allocateInfo, &commandBuffer);

  recorder.Schedule(
      new CvkAllocateCommandBuffers(VK_SUCCESS, device, &allocateInfo, &commandBuffer));
  return commandBuffer;
}

void handleSwapchainCreationForOffscreenApplications(VkDevice device,
                                                     const VkImageCreateInfo* pCreateInfo,
                                                     CRecorder& recorder) {
  auto& vs = SD().internalResources.virtualSwapchain[device];
  auto& offscreenApp = SD().internalResources.offscreenApps[device];
  CWindowParameters windowParameters = {nullptr, nullptr, {0, 0}};

  // Surface / window management
  {
    getWindowInstanceAndHandleAndSize(windowParameters);

    // hWnd is not changed (it still equals to null) when no window was found
    // or if any of it's dimensions are 0 (so we don't have to check dimensions here).
    if (!windowParameters.hWnd) {
      return;
    }

    // Don't do anything if nothing changed
    if ((offscreenApp.hwnd == windowParameters.hWnd) && (vs.extent.width > 0) &&
        (vs.extent.height > 0)) {
      return;
    }

    // Create a surface during the first initialization but also when hwnd changed.
    // The latter means that application's window was destroyed and a new one was created.
    if (offscreenApp.hwnd != windowParameters.hWnd) {
      auto instance =
          SD()._devicestates[device]->physicalDeviceStateStore->instanceStateStore->instanceHandle;

      if (offscreenApp.surface != VK_NULL_HANDLE) {
        recorder.Schedule(new CvkDestroySurfaceKHR(instance, offscreenApp.surface, nullptr));
        vkDestroySurfaceKHR_SD(instance, offscreenApp.surface, nullptr);
        vs.readyToPresent = false;
      }

      VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {
          VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR, // VkStructureType sType;
          nullptr,                                         // const void* pNext;
          0,                                               // VkWin32SurfaceCreateFlagsKHR flags;
          windowParameters.hInstance,                      // HINSTANCE hinstance;
          windowParameters.hWnd                            // HWND hwnd;
      };

      offscreenApp.surface = (VkSurfaceKHR)offscreenApp.uniqueHandleCounter++;
      vkCreateWin32SurfaceKHR_SD(VK_SUCCESS, instance, &surfaceCreateInfo, nullptr,
                                 &offscreenApp.surface);

      recorder.Schedule(
          new CGitsVkCreateNativeWindow(surfaceCreateInfo.hinstance, surfaceCreateInfo.hwnd));
      recorder.Schedule(new CvkCreateWin32SurfaceKHR(VK_SUCCESS, instance, &surfaceCreateInfo,
                                                     nullptr, &offscreenApp.surface));
      recorder.Schedule(new CGitsVkEnumerateDisplayMonitors(true));
      offscreenApp.hwnd = windowParameters.hWnd;
    }
  }

  {
    vs.extent = windowParameters.extent;

    if ((vs.extent.width == 0) || (vs.extent.height == 0)) {
      vs.readyToPresent = false;
      return;
    }
  }

  // One-time initialization of resources that won't change
  {
    CALL_ONCE[&] {
      // Select universal queue and command pool
      {
        auto& deviceState = SD()._devicestates[device];
        for (auto& queueState : deviceState->queueStateStoreList) {
          if ((queueState->queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
              (queueState->queueFlags & VK_QUEUE_COMPUTE_BIT) &&
              (queueState->queueFlags & VK_QUEUE_TRANSFER_BIT)) {
            vs.universalQueue = queueState->queueHandle;
            vs.commandPool = createCommandPool(device, queueState->queueFamilyIndex, recorder);
            break;
          }
        }
      }

      for (uint32_t i = 0; i < vs.imageCount; ++i) {
        auto& data = vs.presentationData[i];

        data.imageAcquiredSemaphore = createSemaphore(device, recorder);
        data.readyToPresentSemaphore = createSemaphore(device, recorder);
        data.fence = createFence(device, true, recorder);
        data.commandBuffer = allocateCommandBuffer(device, vs.commandPool, recorder);
      }
      return true;
    };
  }

  // Create a swapchain (replacing an old one if required after window size changed)
  {
    VkSwapchainKHR oldSwapchain = vs.swapchain;
    VkSwapchainCreateInfoKHR swapchainCreateInfo = {
        VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR, // VkStructureType sType;
        nullptr,                                     // const void* pNext;
        0,                                           // VkSwapchainCreateFlagsKHR flags;
        offscreenApp.surface,                        // VkSurfaceKHR surface;
        3,                                           // uint32_t minImageCount;
        pCreateInfo->format,                         // VkFormat imageFormat;
        VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,           // VkColorSpaceKHR imageColorSpace;
        {
            // VkExtent2D imageExtent;
            vs.extent.width, // uint32_t width;
            vs.extent.height // uint32_t height;
        },
        1,                                // uint32_t imageArrayLayers;
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | // VkImageUsageFlags imageUsage;
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_SHARING_MODE_EXCLUSIVE,             // VkSharingMode imageSharingMode;
        0,                                     // uint32_t queueFamilyIndexCount;
        nullptr,                               // const uint32_t* pQueueFamilyIndices;
        VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR, // VkSurfaceTransformFlagBitsKHR preTransform;
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,     // VkCompositeAlphaFlagBitsKHR compositeAlpha;
        VK_PRESENT_MODE_FIFO_KHR,              // VkPresentModeKHR presentMode;
        VK_FALSE,                              // VkBool32 clipped;
        oldSwapchain                           // VkSwapchainKHR oldSwapchain;
    };

    vs.swapchain = (VkSwapchainKHR)offscreenApp.uniqueHandleCounter++;
    recorder.Schedule(new CvkCreateSwapchainKHR(VK_SUCCESS, device, &swapchainCreateInfo, nullptr,
                                                &vs.swapchain));
    if (oldSwapchain != VK_NULL_HANDLE) {
      recorder.Schedule(new CvkDestroySwapchainKHR(device, oldSwapchain, nullptr));
    }

    // Get swapchain images and create presentation resources
    uint32_t count = 3;
    recorder.Schedule(
        new CvkGetSwapchainImagesKHR(VK_SUCCESS, device, vs.swapchain, &count, nullptr));

    std::vector<VkImage> swapchainImages = {
        (VkImage)offscreenApp.uniqueHandleCounter++,
        (VkImage)offscreenApp.uniqueHandleCounter++,
        (VkImage)offscreenApp.uniqueHandleCounter++,
    };
    recorder.Schedule(new CvkGetSwapchainImagesKHR(VK_SUCCESS, device, vs.swapchain, &count,
                                                   swapchainImages.data()));

    vkCreateFakeSwapchainKHR_SD(device, &swapchainCreateInfo, &vs.swapchain, swapchainImages);

    for (uint32_t i = 0; i < vs.imageCount; ++i) {
      vs.presentationData[i].swapchainImage = swapchainImages[i];
    }
  }
  // From now on (in offscreen applications) we assume that swapchain
  // will provide images in order, starting from 0.
  {
    vs.nextImage = 0;
    vs.readyToPresent = true;

    recorder.Schedule(new CvkAcquireNextImageKHR(
        VK_SUCCESS, device, vs.swapchain, 9000000000,
        vs.presentationData[vs.nextImage].imageAcquiredSemaphore, VK_NULL_HANDLE, &vs.nextImage));
  }
}

void scheduleCopyToSwapchainAndPresent(VkDevice device, VkQueue queue, CRecorder& recorder) {
  auto& vs = SD().internalResources.virtualSwapchain[device];
  auto& offscreenApp = SD().internalResources.offscreenApps[device];
  auto& presentationData = vs.presentationData[vs.nextImage];

  if (!vs.readyToPresent) {
    return;
  }

  recorder.Schedule(
      new CvkWaitForFences(VK_SUCCESS, device, 1, &presentationData.fence, VK_FALSE, 9000000000));
  recorder.Schedule(new CvkResetFences(VK_SUCCESS, device, 1, &presentationData.fence));

  // Copy data to a virtual swapchain
  {
    VkCommandBuffer commandBuffer = presentationData.commandBuffer;
    VkImage srcImage = offscreenApp.imageToPresent;
    auto& srcImageState = SD()._imagestates[srcImage];
    VkImage dstImage = presentationData.swapchainImage;

    // Begin a command buffer for performing copy
    {
      VkCommandBufferBeginInfo beginInfo = {
          VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, // VkStructureType sType;
          nullptr,                                     // const void* pNext;
          VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, // VkCommandBufferUsageFlags flags;
          nullptr // const VkCommandBufferInheritanceInfo* pInheritanceInfo;
      };
      recorder.Schedule(new CvkBeginCommandBuffer(VK_SUCCESS, commandBuffer, &beginInfo));
    }
    // Initial memory barriers before copy
    {
      std::vector<VkImageMemoryBarrier> preTransferImageMemoryBarriers = {
          {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, // VkStructureType sType;
           nullptr,                                // const void* pNext;
           VK_ACCESS_MEMORY_WRITE_BIT,             // VkAccessFlags srcAccessMask;
           VK_ACCESS_TRANSFER_READ_BIT,            // VkAccessFlags dstAccessMask;
           VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,        // VkImageLayout oldLayout;
           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,   // VkImageLayout newLayout;
           VK_QUEUE_FAMILY_IGNORED,                // uint32_t srcQueueFamilyIndex;
           VK_QUEUE_FAMILY_IGNORED,                // uint32_t dstQueueFamilyIndex;
           srcImage,                               // VkImage image;
           {
               // VkImageSubresourceRange subresourceRange;
               VK_IMAGE_ASPECT_COLOR_BIT, // VkImageAspectFlags aspectMask;
               0,                         // uint32_t baseMipLevel;
               1,                         // uint32_t levelCount;
               0,                         // uint32_t baseArrayLayer;
               1                          // uint32_t layerCount;
           }},
          {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, // VkStructureType sType;
           nullptr,                                // const void* pNext;
           VK_ACCESS_MEMORY_READ_BIT,              // VkAccessFlags srcAccessMask;
           VK_ACCESS_TRANSFER_WRITE_BIT,           // VkAccessFlags dstAccessMask;
           VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,        // VkImageLayout oldLayout;
           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,   // VkImageLayout newLayout;
           VK_QUEUE_FAMILY_IGNORED,                // uint32_t srcQueueFamilyIndex;
           VK_QUEUE_FAMILY_IGNORED,                // uint32_t dstQueueFamilyIndex;
           dstImage,                               // VkImage image;
           {
               // VkImageSubresourceRange subresourceRange;
               VK_IMAGE_ASPECT_COLOR_BIT, // VkImageAspectFlags aspectMask;
               0,                         // uint32_t baseMipLevel;
               1,                         // uint32_t levelCount;
               0,                         // uint32_t baseArrayLayer;
               1                          // uint32_t layerCount;
           }}};
      recorder.Schedule(new CvkCmdPipelineBarrier(
          commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
          VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr,
          (uint32_t)preTransferImageMemoryBarriers.size(), preTransferImageMemoryBarriers.data()));
    }
    // Data copy
    {
      VkImageBlit imageBlit = {{
                                   // VkImageSubresourceLayers srcSubresource;
                                   VK_IMAGE_ASPECT_COLOR_BIT, // VkImageAspectFlags aspectMask;
                                   0,                         // uint32_t mipLevel;
                                   0,                         // uint32_t baseArrayLayer;
                                   1                          // uint32_t layerCount;
                               },
                               {// VkOffset3D srcOffsets[2];
                                {0, 0, 0},
                                {srcImageState->width, srcImageState->height, 1}},
                               {
                                   // VkImageSubresourceLayers dstSubresource;
                                   VK_IMAGE_ASPECT_COLOR_BIT, // VkImageAspectFlags aspectMask;
                                   0,                         // uint32_t mipLevel;
                                   0,                         // uint32_t baseArrayLayer;
                                   1                          // uint32_t layerCount;
                               },
                               {// VkOffset3D dstOffsets[2];
                                {0, 0, 0},
                                {(int32_t)vs.extent.width, (int32_t)vs.extent.height, 1}}};
      recorder.Schedule(new CvkCmdBlitImage(
          commandBuffer, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstImage,
          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR));
    }
    // Post-copy, pre-present memory barriers
    {
      std::vector<VkImageMemoryBarrier> postTransferImageMemoryBarriers = {
          {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, // VkStructureType sType;
           nullptr,                                // const void* pNext;
           VK_ACCESS_TRANSFER_READ_BIT,            // VkAccessFlags srcAccessMask;
           0,                                      // VkAccessFlags dstAccessMask;
           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,   // VkImageLayout oldLayout;
           VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,        // VkImageLayout newLayout;
           VK_QUEUE_FAMILY_IGNORED,                // uint32_t srcQueueFamilyIndex;
           VK_QUEUE_FAMILY_IGNORED,                // uint32_t dstQueueFamilyIndex;
           srcImage,                               // VkImage image;
           {
               // VkImageSubresourceRange subresourceRange;
               VK_IMAGE_ASPECT_COLOR_BIT, // VkImageAspectFlags aspectMask;
               0,                         // uint32_t baseMipLevel;
               1,                         // uint32_t levelCount;
               0,                         // uint32_t baseArrayLayer;
               1                          // uint32_t layerCount;
           }},
          {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, // VkStructureType sType;
           nullptr,                                // const void* pNext;
           VK_ACCESS_TRANSFER_WRITE_BIT,           // VkAccessFlags srcAccessMask;
           VK_ACCESS_MEMORY_READ_BIT,              // VkAccessFlags dstAccessMask;
           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,   // VkImageLayout oldLayout;
           VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,        // VkImageLayout newLayout;
           VK_QUEUE_FAMILY_IGNORED,                // uint32_t srcQueueFamilyIndex;
           VK_QUEUE_FAMILY_IGNORED,                // uint32_t dstQueueFamilyIndex;
           dstImage,                               // VkImage image;
           {
               // VkImageSubresourceRange subresourceRange;
               VK_IMAGE_ASPECT_COLOR_BIT, // VkImageAspectFlags aspectMask;
               0,                         // uint32_t baseMipLevel;
               1,                         // uint32_t levelCount;
               0,                         // uint32_t baseArrayLayer;
               1                          // uint32_t layerCount;
           }}};
      recorder.Schedule(new CvkCmdPipelineBarrier(
          commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
          VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr,
          (uint32_t)postTransferImageMemoryBarriers.size(),
          postTransferImageMemoryBarriers.data()));
    }
    // End the command buffer
    { recorder.Schedule(new CvkEndCommandBuffer(VK_SUCCESS, commandBuffer)); }
    // Submit
    {
      VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
      VkSubmitInfo submitInfo = {
          VK_STRUCTURE_TYPE_SUBMIT_INFO,            // VkStructureType sType;
          nullptr,                                  // const void* pNext;
          1,                                        // uint32_t waitSemaphoreCount;
          &presentationData.imageAcquiredSemaphore, // const VkSemaphore* pWaitSemaphores;
          &waitStages,    // const VkPipelineStageFlags* pWaitDstStageMask;
          1,              // uint32_t commandBufferCount;
          &commandBuffer, // const VkCommandBuffer* pCommandBuffers;
          1,              // uint32_t signalSemaphoreCount;
          &presentationData.readyToPresentSemaphore // const VkSemaphore* pSignalSemaphores;
      };
      recorder.Schedule(
          new CvkQueueSubmit(VK_SUCCESS, queue, 1, &submitInfo, presentationData.fence));
    }
  }
  // Present an image on screen
  {
    VkPresentInfoKHR presentInfo = {
        VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,        // VkStructureType sType;
        nullptr,                                   // const void* pNext;
        1,                                         // uint32_t waitSemaphoreCount;
        &presentationData.readyToPresentSemaphore, // const VkSemaphore* pWaitSemaphores;
        1,                                         // uint32_t swapchainCount;
        &vs.swapchain,                             // const VkSwapchainKHR* pSwapchains;
        &vs.nextImage,                             // const uint32_t* pImageIndices;
        nullptr                                    // VkResult* pResults;
    };

    vkQueuePresentKHR_RECWRAP(VK_SUCCESS, queue, &presentInfo, recorder);
    recorder.EndFramePost();
  }
  // Acquire next image
  {
    vs.nextImage = (vs.nextImage + 1) % vs.imageCount;
    recorder.Schedule(new CvkAcquireNextImageKHR(
        VK_SUCCESS, device, vs.swapchain, 9000000000,
        vs.presentationData[vs.nextImage].imageAcquiredSemaphore, VK_NULL_HANDLE, &vs.nextImage));
  }
}

} // namespace
inline void vkQueuePresentKHR_RECWRAP(VkResult return_value,
                                      VkQueue queue,
                                      const VkPresentInfoKHR* pPresentInfo,
                                      CRecorder& recorder) {
  if (recorder.Running()) {
    std::unordered_set<std::shared_ptr<CSurfaceKHRState>> surfacesStates;

    for (size_t i = 0; i < pPresentInfo->swapchainCount; i++) {
      surfacesStates.insert(
          SD()._swapchainkhrstates[pPresentInfo->pSwapchains[i]]->surfaceKHRStateStore);
    }

#ifdef GITS_PLATFORM_WINDOWS
    for (auto& surfaceState : surfacesStates) {
      auto& hwndState = SD()._hwndstates[surfaceState->surfaceCreateInfoWin32Data.Value()->hwnd];

      int x = hwndState->x;
      int y = hwndState->y;
      int w = hwndState->w;
      int h = hwndState->h;
      bool vis = hwndState->vis;

      window_handle win(surfaceState->surfaceCreateInfoWin32Data.Value()->hwnd);
      int xx, yy, ww, hh;
      win.get_dimensions(xx, yy, ww, hh);
      if ((xx != x) || (yy != y) || (ww != w) || (hh != h) || (win.is_visible() != vis)) {
        recorder.Schedule(
            new CGitsVkUpdateNativeWindow(surfaceState->surfaceCreateInfoWin32Data.Value()->hwnd));
      }
    }
#endif
#ifdef GITS_PLATFORM_X11
    for (auto& surfaceState : surfacesStates) {
      VkXcbSurfaceCreateInfoKHR* xcbCreateInfoPtr = surfaceState->surfaceCreateInfoXcbData.Value();
      VkXlibSurfaceCreateInfoKHR* xlibCreateInfoPtr =
          surfaceState->surfaceCreateInfoXlibData.Value();
      if (xcbCreateInfoPtr != nullptr) {
        auto& hwndState = SD()._hwndstates[xcbCreateInfoPtr->window];

        int x = hwndState->x;
        int y = hwndState->y;
        int w = hwndState->w;
        int h = hwndState->h;
        bool vis = hwndState->vis;

        xcb_handle win(xcbCreateInfoPtr->connection, xcbCreateInfoPtr->window);
        int xx, yy, ww, hh;
        win.get_dimensions(xx, yy, ww, hh);
        if ((xx != x) || (yy != y) || (ww != w) || (hh != h) || (win.is_visible() != vis)) {
          recorder.Schedule(new CGitsVkUpdateNativeWindow(xcbCreateInfoPtr->window));
        }
      } else if (xlibCreateInfoPtr != nullptr) {
        auto& hwndState = SD()._hwndstates[xlibCreateInfoPtr->window];

        int x = hwndState->x;
        int y = hwndState->y;
        int w = hwndState->w;
        int h = hwndState->h;
        bool vis = hwndState->vis;

        xlib_handle win(xlibCreateInfoPtr->dpy, xlibCreateInfoPtr->window);
        int xx, yy, ww, hh;
        win.get_dimensions(xx, yy, ww, hh);
        if ((xx != x) || (yy != y) || (ww != w) || (hh != h) || (win.is_visible() != vis)) {
          recorder.Schedule(new CGitsVkUpdateNativeWindow(xlibCreateInfoPtr->window));
        }
      }
    }
#endif

    if (Config::Get()
            .recorder.vulkan.images.dumpScreenshots[gits::CGits::Instance().CurrentFrame()]) {
      writeScreenshot(queue, *pPresentInfo);
    }

    recorder.Schedule(new CvkQueuePresentKHR(return_value, queue, pPresentInfo));
  }
  vkQueuePresentKHR_SD(return_value, queue, pPresentInfo);
  if (GITSRecorderVulkan()->IsNextQueuePresentIgnored()) {
    GITSRecorderVulkan()->AcceptNextQueuePresentGITS();
  } else {
    recorder.FrameEnd();
  }
}

inline void ScheduleTokens(gits::Vulkan::CFunction* token) {
  gits::CRecorder::Instance().Schedule(token);
}

inline void vkQueueSubmit_RECWRAP(VkResult return_value,
                                  VkQueue queue,
                                  uint32_t submitCount,
                                  const VkSubmitInfo* pSubmits,
                                  VkFence fence,
                                  CRecorder& recorder) {
  std::unordered_set<VkDeviceMemory> _memoryToUpdate;

  if (updateOnlyUsedMemory()) {
    std::unordered_set<VkBuffer> updatedBuffers;
    std::unordered_set<VkImage> updatedImages;

    for (uint32_t i = 0; i < submitCount; i++) {
      for (uint32_t j = 0; j < pSubmits[i].commandBufferCount; j++) {
        // Process memory bound to buffers
        for (auto buffer : SD().bindingBuffers[pSubmits[i].pCommandBuffers[j]]) {
          if ((SD()._bufferstates.find(buffer) != SD()._bufferstates.end()) &&
              (updatedBuffers.find(buffer) == updatedBuffers.end())) {
            auto& bufferState = SD()._bufferstates[buffer];

            if ((bufferState->binding) &&
                (SD()._devicememorystates.find(
                     bufferState->binding->deviceMemoryStateStore->deviceMemoryHandle) !=
                 SD()._devicememorystates.end()) &&
                (bufferState->binding->deviceMemoryStateStore->IsMapped())) {
              _memoryToUpdate.insert(
                  bufferState->binding->deviceMemoryStateStore->deviceMemoryHandle);
            }
          }
          updatedBuffers.insert(buffer);
        }

        // Process memory bound to images
        for (auto image : SD().bindingImages[pSubmits[i].pCommandBuffers[j]]) {
          if ((SD()._imagestates.find(image) != SD()._imagestates.end()) &&
              (updatedImages.find(image) == updatedImages.end())) {
            auto& imageState = SD()._imagestates[image];

            if ((imageState->binding) &&
                (SD()._devicememorystates.find(
                     imageState->binding->deviceMemoryStateStore->deviceMemoryHandle) !=
                 SD()._devicememorystates.end()) &&
                (imageState->binding->deviceMemoryStateStore->IsMapped())) {
              _memoryToUpdate.insert(
                  imageState->binding->deviceMemoryStateStore->deviceMemoryHandle);
            }
          }
          updatedImages.insert(image);
        }
      }
    }

    for (auto& bufferState : CBufferState::shaderDeviceAddressBuffers) {
      if ((bufferState.second->binding != nullptr) &&
          (SD()._devicememorystates.find(
               bufferState.second->binding->deviceMemoryStateStore->deviceMemoryHandle) !=
           SD()._devicememorystates.end()) &&
          bufferState.second->binding->deviceMemoryStateStore->IsMapped()) {
        _memoryToUpdate.insert(
            bufferState.second->binding->deviceMemoryStateStore->deviceMemoryHandle);
      }
    }
  } else if (TMemoryUpdateStates::MEMORY_STATE_UPDATE_ALL_MAPPED ==
             Config::Get().recorder.vulkan.utilities.memoryUpdateState) {
    for (auto& memoryState : SD()._devicememorystates) {
      if (memoryState.second->IsMapped()) {
        _memoryToUpdate.insert(memoryState.first);
      }
    }
  }

  if (recorder.Running()) {
    if (Config::Get().recorder.vulkan.utilities.scheduleCommandBuffersBeforeQueueSubmit) {
      for (uint32_t i = 0; i < submitCount; i++) {
        for (uint32_t j = 0; j < pSubmits[i].commandBufferCount; j++) {
          auto& commandBuffState = SD()._commandbufferstates[pSubmits[i].pCommandBuffers[j]];

          if ((!commandBuffState->restored) && commandBuffState->beginCommandBuffer) {
            recorder.Schedule(new CvkBeginCommandBuffer(
                VK_SUCCESS, pSubmits[i].pCommandBuffers[j],
                commandBuffState->beginCommandBuffer->commandBufferBeginInfoData.Value()));
            commandBuffState->tokensBuffer.Flush(ScheduleTokens);

            if (commandBuffState->ended) {
              recorder.Schedule(
                  new CvkEndCommandBuffer(VK_SUCCESS, pSubmits[i].pCommandBuffers[j]));
            }
            commandBuffState->restored = true;
          }
        }
      }
    }

    if (Config::Get().recorder.vulkan.utilities.memoryUpdateState !=
        TMemoryUpdateStates::MEMORY_STATE_UPDATE_USING_TAGS) {
      for (auto memory : _memoryToUpdate) {
        std::vector<VkBufferCopy> updatedRanges;
        getRangesForMemoryUpdate(memory, updatedRanges, false);
        if (updatedRanges.size() > 0) {
          recorder.Schedule(
              new CGitsVkMemoryUpdate2(memory, updatedRanges.size(), updatedRanges.data()));
        }
      }
    }

    recorder.Schedule(new CvkQueueSubmit(return_value, queue, submitCount, pSubmits, fence));

    // Offscreen applications support
    if (!SD().internalResources.attachedToGITS && usePresentSrcLayoutTransitionAsAFrameBoundary()) {
      auto device = SD()._queuestates[queue]->deviceStateStore->deviceHandle;
      auto& offscreenApp = SD().internalResources.offscreenApps[device];

      bool continueLoop = true;
      if (offscreenApp.commandBufferWithTransitionToPresentSRC != VK_NULL_HANDLE) {
        for (uint32_t i = 0; (i < submitCount) && continueLoop; i++) {
          for (uint32_t j = 0; (j < pSubmits[i].commandBufferCount) && continueLoop; j++) {
            if (offscreenApp.commandBufferWithTransitionToPresentSRC ==
                pSubmits[i].pCommandBuffers[j]) {
              offscreenApp.commandBufferWithTransitionToPresentSRC = VK_NULL_HANDLE;

              scheduleCopyToSwapchainAndPresent(device, queue, recorder);

              continueLoop = false;
            }
          }
        }
      }
    }
  } else if (Config::Get().recorder.vulkan.utilities.shadowMemory) {
    for (auto memory : _memoryToUpdate) {
      flushShadowMemory(memory, false);
    }
  }

  SD().lastQueueSubmit =
      std::make_shared<CQueueSubmitState>(&submitCount, pSubmits, fence, SD()._queuestates[queue]);

  recorder.QueueSubmitEnd();
  vkQueueSubmit_SD(return_value, queue, submitCount, pSubmits, fence);
}

inline void vkQueueSubmit2_RECWRAP(VkResult return_value,
                                   VkQueue queue,
                                   uint32_t submitCount,
                                   const VkSubmitInfo2* pSubmits,
                                   VkFence fence,
                                   CRecorder& recorder) {
  std::unordered_set<VkDeviceMemory> _memoryToUpdate;

  if (updateOnlyUsedMemory()) {
    std::unordered_set<VkBuffer> updatedBuffers;
    std::unordered_set<VkImage> updatedImages;

    for (uint32_t i = 0; i < submitCount; i++) {
      for (uint32_t j = 0; j < pSubmits[i].commandBufferInfoCount; j++) {
        // Process memory bound to buffers
        for (auto buffer : SD().bindingBuffers[pSubmits[i].pCommandBufferInfos[j].commandBuffer]) {
          if ((SD()._bufferstates.find(buffer) != SD()._bufferstates.end()) &&
              (updatedBuffers.find(buffer) == updatedBuffers.end())) {
            auto& bufferState = SD()._bufferstates[buffer];

            if ((bufferState->binding) &&
                (SD()._devicememorystates.find(
                     bufferState->binding->deviceMemoryStateStore->deviceMemoryHandle) !=
                 SD()._devicememorystates.end()) &&
                (bufferState->binding->deviceMemoryStateStore->IsMapped())) {
              _memoryToUpdate.insert(
                  bufferState->binding->deviceMemoryStateStore->deviceMemoryHandle);
            }
          }
          updatedBuffers.insert(buffer);
        }

        // Process memory bound to images
        for (auto image : SD().bindingImages[pSubmits[i].pCommandBufferInfos[j].commandBuffer]) {
          if ((SD()._imagestates.find(image) != SD()._imagestates.end()) &&
              (updatedImages.find(image) == updatedImages.end())) {
            auto& imageState = SD()._imagestates[image];

            if ((imageState->binding) &&
                (SD()._devicememorystates.find(
                     imageState->binding->deviceMemoryStateStore->deviceMemoryHandle) !=
                 SD()._devicememorystates.end()) &&
                (imageState->binding->deviceMemoryStateStore->IsMapped())) {
              _memoryToUpdate.insert(
                  imageState->binding->deviceMemoryStateStore->deviceMemoryHandle);
            }
          }
          updatedImages.insert(image);
        }
      }
    }

    for (auto& bufferState : CBufferState::shaderDeviceAddressBuffers) {
      if ((bufferState.second->binding != nullptr) &&
          (SD()._devicememorystates.find(
               bufferState.second->binding->deviceMemoryStateStore->deviceMemoryHandle) !=
           SD()._devicememorystates.end()) &&
          bufferState.second->binding->deviceMemoryStateStore->IsMapped()) {
        _memoryToUpdate.insert(
            bufferState.second->binding->deviceMemoryStateStore->deviceMemoryHandle);
      }
    }
  } else if (TMemoryUpdateStates::MEMORY_STATE_UPDATE_ALL_MAPPED ==
             Config::Get().recorder.vulkan.utilities.memoryUpdateState) {
    for (auto& memoryState : SD()._devicememorystates) {
      if (memoryState.second->IsMapped()) {
        _memoryToUpdate.insert(memoryState.first);
      }
    }
  }

  if (recorder.Running()) {
    if (Config::Get().recorder.vulkan.utilities.scheduleCommandBuffersBeforeQueueSubmit) {
      for (uint32_t i = 0; i < submitCount; i++) {
        for (uint32_t j = 0; j < pSubmits[i].commandBufferInfoCount; j++) {
          auto& commandBuffState =
              SD()._commandbufferstates[pSubmits[i].pCommandBufferInfos[j].commandBuffer];

          if ((!commandBuffState->restored) && commandBuffState->beginCommandBuffer) {
            recorder.Schedule(new CvkBeginCommandBuffer(
                VK_SUCCESS, pSubmits[i].pCommandBufferInfos[j].commandBuffer,
                commandBuffState->beginCommandBuffer->commandBufferBeginInfoData.Value()));
            commandBuffState->tokensBuffer.Flush(ScheduleTokens);

            if (commandBuffState->ended) {
              recorder.Schedule(new CvkEndCommandBuffer(
                  VK_SUCCESS, pSubmits[i].pCommandBufferInfos[j].commandBuffer));
            }
            commandBuffState->restored = true;
          }
        }
      }
    }

    if (Config::Get().recorder.vulkan.utilities.memoryUpdateState !=
        TMemoryUpdateStates::MEMORY_STATE_UPDATE_USING_TAGS) {
      for (auto memory : _memoryToUpdate) {
        std::vector<VkBufferCopy> updatedRanges;
        getRangesForMemoryUpdate(memory, updatedRanges, false);
        if (updatedRanges.size() > 0) {
          recorder.Schedule(
              new CGitsVkMemoryUpdate2(memory, updatedRanges.size(), updatedRanges.data()));
        }
      }
    }

    recorder.Schedule(new CvkQueueSubmit2(return_value, queue, submitCount, pSubmits, fence));

    // Offscreen applications support
    if (!SD().internalResources.attachedToGITS && usePresentSrcLayoutTransitionAsAFrameBoundary()) {
      auto device = SD()._queuestates[queue]->deviceStateStore->deviceHandle;
      auto& offscreenApp = SD().internalResources.offscreenApps[device];

      bool continueLoop = true;
      if (offscreenApp.commandBufferWithTransitionToPresentSRC != VK_NULL_HANDLE) {
        for (uint32_t i = 0; (i < submitCount) && continueLoop; i++) {
          for (uint32_t j = 0; (j < pSubmits[i].commandBufferInfoCount) && continueLoop; j++) {
            if (offscreenApp.commandBufferWithTransitionToPresentSRC ==
                pSubmits[i].pCommandBufferInfos[j].commandBuffer) {
              offscreenApp.commandBufferWithTransitionToPresentSRC = VK_NULL_HANDLE;

              scheduleCopyToSwapchainAndPresent(device, queue, recorder);

              continueLoop = false;
            }
          }
        }
      }
    }
  } else if (Config::Get().recorder.vulkan.utilities.shadowMemory) {
    for (auto memory : _memoryToUpdate) {
      flushShadowMemory(memory, false);
    }
  }

  //SD().lastQueueSubmit = std::make_shared<CQueueSubmitState>(&submitCount, pSubmits, fence, SD()._queuestates[queue]); //TODO

  recorder.QueueSubmitEnd();
  vkQueueSubmit2_SD(return_value, queue, submitCount, pSubmits, fence);
}

inline void vkBeginCommandBuffer_RECWRAP(VkResult return_value,
                                         VkCommandBuffer commandBuffer,
                                         const VkCommandBufferBeginInfo* pBeginInfo,
                                         CRecorder& recorder) {
  if (recorder.Running() &&
      !Config::Get().recorder.vulkan.utilities.scheduleCommandBuffersBeforeQueueSubmit) {
    recorder.Schedule(new CvkBeginCommandBuffer(return_value, commandBuffer, pBeginInfo));
  }
  vkBeginCommandBuffer_SD(return_value, commandBuffer, pBeginInfo);
}

inline void vkEndCommandBuffer_RECWRAP(VkResult return_value,
                                       VkCommandBuffer commandBuffer,
                                       CRecorder& recorder) {
  if (recorder.Running() &&
      !Config::Get().recorder.vulkan.utilities.scheduleCommandBuffersBeforeQueueSubmit) {
    recorder.Schedule(new CvkEndCommandBuffer(return_value, commandBuffer));
  }
  vkEndCommandBuffer_SD(return_value, commandBuffer);
}

inline void vkDestroyDescriptorPool_RECWRAP(VkDevice device,
                                            VkDescriptorPool descriptorPool,
                                            const VkAllocationCallbacks* pAllocator,
                                            CRecorder& recorder) {
  if (recorder.Running()) {
    std::vector<VkDescriptorSet> descriptorSets;

    for (auto& descriptorSetState :
         SD()._descriptorpoolstates[descriptorPool]->descriptorSetStateStoreList) {
      descriptorSets.push_back(descriptorSetState->descriptorSetHandle);
    }

    if (descriptorSets.size() > 0) {
      recorder.Schedule(
          new CDestroyVulkanDescriptorSets((size_t)descriptorSets.size(), descriptorSets.data()));
    }

    recorder.Schedule(new CvkDestroyDescriptorPool(device, descriptorPool, pAllocator));
  }
  vkDestroyDescriptorPool_SD(device, descriptorPool, pAllocator);
}

inline void vkResetDescriptorPool_RECWRAP(VkResult return_value,
                                          VkDevice device,
                                          VkDescriptorPool descriptorPool,
                                          VkDescriptorPoolResetFlags flags,
                                          CRecorder& recorder) {
  if (recorder.Running()) {
    std::vector<VkDescriptorSet> descriptorSets;

    for (auto& descriptorSetState :
         SD()._descriptorpoolstates[descriptorPool]->descriptorSetStateStoreList) {
      descriptorSets.push_back(descriptorSetState->descriptorSetHandle);
    }

    if (descriptorSets.size() > 0) {
      recorder.Schedule(
          new CDestroyVulkanDescriptorSets((size_t)descriptorSets.size(), descriptorSets.data()));
    }

    recorder.Schedule(new CvkResetDescriptorPool(return_value, device, descriptorPool, flags));
  }
  vkResetDescriptorPool_SD(return_value, device, descriptorPool, flags);
}

inline void vkDestroyCommandPool_RECWRAP(VkDevice device,
                                         VkCommandPool commandPool,
                                         const VkAllocationCallbacks* pAllocator,
                                         CRecorder& recorder) {
  if (recorder.Running() && (commandPool != 0)) {
    std::vector<VkCommandBuffer> commandBuffers;

    for (auto& commandBufferState :
         SD()._commandpoolstates[commandPool]->commandBufferStateStoreList) {
      commandBuffers.push_back(commandBufferState->commandBufferHandle);
    }

    if (commandBuffers.size() > 0) {
      recorder.Schedule(
          new CDestroyVulkanCommandBuffers((size_t)commandBuffers.size(), commandBuffers.data()));
    }

    recorder.Schedule(new CvkDestroyCommandPool(device, commandPool, pAllocator));
  }
  vkDestroyCommandPool_SD(device, commandPool, pAllocator);
}

inline void vkUnmapMemory_RECWRAP(VkDevice device, VkDeviceMemory memory, CRecorder& recorder) {
  if (recorder.Running()) {
    if (Config::Get().recorder.vulkan.utilities.memoryUpdateState !=
        TMemoryUpdateStates::MEMORY_STATE_UPDATE_USING_TAGS) {
      std::vector<VkBufferCopy> updatedRanges;
      getRangesForMemoryUpdate(memory, updatedRanges, true);
      if (updatedRanges.size() > 0) {
        recorder.Schedule(
            new CGitsVkMemoryUpdate2(memory, updatedRanges.size(), updatedRanges.data()));
      }
    }
    recorder.Schedule(new CvkUnmapMemory(device, memory));
  } else if (Config::Get().recorder.vulkan.utilities.shadowMemory) {
    flushShadowMemory(memory, true);
  }
  vkUnmapMemory_SD(device, memory);
}

inline void vkPassPhysicalDeviceMemoryPropertiesGITS_RECWRAP(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceMemoryProperties* pMemoryProperties,
    CRecorder& recorder) {
  // Here we pass memory properties recorded on original platform to the recorder of a (sub)stream
  // Token is scheduled only once through vkCreateDevice_RECWRAP() function or in state restore
  vkPassPhysicalDeviceMemoryPropertiesGITS_SD(physicalDevice, pMemoryProperties);
}

inline void vkCreateDevice_RECWRAP(VkResult return_value,
                                   VkPhysicalDevice physicalDevice,
                                   const VkDeviceCreateInfo* pCreateInfo,
                                   const VkAllocationCallbacks* pAllocator,
                                   VkDevice* pDevice,
                                   CRecorder& recorder) {
  if (recorder.Running()) {
    auto& physicalDeviceState = SD()._physicaldevicestates[physicalDevice];
    if (physicalDeviceState->memoryProperties.memoryHeapCount == 0) {
      VkPhysicalDeviceMemoryProperties memoryProperties;
      drvVk.vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
      vkPassPhysicalDeviceMemoryPropertiesGITS_SD(physicalDevice, &memoryProperties);
    }
    recorder.Schedule(new CvkPassPhysicalDeviceMemoryPropertiesGITS(
        physicalDevice, &physicalDeviceState->memoryProperties));
    recorder.Schedule(
        new CvkCreateDevice(return_value, physicalDevice, pCreateInfo, pAllocator, pDevice));
  }
  vkCreateDevice_SD(return_value, physicalDevice, pCreateInfo, pAllocator, pDevice);
}

inline void vkCreateBuffer_RECWRAP(VkResult return_value,
                                   VkDevice device,
                                   const VkBufferCreateInfo* pCreateInfo,
                                   const VkAllocationCallbacks* pAllocator,
                                   VkBuffer* pBuffer,
                                   CRecorder& recorder) {
  if (recorder.Running()) {
    auto bufferCreateInfo = *pCreateInfo;

    // Core 1.2 or KHR version
    VkBufferOpaqueCaptureAddressCreateInfo opaqueAddressCreateInfo = {
        VK_STRUCTURE_TYPE_BUFFER_OPAQUE_CAPTURE_ADDRESS_CREATE_INFO, // VkStructureType    sType;
        bufferCreateInfo.pNext,                                      // const void       * pNext;
        0 // uint64_t           opaqueCaptureAddress;
    };

    // EXT version
    VkBufferDeviceAddressCreateInfoEXT deviceAddressCreateInfo = {
        VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_CREATE_INFO_EXT, // VkStructureType    sType;
        bufferCreateInfo.pNext,                                  // const void       * pNext;
        0 // VkDeviceAddress    deviceAddress;
    };

    // Record opaque/capture device address only when requested
    if ((Config::Get()
             .recorder.vulkan.utilities
             .useCaptureReplayFeaturesForBuffersAndAccelerationStructures) &&
        ((pCreateInfo->flags & VK_BUFFER_CREATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT) ==
         VK_BUFFER_CREATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT)) {
      VkBufferDeviceAddressInfo addressInfo = {
          VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, // VkStructureType    sType;
          nullptr,                                      // const void       * pNext;
          *pBuffer                                      // VkBuffer           buffer;
      };

      // Core 1.2 or KHR version
      if (drvVk.GetDeviceDispatchTable(device).vkGetBufferOpaqueCaptureAddressUnifiedGITS) {
        opaqueAddressCreateInfo.opaqueCaptureAddress =
            drvVk.vkGetBufferOpaqueCaptureAddressUnifiedGITS(device, &addressInfo);
        bufferCreateInfo.pNext = &opaqueAddressCreateInfo;
      }
      // EXT version
      else {
        deviceAddressCreateInfo.deviceAddress =
            drvVk.vkGetBufferDeviceAddressUnifiedGITS(device, &addressInfo);
        bufferCreateInfo.pNext = &deviceAddressCreateInfo;
      }
    }
    recorder.Schedule(
        new CvkCreateBuffer(return_value, device, &bufferCreateInfo, pAllocator, pBuffer));
  }
  vkCreateBuffer_SD(return_value, device, pCreateInfo, pAllocator, pBuffer);
}

inline void vkGetBufferDeviceAddressUnifiedGITS_RECWRAP(VkDeviceAddress return_value,
                                                        VkDevice device,
                                                        const VkBufferDeviceAddressInfo* pInfo,
                                                        CRecorder& recorder) {
  if (recorder.Running()) {
    recorder.Schedule(new CvkGetBufferDeviceAddressUnifiedGITS(return_value, device, pInfo));
  }
  vkGetBufferDeviceAddressUnifiedGITS_SD(return_value, device, pInfo);
}

inline void vkGetBufferDeviceAddress_RECWRAP(VkDeviceAddress return_value,
                                             VkDevice device,
                                             const VkBufferDeviceAddressInfo* pInfo,
                                             CRecorder& recorder) {
  vkGetBufferDeviceAddressUnifiedGITS_RECWRAP(return_value, device, pInfo, recorder);
}

inline void vkGetBufferDeviceAddressEXT_RECWRAP(VkDeviceAddress return_value,
                                                VkDevice device,
                                                const VkBufferDeviceAddressInfo* pInfo,
                                                CRecorder& recorder) {
  vkGetBufferDeviceAddressUnifiedGITS_RECWRAP(return_value, device, pInfo, recorder);
}

inline void vkGetBufferDeviceAddressKHR_RECWRAP(VkDeviceAddress return_value,
                                                VkDevice device,
                                                const VkBufferDeviceAddressInfo* pInfo,
                                                CRecorder& recorder) {
  vkGetBufferDeviceAddressUnifiedGITS_RECWRAP(return_value, device, pInfo, recorder);
}

inline void vkTagMemoryContentsUpdateGITS_RECWRAP(VkDevice /* device */,
                                                  VkDeviceMemory memory,
                                                  uint32_t regionCount,
                                                  const VkBufferCopy* pRegions,
                                                  CRecorder& recorder) {
  if (recorder.Running() && (Config::Get().recorder.vulkan.utilities.memoryUpdateState ==
                             TMemoryUpdateStates::MEMORY_STATE_UPDATE_USING_TAGS)) {
    recorder.Schedule(new CGitsVkMemoryUpdate2(memory, regionCount, pRegions));
  }
}

inline void vkAllocateMemory_RECWRAP(VkResult return_value,
                                     VkDevice device,
                                     const VkMemoryAllocateInfo* pAllocateInfo,
                                     const VkAllocationCallbacks* pAllocator,
                                     VkDeviceMemory* pMemory,
                                     CRecorder& recorder) {
  if (recorder.Running()) {
    recorder.Schedule(
        new CvkAllocateMemory(return_value, device, pAllocateInfo, pAllocator, pMemory));
  }
  // vkAllocateMemory_SD() function is called inside recExecWrap_vkAllocateMemory() function
}

namespace {

// Offscreen applications support
inline void barriers2HelperForOffscreenApplications(VkCommandBuffer commandBuffer,
                                                    const VkDependencyInfo* pDependencyInfo,
                                                    CRecorder& recorder) {

  if (usePresentSrcLayoutTransitionAsAFrameBoundary()) {
    auto device = SD()._commandbufferstates[commandBuffer]
                      ->commandPoolStateStore->deviceStateStore->deviceHandle;

    for (unsigned int i = 0; i < pDependencyInfo->imageMemoryBarrierCount; i++) {
      if (pDependencyInfo->pImageMemoryBarriers[i].newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
        {
          auto pImageCreateInfo = SD()._imagestates[pDependencyInfo->pImageMemoryBarriers[i].image]
                                      ->imageCreateInfoData.Value();
          handleSwapchainCreationForOffscreenApplications(device, pImageCreateInfo, recorder);
        }
        {
          auto& offscreenApp = SD().internalResources.offscreenApps[device];
          offscreenApp.commandBufferWithTransitionToPresentSRC = commandBuffer;
          offscreenApp.imageToPresent = pDependencyInfo->pImageMemoryBarriers[i].image;
        }
        break;
      }
    }
  }
}

} // namespace

inline void vkCmdPipelineBarrier_RECWRAP(VkCommandBuffer commandBuffer,
                                         VkPipelineStageFlags srcStageMask,
                                         VkPipelineStageFlags dstStageMask,
                                         VkDependencyFlags dependencyFlags,
                                         uint32_t memoryBarrierCount,
                                         const VkMemoryBarrier* pMemoryBarriers,
                                         uint32_t bufferMemoryBarrierCount,
                                         const VkBufferMemoryBarrier* pBufferMemoryBarriers,
                                         uint32_t imageMemoryBarrierCount,
                                         const VkImageMemoryBarrier* pImageMemoryBarriers,
                                         CRecorder& recorder) {
  if (recorder.Running() &&
      !Config::Get().recorder.vulkan.utilities.scheduleCommandBuffersBeforeQueueSubmit) {
    recorder.Schedule(new CvkCmdPipelineBarrier(
        commandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount,
        pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount,
        pImageMemoryBarriers));

    // Offscreen applications support
    if (usePresentSrcLayoutTransitionAsAFrameBoundary()) {
      auto device = SD()._commandbufferstates[commandBuffer]
                        ->commandPoolStateStore->deviceStateStore->deviceHandle;

      for (unsigned int i = 0; i < imageMemoryBarrierCount; i++) {
        if (pImageMemoryBarriers[i].newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
          {
            auto pImageCreateInfo =
                SD()._imagestates[pImageMemoryBarriers[i].image]->imageCreateInfoData.Value();
            handleSwapchainCreationForOffscreenApplications(device, pImageCreateInfo, recorder);
          }
          {
            auto& offscreenApp = SD().internalResources.offscreenApps[device];
            offscreenApp.commandBufferWithTransitionToPresentSRC = commandBuffer;
            offscreenApp.imageToPresent = pImageMemoryBarriers[i].image;
          }
          break;
        }
      }
    }
  } else {
    SD()._commandbufferstates[commandBuffer]->tokensBuffer.Add(new CvkCmdPipelineBarrier(
        commandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount,
        pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount,
        pImageMemoryBarriers));
  }
  vkCmdPipelineBarrier_SD(commandBuffer, srcStageMask, dstStageMask, dependencyFlags,
                          memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount,
                          pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
}

inline void vkCmdPipelineBarrier2UnifiedGITS_RECWRAP(VkCommandBuffer commandBuffer,
                                                     const VkDependencyInfo* pDependencyInfo,
                                                     CRecorder& recorder) {
  if (recorder.Running() &&
      !Config::Get().recorder.vulkan.utilities.scheduleCommandBuffersBeforeQueueSubmit) {
    recorder.Schedule(new CvkCmdPipelineBarrier2UnifiedGITS(commandBuffer, pDependencyInfo));

    barriers2HelperForOffscreenApplications(commandBuffer, pDependencyInfo, recorder);
  } else {
    SD()._commandbufferstates[commandBuffer]->tokensBuffer.Add(
        new CvkCmdPipelineBarrier2UnifiedGITS(commandBuffer, pDependencyInfo));
  }
  vkCmdPipelineBarrier2UnifiedGITS_SD(commandBuffer, pDependencyInfo);
}

inline void vkCmdPipelineBarrier2_RECWRAP(VkCommandBuffer commandBuffer,
                                          const VkDependencyInfo* pDependencyInfo,
                                          CRecorder& recorder) {
  if (recorder.Running() &&
      !Config::Get().recorder.vulkan.utilities.scheduleCommandBuffersBeforeQueueSubmit) {
    recorder.Schedule(new CvkCmdPipelineBarrier2(commandBuffer, pDependencyInfo));

    barriers2HelperForOffscreenApplications(commandBuffer, pDependencyInfo, recorder);
  } else {
    SD()._commandbufferstates[commandBuffer]->tokensBuffer.Add(
        new CvkCmdPipelineBarrier2(commandBuffer, pDependencyInfo));
  }
  vkCmdPipelineBarrier2_SD(commandBuffer, pDependencyInfo);
}

inline void vkCmdPipelineBarrier2KHR_RECWRAP(VkCommandBuffer commandBuffer,
                                             const VkDependencyInfo* pDependencyInfo,
                                             CRecorder& recorder) {
  if (recorder.Running() &&
      !Config::Get().recorder.vulkan.utilities.scheduleCommandBuffersBeforeQueueSubmit) {
    recorder.Schedule(new CvkCmdPipelineBarrier2KHR(commandBuffer, pDependencyInfo));

    barriers2HelperForOffscreenApplications(commandBuffer, pDependencyInfo, recorder);
  } else {
    SD()._commandbufferstates[commandBuffer]->tokensBuffer.Add(
        new CvkCmdPipelineBarrier2KHR(commandBuffer, pDependencyInfo));
  }
  vkCmdPipelineBarrier2KHR_SD(commandBuffer, pDependencyInfo);
}

inline void vkCmdSetEvent2_RECWRAP(VkCommandBuffer commandBuffer,
                                   VkEvent event,
                                   const VkDependencyInfo* pDependencyInfo,
                                   CRecorder& recorder) {
  if (recorder.Running() &&
      !Config::Get().recorder.vulkan.utilities.scheduleCommandBuffersBeforeQueueSubmit) {
    recorder.Schedule(new CvkCmdSetEvent2(commandBuffer, event, pDependencyInfo));

    barriers2HelperForOffscreenApplications(commandBuffer, pDependencyInfo, recorder);
  } else {
    SD()._commandbufferstates[commandBuffer]->tokensBuffer.Add(
        new CvkCmdSetEvent2(commandBuffer, event, pDependencyInfo));
  }
  vkCmdSetEvent2_SD(commandBuffer, event, pDependencyInfo);
}

inline void vkCmdSetEvent2KHR_RECWRAP(VkCommandBuffer commandBuffer,
                                      VkEvent event,
                                      const VkDependencyInfo* pDependencyInfo,
                                      CRecorder& recorder) {
  if (recorder.Running() &&
      !Config::Get().recorder.vulkan.utilities.scheduleCommandBuffersBeforeQueueSubmit) {
    recorder.Schedule(new CvkCmdSetEvent2KHR(commandBuffer, event, pDependencyInfo));

    barriers2HelperForOffscreenApplications(commandBuffer, pDependencyInfo, recorder);
  } else {
    SD()._commandbufferstates[commandBuffer]->tokensBuffer.Add(
        new CvkCmdSetEvent2KHR(commandBuffer, event, pDependencyInfo));
  }
  vkCmdSetEvent2KHR_SD(commandBuffer, event, pDependencyInfo);
}

} // namespace Vulkan
} // namespace gits

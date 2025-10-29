// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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
#include "vulkanPreToken.h"
#include "vulkanFunctions.h"
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

  VkCommandBuffer cmdBuf;
  if (drvVk.vkAllocateCommandBuffers(device, &allocateInfo, &cmdBuf) != VK_SUCCESS) {
    throw std::runtime_error("Could not allocate a command buffer.");
  }
  vkAllocateCommandBuffers_SD(VK_SUCCESS, device, &allocateInfo, &cmdBuf);

  recorder.Schedule(new CvkAllocateCommandBuffers(VK_SUCCESS, device, &allocateInfo, &cmdBuf));
  return cmdBuf;
}

void handleSwapchainCreationForOffscreenApplications(VkDevice device,
                                                     const VkImageCreateInfo* pCreateInfo,
                                                     CRecorder& recorder) {
  auto& vs = SD().internalResources.virtualSwapchain[device];
  auto& offscreenApp = SD().internalResources.offscreenApps[device];

  static bool init = [&]() {
    CWindowParameters windowParameters = {(HINSTANCE)offscreenApp.uniqueHandleCounter++,
                                          (HWND)offscreenApp.uniqueHandleCounter++,
                                          {pCreateInfo->extent.width, pCreateInfo->extent.height}};

    auto instance =
        SD()._devicestates[device]->physicalDeviceStateStore->instanceStateStore->instanceHandle;

    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {
        VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR, // VkStructureType sType;
        nullptr,                                         // const void* pNext;
        0,                                               // VkWin32SurfaceCreateFlagsKHR flags;
        windowParameters.hInstance,                      // HINSTANCE hinstance;
        windowParameters.hWnd                            // HWND hwnd;
    };

    offscreenApp.surface = (VkSurfaceKHR)offscreenApp.uniqueHandleCounter++;

    recorder.Schedule(new CGitsVkCreateNativeWindow(
        surfaceCreateInfo.hinstance, surfaceCreateInfo.hwnd, windowParameters.extent.width,
        windowParameters.extent.height));
    recorder.Schedule(new CvkCreateWin32SurfaceKHR(VK_SUCCESS, instance, &surfaceCreateInfo,
                                                   nullptr, &offscreenApp.surface));
    recorder.Schedule(new CGitsVkEnumerateDisplayMonitors(true));

    vkCreateWin32SurfaceKHR_SD(VK_SUCCESS, instance, &surfaceCreateInfo, nullptr,
                               &offscreenApp.surface);

    offscreenApp.hwnd = windowParameters.hWnd;
    vs.extent = windowParameters.extent;

    // One-time initialization of resources that won't change
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
  }();

  // Image size has not changed and swapchain already created
  if (vs.extent.width == pCreateInfo->extent.width &&
      vs.extent.height == pCreateInfo->extent.height && vs.swapchain) {
    return;
  }

  // Update window when image size chamged
  if (vs.extent.width != pCreateInfo->extent.width ||
      vs.extent.height != pCreateInfo->extent.height) {
    vs.extent = {pCreateInfo->extent.width, pCreateInfo->extent.height};
    recorder.Schedule(
        new CGitsVkUpdateNativeWindow(offscreenApp.hwnd, vs.extent.width, vs.extent.height));
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

    recorder.Schedule(new CvkAcquireNextImageKHR(
        VK_SUCCESS, device, vs.swapchain, 9000000000,
        vs.presentationData[vs.nextImage].imageAcquiredSemaphore, VK_NULL_HANDLE, &vs.nextImage));
  }
}

void scheduleCopyToSwapchainAndPresent(VkDevice device, VkQueue queue, CRecorder& recorder) {
  auto& vs = SD().internalResources.virtualSwapchain[device];
  auto& offscreenApp = SD().internalResources.offscreenApps[device];
  auto& presentationData = vs.presentationData[vs.nextImage];

  recorder.Schedule(
      new CvkWaitForFences(VK_SUCCESS, device, 1, &presentationData.fence, VK_FALSE, 9000000000));
  recorder.Schedule(new CvkResetFences(VK_SUCCESS, device, 1, &presentationData.fence));

  // Copy data to a virtual swapchain
  {
    const auto cmdBuf = presentationData.commandBuffer;
    const auto srcImage = offscreenApp.imageToPresent;
    const auto& srcImageState = SD()._imagestates[srcImage];
    const auto dstImage = presentationData.swapchainImage;

    // Begin a command buffer for performing copy
    {
      VkCommandBufferBeginInfo beginInfo = {
          VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, // VkStructureType sType;
          nullptr,                                     // const void* pNext;
          VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, // VkCommandBufferUsageFlags flags;
          nullptr // const VkCommandBufferInheritanceInfo* pInheritanceInfo;
      };
      recorder.Schedule(new CvkBeginCommandBuffer(VK_SUCCESS, cmdBuf, &beginInfo));
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
          cmdBuf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
          VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr,
          (uint32_t)preTransferImageMemoryBarriers.size(), preTransferImageMemoryBarriers.data()));
    }

    // Data copy
    {
      if (srcImageState->imageCreateInfoData.Value()->samples == VK_SAMPLE_COUNT_1_BIT) {
        VkImageCopy imageCopy = {{
                                     // VkImageSubresourceLayers srcSubresource;
                                     VK_IMAGE_ASPECT_COLOR_BIT, // VkImageAspectFlags aspectMask;
                                     0,                         // uint32_t mipLevel;
                                     0,                         // uint32_t baseArrayLayer;
                                     1                          // uint32_t layerCount;
                                 },
                                 {0, 0, 0}, // VkOffset3D srcOffset;
                                 {
                                     // VkImageSubresourceLayers dstSubresource;
                                     VK_IMAGE_ASPECT_COLOR_BIT, // VkImageAspectFlags aspectMask;
                                     0,                         // uint32_t mipLevel;
                                     0,                         // uint32_t baseArrayLayer;
                                     1                          // uint32_t layerCount;
                                 },
                                 {0, 0, 0}, // VkOffset3D dstOffset;
                                 {static_cast<uint32_t>(srcImageState->width),
                                  static_cast<uint32_t>(srcImageState->height), 1}};
        recorder.Schedule(new CvkCmdCopyImage(cmdBuf, srcImage,
                                              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstImage,
                                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy));
      } else {
        VkImageResolve region = {
            {
                // VkImageSubresourceLayers srcSubresource;
                VK_IMAGE_ASPECT_COLOR_BIT, // VkImageAspectFlags aspectMask;
                0,                         // uint32_t mipLevel;
                0,                         // uint32_t baseArrayLayer;
                1                          // uint32_t layerCount;
            },
            {0, 0, 0}, //VkOffset3D srcOffset;
            {
                // VkImageSubresourceLayers dstSubresource;
                VK_IMAGE_ASPECT_COLOR_BIT, // VkImageAspectFlags aspectMask;
                0,                         // uint32_t mipLevel;
                0,                         // uint32_t baseArrayLayer;
                1                          // uint32_t layerCount;
            },
            {0, 0, 0}, //VkOffset3D dstOffset;
            {static_cast<uint32_t>(srcImageState->width),
             static_cast<uint32_t>(srcImageState->height), 1} //VkExtent3D extent;
        };
        recorder.Schedule(new CvkCmdResolveImage(cmdBuf, srcImage,
                                                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstImage,
                                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region));
      }
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
          cmdBuf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
          VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr,
          (uint32_t)postTransferImageMemoryBarriers.size(),
          postTransferImageMemoryBarriers.data()));
    }

    // End the command buffer
    recorder.Schedule(new CvkEndCommandBuffer(VK_SUCCESS, cmdBuf));

    // Submit
    {
      VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
      VkSubmitInfo submitInfo = {
          VK_STRUCTURE_TYPE_SUBMIT_INFO,            // VkStructureType sType;
          nullptr,                                  // const void* pNext;
          1,                                        // uint32_t waitSemaphoreCount;
          &presentationData.imageAcquiredSemaphore, // const VkSemaphore* pWaitSemaphores;
          &waitStages, // const VkPipelineStageFlags* pWaitDstStageMask;
          1,           // uint32_t commandBufferCount;
          &cmdBuf,     // const VkCommandBuffer* pCommandBuffers;
          1,           // uint32_t signalSemaphoreCount;
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

void vkCreateInstance_RECWRAP(VkResult return_value,
                              const VkInstanceCreateInfo* pCreateInfo,
                              const VkAllocationCallbacks* pAllocator,
                              VkInstance* pInstance,
                              CRecorder& recorder) {
  vkCreateInstance_SD(return_value, pCreateInfo, pAllocator, pInstance);

  if (recorder.Running()) {
    recorder.Schedule(new CvkCreateInstance(return_value, pCreateInfo, pAllocator, pInstance));
  }
}

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
    if (!Configurator::Get().vulkan.recorder.usePresentSrcLayoutTransitionAsAFrameBoundary) {
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
          recorder.Schedule(new CGitsVkUpdateNativeWindow(
              surfaceState->surfaceCreateInfoWin32Data.Value()->hwnd));
        }
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

    if (Configurator::Get()
            .vulkan.recorder.dumpScreenshots[gits::CGits::Instance().CurrentFrame()]) {
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
        const auto cmdBuf = pSubmits[i].pCommandBuffers[j];

        // Process memory bound to buffers
        for (auto& bufferState : SD().bindingBuffers[cmdBuf]) {
          if (updatedBuffers.find(bufferState->bufferHandle) == updatedBuffers.end()) {
            auto& bufferBinding = bufferState->binding;

            if (bufferBinding &&
                (SD()._devicememorystates.find(
                     bufferBinding->deviceMemoryStateStore->deviceMemoryHandle) !=
                 SD()._devicememorystates.end()) &&
                bufferBinding->deviceMemoryStateStore->IsMapped()) {
              _memoryToUpdate.insert(bufferBinding->deviceMemoryStateStore->deviceMemoryHandle);
            }
            updatedBuffers.insert(bufferState->bufferHandle);
          }
        }

        // Process memory bound to images
        for (auto& imageState : SD().bindingImages[cmdBuf]) {
          if (updatedImages.find(imageState->imageHandle) == updatedImages.end()) {
            auto& imageBinding = imageState->binding;

            if (imageBinding &&
                (SD()._devicememorystates.find(
                     imageBinding->deviceMemoryStateStore->deviceMemoryHandle) !=
                 SD()._devicememorystates.end()) &&
                imageBinding->deviceMemoryStateStore->IsMapped()) {
              _memoryToUpdate.insert(imageBinding->deviceMemoryStateStore->deviceMemoryHandle);
            }
            updatedImages.insert(imageState->imageHandle);
          }
        }
      }
    }

    // BUFFER DEVICE ADDRESS GROUP COMMENT TOKEN
    // Please, (un)comment all the areas with the above token together, at the same time
    //
    // for (auto& bufferState : CBufferState::shaderDeviceAddressBuffers) {
    //   if ((bufferState.second->binding != nullptr) &&
    //       (SD()._devicememorystates.find(
    //            bufferState.second->binding->deviceMemoryStateStore->deviceMemoryHandle) !=
    //        SD()._devicememorystates.end()) &&
    //       bufferState.second->binding->deviceMemoryStateStore->IsMapped()) {
    //     _memoryToUpdate.insert(
    //         bufferState.second->binding->deviceMemoryStateStore->deviceMemoryHandle);
    //   }
    // }
  } else if (TMemoryUpdateStates::ALL_MAPPED ==
             Configurator::Get().vulkan.recorder.memoryUpdateState) {
    for (auto& memoryState : SD()._devicememorystates) {
      if (memoryState.second->IsMapped()) {
        _memoryToUpdate.insert(memoryState.first);
      }
    }
  }

  if (recorder.Running()) {
    if (Configurator::Get().vulkan.recorder.scheduleCommandBuffersBeforeQueueSubmit) {
      for (uint32_t i = 0; i < submitCount; i++) {
        for (uint32_t j = 0; j < pSubmits[i].commandBufferCount; j++) {
          const auto cmdBuf = pSubmits[i].pCommandBuffers[j];
          auto& commandBuffState = SD()._commandbufferstates[cmdBuf];

          if ((!commandBuffState->restored) && commandBuffState->beginCommandBuffer) {
            recorder.Schedule(new CvkBeginCommandBuffer(
                VK_SUCCESS, cmdBuf,
                commandBuffState->beginCommandBuffer->commandBufferBeginInfoData.Value()));
            commandBuffState->tokensBuffer.Flush(ScheduleTokens);

            if (commandBuffState->ended) {
              recorder.Schedule(new CvkEndCommandBuffer(VK_SUCCESS, cmdBuf));
            }
            commandBuffState->restored = true;
          }
        }
      }
    }

    if (Configurator::Get().vulkan.recorder.memoryUpdateState != TMemoryUpdateStates::USING_TAGS) {
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
  } else if (Configurator::Get().vulkan.recorder.shadowMemory) {
    for (auto memory : _memoryToUpdate) {
      flushShadowMemory(memory, false);
    }
  }

#ifdef GITS_PLATFORM_WINDOWS
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
            if (recorder.Running()) {
              scheduleCopyToSwapchainAndPresent(device, queue, recorder);
            } else {
              recorder.FrameEnd();
            }
            continueLoop = false;
          }
        }
      }
    }
  }
#endif

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
                                   CRecorder& recorder,
                                   bool isKHR = false) {
  std::unordered_set<VkDeviceMemory> _memoryToUpdate;

  if (updateOnlyUsedMemory()) {
    std::unordered_set<VkBuffer> updatedBuffers;
    std::unordered_set<VkImage> updatedImages;

    for (uint32_t i = 0; i < submitCount; i++) {
      for (uint32_t j = 0; j < pSubmits[i].commandBufferInfoCount; j++) {
        const auto cmdBuf = pSubmits[i].pCommandBufferInfos[j].commandBuffer;

        // Process memory bound to buffers
        for (auto& bufferState : SD().bindingBuffers[cmdBuf]) {
          if (updatedBuffers.find(bufferState->bufferHandle) == updatedBuffers.end()) {
            auto& bufferBinding = bufferState->binding;

            if (bufferBinding &&
                (SD()._devicememorystates.find(
                     bufferBinding->deviceMemoryStateStore->deviceMemoryHandle) !=
                 SD()._devicememorystates.end()) &&
                bufferBinding->deviceMemoryStateStore->IsMapped()) {
              _memoryToUpdate.insert(bufferBinding->deviceMemoryStateStore->deviceMemoryHandle);
            }
            updatedBuffers.insert(bufferState->bufferHandle);
          }
        }

        // Process memory bound to images
        for (auto& imageState : SD().bindingImages[cmdBuf]) {
          if (updatedImages.find(imageState->imageHandle) == updatedImages.end()) {
            auto& imageBinding = imageState->binding;

            if (imageBinding &&
                (SD()._devicememorystates.find(
                     imageBinding->deviceMemoryStateStore->deviceMemoryHandle) !=
                 SD()._devicememorystates.end()) &&
                imageBinding->deviceMemoryStateStore->IsMapped()) {
              _memoryToUpdate.insert(imageBinding->deviceMemoryStateStore->deviceMemoryHandle);
            }
            updatedImages.insert(imageState->imageHandle);
          }
        }
      }
    }

    // BUFFER DEVICE ADDRESS GROUP COMMENT TOKEN
    // Please, (un)comment all the areas with the above token together, at the same time
    //
    // for (auto& bufferState : CBufferState::shaderDeviceAddressBuffers) {
    //   if ((bufferState.second->binding != nullptr) &&
    //       (SD()._devicememorystates.find(
    //            bufferState.second->binding->deviceMemoryStateStore->deviceMemoryHandle) !=
    //        SD()._devicememorystates.end()) &&
    //       bufferState.second->binding->deviceMemoryStateStore->IsMapped()) {
    //     _memoryToUpdate.insert(
    //         bufferState.second->binding->deviceMemoryStateStore->deviceMemoryHandle);
    //   }
    // }
  } else if (TMemoryUpdateStates::ALL_MAPPED ==
             Configurator::Get().vulkan.recorder.memoryUpdateState) {
    for (auto& memoryState : SD()._devicememorystates) {
      if (memoryState.second->IsMapped()) {
        _memoryToUpdate.insert(memoryState.first);
      }
    }
  }

  if (recorder.Running()) {
    if (Configurator::Get().vulkan.recorder.scheduleCommandBuffersBeforeQueueSubmit) {
      for (uint32_t i = 0; i < submitCount; i++) {
        for (uint32_t j = 0; j < pSubmits[i].commandBufferInfoCount; j++) {
          const auto cmdBuf = pSubmits[i].pCommandBufferInfos[j].commandBuffer;
          auto& cmdBufState = SD()._commandbufferstates[cmdBuf];

          if ((!cmdBufState->restored) && cmdBufState->beginCommandBuffer) {
            recorder.Schedule(new CvkBeginCommandBuffer(
                VK_SUCCESS, cmdBuf,
                cmdBufState->beginCommandBuffer->commandBufferBeginInfoData.Value()));
            cmdBufState->tokensBuffer.Flush(ScheduleTokens);

            if (cmdBufState->ended) {
              recorder.Schedule(new CvkEndCommandBuffer(VK_SUCCESS, cmdBuf));
            }
            cmdBufState->restored = true;
          }
        }
      }
    }

    if (Configurator::Get().vulkan.recorder.memoryUpdateState != TMemoryUpdateStates::USING_TAGS) {
      for (auto memory : _memoryToUpdate) {
        std::vector<VkBufferCopy> updatedRanges;
        getRangesForMemoryUpdate(memory, updatedRanges, false);
        if (updatedRanges.size() > 0) {
          recorder.Schedule(
              new CGitsVkMemoryUpdate2(memory, updatedRanges.size(), updatedRanges.data()));
        }
      }
    }

    if (isKHR) {
      recorder.Schedule(new CvkQueueSubmit2KHR(return_value, queue, submitCount, pSubmits, fence));
    } else {
      recorder.Schedule(new CvkQueueSubmit2(return_value, queue, submitCount, pSubmits, fence));
    }
  } else if (Configurator::Get().vulkan.recorder.shadowMemory) {
    for (auto memory : _memoryToUpdate) {
      flushShadowMemory(memory, false);
    }
  }

#ifdef GITS_PLATFORM_WINDOWS
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
            if (recorder.Running()) {
              scheduleCopyToSwapchainAndPresent(device, queue, recorder);
            } else {
              recorder.FrameEnd();
            }
            continueLoop = false;
          }
        }
      }
    }
  }
#endif

  SD().lastQueueSubmit =
      std::make_shared<CQueueSubmitState>(&submitCount, pSubmits, fence, SD()._queuestates[queue]);

  recorder.QueueSubmitEnd();
  vkQueueSubmit2_SD(return_value, queue, submitCount, pSubmits, fence);
}

inline void vkQueueSubmit2KHR_RECWRAP(VkResult return_value,
                                      VkQueue queue,
                                      uint32_t submitCount,
                                      const VkSubmitInfo2* pSubmits,
                                      VkFence fence,
                                      CRecorder& recorder) {
  vkQueueSubmit2_RECWRAP(return_value, queue, submitCount, pSubmits, fence, recorder, true);
}

inline void vkBeginCommandBuffer_RECWRAP(VkResult return_value,
                                         VkCommandBuffer cmdBuf,
                                         const VkCommandBufferBeginInfo* pBeginInfo,
                                         CRecorder& recorder) {
  if (recorder.Running() &&
      !Configurator::Get().vulkan.recorder.scheduleCommandBuffersBeforeQueueSubmit) {
    recorder.Schedule(new CvkBeginCommandBuffer(return_value, cmdBuf, pBeginInfo));
  }
  vkBeginCommandBuffer_SD(return_value, cmdBuf, pBeginInfo);
}

inline void vkEndCommandBuffer_RECWRAP(VkResult return_value,
                                       VkCommandBuffer cmdBuf,
                                       CRecorder& recorder) {
  if (recorder.Running() &&
      !Configurator::Get().vulkan.recorder.scheduleCommandBuffersBeforeQueueSubmit) {
    recorder.Schedule(new CvkEndCommandBuffer(return_value, cmdBuf));
  }
  vkEndCommandBuffer_SD(return_value, cmdBuf);
}

inline void vkDestroyDescriptorPool_RECWRAP(VkDevice device,
                                            VkDescriptorPool descriptorPool,
                                            const VkAllocationCallbacks* pAllocator,
                                            CRecorder& recorder) {
  if (recorder.Running()) {
    if (descriptorPool != VK_NULL_HANDLE) {
      std::vector<VkDescriptorSet> descriptorSets;

      for (auto& descriptorSetState :
           SD()._descriptorpoolstates[descriptorPool]->descriptorSetStateStoreList) {
        descriptorSets.push_back(descriptorSetState->descriptorSetHandle);
      }

      if (descriptorSets.size() > 0) {
        recorder.Schedule(
            new CDestroyVulkanDescriptorSets((size_t)descriptorSets.size(), descriptorSets.data()));
      }
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
  if (recorder.Running() && (commandPool != VK_NULL_HANDLE)) {
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
    if (Configurator::Get().vulkan.recorder.memoryUpdateState != TMemoryUpdateStates::USING_TAGS) {
      std::vector<VkBufferCopy> updatedRanges;
      getRangesForMemoryUpdate(memory, updatedRanges, true);
      if (updatedRanges.size() > 0) {
        recorder.Schedule(
            new CGitsVkMemoryUpdate2(memory, updatedRanges.size(), updatedRanges.data()));
      }
    }
    recorder.Schedule(new CvkUnmapMemory(device, memory));
  } else if (Configurator::Get().vulkan.recorder.shadowMemory) {
    flushShadowMemory(memory, true);
  }
  vkUnmapMemory_SD(device, memory);
}

inline void vkPassPhysicalDeviceMemoryPropertiesGITS_RECWRAP(
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceMemoryProperties* pMemoryProperties,
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
    if (physicalDeviceState->memoryPropertiesOriginal.memoryHeapCount == 0) {
      VkPhysicalDeviceMemoryProperties memoryProperties;
      drvVk.vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
      vkPassPhysicalDeviceMemoryPropertiesGITS_SD(physicalDevice, &memoryProperties);
    }

    recorder.Schedule(new CvkPassPhysicalDeviceMemoryPropertiesGITS(
        physicalDevice, &physicalDeviceState->memoryPropertiesOriginal));
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
      0                                                        // VkDeviceAddress    deviceAddress;
  };

  // Record opaque/capture device address only when requested
  if (Configurator::Get()
          .vulkan.recorder.useCaptureReplayFeaturesForBuffersAndAccelerationStructures &&
      isBitSet(pCreateInfo->flags, VK_BUFFER_CREATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT)) {
    VkBufferDeviceAddressInfo addressInfo = {
        VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, // VkStructureType    sType;
        nullptr,                                      // const void       * pNext;
        *pBuffer                                      // VkBuffer           buffer;
    };

    // If no device address is assigned to a buffer
    if ((getPNextStructure(bufferCreateInfo.pNext,
                           VK_STRUCTURE_TYPE_BUFFER_OPAQUE_CAPTURE_ADDRESS_CREATE_INFO) ==
         nullptr) &&
        (getPNextStructure(bufferCreateInfo.pNext,
                           VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_CREATE_INFO_EXT) == nullptr)) {
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
  }

  if (recorder.Running()) {
    recorder.Schedule(
        new CvkCreateBuffer(return_value, device, &bufferCreateInfo, pAllocator, pBuffer));
  }

  vkCreateBuffer_SD(return_value, device, &bufferCreateInfo, pAllocator, pBuffer);
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
  if (recorder.Running() &&
      (Configurator::Get().vulkan.recorder.memoryUpdateState == TMemoryUpdateStates::USING_TAGS)) {
    recorder.Schedule(new CGitsVkMemoryUpdate2(memory, regionCount, pRegions));
  }
}

inline void vkAllocateMemory_RECWRAP(VkResult return_value,
                                     VkDevice device,
                                     const VkMemoryAllocateInfo* pAllocateInfo,
                                     const VkAllocationCallbacks* pAllocator,
                                     VkDeviceMemory* pMemory,
                                     CRecorder& recorder) {
  auto memoryAllocateInfo = *pAllocateInfo;

  VkMemoryOpaqueCaptureAddressAllocateInfo opaqueAddressAllocateInfo = {
      VK_STRUCTURE_TYPE_MEMORY_OPAQUE_CAPTURE_ADDRESS_ALLOCATE_INFO, // VkStructureType sType;
      pAllocateInfo->pNext,                                          // const void* pNext;
      0 // uint64_t opaqueCaptureAddress;
  };

  // Record opaque/capture device address only when requested
  if (Configurator::Get()
          .vulkan.recorder.useCaptureReplayFeaturesForBuffersAndAccelerationStructures) {
    auto allocateFlagsInfo = (VkMemoryAllocateFlagsInfo*)getPNextStructure(
        pAllocateInfo->pNext, VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO);
    if ((allocateFlagsInfo != nullptr) &&
        isBitSet(allocateFlagsInfo->flags, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT)) {
      if (drvVk.GetDeviceDispatchTable(device).vkGetDeviceMemoryOpaqueCaptureAddressUnifiedGITS) {
        VkDeviceMemoryOpaqueCaptureAddressInfo addressInfo = {
            VK_STRUCTURE_TYPE_DEVICE_MEMORY_OPAQUE_CAPTURE_ADDRESS_INFO, // VkStructureType sType;
            nullptr,                                                     // const void* pNext;
            *pMemory                                                     // VkDeviceMemory memory;
        };
        opaqueAddressAllocateInfo.opaqueCaptureAddress =
            drvVk.vkGetDeviceMemoryOpaqueCaptureAddressUnifiedGITS(device, &addressInfo);
        memoryAllocateInfo.pNext = &opaqueAddressAllocateInfo;
      }
    }
  }

  if (recorder.Running()) {
    recorder.Schedule(
        new CvkAllocateMemory(return_value, device, &memoryAllocateInfo, pAllocator, pMemory));
  }
  // vkAllocateMemory_SD() function is called inside recExecWrap_vkAllocateMemory() function
}

inline void vkCreateAccelerationStructureKHR_RECWRAP(
    VkResult return_value,
    VkDevice device,
    const VkAccelerationStructureCreateInfoKHR* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkAccelerationStructureKHR* pAccelerationStructure,
    CRecorder& recorder) {
  auto accelerationStructureCreateInfo = *pCreateInfo;

  if (Configurator::Get()
          .vulkan.recorder.useCaptureReplayFeaturesForBuffersAndAccelerationStructures) {
    VkAccelerationStructureDeviceAddressInfoKHR addressInfo = {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR, // VkStructureType              sType;
        nullptr,                // const void                 * pNext;
        *pAccelerationStructure // VkAccelerationStructureKHR   accelerationStructure;
    };

    accelerationStructureCreateInfo.createFlags |=
        VK_ACCELERATION_STRUCTURE_CREATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT_KHR;
    accelerationStructureCreateInfo.deviceAddress =
        drvVk.vkGetAccelerationStructureDeviceAddressUnifiedGITS(device, &addressInfo);
  }
  if (recorder.Running()) {
    recorder.Schedule(new CvkCreateAccelerationStructureKHR(return_value, device,
                                                            &accelerationStructureCreateInfo,
                                                            pAllocator, pAccelerationStructure));
  }
  vkCreateAccelerationStructureKHR_SD(return_value, device, &accelerationStructureCreateInfo,
                                      pAllocator, pAccelerationStructure);
}

inline void vkGetAccelerationStructureDeviceAddressUnifiedGITS_RECWRAP(
    VkDeviceAddress return_value,
    VkDevice device,
    const VkAccelerationStructureDeviceAddressInfoKHR* pInfo,
    CRecorder& recorder) {
  if (recorder.Running()) {
    recorder.Schedule(
        new CvkGetAccelerationStructureDeviceAddressUnifiedGITS(return_value, device, pInfo));
    vkGetAccelerationStructureDeviceAddressUnifiedGITS_SD(return_value, device, pInfo);
  }
}

inline void vkGetAccelerationStructureDeviceAddressKHR_RECWRAP(
    VkDeviceAddress return_value,
    VkDevice device,
    const VkAccelerationStructureDeviceAddressInfoKHR* pInfo,
    CRecorder& recorder) {
  vkGetAccelerationStructureDeviceAddressUnifiedGITS_RECWRAP(return_value, device, pInfo, recorder);
}

inline void vkCreateRayTracingPipelinesKHR_RECWRAP(
    VkResult return_value,
    VkDevice device,
    VkDeferredOperationKHR deferredOperation,
    VkPipelineCache pipelineCache,
    uint32_t createInfoCount,
    const VkRayTracingPipelineCreateInfoKHR* pCreateInfos,
    const VkAllocationCallbacks* pAllocator,
    VkPipeline* pPipelines,
    CRecorder& recorder) {
  vkCreateRayTracingPipelinesKHR_SD(return_value, device, deferredOperation, pipelineCache,
                                    createInfoCount, pCreateInfos, pAllocator, pPipelines);

  std::vector<std::vector<uint8_t>> pipelineCaptureReplayHandles;
  std::vector<VkRayTracingPipelineCreateInfoKHR> allCreateInfos;
  std::vector<std::vector<VkRayTracingShaderGroupCreateInfoKHR>> allShaderGroups;
  std::list<VkOriginalShaderGroupHandlesGITS> shaderGroupHandlesPNexts;

  for (uint32_t p = 0; p < createInfoCount; ++p) {
    allCreateInfos.emplace_back(pCreateInfos[p]);
    allShaderGroups.emplace_back();

    auto& currentCreateInfo = allCreateInfos.back();
    auto& groupsForCurrentCreateInfo = allShaderGroups.back();

    for (uint32_t g = 0; g < currentCreateInfo.groupCount; ++g) {
      groupsForCurrentCreateInfo.emplace_back(currentCreateInfo.pGroups[g]);

      if (Configurator::Get().vulkan.recorder.useCaptureReplayFeaturesForRayTracingPipelines) {
        auto& currentGroup = groupsForCurrentCreateInfo.back();
        auto handleSize = getRayTracingShaderGroupCaptureReplayHandleSize(device);

        std::vector<uint8_t> handle(handleSize);
        drvVk.vkGetRayTracingCaptureReplayShaderGroupHandlesKHR(device, pPipelines[p], g, 1,
                                                                handleSize, handle.data());
        pipelineCaptureReplayHandles.push_back(std::move(handle));

        currentGroup.pShaderGroupCaptureReplayHandle = pipelineCaptureReplayHandles.back().data();
      }
    }

    currentCreateInfo.pGroups = groupsForCurrentCreateInfo.data();

    auto* pNext = (VkOriginalShaderGroupHandlesGITS*)getPNextStructure(
        currentCreateInfo.pNext, VK_STRUCTURE_TYPE_ORIGINAL_SHADER_GROUP_HANDLES_GITS);
    if (!pNext) {
      auto pipeline = pPipelines[p];
      if (pipeline == VK_NULL_HANDLE) {
        continue;
      }

      auto& shaderGroup = SD()._pipelinestates[pipeline]->shaderGroupHandles;

      shaderGroupHandlesPNexts.emplace_back(VkOriginalShaderGroupHandlesGITS{
          VK_STRUCTURE_TYPE_ORIGINAL_SHADER_GROUP_HANDLES_GITS, // VkStructureType sType;
          currentCreateInfo.pNext,                              // const void* pNext;
          shaderGroup.dataSize,                                 // uint32_t dataSize;
          shaderGroup.originalHandles.data()                    // const void* pData;
      });

      // This pNext will dangle when the function ends, but then allCreateInfos will also be
      // destroyed. Passing it to CvkCreateRayTracingPipelinesKHR constructor is also not a problem,
      // as it will make copies of all the structures. So after Schedule(...) returns, the pNext no
      // longer needs to be valid.
      currentCreateInfo.pNext = &shaderGroupHandlesPNexts.back();
    }
  }

  if (recorder.Running()) {
    recorder.Schedule(new CvkCreateRayTracingPipelinesKHR(
        return_value, device, VK_NULL_HANDLE /* deferredOperation */, pipelineCache,
        createInfoCount, allCreateInfos.data(), pAllocator, pPipelines));
  }
}

void vkCmdBuildAccelerationStructuresKHR_RECWRAP(
    VkCommandBuffer cmdBuf,
    uint32_t infoCount,
    const VkAccelerationStructureBuildGeometryInfoKHR* pInfos,
    const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos,
    CRecorder& recorder) {
  // When capture/replay features are used, all the device addresses are supposed to remain unchanged,
  // even during replay, so they don't need to be updated and we don't need to do anything with them...
  //
  // unless substreams are recorded. State restoration phase of substream requires contents of input
  // buffers to be kept for each acceleration structure so building commands can be recreated.
  //
  // But when capture/replay features are NOT used and/or when substreams are being recorded, then
  // metadata for all the device addresses needs to be prepared as well so it is possible to update,
  // recreate those addresses during a stream replay. Single metadata contains:
  // - original device address value
  // - a handle of a buffer from which a device address was retrieved
  // - an offset from the beggining of a buffer
  //
  // How this metadata is acquired depends on the origins of each device address. In most cases, device
  // addresses point directly to a buffer so metadata acquisition is straightforward - GITS just needs
  // to find a resource associated with a provided address. But in case of vertex data - final device
  // address depends also on a value of a vertex index which is added to the base device address.
  // That's why a vertex index needs to be retrieved before the buffer can be found from the specified
  // address. The index is copied and the final device address calculations are performed on a CPU -
  // - acquired vertex index is multiplied by a vertex stride and added to the base device address;
  // this value is then used to find a source buffer with a vertex data.
  //
  // Another exception is an instance data buffer provided for building top-level acceleration structures.
  // This buffer contains device addresses of all bottom-level acceleration structures built into the TLAS.
  // When capture/replay features are not used, GITS needs to translate/update those device addresses during
  // stream replay, so first it needs to gather information about handles of all the bottom-level acceleration
  // structures. In order to do this, it needs to copy instance data buffer contents from memory locations
  // pointed to by a device address and then look through it. The buffer contains device addresses, so
  // acceleration structures associated with them need to be find and their handles need to be acquired.

  vkCmdBuildAccelerationStructuresKHR_SD(cmdBuf, infoCount, pInfos, ppBuildRangeInfos);

  if (recorder.Running()) {
    // Schedule a token which updates/patches a list of device addresses
    if (!useCaptureReplayFeaturesForBuffersAndAccelerationStructures()) {
      auto& addressPatchers = SD()._commandbufferstates[cmdBuf]->addressPatchers;
      auto it = addressPatchers.find(
          CAccelerationStructureKHRState::globalAccelerationStructureBuildCommandIndex);
      if ((it != addressPatchers.end()) && (it->second.Count() > 0)) {
        recorder.Schedule(new CGitsVkCmdPatchDeviceAddresses(
            cmdBuf, it->second,
            CAccelerationStructureKHRState::globalAccelerationStructureBuildCommandIndex));
      }
    }

    recorder.Schedule(
        new CvkCmdBuildAccelerationStructuresKHR(cmdBuf, infoCount, pInfos, ppBuildRangeInfos));
  }
}

#ifdef GITS_PLATFORM_WINDOWS
namespace {

// Offscreen applications support
inline void barriers2HelperForOffscreenApplications(VkCommandBuffer cmdBuf,
                                                    const VkDependencyInfo* pDependencyInfo,
                                                    CRecorder& recorder) {

  if (usePresentSrcLayoutTransitionAsAFrameBoundary()) {
    auto device =
        SD()._commandbufferstates[cmdBuf]->commandPoolStateStore->deviceStateStore->deviceHandle;

    for (unsigned int i = 0; i < pDependencyInfo->imageMemoryBarrierCount; i++) {
      if (pDependencyInfo->pImageMemoryBarriers[i].newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
        if (recorder.Running()) {
          auto pImageCreateInfo = SD()._imagestates[pDependencyInfo->pImageMemoryBarriers[i].image]
                                      ->imageCreateInfoData.Value();
          handleSwapchainCreationForOffscreenApplications(device, pImageCreateInfo, recorder);
        }
        {
          auto& offscreenApp = SD().internalResources.offscreenApps[device];
          offscreenApp.commandBufferWithTransitionToPresentSRC = cmdBuf;
          offscreenApp.imageToPresent = pDependencyInfo->pImageMemoryBarriers[i].image;
        }
        break;
      }
    }
  }
}

} // namespace
#endif

inline void vkCmdPipelineBarrier_RECWRAP(VkCommandBuffer cmdBuf,
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
      !Configurator::Get().vulkan.recorder.scheduleCommandBuffersBeforeQueueSubmit) {
    recorder.Schedule(new CvkCmdPipelineBarrier(cmdBuf, srcStageMask, dstStageMask, dependencyFlags,
                                                memoryBarrierCount, pMemoryBarriers,
                                                bufferMemoryBarrierCount, pBufferMemoryBarriers,
                                                imageMemoryBarrierCount, pImageMemoryBarriers));
  } else {
    SD()._commandbufferstates[cmdBuf]->tokensBuffer.Add(new CvkCmdPipelineBarrier(
        cmdBuf, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount, pMemoryBarriers,
        bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount,
        pImageMemoryBarriers));
  }
#ifdef GITS_PLATFORM_WINDOWS
  // Offscreen applications support
  if (!Configurator::Get().vulkan.recorder.scheduleCommandBuffersBeforeQueueSubmit &&
      usePresentSrcLayoutTransitionAsAFrameBoundary()) {
    auto device =
        SD()._commandbufferstates[cmdBuf]->commandPoolStateStore->deviceStateStore->deviceHandle;

    for (unsigned int i = 0; i < imageMemoryBarrierCount; i++) {
      if (pImageMemoryBarriers[i].newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
        if (recorder.Running()) {
          auto pImageCreateInfo =
              SD()._imagestates[pImageMemoryBarriers[i].image]->imageCreateInfoData.Value();
          handleSwapchainCreationForOffscreenApplications(device, pImageCreateInfo, recorder);
        }
        {
          auto& offscreenApp = SD().internalResources.offscreenApps[device];
          offscreenApp.commandBufferWithTransitionToPresentSRC = cmdBuf;
          offscreenApp.imageToPresent = pImageMemoryBarriers[i].image;
        }
        break;
      }
    }
  }
#endif
  vkCmdPipelineBarrier_SD(cmdBuf, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount,
                          pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers,
                          imageMemoryBarrierCount, pImageMemoryBarriers);
}

inline void vkCmdPipelineBarrier2UnifiedGITS_RECWRAP(VkCommandBuffer cmdBuf,
                                                     const VkDependencyInfo* pDependencyInfo,
                                                     CRecorder& recorder) {
  if (recorder.Running() &&
      !Configurator::Get().vulkan.recorder.scheduleCommandBuffersBeforeQueueSubmit) {
    recorder.Schedule(new CvkCmdPipelineBarrier2UnifiedGITS(cmdBuf, pDependencyInfo));
  } else {
    SD()._commandbufferstates[cmdBuf]->tokensBuffer.Add(
        new CvkCmdPipelineBarrier2UnifiedGITS(cmdBuf, pDependencyInfo));
  }
#ifdef GITS_PLATFORM_WINDOWS
  if (!Configurator::Get().vulkan.recorder.scheduleCommandBuffersBeforeQueueSubmit) {
    barriers2HelperForOffscreenApplications(cmdBuf, pDependencyInfo, recorder);
  }
#endif
  vkCmdPipelineBarrier2UnifiedGITS_SD(cmdBuf, pDependencyInfo);
}

inline void vkCmdPipelineBarrier2_RECWRAP(VkCommandBuffer cmdBuf,
                                          const VkDependencyInfo* pDependencyInfo,
                                          CRecorder& recorder) {
  if (recorder.Running() &&
      !Configurator::Get().vulkan.recorder.scheduleCommandBuffersBeforeQueueSubmit) {
    recorder.Schedule(new CvkCmdPipelineBarrier2(cmdBuf, pDependencyInfo));
  } else {
    SD()._commandbufferstates[cmdBuf]->tokensBuffer.Add(
        new CvkCmdPipelineBarrier2(cmdBuf, pDependencyInfo));
  }
#ifdef GITS_PLATFORM_WINDOWS
  if (!Configurator::Get().vulkan.recorder.scheduleCommandBuffersBeforeQueueSubmit) {
    barriers2HelperForOffscreenApplications(cmdBuf, pDependencyInfo, recorder);
  }
#endif
  vkCmdPipelineBarrier2_SD(cmdBuf, pDependencyInfo);
}

inline void vkCmdPipelineBarrier2KHR_RECWRAP(VkCommandBuffer cmdBuf,
                                             const VkDependencyInfo* pDependencyInfo,
                                             CRecorder& recorder) {
  if (recorder.Running() &&
      !Configurator::Get().vulkan.recorder.scheduleCommandBuffersBeforeQueueSubmit) {
    recorder.Schedule(new CvkCmdPipelineBarrier2KHR(cmdBuf, pDependencyInfo));
  } else {
    SD()._commandbufferstates[cmdBuf]->tokensBuffer.Add(
        new CvkCmdPipelineBarrier2KHR(cmdBuf, pDependencyInfo));
  }
#ifdef GITS_PLATFORM_WINDOWS
  if (!Configurator::Get().vulkan.recorder.scheduleCommandBuffersBeforeQueueSubmit) {
    barriers2HelperForOffscreenApplications(cmdBuf, pDependencyInfo, recorder);
  }
#endif
  vkCmdPipelineBarrier2KHR_SD(cmdBuf, pDependencyInfo);
}

inline void vkCmdSetEvent2_RECWRAP(VkCommandBuffer cmdBuf,
                                   VkEvent event,
                                   const VkDependencyInfo* pDependencyInfo,
                                   CRecorder& recorder) {
  if (recorder.Running() &&
      !Configurator::Get().vulkan.recorder.scheduleCommandBuffersBeforeQueueSubmit) {
    recorder.Schedule(new CvkCmdSetEvent2(cmdBuf, event, pDependencyInfo));
  } else {
    SD()._commandbufferstates[cmdBuf]->tokensBuffer.Add(
        new CvkCmdSetEvent2(cmdBuf, event, pDependencyInfo));
  }
#ifdef GITS_PLATFORM_WINDOWS
  if (!Configurator::Get().vulkan.recorder.scheduleCommandBuffersBeforeQueueSubmit) {
    barriers2HelperForOffscreenApplications(cmdBuf, pDependencyInfo, recorder);
  }
#endif
  vkCmdSetEvent2_SD(cmdBuf, event, pDependencyInfo);
}

inline void vkCmdSetEvent2KHR_RECWRAP(VkCommandBuffer cmdBuf,
                                      VkEvent event,
                                      const VkDependencyInfo* pDependencyInfo,
                                      CRecorder& recorder) {
  if (recorder.Running() &&
      !Configurator::Get().vulkan.recorder.scheduleCommandBuffersBeforeQueueSubmit) {
    recorder.Schedule(new CvkCmdSetEvent2KHR(cmdBuf, event, pDependencyInfo));
  } else {
    SD()._commandbufferstates[cmdBuf]->tokensBuffer.Add(
        new CvkCmdSetEvent2KHR(cmdBuf, event, pDependencyInfo));
  }
#ifdef GITS_PLATFORM_WINDOWS
  if (!Configurator::Get().vulkan.recorder.scheduleCommandBuffersBeforeQueueSubmit) {
    barriers2HelperForOffscreenApplications(cmdBuf, pDependencyInfo, recorder);
  }
#endif
  vkCmdSetEvent2KHR_SD(cmdBuf, event, pDependencyInfo);
}

inline void vkCreateRenderPass_RECWRAP(VkResult return_value,
                                       VkDevice device,
                                       const VkRenderPassCreateInfo* pCreateInfo,
                                       const VkAllocationCallbacks* pAllocator,
                                       VkRenderPass* pRenderPass,
                                       CRecorder& recorder) {
  if (recorder.Running()) {
    recorder.Schedule(
        new CvkCreateRenderPass(return_value, device, pCreateInfo, pAllocator, pRenderPass));
  }
  vkCreateRenderPass_SD(return_value, device, pCreateInfo, pAllocator, pRenderPass);
  CreateRenderPasses_helper<VkRenderPassCreateInfo, VkAttachmentDescription>(
      device, *pRenderPass, *pCreateInfo, SD()._renderpassstates[*pRenderPass]->createdWith);
}
inline void vkCreateRenderPass2_RECWRAP(VkResult return_value,
                                        VkDevice device,
                                        const VkRenderPassCreateInfo2* pCreateInfo,
                                        const VkAllocationCallbacks* pAllocator,
                                        VkRenderPass* pRenderPass,
                                        CRecorder& recorder) {
  if (recorder.Running()) {
    recorder.Schedule(
        new CvkCreateRenderPass2(return_value, device, pCreateInfo, pAllocator, pRenderPass));
  }
  vkCreateRenderPass2_SD(return_value, device, pCreateInfo, pAllocator, pRenderPass);
  CreateRenderPasses_helper<VkRenderPassCreateInfo2, VkAttachmentDescription2>(
      device, *pRenderPass, *pCreateInfo, SD()._renderpassstates[*pRenderPass]->createdWith);
}
inline void vkCreateRenderPass2KHR_RECWRAP(VkResult return_value,
                                           VkDevice device,
                                           const VkRenderPassCreateInfo2* pCreateInfo,
                                           const VkAllocationCallbacks* pAllocator,
                                           VkRenderPass* pRenderPass,
                                           CRecorder& recorder) {
  if (recorder.Running()) {
    recorder.Schedule(
        new CvkCreateRenderPass2KHR(return_value, device, pCreateInfo, pAllocator, pRenderPass));
  }
  vkCreateRenderPass2KHR_SD(return_value, device, pCreateInfo, pAllocator, pRenderPass);
  CreateRenderPasses_helper<VkRenderPassCreateInfo2, VkAttachmentDescription2>(
      device, *pRenderPass, *pCreateInfo, SD()._renderpassstates[*pRenderPass]->createdWith);
}
} // namespace Vulkan
} // namespace gits

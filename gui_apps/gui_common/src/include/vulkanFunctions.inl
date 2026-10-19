// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

// Global functions (loaded with vkGetInstanceProcAddr(NULL, ...))
#ifdef VK_GLOBAL_FUNCTION
VK_GLOBAL_FUNCTION(vkCreateInstance)
VK_GLOBAL_FUNCTION(vkEnumerateInstanceExtensionProperties)
VK_GLOBAL_FUNCTION(vkEnumerateInstanceLayerProperties)
VK_GLOBAL_FUNCTION(vkEnumerateInstanceVersion)
#undef VK_GLOBAL_FUNCTION
#endif

// Instance functions (loaded with vkGetInstanceProcAddr(instance, ...))
#ifdef VK_INSTANCE_FUNCTION
VK_INSTANCE_FUNCTION(vkDestroyInstance)
VK_INSTANCE_FUNCTION(vkEnumeratePhysicalDevices)
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceProperties)
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceProperties2)
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceFeatures)
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceFeatures2)
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceQueueFamilyProperties)
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceQueueFamilyProperties2)
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceMemoryProperties)
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceMemoryProperties2)
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceFormatProperties)
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceFormatProperties2)
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceImageFormatProperties)
VK_INSTANCE_FUNCTION(vkCreateDevice)
VK_INSTANCE_FUNCTION(vkGetDeviceProcAddr)
VK_INSTANCE_FUNCTION(vkEnumerateDeviceExtensionProperties)
VK_INSTANCE_FUNCTION(vkEnumerateDeviceLayerProperties)

// Surface functions
VK_INSTANCE_FUNCTION(vkDestroySurfaceKHR)
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceSurfaceSupportKHR)
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceSurfaceFormatsKHR)
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceSurfacePresentModesKHR)

// Platform-specific surface creation
#ifdef VK_USE_PLATFORM_WIN32_KHR
VK_INSTANCE_FUNCTION(vkCreateWin32SurfaceKHR)
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceWin32PresentationSupportKHR)
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
VK_INSTANCE_FUNCTION(vkCreateXlibSurfaceKHR)
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceXlibPresentationSupportKHR)
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
VK_INSTANCE_FUNCTION(vkCreateXcbSurfaceKHR)
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceXcbPresentationSupportKHR)
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
VK_INSTANCE_FUNCTION(vkCreateWaylandSurfaceKHR)
VK_INSTANCE_FUNCTION(vkGetPhysicalDeviceWaylandPresentationSupportKHR)
#endif

// Debug utils (if validation layers are used)
VK_INSTANCE_FUNCTION(vkCreateDebugUtilsMessengerEXT)
VK_INSTANCE_FUNCTION(vkDestroyDebugUtilsMessengerEXT)

#undef VK_INSTANCE_FUNCTION
#endif

// Device functions (loaded with vkGetDeviceProcAddr(device, ...))
#ifdef VK_DEVICE_FUNCTION
// Core device functions
VK_DEVICE_FUNCTION(vkDestroyDevice)
VK_DEVICE_FUNCTION(vkGetDeviceQueue)
VK_DEVICE_FUNCTION(vkGetDeviceQueue2)
VK_DEVICE_FUNCTION(vkDeviceWaitIdle)

// Memory management
VK_DEVICE_FUNCTION(vkAllocateMemory)
VK_DEVICE_FUNCTION(vkFreeMemory)
VK_DEVICE_FUNCTION(vkMapMemory)
VK_DEVICE_FUNCTION(vkUnmapMemory)
VK_DEVICE_FUNCTION(vkFlushMappedMemoryRanges)
VK_DEVICE_FUNCTION(vkInvalidateMappedMemoryRanges)
VK_DEVICE_FUNCTION(vkGetDeviceMemoryCommitment)
VK_DEVICE_FUNCTION(vkBindBufferMemory)
VK_DEVICE_FUNCTION(vkBindBufferMemory2)
VK_DEVICE_FUNCTION(vkBindImageMemory)
VK_DEVICE_FUNCTION(vkBindImageMemory2)
VK_DEVICE_FUNCTION(vkGetBufferMemoryRequirements)
VK_DEVICE_FUNCTION(vkGetBufferMemoryRequirements2)
VK_DEVICE_FUNCTION(vkGetImageMemoryRequirements)
VK_DEVICE_FUNCTION(vkGetImageMemoryRequirements2)

// Buffer operations
VK_DEVICE_FUNCTION(vkCreateBuffer)
VK_DEVICE_FUNCTION(vkDestroyBuffer)
VK_DEVICE_FUNCTION(vkCreateBufferView)
VK_DEVICE_FUNCTION(vkDestroyBufferView)

// Image operations
VK_DEVICE_FUNCTION(vkCreateImage)
VK_DEVICE_FUNCTION(vkDestroyImage)
VK_DEVICE_FUNCTION(vkGetImageSubresourceLayout)
VK_DEVICE_FUNCTION(vkCreateImageView)
VK_DEVICE_FUNCTION(vkDestroyImageView)

// Sampler
VK_DEVICE_FUNCTION(vkCreateSampler)
VK_DEVICE_FUNCTION(vkDestroySampler)

// Descriptors
VK_DEVICE_FUNCTION(vkCreateDescriptorSetLayout)
VK_DEVICE_FUNCTION(vkDestroyDescriptorSetLayout)
VK_DEVICE_FUNCTION(vkCreateDescriptorPool)
VK_DEVICE_FUNCTION(vkDestroyDescriptorPool)
VK_DEVICE_FUNCTION(vkResetDescriptorPool)
VK_DEVICE_FUNCTION(vkAllocateDescriptorSets)
VK_DEVICE_FUNCTION(vkFreeDescriptorSets)
VK_DEVICE_FUNCTION(vkUpdateDescriptorSets)

// Pipeline operations
VK_DEVICE_FUNCTION(vkCreatePipelineLayout)
VK_DEVICE_FUNCTION(vkDestroyPipelineLayout)
VK_DEVICE_FUNCTION(vkCreatePipelineCache)
VK_DEVICE_FUNCTION(vkDestroyPipelineCache)
VK_DEVICE_FUNCTION(vkGetPipelineCacheData)
VK_DEVICE_FUNCTION(vkMergePipelineCaches)
VK_DEVICE_FUNCTION(vkCreateGraphicsPipelines)
VK_DEVICE_FUNCTION(vkCreateComputePipelines)
VK_DEVICE_FUNCTION(vkDestroyPipeline)
VK_DEVICE_FUNCTION(vkCreateShaderModule)
VK_DEVICE_FUNCTION(vkDestroyShaderModule)

// Render pass and framebuffer
VK_DEVICE_FUNCTION(vkCreateRenderPass)
VK_DEVICE_FUNCTION(vkCreateRenderPass2)
VK_DEVICE_FUNCTION(vkDestroyRenderPass)
VK_DEVICE_FUNCTION(vkGetRenderAreaGranularity)
VK_DEVICE_FUNCTION(vkCreateFramebuffer)
VK_DEVICE_FUNCTION(vkDestroyFramebuffer)

// Command operations
VK_DEVICE_FUNCTION(vkCreateCommandPool)
VK_DEVICE_FUNCTION(vkDestroyCommandPool)
VK_DEVICE_FUNCTION(vkResetCommandPool)
VK_DEVICE_FUNCTION(vkAllocateCommandBuffers)
VK_DEVICE_FUNCTION(vkFreeCommandBuffers)
VK_DEVICE_FUNCTION(vkBeginCommandBuffer)
VK_DEVICE_FUNCTION(vkEndCommandBuffer)
VK_DEVICE_FUNCTION(vkResetCommandBuffer)

// Command buffer recording
VK_DEVICE_FUNCTION(vkCmdBindPipeline)
VK_DEVICE_FUNCTION(vkCmdSetViewport)
VK_DEVICE_FUNCTION(vkCmdSetScissor)
VK_DEVICE_FUNCTION(vkCmdSetLineWidth)
VK_DEVICE_FUNCTION(vkCmdSetDepthBias)
VK_DEVICE_FUNCTION(vkCmdSetBlendConstants)
VK_DEVICE_FUNCTION(vkCmdSetDepthBounds)
VK_DEVICE_FUNCTION(vkCmdSetStencilCompareMask)
VK_DEVICE_FUNCTION(vkCmdSetStencilWriteMask)
VK_DEVICE_FUNCTION(vkCmdSetStencilReference)
VK_DEVICE_FUNCTION(vkCmdBindDescriptorSets)
VK_DEVICE_FUNCTION(vkCmdBindIndexBuffer)
VK_DEVICE_FUNCTION(vkCmdBindVertexBuffers)
VK_DEVICE_FUNCTION(vkCmdDraw)
VK_DEVICE_FUNCTION(vkCmdDrawIndexed)
VK_DEVICE_FUNCTION(vkCmdDrawIndirect)
VK_DEVICE_FUNCTION(vkCmdDrawIndexedIndirect)
VK_DEVICE_FUNCTION(vkCmdDispatch)
VK_DEVICE_FUNCTION(vkCmdDispatchIndirect)
VK_DEVICE_FUNCTION(vkCmdCopyBuffer)
VK_DEVICE_FUNCTION(vkCmdCopyImage)
VK_DEVICE_FUNCTION(vkCmdBlitImage)
VK_DEVICE_FUNCTION(vkCmdCopyBufferToImage)
VK_DEVICE_FUNCTION(vkCmdCopyImageToBuffer)
VK_DEVICE_FUNCTION(vkCmdUpdateBuffer)
VK_DEVICE_FUNCTION(vkCmdFillBuffer)
VK_DEVICE_FUNCTION(vkCmdClearColorImage)
VK_DEVICE_FUNCTION(vkCmdClearDepthStencilImage)
VK_DEVICE_FUNCTION(vkCmdClearAttachments)
VK_DEVICE_FUNCTION(vkCmdResolveImage)
VK_DEVICE_FUNCTION(vkCmdSetEvent)
VK_DEVICE_FUNCTION(vkCmdResetEvent)
VK_DEVICE_FUNCTION(vkCmdWaitEvents)
VK_DEVICE_FUNCTION(vkCmdPipelineBarrier)
VK_DEVICE_FUNCTION(vkCmdBeginQuery)
VK_DEVICE_FUNCTION(vkCmdEndQuery)
VK_DEVICE_FUNCTION(vkCmdResetQueryPool)
VK_DEVICE_FUNCTION(vkCmdWriteTimestamp)
VK_DEVICE_FUNCTION(vkCmdCopyQueryPoolResults)
VK_DEVICE_FUNCTION(vkCmdPushConstants)
VK_DEVICE_FUNCTION(vkCmdBeginRenderPass)
VK_DEVICE_FUNCTION(vkCmdBeginRenderPass2)
VK_DEVICE_FUNCTION(vkCmdNextSubpass)
VK_DEVICE_FUNCTION(vkCmdNextSubpass2)
VK_DEVICE_FUNCTION(vkCmdEndRenderPass)
VK_DEVICE_FUNCTION(vkCmdEndRenderPass2)
VK_DEVICE_FUNCTION(vkCmdExecuteCommands)

// Synchronization
VK_DEVICE_FUNCTION(vkCreateSemaphore)
VK_DEVICE_FUNCTION(vkDestroySemaphore)
VK_DEVICE_FUNCTION(vkCreateEvent)
VK_DEVICE_FUNCTION(vkDestroyEvent)
VK_DEVICE_FUNCTION(vkGetEventStatus)
VK_DEVICE_FUNCTION(vkSetEvent)
VK_DEVICE_FUNCTION(vkResetEvent)
VK_DEVICE_FUNCTION(vkCreateFence)
VK_DEVICE_FUNCTION(vkDestroyFence)
VK_DEVICE_FUNCTION(vkResetFences)
VK_DEVICE_FUNCTION(vkGetFenceStatus)
VK_DEVICE_FUNCTION(vkWaitForFences)
VK_DEVICE_FUNCTION(vkQueueSubmit)
VK_DEVICE_FUNCTION(vkQueueWaitIdle)
VK_DEVICE_FUNCTION(vkQueueBindSparse)

// Query pools
VK_DEVICE_FUNCTION(vkCreateQueryPool)
VK_DEVICE_FUNCTION(vkDestroyQueryPool)
VK_DEVICE_FUNCTION(vkGetQueryPoolResults)

// Swapchain (KHR extension)
VK_DEVICE_FUNCTION(vkCreateSwapchainKHR)
VK_DEVICE_FUNCTION(vkDestroySwapchainKHR)
VK_DEVICE_FUNCTION(vkGetSwapchainImagesKHR)
VK_DEVICE_FUNCTION(vkAcquireNextImageKHR)
VK_DEVICE_FUNCTION(vkQueuePresentKHR)
VK_DEVICE_FUNCTION(vkGetDeviceGroupPresentCapabilitiesKHR)
VK_DEVICE_FUNCTION(vkGetDeviceGroupSurfacePresentModesKHR)
VK_DEVICE_FUNCTION(vkAcquireNextImage2KHR)

#undef VK_DEVICE_FUNCTION
#endif

// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

// This file contains defines that change default Vulkan calls used by Dear Imgui into our custom driver calls

#pragma once

#ifdef IMGUI_USE_GITS_VULKAN_HEADERS
#include "vulkanDrivers.h"

#define vkGetPhysicalDeviceMemoryProperties gits::Vulkan::drvVk.vkGetPhysicalDeviceMemoryProperties
#define vkDestroyBuffer                     gits::Vulkan::drvVk.vkDestroyBuffer
#define vkFreeMemory                        gits::Vulkan::drvVk.vkFreeMemory
#define vkCreateBuffer                      gits::Vulkan::drvVk.vkCreateBuffer
#define vkGetBufferMemoryRequirements       gits::Vulkan::drvVk.vkGetBufferMemoryRequirements
#define vkAllocateMemory                    gits::Vulkan::drvVk.vkAllocateMemory
#define vkBindBufferMemory                  gits::Vulkan::drvVk.vkBindBufferMemory
#define vkCmdBindPipeline                   gits::Vulkan::drvVk.vkCmdBindPipeline
#define vkCmdBindVertexBuffers              gits::Vulkan::drvVk.vkCmdBindVertexBuffers
#define vkCmdBindIndexBuffer                gits::Vulkan::drvVk.vkCmdBindIndexBuffer
#define vkCmdSetViewport                    gits::Vulkan::drvVk.vkCmdSetViewport
#define vkCmdPushConstants                  gits::Vulkan::drvVk.vkCmdPushConstants
#define vkMapMemory                         gits::Vulkan::drvVk.vkMapMemory
#define vkFlushMappedMemoryRanges           gits::Vulkan::drvVk.vkFlushMappedMemoryRanges
#define vkUnmapMemory                       gits::Vulkan::drvVk.vkUnmapMemory
#define vkCmdSetScissor                     gits::Vulkan::drvVk.vkCmdSetScissor
#define vkCmdBindDescriptorSets             gits::Vulkan::drvVk.vkCmdBindDescriptorSets
#define vkCmdDrawIndexed                    gits::Vulkan::drvVk.vkCmdDrawIndexed
#define vkQueueWaitIdle                     gits::Vulkan::drvVk.vkQueueWaitIdle
#define vkCreateCommandPool                 gits::Vulkan::drvVk.vkCreateCommandPool
#define vkAllocateCommandBuffers            gits::Vulkan::drvVk.vkAllocateCommandBuffers
#define vkResetCommandPool                  gits::Vulkan::drvVk.vkResetCommandPool
#define vkBeginCommandBuffer                gits::Vulkan::drvVk.vkBeginCommandBuffer
#define vkCreateImage                       gits::Vulkan::drvVk.vkCreateImage
#define vkGetImageMemoryRequirements        gits::Vulkan::drvVk.vkGetImageMemoryRequirements
#define vkBindImageMemory                   gits::Vulkan::drvVk.vkBindImageMemory
#define vkCreateImageView                   gits::Vulkan::drvVk.vkCreateImageView
#define vkCmdPipelineBarrier                gits::Vulkan::drvVk.vkCmdPipelineBarrier
#define vkCmdCopyBufferToImage              gits::Vulkan::drvVk.vkCmdCopyBufferToImage
#define vkEndCommandBuffer                  gits::Vulkan::drvVk.vkEndCommandBuffer
#define vkQueueSubmit                       gits::Vulkan::drvVk.vkQueueSubmit
#define vkDestroyImageView                  gits::Vulkan::drvVk.vkDestroyImageView
#define vkDestroyImage                      gits::Vulkan::drvVk.vkDestroyImage
#define vkCreateShaderModule                gits::Vulkan::drvVk.vkCreateShaderModule
#define vkCreateGraphicsPipelines           gits::Vulkan::drvVk.vkCreateGraphicsPipelines
#define vkCreateSampler                     gits::Vulkan::drvVk.vkCreateSampler
#define vkCreateDescriptorSetLayout         gits::Vulkan::drvVk.vkCreateDescriptorSetLayout
#define vkCreateDescriptorPool              gits::Vulkan::drvVk.vkCreateDescriptorPool
#define vkCreatePipelineLayout              gits::Vulkan::drvVk.vkCreatePipelineLayout
#define vkFreeCommandBuffers                gits::Vulkan::drvVk.vkFreeCommandBuffers
#define vkDestroyCommandPool                gits::Vulkan::drvVk.vkDestroyCommandPool
#define vkDestroySampler                    gits::Vulkan::drvVk.vkDestroySampler
#define vkDestroyShaderModule               gits::Vulkan::drvVk.vkDestroyShaderModule
#define vkDestroyDescriptorSetLayout        gits::Vulkan::drvVk.vkDestroyDescriptorSetLayout
#define vkDestroyPipelineLayout             gits::Vulkan::drvVk.vkDestroyPipelineLayout
#define vkDestroyPipeline                   gits::Vulkan::drvVk.vkDestroyPipeline
#define vkDestroyDescriptorPool             gits::Vulkan::drvVk.vkDestroyDescriptorPool
#define vkGetInstanceProcAddr               gits::Vulkan::drvVk.vkGetInstanceProcAddr
#define vkDeviceWaitIdle                    gits::Vulkan::drvVk.vkDeviceWaitIdle
#define vkAllocateDescriptorSets            gits::Vulkan::drvVk.vkAllocateDescriptorSets
#define vkUpdateDescriptorSets              gits::Vulkan::drvVk.vkUpdateDescriptorSets
#define vkFreeDescriptorSets                gits::Vulkan::drvVk.vkFreeDescriptorSets
#define vkGetPhysicalDeviceSurfaceFormatsKHR                                                       \
  gits::Vulkan::drvVk.vkGetPhysicalDeviceSurfaceFormatsKHR
#define vkGetPhysicalDeviceSurfacePresentModesKHR                                                  \
  gits::Vulkan::drvVk.vkGetPhysicalDeviceSurfacePresentModesKHR
#define vkEnumeratePhysicalDevices    gits::Vulkan::drvVk.vkEnumeratePhysicalDevices
#define vkGetPhysicalDeviceProperties gits::Vulkan::drvVk.vkGetPhysicalDeviceProperties
#define vkGetPhysicalDeviceQueueFamilyProperties                                                   \
  gits::Vulkan::drvVk.vkGetPhysicalDeviceQueueFamilyProperties
#define vkCreateFence     gits::Vulkan::drvVk.vkCreateFence
#define vkCreateSemaphore gits::Vulkan::drvVk.vkCreateSemaphore
#define vkGetPhysicalDeviceSurfaceCapabilitiesKHR                                                  \
  gits::Vulkan::drvVk.vkGetPhysicalDeviceSurfaceCapabilitiesKHR
#define vkCreateSwapchainKHR    gits::Vulkan::drvVk.vkCreateSwapchainKHR
#define vkGetSwapchainImagesKHR gits::Vulkan::drvVk.vkGetSwapchainImagesKHR
#define vkDestroySwapchainKHR   gits::Vulkan::drvVk.vkDestroySwapchainKHR
#define vkCreateRenderPass      gits::Vulkan::drvVk.vkCreateRenderPass
#define vkCreateFramebuffer     gits::Vulkan::drvVk.vkCreateFramebuffer
#define vkDestroyFence          gits::Vulkan::drvVk.vkDestroyFence
#define vkDestroyCommandPool    gits::Vulkan::drvVk.vkDestroyCommandPool
#define vkDestroyPipeline       gits::Vulkan::drvVk.vkDestroyPipeline
#define vkDestroyRenderPass     gits::Vulkan::drvVk.vkDestroyRenderPass
#define vkDestroySwapchainKHR   gits::Vulkan::drvVk.vkDestroySwapchainKHR
#define vkDestroySurfaceKHR     gits::Vulkan::drvVk.vkDestroySurfaceKHR
#define vkDestroyFramebuffer    gits::Vulkan::drvVk.vkDestroyFramebuffer
#define vkDestroySemaphore      gits::Vulkan::drvVk.vkDestroySemaphore
#endif

// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "hudLayer.h"

#include "layerAuto.h"
#include "log.h"
#include "configurationLib.h"
#include <imgui.h>
#if defined(_WIN32)
#include <imgui_impl_win32.h>
#endif
#include <imgui_impl_vulkan.h>
#include "imGuiHUD.h"
#include "gits.h"

#include <algorithm>
#include <cstring>
#include <mutex>
#include <string>

namespace {

struct PresentInfoWaitCopy {
  uint32_t waitSemaphoreCount = 0;
  const VkSemaphore* pWaitSemaphores = nullptr;
  bool restore = false;
};

thread_local PresentInfoWaitCopy tl_hudPresentInfoCopy;
thread_local std::vector<VkSemaphore> tl_hudModifiedPresentWaitSemaphores;

void CheckVkResult(VkResult result) {
  if (result != VK_SUCCESS) {
    GITS_ASSERT(0, ("Vulkan call failed. VkResult: " + std::to_string(result)).c_str());
  }
}

} // namespace

namespace gits {
namespace vulkan {

HudLayer::HudLayer(DispatchTablesHolder& dispatchTablesHolder)
    : Layer(LAYER_NAME), m_DispatchTablesHolder(dispatchTablesHolder) {}

PFN_vkVoidFunction HudLayer::ImGuiVulkanLoader(const char* functionName, void* userData) {
  auto* self = static_cast<HudLayer*>(userData);

  const auto& instanceDispatchTable =
      self->m_DispatchTablesHolder.GetInstanceDispatchTable(self->m_PhysicalDevice);
  const auto& deviceDispatchTable =
      self->m_DispatchTablesHolder.GetDeviceDispatchTable(self->m_Device);

  if (strcmp(functionName, "vkAllocateCommandBuffers") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkAllocateCommandBuffers;
  }
  if (strcmp(functionName, "vkAllocateDescriptorSets") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkAllocateDescriptorSets;
  }
  if (strcmp(functionName, "vkAllocateMemory") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkAllocateMemory;
  }
  if (strcmp(functionName, "vkBeginCommandBuffer") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkBeginCommandBuffer;
  }
  if (strcmp(functionName, "vkBindBufferMemory") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkBindBufferMemory;
  }
  if (strcmp(functionName, "vkBindImageMemory") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkBindImageMemory;
  }
  if (strcmp(functionName, "vkCmdBindDescriptorSets") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkCmdBindDescriptorSets;
  }
  if (strcmp(functionName, "vkCmdBindIndexBuffer") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkCmdBindIndexBuffer;
  }
  if (strcmp(functionName, "vkCmdBindPipeline") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkCmdBindPipeline;
  }
  if (strcmp(functionName, "vkCmdBindVertexBuffers") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkCmdBindVertexBuffers;
  }
  if (strcmp(functionName, "vkCmdCopyBufferToImage") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkCmdCopyBufferToImage;
  }
  if (strcmp(functionName, "vkCmdDrawIndexed") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkCmdDrawIndexed;
  }
  if (strcmp(functionName, "vkCmdPipelineBarrier") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkCmdPipelineBarrier;
  }
  if (strcmp(functionName, "vkCmdPushConstants") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkCmdPushConstants;
  }
  if (strcmp(functionName, "vkCmdSetScissor") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkCmdSetScissor;
  }
  if (strcmp(functionName, "vkCmdSetViewport") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkCmdSetViewport;
  }
  if (strcmp(functionName, "vkCreateBuffer") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkCreateBuffer;
  }
  if (strcmp(functionName, "vkCreateCommandPool") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkCreateCommandPool;
  }
  if (strcmp(functionName, "vkCreateDescriptorPool") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkCreateDescriptorPool;
  }
  if (strcmp(functionName, "vkCreateDescriptorSetLayout") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkCreateDescriptorSetLayout;
  }
  if (strcmp(functionName, "vkCreateFence") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkCreateFence;
  }
  if (strcmp(functionName, "vkCreateFramebuffer") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkCreateFramebuffer;
  }
  if (strcmp(functionName, "vkCreateGraphicsPipelines") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkCreateGraphicsPipelines;
  }
  if (strcmp(functionName, "vkCreateImage") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkCreateImage;
  }
  if (strcmp(functionName, "vkCreateImageView") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkCreateImageView;
  }
  if (strcmp(functionName, "vkCreatePipelineLayout") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkCreatePipelineLayout;
  }
  if (strcmp(functionName, "vkCreateRenderPass") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkCreateRenderPass;
  }
  if (strcmp(functionName, "vkCreateSampler") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkCreateSampler;
  }
  if (strcmp(functionName, "vkCreateSemaphore") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkCreateSemaphore;
  }
  if (strcmp(functionName, "vkCreateShaderModule") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkCreateShaderModule;
  }
  if (strcmp(functionName, "vkCreateSwapchainKHR") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkCreateSwapchainKHR;
  }
  if (strcmp(functionName, "vkDestroyBuffer") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkDestroyBuffer;
  }
  if (strcmp(functionName, "vkDestroyCommandPool") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkDestroyCommandPool;
  }
  if (strcmp(functionName, "vkDestroyDescriptorPool") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkDestroyDescriptorPool;
  }
  if (strcmp(functionName, "vkDestroyDescriptorSetLayout") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkDestroyDescriptorSetLayout;
  }
  if (strcmp(functionName, "vkDestroyFence") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkDestroyFence;
  }
  if (strcmp(functionName, "vkDestroyFramebuffer") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkDestroyFramebuffer;
  }
  if (strcmp(functionName, "vkDestroyImage") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkDestroyImage;
  }
  if (strcmp(functionName, "vkDestroyImageView") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkDestroyImageView;
  }
  if (strcmp(functionName, "vkDestroyPipeline") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkDestroyPipeline;
  }
  if (strcmp(functionName, "vkDestroyPipelineLayout") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkDestroyPipelineLayout;
  }
  if (strcmp(functionName, "vkDestroyRenderPass") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkDestroyRenderPass;
  }
  if (strcmp(functionName, "vkDestroySampler") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkDestroySampler;
  }
  if (strcmp(functionName, "vkDestroySemaphore") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkDestroySemaphore;
  }
  if (strcmp(functionName, "vkDestroyShaderModule") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkDestroyShaderModule;
  }
  if (strcmp(functionName, "vkDestroySurfaceKHR") == 0) {
    return (PFN_vkVoidFunction)instanceDispatchTable->vkDestroySurfaceKHR;
  }
  if (strcmp(functionName, "vkDestroySwapchainKHR") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkDestroySwapchainKHR;
  }
  if (strcmp(functionName, "vkDeviceWaitIdle") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkDeviceWaitIdle;
  }
  if (strcmp(functionName, "vkEnumeratePhysicalDevices") == 0) {
    return (PFN_vkVoidFunction)instanceDispatchTable->vkEnumeratePhysicalDevices;
  }
  if (strcmp(functionName, "vkEndCommandBuffer") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkEndCommandBuffer;
  }
  if (strcmp(functionName, "vkFlushMappedMemoryRanges") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkFlushMappedMemoryRanges;
  }
  if (strcmp(functionName, "vkFreeCommandBuffers") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkFreeCommandBuffers;
  }
  if (strcmp(functionName, "vkFreeDescriptorSets") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkFreeDescriptorSets;
  }
  if (strcmp(functionName, "vkFreeMemory") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkFreeMemory;
  }
  if (strcmp(functionName, "vkGetBufferMemoryRequirements") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkGetBufferMemoryRequirements;
  }
  if (strcmp(functionName, "vkGetImageMemoryRequirements") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkGetImageMemoryRequirements;
  }
  if (strcmp(functionName, "vkGetPhysicalDeviceProperties") == 0) {
    return (PFN_vkVoidFunction)instanceDispatchTable->vkGetPhysicalDeviceProperties;
  }
  if (strcmp(functionName, "vkGetPhysicalDeviceMemoryProperties") == 0) {
    return (PFN_vkVoidFunction)instanceDispatchTable->vkGetPhysicalDeviceMemoryProperties;
  }
  if (strcmp(functionName, "vkGetPhysicalDeviceQueueFamilyProperties") == 0) {
    return (PFN_vkVoidFunction)instanceDispatchTable->vkGetPhysicalDeviceQueueFamilyProperties;
  }
  if (strcmp(functionName, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR") == 0) {
    return (PFN_vkVoidFunction)instanceDispatchTable->vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
  }
  if (strcmp(functionName, "vkGetPhysicalDeviceSurfaceFormatsKHR") == 0) {
    return (PFN_vkVoidFunction)instanceDispatchTable->vkGetPhysicalDeviceSurfaceFormatsKHR;
  }
  if (strcmp(functionName, "vkGetPhysicalDeviceSurfacePresentModesKHR") == 0) {
    return (PFN_vkVoidFunction)instanceDispatchTable->vkGetPhysicalDeviceSurfacePresentModesKHR;
  }
  if (strcmp(functionName, "vkGetSwapchainImagesKHR") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkGetSwapchainImagesKHR;
  }
  if (strcmp(functionName, "vkMapMemory") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkMapMemory;
  }
  if (strcmp(functionName, "vkQueueSubmit") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkQueueSubmit;
  }
  if (strcmp(functionName, "vkQueueWaitIdle") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkQueueWaitIdle;
  }
  if (strcmp(functionName, "vkResetCommandPool") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkResetCommandPool;
  }
  if (strcmp(functionName, "vkUnmapMemory") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkUnmapMemory;
  }
  if (strcmp(functionName, "vkUpdateDescriptorSets") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkUpdateDescriptorSets;
  }
  if (strcmp(functionName, "vkCmdBeginRendering") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkCmdBeginRendering;
  }
  if (strcmp(functionName, "vkCmdEndRendering") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkCmdEndRendering;
  }
  if (strcmp(functionName, "vkCmdBeginRenderingKHR") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkCmdBeginRenderingKHR;
  }
  if (strcmp(functionName, "vkCmdEndRenderingKHR") == 0) {
    return (PFN_vkVoidFunction)deviceDispatchTable->vkCmdEndRenderingKHR;
  }

  LOG_WARNING << "ImGui requested unknown Vulkan function: " << functionName;
  return nullptr;
}

#if defined(_WIN32)
void HudLayer::SetWindowHandle(HWND hwnd) {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }

  if (!hwnd) {
    GITS_ASSERT(0, "hwnd is null");

    return;
  }

  m_Hwnd = hwnd;
}
#endif

void HudLayer::CreateHudDescriptorPool() {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }

  const auto& dispatchTable = m_DispatchTablesHolder.GetDeviceDispatchTable(m_Device);

  VkDescriptorPoolSize poolSizes[] = {
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
       IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE},
  };
  VkDescriptorPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  poolInfo.maxSets = 0;
  for (const VkDescriptorPoolSize& poolSize : poolSizes) {
    poolInfo.maxSets += poolSize.descriptorCount;
  }
  poolInfo.poolSizeCount = static_cast<uint32_t>(IM_ARRAYSIZE(poolSizes));
  poolInfo.pPoolSizes = poolSizes;
  CheckVkResult(
      dispatchTable->vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_DescriptorPool));
}

bool HudLayer::SelectHudQueue() {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return false;
  }

  const auto& deviceDispatchTable = m_DispatchTablesHolder.GetDeviceDispatchTable(m_Device);

  m_HudQueue = VK_NULL_HANDLE;

  // We select the first graphics-capable queue among queues requested by the application
  for (uint32_t i = 0; i < static_cast<uint32_t>(m_QueueFamilyProperties.size()); i++) {
    if (!(m_QueueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
      continue;
    }
    if (m_RequestedQueueFamilies.find(i) == m_RequestedQueueFamilies.end()) {
      continue;
    }
    if (m_SwapchainSharingMode == VK_SHARING_MODE_CONCURRENT &&
        m_SwapchainConcurrentQueueFamilies.find(i) == m_SwapchainConcurrentQueueFamilies.end()) {
      continue;
    }

    m_HudQueueFamily = i;

    deviceDispatchTable->vkGetDeviceQueue(m_Device, i, 0, &m_HudQueue);
    if (m_HudQueue != VK_NULL_HANDLE) {
      m_QueueFamilyForQueue[m_HudQueue] = i;
      return true;
    }
  }

  return false;
}

void HudLayer::CreateHudCommandPool() {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }

  const auto& dispatchTable = m_DispatchTablesHolder.GetDeviceDispatchTable(m_Device);

  VkCommandPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = m_HudQueueFamily;
  CheckVkResult(dispatchTable->vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool));
}

void HudLayer::CreateHudCommandBuffers() {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }

  const auto& dispatchTable = m_DispatchTablesHolder.GetDeviceDispatchTable(m_Device);

  m_CommandBuffers.resize(m_ImageCount);
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = m_CommandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = m_ImageCount;
  CheckVkResult(
      dispatchTable->vkAllocateCommandBuffers(m_Device, &allocInfo, m_CommandBuffers.data()));
}

void HudLayer::CreateHudFences() {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }

  const auto& dispatchTable = m_DispatchTablesHolder.GetDeviceDispatchTable(m_Device);

  m_Fences.resize(m_ImageCount);
  VkFenceCreateInfo fenceInfo = {};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (uint32_t i = 0; i < m_ImageCount; i++) {
    CheckVkResult(dispatchTable->vkCreateFence(m_Device, &fenceInfo, nullptr, &m_Fences[i]));
  }
}

void HudLayer::CreateHudSemaphores() {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }

  const auto& dispatchTable = m_DispatchTablesHolder.GetDeviceDispatchTable(m_Device);

  m_ReleaseSemaphores.resize(m_ImageCount);
  m_HudFinishedSemaphores.resize(m_ImageCount);
  m_ReadyToPresentSemaphores.resize(m_ImageCount);
  VkSemaphoreCreateInfo semaphoreInfo = {};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  for (uint32_t i = 0; i < m_ImageCount; i++) {
    CheckVkResult(dispatchTable->vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr,
                                                   &m_ReleaseSemaphores[i]));
    CheckVkResult(dispatchTable->vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr,
                                                   &m_HudFinishedSemaphores[i]));
    CheckVkResult(dispatchTable->vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr,
                                                   &m_ReadyToPresentSemaphores[i]));
  }
}

void HudLayer::CreateHudRenderPass() {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }

  const auto& dispatchTable = m_DispatchTablesHolder.GetDeviceDispatchTable(m_Device);

  VkAttachmentDescription attachment = {};
  attachment.format = m_SurfaceFormat.format;
  attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
  attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference colorAttachment = {};
  colorAttachment.attachment = 0;
  colorAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachment;

  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &attachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;

  CheckVkResult(
      dispatchTable->vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass));
}

void HudLayer::CreateHudFramebuffers() {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }

  const auto& dispatchTable = m_DispatchTablesHolder.GetDeviceDispatchTable(m_Device);

  m_Framebuffers.resize(m_ImageCount);

  for (uint32_t i = 0; i < m_ImageCount; i++) {
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_SwapchainImages[i];
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = m_SurfaceFormat.format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    CheckVkResult(dispatchTable->vkCreateImageView(m_Device, &viewInfo, nullptr, &imageView));

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = m_RenderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &imageView;
    framebufferInfo.width = m_SwapchainExtent.width;
    framebufferInfo.height = m_SwapchainExtent.height;
    framebufferInfo.layers = 1;

    CheckVkResult(dispatchTable->vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr,
                                                     &m_Framebuffers[i]));

    // We store the image views for cleanup
    m_ImageViews.push_back(imageView);
  }
}

void HudLayer::CreateHudResources() {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }

  CreateHudCommandPool();
  CreateHudCommandBuffers();
  CreateHudFences();
  CreateHudSemaphores();
  CreateHudRenderPass();
  CreateHudFramebuffers();
  CreateHudDescriptorPool();
}

void HudLayer::DestroyHudResources() {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }

  const auto& dispatchTable = m_DispatchTablesHolder.GetDeviceDispatchTable(m_Device);

  CheckVkResult(dispatchTable->vkDeviceWaitIdle(m_Device));

  dispatchTable->vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
  m_CommandPool = VK_NULL_HANDLE;
  m_CommandBuffers.clear();

  for (auto& fence : m_Fences) {
    dispatchTable->vkDestroyFence(m_Device, fence, nullptr);
  }
  m_Fences.clear();

  for (auto& semaphore : m_ReleaseSemaphores) {
    dispatchTable->vkDestroySemaphore(m_Device, semaphore, nullptr);
  }
  m_ReleaseSemaphores.clear();

  for (auto& semaphore : m_HudFinishedSemaphores) {
    dispatchTable->vkDestroySemaphore(m_Device, semaphore, nullptr);
  }
  m_HudFinishedSemaphores.clear();

  for (auto& semaphore : m_ReadyToPresentSemaphores) {
    dispatchTable->vkDestroySemaphore(m_Device, semaphore, nullptr);
  }
  m_ReadyToPresentSemaphores.clear();

  if (m_PresentFamilyCommandPool != VK_NULL_HANDLE) {
    dispatchTable->vkDestroyCommandPool(m_Device, m_PresentFamilyCommandPool, nullptr);
    m_PresentFamilyCommandPool = VK_NULL_HANDLE;
    m_ReleaseCommandBuffers.clear();
    m_AcquireBackCommandBuffers.clear();
  }

  dispatchTable->vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
  m_RenderPass = VK_NULL_HANDLE;

  for (auto& frameBuffer : m_Framebuffers) {
    dispatchTable->vkDestroyFramebuffer(m_Device, frameBuffer, nullptr);
  }
  m_Framebuffers.clear();

  for (auto& imageView : m_ImageViews) {
    dispatchTable->vkDestroyImageView(m_Device, imageView, nullptr);
  }
  m_ImageViews.clear();

  dispatchTable->vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
  m_DescriptorPool = VK_NULL_HANDLE;
}

void HudLayer::RecreateHudResources(bool reselectQueue) {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }

  if (m_HudDisabled || m_SkipHudForSwapchain) {
    return;
  }

  if (!m_Initialized) {
    return;
  }

  const auto& dispatchTable = m_DispatchTablesHolder.GetDeviceDispatchTable(m_Device);
  CheckVkResult(dispatchTable->vkDeviceWaitIdle(m_Device));

  ImGui_ImplVulkan_Shutdown();

  DestroyHudResources();
  if (reselectQueue && !SelectHudQueue()) {
    LOG_WARNING << "Vulkan HUD skipped for swapchain: no requested graphics-capable queue family "
                   "is compatible with its sharing configuration.";
#if defined(_WIN32)
    ImGui_ImplWin32_Shutdown();
#endif
    ImGui::DestroyContext();
    m_Initialized = false;
    m_SkipHudForSwapchain = true;
    return;
  }
  CreateHudResources();

  InitializeImGuiVulkanBackend();

  CGits::Instance().GetImGuiHUD()->SetBackBufferInfo(m_SwapchainExtent.width,
                                                     m_SwapchainExtent.height, m_ImageCount);
}

void HudLayer::InitializeImGuiVulkanBackend() {
  ImGui_ImplVulkan_InitInfo initInfo = {};
  initInfo.ApiVersion = m_ApiVersion;
  initInfo.Instance = m_Instance;
  initInfo.PhysicalDevice = m_PhysicalDevice;
  initInfo.Device = m_Device;
  initInfo.QueueFamily = m_HudQueueFamily;
  initInfo.Queue = m_HudQueue;
  initInfo.DescriptorPool = m_DescriptorPool;
  initInfo.RenderPass = m_RenderPass;
  initInfo.MinImageCount = m_MinImageCount;
  initInfo.ImageCount = m_ImageCount;
  initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  initInfo.CheckVkResultFn = CheckVkResult;

  ImGui_ImplVulkan_Init(&initInfo);
}

void HudLayer::Initialize() {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }

  if (m_HudDisabled || m_SkipHudForSwapchain) {
    return;
  }

  if (m_Initialized) {
    return;
  }

#if defined(_WIN32)
  if (m_Hwnd == nullptr) {
    LOG_WARNING << "Vulkan HUD disabled: no Win32 window is associated with the swapchain.";
    m_HudDisabled = true;
    return;
  }
#endif

  if (!SelectHudQueue()) {
    LOG_WARNING << "Vulkan HUD skipped for swapchain: no requested graphics-capable queue family "
                   "is compatible with its sharing configuration.";
    m_SkipHudForSwapchain = true;
    return;
  }

  CreateHudResources();

  ImGui::CreateContext();

#if defined(_WIN32)
  if (m_Hwnd == nullptr) {
    GITS_ASSERT(0, "m_Hwnd is null");

    return;
  }

  ImGui_ImplWin32_Init(m_Hwnd);
#else
  m_PreviousFrameTime = {};
#endif

  if (!m_ApiVersion || !m_Instance || !m_PhysicalDevice || !m_Device || !m_HudQueue ||
      !m_DescriptorPool || !m_RenderPass || !m_MinImageCount || !m_ImageCount) {
    GITS_ASSERT(m_ApiVersion, "m_ApiVersion is 0");
    GITS_ASSERT(m_Instance, "m_Instance is null");
    GITS_ASSERT(m_PhysicalDevice, "m_PhysicalDevice is null");
    GITS_ASSERT(m_Device, "m_Device is null");
    GITS_ASSERT(m_HudQueue, "m_HudQueue is null");
    GITS_ASSERT(m_DescriptorPool, "m_DescriptorPool is null");
    GITS_ASSERT(m_RenderPass, "m_RenderPass is null");
    GITS_ASSERT(m_MinImageCount, "m_MinImageCount is 0");
    GITS_ASSERT(m_ImageCount, "m_ImageCount is 0");
  }

  InitializeImGuiVulkanBackend();

#if defined(_WIN32)
  CGits::Instance().GetImGuiHUD()->SetupImGUI(
      std::max(1.0f, ImGui_ImplWin32_GetDpiScaleForHwnd(m_Hwnd)));
#else
  CGits::Instance().GetImGuiHUD()->SetupImGUI(1.0f);
#endif
  CGits::Instance().GetImGuiHUD()->SetBackBufferInfo(m_SwapchainExtent.width,
                                                     m_SwapchainExtent.height, m_ImageCount);

  m_Initialized = true;
}

void HudLayer::ChangeHudQueue(uint32_t queueFamilyIndex, VkQueue queue) {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }

  if (!queue) {
    GITS_ASSERT(0, "queue is null");

    return;
  }

  m_HudQueueFamily = queueFamilyIndex;
  m_HudQueue = queue;
  RecreateHudResources(false);
}

std::mutex& HudLayer::GetQueueMutex(VkQueue queue) {
  std::lock_guard mapGuard{m_MutexMapMutex};
  return m_QueueMutexes[queue];
}

void HudLayer::RecordHudDraw(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
  const auto& dispatchTable = m_DispatchTablesHolder.GetDeviceDispatchTable(m_Device);

  VkRenderPassBeginInfo renderPassBeginInfo = {};
  renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassBeginInfo.renderPass = m_RenderPass;
  renderPassBeginInfo.framebuffer = m_Framebuffers[imageIndex];
  renderPassBeginInfo.renderArea = {{0, 0}, m_SwapchainExtent};

  dispatchTable->vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo,
                                      VK_SUBPASS_CONTENTS_INLINE);

  ImGui::Render();
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

  dispatchTable->vkCmdEndRenderPass(commandBuffer);
}

void HudLayer::Pre(vkQueuePresentKHRCommand& command) {
  std::lock_guard resourcesGuard{m_HudResourcesMutex};

  if (!Configurator::IsHudEnabledForApi(ApiBool::VK) || command.m_Skip) {
    return;
  }

  if (!m_Initialized) {
    if (m_HudDisabled || m_SkipHudForSwapchain) {
      return;
    }
    GITS_ASSERT(0, "Vulkan HUD was not initialized");

    return;
  }

  const auto& deviceDispatchTable = m_DispatchTablesHolder.GetDeviceDispatchTable(m_Device);

  const auto& queue = command.m_queue.Value;

  // Switch to the present queue if it is graphics-capable
  if (m_QueueFamilyProperties[m_QueueFamilyForQueue[queue]].queueFlags & VK_QUEUE_GRAPHICS_BIT &&
      (m_SwapchainSharingMode != VK_SHARING_MODE_CONCURRENT ||
       m_SwapchainConcurrentQueueFamilies.contains(m_QueueFamilyForQueue[queue])) &&
      (m_QueueFamilyForQueue[queue] != m_HudQueueFamily || queue != m_HudQueue)) {
    ChangeHudQueue(m_QueueFamilyForQueue[queue], queue);
  }

  const bool queueFamilyDiffers = m_QueueFamilyForQueue[queue] != m_HudQueueFamily;
  // Queue-family ownership transfer is valid only for exclusive swapchain images
  const bool queueFamilyOwnershipTransferNeeded =
      queueFamilyDiffers && (m_SwapchainSharingMode == VK_SHARING_MODE_EXCLUSIVE);

  const auto& pPresentInfo = command.m_pPresentInfo.Value;
  tl_hudPresentInfoCopy.restore = false;
  if (!pPresentInfo || pPresentInfo->swapchainCount == 0) {
    return;
  }

  const uint32_t appWaitSemaphoreCount = pPresentInfo->waitSemaphoreCount;
  const VkSemaphore* appWaitSemaphores = pPresentInfo->pWaitSemaphores;
  std::vector<VkPipelineStageFlags> appWaitDstStageMasks(appWaitSemaphoreCount,
                                                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

  if (queueFamilyOwnershipTransferNeeded && appWaitSemaphoreCount == 0) {
    std::lock_guard guard{GetQueueMutex(queue)};
    CheckVkResult(deviceDispatchTable->vkQueueWaitIdle(queue));
  }

  // We assume one swapchain
  const uint32_t imageIndex = pPresentInfo->pImageIndices[0];

  const VkImage swapchainImage = m_SwapchainImages[imageIndex];

  CheckVkResult(deviceDispatchTable->vkWaitForFences(m_Device, 1, &m_Fences[imageIndex], VK_TRUE,
                                                     UINT64_MAX));
  CheckVkResult(deviceDispatchTable->vkResetFences(m_Device, 1, &m_Fences[imageIndex]));

  CheckVkResult(deviceDispatchTable->vkResetCommandBuffer(m_CommandBuffers[imageIndex], 0));

  {
    // The backend may submit and wait on the HUD queue while uploading fonts
    std::lock_guard queueGuard{GetQueueMutex(m_HudQueue)};
    ImGui_ImplVulkan_NewFrame();
  }
#if defined(_WIN32)
  ImGui_ImplWin32_NewFrame();
#else
  // On Linux swapchain pixels and a monotonic clock replace a platform backend
  auto& io = ImGui::GetIO();
  io.DisplaySize = ImVec2(static_cast<float>(m_SwapchainExtent.width),
                          static_cast<float>(m_SwapchainExtent.height));
  io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
  const auto now = std::chrono::steady_clock::now();
  const float deltaTime = std::chrono::duration<float>(now - m_PreviousFrameTime).count();
  io.DeltaTime = m_PreviousFrameTime == std::chrono::steady_clock::time_point{} || deltaTime <= 0.0f
                     ? 1.0f / 60.0f
                     : deltaTime;
  m_PreviousFrameTime = now;
#endif
  ImGui::NewFrame();

  CGits::Instance().GetImGuiHUD()->Render();

  if (queueFamilyOwnershipTransferNeeded) {
    const uint32_t presentQueueFamily = m_QueueFamilyForQueue[queue];

    if (m_PresentFamilyCommandPool == VK_NULL_HANDLE ||
        m_PresentQueueFamily != presentQueueFamily) {
      if (m_PresentFamilyCommandPool != VK_NULL_HANDLE) {
        CheckVkResult(deviceDispatchTable->vkDeviceWaitIdle(m_Device));
        deviceDispatchTable->vkDestroyCommandPool(m_Device, m_PresentFamilyCommandPool, nullptr);
        m_ReleaseCommandBuffers.clear();
        m_AcquireBackCommandBuffers.clear();
      }
      m_PresentQueueFamily = presentQueueFamily;

      VkCommandPoolCreateInfo poolInfo = {};
      poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
      poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
      poolInfo.queueFamilyIndex = presentQueueFamily;
      CheckVkResult(deviceDispatchTable->vkCreateCommandPool(m_Device, &poolInfo, nullptr,
                                                             &m_PresentFamilyCommandPool));

      VkCommandBufferAllocateInfo allocInfo = {};
      allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      allocInfo.commandPool = m_PresentFamilyCommandPool;
      allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      allocInfo.commandBufferCount = m_ImageCount;

      m_ReleaseCommandBuffers.resize(m_ImageCount);
      CheckVkResult(deviceDispatchTable->vkAllocateCommandBuffers(m_Device, &allocInfo,
                                                                  m_ReleaseCommandBuffers.data()));

      m_AcquireBackCommandBuffers.resize(m_ImageCount);
      CheckVkResult(deviceDispatchTable->vkAllocateCommandBuffers(
          m_Device, &allocInfo, m_AcquireBackCommandBuffers.data()));
    }

    // Release from the present queue
    CheckVkResult(
        deviceDispatchTable->vkResetCommandBuffer(m_ReleaseCommandBuffers[imageIndex], 0));

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    deviceDispatchTable->vkBeginCommandBuffer(m_ReleaseCommandBuffers[imageIndex], &beginInfo);

    VkImageMemoryBarrier releaseBarrier = {};
    releaseBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    releaseBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
    releaseBarrier.dstAccessMask = 0;
    releaseBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    releaseBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    releaseBarrier.srcQueueFamilyIndex = presentQueueFamily;
    releaseBarrier.dstQueueFamilyIndex = m_HudQueueFamily;
    releaseBarrier.image = swapchainImage;
    releaseBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0,
                                       VK_REMAINING_ARRAY_LAYERS};

    deviceDispatchTable->vkCmdPipelineBarrier(
        m_ReleaseCommandBuffers[imageIndex], VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &releaseBarrier);

    deviceDispatchTable->vkEndCommandBuffer(m_ReleaseCommandBuffers[imageIndex]);

    VkPipelineStageFlags releaseWaitStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    VkSubmitInfo releaseSubmitInfo = {};
    releaseSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    releaseSubmitInfo.waitSemaphoreCount = appWaitSemaphoreCount;
    releaseSubmitInfo.pWaitSemaphores = appWaitSemaphores;
    releaseSubmitInfo.pWaitDstStageMask =
        appWaitSemaphoreCount ? appWaitDstStageMasks.data() : nullptr;
    releaseSubmitInfo.commandBufferCount = 1;
    releaseSubmitInfo.pCommandBuffers = &m_ReleaseCommandBuffers[imageIndex];
    releaseSubmitInfo.signalSemaphoreCount = 1;
    releaseSubmitInfo.pSignalSemaphores = &m_ReleaseSemaphores[imageIndex];

    {
      std::lock_guard guard{GetQueueMutex(queue)};
      CheckVkResult(
          deviceDispatchTable->vkQueueSubmit(queue, 1, &releaseSubmitInfo, VK_NULL_HANDLE));
    }

    // Acquire on the hud queue, draw, and release back
    VkCommandBufferBeginInfo hudBeginInfo = {};
    hudBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    hudBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    deviceDispatchTable->vkBeginCommandBuffer(m_CommandBuffers[imageIndex], &hudBeginInfo);

    VkImageMemoryBarrier acquireBarrier = {};
    acquireBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    acquireBarrier.srcAccessMask = 0;
    acquireBarrier.dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    acquireBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    acquireBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    acquireBarrier.srcQueueFamilyIndex = presentQueueFamily;
    acquireBarrier.dstQueueFamilyIndex = m_HudQueueFamily;
    acquireBarrier.image = swapchainImage;
    acquireBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0,
                                       VK_REMAINING_ARRAY_LAYERS};

    deviceDispatchTable->vkCmdPipelineBarrier(m_CommandBuffers[imageIndex],
                                              VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0,
                                              nullptr, 0, nullptr, 1, &acquireBarrier);

    RecordHudDraw(m_CommandBuffers[imageIndex], imageIndex);

    // Release back to present queue
    VkImageMemoryBarrier releaseBackBarrier = {};
    releaseBackBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    releaseBackBarrier.srcAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    releaseBackBarrier.dstAccessMask = 0;
    releaseBackBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    releaseBackBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    releaseBackBarrier.srcQueueFamilyIndex = m_HudQueueFamily;
    releaseBackBarrier.dstQueueFamilyIndex = presentQueueFamily;
    releaseBackBarrier.image = swapchainImage;
    releaseBackBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0,
                                           VK_REMAINING_ARRAY_LAYERS};

    deviceDispatchTable->vkCmdPipelineBarrier(
        m_CommandBuffers[imageIndex], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &releaseBackBarrier);

    deviceDispatchTable->vkEndCommandBuffer(m_CommandBuffers[imageIndex]);

    VkSubmitInfo hudSubmitInfo = {};
    hudSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    hudSubmitInfo.waitSemaphoreCount = 1;
    hudSubmitInfo.pWaitSemaphores = &m_ReleaseSemaphores[imageIndex];
    hudSubmitInfo.pWaitDstStageMask = &releaseWaitStage;
    hudSubmitInfo.commandBufferCount = 1;
    hudSubmitInfo.pCommandBuffers = &m_CommandBuffers[imageIndex];
    hudSubmitInfo.signalSemaphoreCount = 1;
    hudSubmitInfo.pSignalSemaphores = &m_HudFinishedSemaphores[imageIndex];

    {
      std::lock_guard guard{GetQueueMutex(m_HudQueue)};
      CheckVkResult(
          deviceDispatchTable->vkQueueSubmit(m_HudQueue, 1, &hudSubmitInfo, VK_NULL_HANDLE));
    }

    // Acquire back on present queue
    CheckVkResult(
        deviceDispatchTable->vkResetCommandBuffer(m_AcquireBackCommandBuffers[imageIndex], 0));

    deviceDispatchTable->vkBeginCommandBuffer(m_AcquireBackCommandBuffers[imageIndex], &beginInfo);

    VkImageMemoryBarrier acquireBackBarrier = {};
    acquireBackBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    acquireBackBarrier.srcAccessMask = 0;
    acquireBackBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    acquireBackBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    acquireBackBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    acquireBackBarrier.srcQueueFamilyIndex = m_HudQueueFamily;
    acquireBackBarrier.dstQueueFamilyIndex = presentQueueFamily;
    acquireBackBarrier.image = swapchainImage;
    acquireBackBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0,
                                           VK_REMAINING_ARRAY_LAYERS};

    deviceDispatchTable->vkCmdPipelineBarrier(
        m_AcquireBackCommandBuffers[imageIndex], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &acquireBackBarrier);

    deviceDispatchTable->vkEndCommandBuffer(m_AcquireBackCommandBuffers[imageIndex]);

    VkPipelineStageFlags hudWaitStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    VkSubmitInfo acquireBackSubmitInfo = {};
    acquireBackSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    acquireBackSubmitInfo.waitSemaphoreCount = 1;
    acquireBackSubmitInfo.pWaitSemaphores = &m_HudFinishedSemaphores[imageIndex];
    acquireBackSubmitInfo.pWaitDstStageMask = &hudWaitStage;
    acquireBackSubmitInfo.commandBufferCount = 1;
    acquireBackSubmitInfo.pCommandBuffers = &m_AcquireBackCommandBuffers[imageIndex];
    acquireBackSubmitInfo.signalSemaphoreCount = 1;
    acquireBackSubmitInfo.pSignalSemaphores = &m_ReadyToPresentSemaphores[imageIndex];

    {
      std::lock_guard guard{GetQueueMutex(queue)};
      CheckVkResult(deviceDispatchTable->vkQueueSubmit(queue, 1, &acquireBackSubmitInfo,
                                                       m_Fences[imageIndex]));
    }
  } else {
    // Same-family or concurrent sharing: only layout transitions are needed

    VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
    cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CheckVkResult(deviceDispatchTable->vkBeginCommandBuffer(m_CommandBuffers[imageIndex],
                                                            &cmdBufferBeginInfo));

    VkImageMemoryBarrier transitionToRenderBarrier = {};
    transitionToRenderBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    transitionToRenderBarrier.srcAccessMask =
        VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
    transitionToRenderBarrier.dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    transitionToRenderBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    transitionToRenderBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    transitionToRenderBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    transitionToRenderBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    transitionToRenderBarrier.image = swapchainImage;
    transitionToRenderBarrier.subresourceRange = {
        VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS};

    deviceDispatchTable->vkCmdPipelineBarrier(m_CommandBuffers[imageIndex],
                                              VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0,
                                              nullptr, 0, nullptr, 1, &transitionToRenderBarrier);

    RecordHudDraw(m_CommandBuffers[imageIndex], imageIndex);

    VkImageMemoryBarrier transitionToPresentBarrier = {};
    transitionToPresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    transitionToPresentBarrier.srcAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    transitionToPresentBarrier.dstAccessMask = 0;
    transitionToPresentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    transitionToPresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    transitionToPresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    transitionToPresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    transitionToPresentBarrier.image = swapchainImage;
    transitionToPresentBarrier.subresourceRange = {
        VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS};

    deviceDispatchTable->vkCmdPipelineBarrier(m_CommandBuffers[imageIndex],
                                              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                              VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr,
                                              0, nullptr, 1, &transitionToPresentBarrier);

    CheckVkResult(deviceDispatchTable->vkEndCommandBuffer(m_CommandBuffers[imageIndex]));

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = appWaitSemaphoreCount;
    submitInfo.pWaitSemaphores = appWaitSemaphores;
    submitInfo.pWaitDstStageMask = appWaitSemaphoreCount ? appWaitDstStageMasks.data() : nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_CommandBuffers[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &m_ReadyToPresentSemaphores[imageIndex];

    {
      std::lock_guard guard{GetQueueMutex(m_HudQueue)};
      CheckVkResult(
          deviceDispatchTable->vkQueueSubmit(m_HudQueue, 1, &submitInfo, m_Fences[imageIndex]));
    }
  }

  tl_hudPresentInfoCopy.waitSemaphoreCount = pPresentInfo->waitSemaphoreCount;
  tl_hudPresentInfoCopy.pWaitSemaphores = pPresentInfo->pWaitSemaphores;
  tl_hudPresentInfoCopy.restore = true;

  tl_hudModifiedPresentWaitSemaphores.clear();
  tl_hudModifiedPresentWaitSemaphores.push_back(m_ReadyToPresentSemaphores[imageIndex]);
  pPresentInfo->waitSemaphoreCount =
      static_cast<uint32_t>(tl_hudModifiedPresentWaitSemaphores.size());
  pPresentInfo->pWaitSemaphores = tl_hudModifiedPresentWaitSemaphores.data();
}

void HudLayer::Pre(vkDestroyDeviceCommand& command) {
  std::lock_guard resourcesGuard{m_HudResourcesMutex};

  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }

  if (command.m_device.Value != m_Device || !m_Initialized) {
    return;
  }

  const auto& dispatchTable = m_DispatchTablesHolder.GetDeviceDispatchTable(m_Device);
  CheckVkResult(dispatchTable->vkDeviceWaitIdle(m_Device));

  ImGui_ImplVulkan_Shutdown();
#if defined(_WIN32)
  ImGui_ImplWin32_Shutdown();
#endif
  ImGui::DestroyContext();

  DestroyHudResources();

  m_Initialized = false;
}

void HudLayer::Post(vkCreateInstanceCommand& command) {
  std::lock_guard resourcesGuard{m_HudResourcesMutex};

  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }

  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }

  const auto& pCreateInfo = command.m_pCreateInfo.Value;
  const auto& pInstance = command.m_pInstance.Value;

  if (!pCreateInfo || !pInstance || !*pInstance) {
    GITS_ASSERT(pCreateInfo, "pCreateinfo is nullptr");
    GITS_ASSERT(pInstance, "pInstance is nullptr");
    GITS_ASSERT(*pInstance, "Instance is nullptr");

    return;
  }

  m_ApiVersion = pCreateInfo->pApplicationInfo && pCreateInfo->pApplicationInfo->apiVersion
                     ? pCreateInfo->pApplicationInfo->apiVersion
                     : VK_API_VERSION_1_0;
  m_Instance = *pInstance;
}

#if defined(_WIN32)
void HudLayer::Post(vkCreateWin32SurfaceKHRCommand& command) {
  std::lock_guard resourcesGuard{m_HudResourcesMutex};

  SetWindowHandle(command.m_pCreateInfo.Value->hwnd);
}
#endif

void HudLayer::Post(vkCreateDeviceCommand& command) {
  std::lock_guard resourcesGuard{m_HudResourcesMutex};

  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }

  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }

  const auto& physicalDevice = command.m_physicalDevice.Value;
  const auto& pDevice = command.m_pDevice.Value;
  const auto& pCreateInfo = command.m_pCreateInfo.Value;

  if (!physicalDevice || !pDevice || !*pDevice) {
    GITS_ASSERT(physicalDevice, "physicalDevice is null");
    GITS_ASSERT(pDevice, "pDevice is nullptr");
    GITS_ASSERT(*pDevice, "Device is nullptr");

    return;
  }

  m_PhysicalDevice = physicalDevice;
  m_Device = *pDevice;
  m_HudDisabled = false;
  m_SkipHudForSwapchain = false;

  m_RequestedQueueFamilies.clear();
  if (pCreateInfo && pCreateInfo->pQueueCreateInfos) {
    for (uint32_t i = 0; i < pCreateInfo->queueCreateInfoCount; ++i) {
      m_RequestedQueueFamilies.insert(pCreateInfo->pQueueCreateInfos[i].queueFamilyIndex);
    }
  }

  const auto& instanceDispatchTable =
      m_DispatchTablesHolder.GetInstanceDispatchTable(physicalDevice);
  uint32_t queueFamilyCount = 0;
  instanceDispatchTable->vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                                                  nullptr);
  m_QueueFamilyProperties.resize(queueFamilyCount);
  instanceDispatchTable->vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                                                  m_QueueFamilyProperties.data());
}

void HudLayer::Post(vkCreateSwapchainKHRCommand& command) {
  std::lock_guard resourcesGuard{m_HudResourcesMutex};

  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }

  if (command.m_Return.Value != VK_SUCCESS) {
    return;
  }

  const auto& device = command.m_device.Value;
  const auto& pCreateInfo = command.m_pCreateInfo.Value;
  const auto& pSwapchain = command.m_pSwapchain.Value;

  if (!pCreateInfo || !pSwapchain || !*pSwapchain) {
    GITS_ASSERT(pCreateInfo, "pCreateInfo is nullptr");
    GITS_ASSERT(pSwapchain, "pSwapchain is nullptr");
    GITS_ASSERT(*pSwapchain, "Swapchain is null");

    return;
  }

  const bool supportsColorAttachment =
      (pCreateInfo->imageUsage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) != 0;
  if (!supportsColorAttachment) {
    LOG_WARNING << "Vulkan HUD skipped for swapchain: imageUsage does not include "
                   "VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT.";
    m_SkipHudForSwapchain = true;

    if (m_Initialized) {
      const auto& dispatchTable = m_DispatchTablesHolder.GetDeviceDispatchTable(m_Device);
      CheckVkResult(dispatchTable->vkDeviceWaitIdle(m_Device));

      ImGui_ImplVulkan_Shutdown();
#if defined(_WIN32)
      ImGui_ImplWin32_Shutdown();
#endif
      ImGui::DestroyContext();
      DestroyHudResources();
      m_Initialized = false;
    }

    return;
  }

  m_SkipHudForSwapchain = false;

  if (!m_ImGuiFunctionsLoaded) {
    m_ImGuiFunctionsLoaded =
        ImGui_ImplVulkan_LoadFunctions(m_ApiVersion, HudLayer::ImGuiVulkanLoader, this);
    if (!m_ImGuiFunctionsLoaded) {
      LOG_WARNING << "Vulkan HUD disabled: failed to load ImGui Vulkan functions.";
      m_HudDisabled = true;
      return;
    }
  }

  m_MinImageCount = pCreateInfo->minImageCount;
  uint32_t imageCount;

  const auto& dispatchTable = m_DispatchTablesHolder.GetDeviceDispatchTable(device);

  CheckVkResult(dispatchTable->vkGetSwapchainImagesKHR(device, *pSwapchain, &imageCount, nullptr));
  m_ImageCount = imageCount;
  m_SwapchainImages.clear();
  m_SwapchainImages.resize(imageCount);
  CheckVkResult(dispatchTable->vkGetSwapchainImagesKHR(device, *pSwapchain, &imageCount,
                                                       m_SwapchainImages.data()));
  m_SwapchainSharingMode = pCreateInfo->imageSharingMode;
  m_SwapchainConcurrentQueueFamilies.clear();
  if (m_SwapchainSharingMode == VK_SHARING_MODE_CONCURRENT) {
    for (uint32_t i = 0; i < pCreateInfo->queueFamilyIndexCount; ++i) {
      m_SwapchainConcurrentQueueFamilies.insert(pCreateInfo->pQueueFamilyIndices[i]);
    }
  }
  m_SwapchainExtent = pCreateInfo->imageExtent;
  m_SurfaceFormat.format = pCreateInfo->imageFormat;
  m_SurfaceFormat.colorSpace = pCreateInfo->imageColorSpace;
  m_Swapchain = *pSwapchain;

  if (m_Initialized) {
    RecreateHudResources(true);
  } else {
    Initialize();
  }
}

void HudLayer::Post(vkGetDeviceQueueCommand& command) {
  std::lock_guard resourcesGuard{m_HudResourcesMutex};

  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }

  if (command.m_pQueue.Value == nullptr) {
    GITS_ASSERT(command.m_pQueue.Value, "pQueue is nullptr");

    return;
  }

  m_QueueFamilyForQueue[*command.m_pQueue.Value] = command.m_queueFamilyIndex.Value;
}

void HudLayer::Post(vkGetDeviceQueue2Command& command) {
  std::lock_guard resourcesGuard{m_HudResourcesMutex};

  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }

  if (command.m_pQueue.Value == nullptr || command.m_pQueueInfo.Value == nullptr) {
    GITS_ASSERT(command.m_pQueue.Value, "pQueue is nullptr");
    GITS_ASSERT(command.m_pQueueInfo.Value, "pQueueInfo is nullptr");

    return;
  }

  m_QueueFamilyForQueue[*command.m_pQueue.Value] = command.m_pQueueInfo.Value->queueFamilyIndex;
}

void HudLayer::Post(vkQueuePresentKHRCommand& command) {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }

  CGits::Instance().FrameCountUp();

  if (tl_hudPresentInfoCopy.restore && command.m_pPresentInfo.Value) {
    command.m_pPresentInfo.Value->waitSemaphoreCount = tl_hudPresentInfoCopy.waitSemaphoreCount;
    command.m_pPresentInfo.Value->pWaitSemaphores = tl_hudPresentInfoCopy.pWaitSemaphores;
  }

  tl_hudPresentInfoCopy.restore = false;
}

void HudLayer::Pre(vkQueueSubmitCommand& command) {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }
  GetQueueMutex(command.m_queue.Value).lock();
}

void HudLayer::Pre(vkQueueSubmit2Command& command) {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }
  GetQueueMutex(command.m_queue.Value).lock();
}

void HudLayer::Pre(vkQueueSubmit2KHRCommand& command) {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }
  GetQueueMutex(command.m_queue.Value).lock();
}

void HudLayer::Pre(vkQueueBindSparseCommand& command) {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }
  GetQueueMutex(command.m_queue.Value).lock();
}

void HudLayer::Pre(vkQueueWaitIdleCommand& command) {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }
  GetQueueMutex(command.m_queue.Value).lock();
}

void HudLayer::Post(vkQueueSubmitCommand& command) {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }
  GetQueueMutex(command.m_queue.Value).unlock();
}

void HudLayer::Post(vkQueueSubmit2Command& command) {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }
  GetQueueMutex(command.m_queue.Value).unlock();
}

void HudLayer::Post(vkQueueSubmit2KHRCommand& command) {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }
  GetQueueMutex(command.m_queue.Value).unlock();
}

void HudLayer::Post(vkQueueBindSparseCommand& command) {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }
  GetQueueMutex(command.m_queue.Value).unlock();
}

void HudLayer::Post(vkQueueWaitIdleCommand& command) {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }
  GetQueueMutex(command.m_queue.Value).unlock();
}

} // namespace vulkan
} // namespace gits

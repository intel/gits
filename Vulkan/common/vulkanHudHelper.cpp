// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "include/vulkanHudHelper.h"
#include "include/vulkanDrivers.h"
#include "log.h"
#include "gits.h"

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_vulkan.h>
#include "imGuiHud.h"

#include <cassert>
#include <utility>

namespace gits {
namespace Vulkan {

namespace {

void CheckVkResult(VkResult result) {
  if (result != VK_SUCCESS) {
    assert(0 && ("Vulkan call failed. VkResult: " + std::to_string(result)).c_str());
  }
}

} // namespace

VulkanHudHelper::VulkanHudHelper() {}

void VulkanHudHelper::CreateHudDescriptorPool() {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }

  VkDescriptorPoolSize pool_sizes[] = {
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
       IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE},
  };
  VkDescriptorPoolCreateInfo pool_info = {};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  pool_info.maxSets = 0;
  for (VkDescriptorPoolSize& pool_size : pool_sizes) {
    pool_info.maxSets += pool_size.descriptorCount;
  }
  pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
  pool_info.pPoolSizes = pool_sizes;
  CheckVkResult(drvVk.vkCreateDescriptorPool(m_Device, &pool_info, nullptr, &m_DescriptorPool));
}

void VulkanHudHelper::SelectHudQueue() {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }

  uint32_t queueFamilyCount = 0;
  drvVk.vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);
  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  drvVk.vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount,
                                                 queueFamilies.data());

  // We select the first graphics-capable queue found
  for (uint32_t i = 0; i < queueFamilyCount; i++) {
    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      m_HudQueueFamily = i;

      drvVk.vkGetDeviceQueue(m_Device, i, 0, &m_HudQueue);
      m_QueueFamilyForQueue[m_HudQueue] = i;

      return;
    }
  }
}

void VulkanHudHelper::CreateHudCommandPool() {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }

  VkCommandPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = m_HudQueueFamily;
  CheckVkResult(drvVk.vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool));
}
void VulkanHudHelper::CreateHudCommandBuffers() {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }

  m_CommandBuffers.resize(m_ImageCount);
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = m_CommandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = m_ImageCount;
  CheckVkResult(drvVk.vkAllocateCommandBuffers(m_Device, &allocInfo, m_CommandBuffers.data()));
}
void VulkanHudHelper::CreateHudFences() {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }

  m_Fences.resize(m_ImageCount);
  VkFenceCreateInfo fenceInfo = {};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (uint32_t i = 0; i < m_ImageCount; i++) {
    CheckVkResult(drvVk.vkCreateFence(m_Device, &fenceInfo, nullptr, &m_Fences[i]));
  }
}
void VulkanHudHelper::CreateHudRenderPass() {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }

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

  CheckVkResult(drvVk.vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass));
}
void VulkanHudHelper::CreateHudFramebuffers() {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }

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
    CheckVkResult(drvVk.vkCreateImageView(m_Device, &viewInfo, nullptr, &imageView));

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = m_RenderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &imageView;
    framebufferInfo.width = m_SwapchainExtent.width;
    framebufferInfo.height = m_SwapchainExtent.height;
    framebufferInfo.layers = 1;

    CheckVkResult(
        drvVk.vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr, &m_Framebuffers[i]));

    // We store the image views for cleanup
    m_ImageViews.push_back(imageView);
  }
}
void VulkanHudHelper::CreateHudResources() {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }

  CreateHudCommandPool();
  CreateHudCommandBuffers();
  CreateHudFences();
  CreateHudRenderPass();
  CreateHudFramebuffers();
  CreateHudDescriptorPool();
}

void VulkanHudHelper::DestroyHudResources() {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }

  CheckVkResult(drvVk.vkDeviceWaitIdle(m_Device));

  ImGui_ImplVulkan_Shutdown();

  drvVk.vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
  m_CommandPool = VK_NULL_HANDLE;
  m_CommandBuffers.clear();

  for (auto& fence : m_Fences) {
    drvVk.vkDestroyFence(m_Device, fence, nullptr);
  }
  m_Fences.clear();

  drvVk.vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
  m_RenderPass = VK_NULL_HANDLE;

  for (auto& imageView : m_ImageViews) {
    drvVk.vkDestroyImageView(m_Device, imageView, nullptr);
  }
  m_ImageViews.clear();

  for (auto& frameBuffer : m_Framebuffers) {
    drvVk.vkDestroyFramebuffer(m_Device, frameBuffer, nullptr);
  }
  m_Framebuffers.clear();

  drvVk.vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
  m_DescriptorPool = VK_NULL_HANDLE;
}

void VulkanHudHelper::RecreateHudResources() {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }

  if (!m_Initialized) {
    return;
  }

  DestroyHudResources();
  SelectHudQueue();
  CreateHudResources();

  // Reinitialize ImGui Vulkan backend with new resources
  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.ApiVersion = m_ApiVersion;
  init_info.Instance = m_Instance;
  init_info.PhysicalDevice = m_PhysicalDevice;
  init_info.Device = m_Device;
  init_info.QueueFamily = m_HudQueueFamily;
  init_info.Queue = m_HudQueue;
  init_info.DescriptorPool = m_DescriptorPool;
  init_info.RenderPass = m_RenderPass;
  init_info.MinImageCount = m_MinImageCount;
  init_info.ImageCount = m_ImageCount;
  init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  init_info.CheckVkResultFn = CheckVkResult;

  ImGui_ImplVulkan_Init(&init_info);

  CGits::Instance().GetImGuiHUD()->SetBackBufferInfo(m_SwapchainExtent.width,
                                                     m_SwapchainExtent.height, m_ImageCount);
}

void VulkanHudHelper::Initialize() {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }

  if (m_Initialized) {
    return;
  }

  SelectHudQueue();
  CreateHudResources();

  ImGui::CreateContext();

  if (m_Hwnd == NULL) {
    assert(0 && "m_Hwnd is null");

    return;
  }

  ImGui_ImplWin32_Init(m_Hwnd);

  if (!m_ApiVersion || !m_Instance || !m_PhysicalDevice || !m_Device || !m_HudQueue ||
      !m_DescriptorPool || !m_RenderPass || !m_MinImageCount || !m_ImageCount) {
    assert(m_ApiVersion && "m_ApiVersion is 0");
    assert(m_Instance && "m_Instance is null");
    assert(m_PhysicalDevice && "m_PhysicalDevice is null");
    assert(m_Device && "m_Device is null");
    assert(m_HudQueue && "m_HudQueue is null");
    assert(m_DescriptorPool && "m_DescriptorPool is null");
    assert(m_RenderPass && "m_RenderPass is null");
    assert(m_MinImageCount && "m_MinImageCount is 0");
    assert(m_ImageCount && "m_ImageCount is 0");
  }

  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.ApiVersion = m_ApiVersion;
  init_info.Instance = m_Instance;
  init_info.PhysicalDevice = m_PhysicalDevice;
  init_info.Device = m_Device;
  init_info.QueueFamily = m_HudQueueFamily;
  init_info.Queue = m_HudQueue;
  init_info.DescriptorPool = m_DescriptorPool;
  init_info.RenderPass = m_RenderPass;
  init_info.MinImageCount = m_MinImageCount;
  init_info.ImageCount = m_ImageCount;
  init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  init_info.CheckVkResultFn = CheckVkResult;

  ImGui_ImplVulkan_Init(&init_info);

  CGits::Instance().GetImGuiHUD()->SetupImGUI(
      std::max(1.0f, ImGui_ImplWin32_GetDpiScaleForHwnd(m_Hwnd)));
  CGits::Instance().GetImGuiHUD()->SetBackBufferInfo(m_SwapchainExtent.width,
                                                     m_SwapchainExtent.height, m_ImageCount);

  m_Initialized = true;
}

void VulkanHudHelper::ChangeHudQueue(uint32_t queueFamilyIndex, VkQueue queue) {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }

  if (!queue) {
    assert(0 && "queue is null");

    return;
  }

  m_HudQueueFamily = queueFamilyIndex;
  m_HudQueue = queue;
  RecreateHudResources();
}

void VulkanHudHelper::OnVkCreateInstance(const VkInstanceCreateInfo* pCreateInfo,
                                         const VkAllocationCallbacks* pAllocator,
                                         VkInstance* pInstance) {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }

  if (!pCreateInfo || !pInstance || !*pInstance) {
    assert(pCreateInfo && "pCreateinfo is nullptr");
    assert(pInstance && "pInstance is nullptr");
    assert(*pInstance && "Instance is nullptr");

    return;
  }

  m_ApiVersion = pCreateInfo->pApplicationInfo->apiVersion;
  m_Instance = *pInstance;
}

void VulkanHudHelper::SetWindowHandle(HWND hwnd) {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }

  if (!hwnd) {
    assert(0 && "hwnd is null");

    return;
  }

  m_Hwnd = hwnd;
}

void VulkanHudHelper::OnVkCreateDevice(VkPhysicalDevice physicalDevice,
                                       const VkDeviceCreateInfo* pCreateInfo,
                                       const VkAllocationCallbacks* pAllocator,
                                       VkDevice* pDevice) {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }

  if (!physicalDevice || !pDevice || !*pDevice) {
    assert(physicalDevice && "physicalDevice is null");
    assert(pDevice && "pDevice is nullptr");
    assert(*pDevice && "Device is nullptr");

    return;
  }

  m_PhysicalDevice = physicalDevice;
  m_Device = *pDevice;
}

void VulkanHudHelper::OnVkCreateSwapchainKHR(VkDevice device,
                                             const VkSwapchainCreateInfoKHR* pCreateInfo,
                                             const VkAllocationCallbacks* pAllocator,
                                             VkSwapchainKHR* pSwapchain) {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }

  if (!pCreateInfo || !pSwapchain || !*pSwapchain) {
    assert(pCreateInfo && "pCreateInfo is nullptr");
    assert(pSwapchain && "pSwapchain is nullptr");
    assert(*pSwapchain && "Swapchain is null");

    return;
  }

  m_MinImageCount = pCreateInfo->minImageCount;
  uint32_t imageCount;
  CheckVkResult(drvVk.vkGetSwapchainImagesKHR(device, *pSwapchain, &imageCount, nullptr));
  m_ImageCount = imageCount;
  m_SwapchainImages.clear();
  m_SwapchainImages.resize(imageCount);
  CheckVkResult(
      drvVk.vkGetSwapchainImagesKHR(device, *pSwapchain, &imageCount, m_SwapchainImages.data()));
  m_SwapchainExtent = pCreateInfo->imageExtent;
  m_SurfaceFormat.format = pCreateInfo->imageFormat;
  m_SurfaceFormat.colorSpace = pCreateInfo->imageColorSpace;
  m_Swapchain = *pSwapchain;

  if (m_Initialized) {
    RecreateHudResources();
  } else {
    Initialize();
  }
}

void VulkanHudHelper::OnVkGetDeviceQueue(VkDevice device,
                                         uint32_t queueFamilyIndex,
                                         uint32_t queueIndex,
                                         VkQueue* pQueue) {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }

  m_QueueFamilyForQueue[*pQueue] = queueFamilyIndex;
}

VkResult CreateOneTimeCommandBuffer(VkDevice device,
                                    uint32_t queueFamilyIndex,
                                    VkCommandPool* pCommandPool,
                                    VkCommandBuffer* pCommandBuffer) {
  // We need a command buffer allocated from the same queue family as the queue we are transferring from / to
  // TODO: Improve how we allocate command buffers for the ownership transfer operations
  VkCommandPoolCreateInfo poolCreateInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr,
                                            VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, queueFamilyIndex};
  auto result = drvVk.vkCreateCommandPool(device, &poolCreateInfo, NULL, pCommandPool);
  if (result != VK_SUCCESS) {
    return result;
  }

  VkCommandBufferAllocateInfo bufferAllocateInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                                    nullptr, *pCommandPool,
                                                    VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1};
  result = drvVk.vkAllocateCommandBuffers(device, &bufferAllocateInfo, pCommandBuffer);
  if (result != VK_SUCCESS) {
    drvVk.vkDestroyCommandPool(device, *pCommandPool, NULL);
    return result;
  }

  return VK_SUCCESS;
}

void VulkanHudHelper::OnVkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR* pPresentInfo) {
  if (!Configurator::IsHudEnabledForApi(ApiBool::VK)) {
    return;
  }

  if (!m_Initialized) {
    assert(0 && "Vulkan HUD was not initialized");

    return;
  }

  // Check the properties of the present queue's family
  uint32_t queueFamilyPropertiesCount = 0;
  drvVk.vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyPropertiesCount,
                                                 nullptr);
  std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertiesCount);
  drvVk.vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyPropertiesCount,
                                                 queueFamilyProperties.data());

  if (queueFamilyProperties[m_QueueFamilyForQueue[queue]].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
    // If the present queue is graphics-capable we can use it for hud rendering
    ChangeHudQueue(m_QueueFamilyForQueue[queue], queue);
  }

  // If the present queue family differs from the imgui rendering queue family, we need an ownership transfer between them
  bool queueFamilyDiffers = false;

  if (m_QueueFamilyForQueue[queue] != m_HudQueueFamily) {
    queueFamilyDiffers = true;
  }

  // We assume one swapchain
  uint32_t imageIndex = pPresentInfo->pImageIndices[0];
  VkSwapchainKHR swapchain = pPresentInfo->pSwapchains[0];
  VkImage swapchainImage = m_SwapchainImages[imageIndex];

  CheckVkResult(drvVk.vkWaitForFences(m_Device, 1, &m_Fences[imageIndex], VK_TRUE, UINT64_MAX));
  CheckVkResult(drvVk.vkResetFences(m_Device, 1, &m_Fences[imageIndex]));

  CheckVkResult(drvVk.vkResetCommandBuffer(m_CommandBuffers[imageIndex], 0));

  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();

  CGits::Instance().GetImGuiHUD()->Render();

  if (queueFamilyDiffers) {
    // Present queue family and the hud queue family differ, ownership transfer is needed

    // Release from the present queue
    VkCommandPool releaseCommandPool = VK_NULL_HANDLE;
    VkCommandBuffer releaseCommandBuffer = VK_NULL_HANDLE;

    CreateOneTimeCommandBuffer(m_Device, m_QueueFamilyForQueue[queue], &releaseCommandPool,
                               &releaseCommandBuffer);

    VkCommandBufferBeginInfo releaseCommandBufferBeginInfo = {};
    releaseCommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    releaseCommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    drvVk.vkBeginCommandBuffer(releaseCommandBuffer, &releaseCommandBufferBeginInfo);

    VkImageMemoryBarrier releaseBarrier = {};
    releaseBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    releaseBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
    releaseBarrier.dstAccessMask = 0;
    releaseBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    releaseBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    releaseBarrier.srcQueueFamilyIndex = m_QueueFamilyForQueue[queue];
    releaseBarrier.dstQueueFamilyIndex = m_HudQueueFamily;
    releaseBarrier.image = swapchainImage;
    releaseBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0,
                                       VK_REMAINING_ARRAY_LAYERS};

    drvVk.vkCmdPipelineBarrier(releaseCommandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                               VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1,
                               &releaseBarrier);

    drvVk.vkEndCommandBuffer(releaseCommandBuffer);

    VkSubmitInfo releaseSubmitInfo = {};
    releaseSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    releaseSubmitInfo.commandBufferCount = 1;
    releaseSubmitInfo.pCommandBuffers = &releaseCommandBuffer;

    CheckVkResult(drvVk.vkQueueSubmit(queue, 1, &releaseSubmitInfo, VK_NULL_HANDLE));

    // TODO: Improve synchronization
    drvVk.vkQueueWaitIdle(queue);

    drvVk.vkDestroyCommandPool(m_Device, releaseCommandPool, nullptr);

    // Acquire on the hud queue
    VkCommandPool hudCommandPool = VK_NULL_HANDLE;
    VkCommandBuffer hudCommandBuffer = VK_NULL_HANDLE;

    CreateOneTimeCommandBuffer(m_Device, m_HudQueueFamily, &hudCommandPool, &hudCommandBuffer);

    VkCommandBufferBeginInfo hudCommandBufferBeginInfo = {};
    hudCommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    hudCommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    drvVk.vkBeginCommandBuffer(hudCommandBuffer, &hudCommandBufferBeginInfo);

    VkImageMemoryBarrier acquireBarrier = {};
    acquireBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    acquireBarrier.srcAccessMask = 0;
    acquireBarrier.dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    acquireBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    acquireBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    acquireBarrier.srcQueueFamilyIndex = m_QueueFamilyForQueue[queue];
    acquireBarrier.dstQueueFamilyIndex = m_HudQueueFamily;
    acquireBarrier.image = swapchainImage;
    acquireBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0,
                                       VK_REMAINING_ARRAY_LAYERS};

    drvVk.vkCmdPipelineBarrier(hudCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                               VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0,
                               nullptr, 1, &acquireBarrier);

    // Draw (on the hud queue)

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = m_RenderPass;
    renderPassBeginInfo.framebuffer = m_Framebuffers[imageIndex];
    renderPassBeginInfo.renderArea = {{0, 0}, m_SwapchainExtent};

    drvVk.vkCmdBeginRenderPass(hudCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), hudCommandBuffer);

    drvVk.vkCmdEndRenderPass(hudCommandBuffer);

    // Release back to present queue

    VkImageMemoryBarrier releaseBackBarrier = {};
    releaseBackBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    releaseBackBarrier.srcAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    releaseBackBarrier.dstAccessMask = 0;
    releaseBackBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    releaseBackBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    releaseBackBarrier.srcQueueFamilyIndex = m_HudQueueFamily;
    releaseBackBarrier.dstQueueFamilyIndex = m_QueueFamilyForQueue[queue];
    releaseBackBarrier.image = swapchainImage;
    releaseBackBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0,
                                           VK_REMAINING_ARRAY_LAYERS};

    drvVk.vkCmdPipelineBarrier(hudCommandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                               VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1,
                               &releaseBackBarrier);

    drvVk.vkEndCommandBuffer(hudCommandBuffer);

    VkSubmitInfo hudSubmitInfo = {};
    hudSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    hudSubmitInfo.commandBufferCount = 1;
    hudSubmitInfo.pCommandBuffers = &hudCommandBuffer;

    CheckVkResult(drvVk.vkQueueSubmit(m_HudQueue, 1, &hudSubmitInfo, VK_NULL_HANDLE));

    drvVk.vkQueueWaitIdle(m_HudQueue);

    drvVk.vkDestroyCommandPool(m_Device, hudCommandPool, nullptr);

    // Acquire back on present queue

    VkCommandPool acquireBackCommandPool = VK_NULL_HANDLE;
    VkCommandBuffer acquireBackCommandBuffer = VK_NULL_HANDLE;

    CreateOneTimeCommandBuffer(m_Device, m_QueueFamilyForQueue[queue], &acquireBackCommandPool,
                               &acquireBackCommandBuffer);

    VkCommandBufferBeginInfo acquireBackCommandBufferBeginInfo = {};
    acquireBackCommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    acquireBackCommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    drvVk.vkBeginCommandBuffer(acquireBackCommandBuffer, &acquireBackCommandBufferBeginInfo);

    VkImageMemoryBarrier acquireBackBarrier = {};
    acquireBackBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    acquireBackBarrier.srcAccessMask = 0;
    acquireBackBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    acquireBackBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    acquireBackBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    acquireBackBarrier.srcQueueFamilyIndex = m_HudQueueFamily;
    acquireBackBarrier.dstQueueFamilyIndex = m_QueueFamilyForQueue[queue];
    acquireBackBarrier.image = swapchainImage;
    acquireBackBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0,
                                           VK_REMAINING_ARRAY_LAYERS};

    drvVk.vkCmdPipelineBarrier(acquireBackCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                               VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1,
                               &acquireBackBarrier);

    drvVk.vkEndCommandBuffer(acquireBackCommandBuffer);

    VkSubmitInfo acquireBackSubmitInfo = {};
    acquireBackSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    acquireBackSubmitInfo.commandBufferCount = 1;
    acquireBackSubmitInfo.pCommandBuffers = &acquireBackCommandBuffer;

    CheckVkResult(drvVk.vkQueueSubmit(queue, 1, &acquireBackSubmitInfo, m_Fences[imageIndex]));

    CheckVkResult(drvVk.vkWaitForFences(m_Device, 1, &m_Fences[imageIndex], VK_TRUE, UINT64_MAX));

    drvVk.vkDestroyCommandPool(m_Device, acquireBackCommandPool, nullptr);
  } else {
    // Present queue family and the hud queue family are the same, no ownership transfer is needed

    VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
    cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CheckVkResult(drvVk.vkBeginCommandBuffer(m_CommandBuffers[imageIndex], &cmdBufferBeginInfo));

    // In this case we just need layout transitions
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

    drvVk.vkCmdPipelineBarrier(m_CommandBuffers[imageIndex], VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                               VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0,
                               nullptr, 1, &transitionToRenderBarrier);

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = m_RenderPass;
    renderPassBeginInfo.framebuffer = m_Framebuffers[imageIndex];
    renderPassBeginInfo.renderArea = {{0, 0}, m_SwapchainExtent};

    drvVk.vkCmdBeginRenderPass(m_CommandBuffers[imageIndex], &renderPassBeginInfo,
                               VK_SUBPASS_CONTENTS_INLINE);

    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_CommandBuffers[imageIndex]);

    drvVk.vkCmdEndRenderPass(m_CommandBuffers[imageIndex]);

    VkImageMemoryBarrier transitionToPresentBarrier = {};
    transitionToPresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    transitionToPresentBarrier.srcAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    transitionToPresentBarrier.dstAccessMask =
        VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
    transitionToPresentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    transitionToPresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    transitionToPresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    transitionToPresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    transitionToPresentBarrier.image = swapchainImage;
    transitionToPresentBarrier.subresourceRange = {
        VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS};

    drvVk.vkCmdPipelineBarrier(m_CommandBuffers[imageIndex],
                               VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                               VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1,
                               &transitionToPresentBarrier);

    CheckVkResult(drvVk.vkEndCommandBuffer(m_CommandBuffers[imageIndex]));

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_CommandBuffers[imageIndex];

    CheckVkResult(drvVk.vkQueueSubmit(m_HudQueue, 1, &submitInfo, m_Fences[imageIndex]));

    CheckVkResult(drvVk.vkWaitForFences(m_Device, 1, &m_Fences[imageIndex], VK_TRUE, UINT64_MAX));
  }
}

} // namespace Vulkan
} // namespace gits

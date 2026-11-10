// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "resourcesDumpLayerAuto.h"

#include <algorithm>
#include <optional>
#include <vector>

namespace gits {
namespace vulkan {

namespace {

std::vector<VkCommandBuffer> SubmittedCommandBuffers(uint32_t submitCount,
                                                     const VkSubmitInfo* submits) {
  std::vector<VkCommandBuffer> commandBuffers;
  for (uint32_t i = 0; submits && i < submitCount; ++i) {
    for (uint32_t j = 0; submits[i].pCommandBuffers && j < submits[i].commandBufferCount; ++j) {
      commandBuffers.push_back(submits[i].pCommandBuffers[j]);
    }
  }
  return commandBuffers;
}

std::vector<VkCommandBuffer> SubmittedCommandBuffers(uint32_t submitCount,
                                                     const VkSubmitInfo2* submits) {
  std::vector<VkCommandBuffer> commandBuffers;
  for (uint32_t i = 0; submits && i < submitCount; ++i) {
    const VkSubmitInfo2& submit = submits[i];
    for (uint32_t j = 0; submit.pCommandBufferInfos && j < submit.commandBufferInfoCount; ++j) {
      commandBuffers.push_back(submit.pCommandBufferInfos[j].commandBuffer);
    }
  }
  return commandBuffers;
}

VkImageLayout StencilFinalLayout(const VkAttachmentDescription& description) {
  return description.finalLayout;
}

VkImageLayout StencilFinalLayout(const VkAttachmentDescription2& description) {
  for (auto* node = static_cast<const VkBaseInStructure*>(description.pNext); node != nullptr;
       node = node->pNext) {
    if (node->sType == VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_STENCIL_LAYOUT) {
      return reinterpret_cast<const VkAttachmentDescriptionStencilLayout*>(node)
          ->stencilFinalLayout;
    }
  }
  return description.finalLayout;
}

template <typename CreateInfo>
std::vector<ResourcesDumpService::AttachmentFinalLayouts> FinalLayouts(const CreateInfo* info) {
  std::vector<ResourcesDumpService::AttachmentFinalLayouts> layouts;
  for (uint32_t i = 0; info && info->pAttachments && i < info->attachmentCount; ++i) {
    layouts.push_back(
        {info->pAttachments[i].finalLayout, StencilFinalLayout(info->pAttachments[i])});
  }
  return layouts;
}

uint32_t ViewMask(const VkRenderPassCreateInfo* info) {
  uint32_t viewMask = 0;
  for (auto* node = info ? static_cast<const VkBaseInStructure*>(info->pNext) : nullptr; node;
       node = node->pNext) {
    if (node->sType == VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO) {
      const auto* multiview = reinterpret_cast<const VkRenderPassMultiviewCreateInfo*>(node);
      for (uint32_t i = 0; multiview->pViewMasks && i < multiview->subpassCount; ++i) {
        viewMask |= multiview->pViewMasks[i];
      }
    }
  }
  return viewMask;
}

uint32_t ViewMask(const VkRenderPassCreateInfo2* info) {
  uint32_t viewMask = 0;
  for (uint32_t i = 0; info && info->pSubpasses && i < info->subpassCount; ++i) {
    viewMask |= info->pSubpasses[i].viewMask;
  }
  return viewMask;
}

const char* DeferSignals(SubmitSignalsDeferral& deferral,
                         vkQueueSubmitCommand& command,
                         SubmitSignals::Function,
                         const MayWaitForLaterSignals& mayWaitForLaterSignals,
                         bool earlierWorkMayWaitForLaterSignals) {
  return deferral.Defer(command.m_submitCount.Value, command.m_pSubmits.Value,
                        command.m_fence.Value, mayWaitForLaterSignals,
                        earlierWorkMayWaitForLaterSignals);
}

template <typename CommandT>
const char* DeferSignals(SubmitSignalsDeferral& deferral,
                         CommandT& command,
                         SubmitSignals::Function function,
                         const MayWaitForLaterSignals& mayWaitForLaterSignals,
                         bool earlierWorkMayWaitForLaterSignals) {
  return deferral.Defer(command.m_submitCount.Value, command.m_pSubmits.Value,
                        command.m_fence.Value, function, mayWaitForLaterSignals,
                        earlierWorkMayWaitForLaterSignals);
}

} // namespace

ResourcesDumpLayer::ResourcesDumpLayer(DispatchTablesHolder& dispatchTablesHolder)
    : Layer(LAYER_NAME), m_Service(dispatchTablesHolder) {}

void ResourcesDumpLayer::Post(RestoreContentDataCommand& command) {
  for (const auto& applied : command.m_AppliedImageLayouts) {
    m_Service.RestoredImageLayouts(applied.Queue, applied.Layouts);
  }
}

void ResourcesDumpLayer::Post(vkQueuePresentKHRCommand& command) {
  const VkPresentInfoKHR* info = command.m_pPresentInfo.Value;
  for (uint32_t i = 0; info && info->pSwapchains && info->pImageIndices && i < info->swapchainCount;
       ++i) {
    m_Service.ReleaseSwapchainImages(info->pSwapchains[i], 1, &info->pImageIndices[i]);
  }
}

void ResourcesDumpLayer::Post(vkCreateDeviceCommand& command) {
  if (command.m_Return.Value == VK_SUCCESS && command.m_pDevice.Value &&
      command.m_pCreateInfo.Value) {
    m_Service.CreateDevice(command.m_physicalDevice.Value, *command.m_pDevice.Value,
                           *command.m_pCreateInfo.Value);
  }
}

void ResourcesDumpLayer::Pre(vkDestroyDeviceCommand& command) {
  m_Service.DestroyDevice(command.m_device.Value);
}

void ResourcesDumpLayer::Post(vkGetDeviceQueueCommand& command) {
  if (command.m_pQueue.Value) {
    m_Service.GetDeviceQueue(command.m_device.Value, command.m_queueFamilyIndex.Value,
                             *command.m_pQueue.Value);
  }
}

void ResourcesDumpLayer::Post(vkGetDeviceQueue2Command& command) {
  if (command.m_pQueue.Value && command.m_pQueueInfo.Value) {
    m_Service.GetDeviceQueue(command.m_device.Value, command.m_pQueueInfo.Value->queueFamilyIndex,
                             *command.m_pQueue.Value);
  }
}

void ResourcesDumpLayer::Post(vkCreateBufferCommand& command) {
  if (command.m_Return.Value == VK_SUCCESS && command.m_pBuffer.Value &&
      command.m_pCreateInfo.Value) {
    m_Service.CreateBuffer(command.m_pBuffer.Key, command.m_device.Value, *command.m_pBuffer.Value,
                           *command.m_pCreateInfo.Value);
  }
}

void ResourcesDumpLayer::Pre(vkDestroyBufferCommand& command) {
  m_Service.DestroyBuffer(command.m_buffer.Value);
}

void ResourcesDumpLayer::Post(vkBindBufferMemoryCommand& command) {
  if (command.m_Return.Value == VK_SUCCESS) {
    m_Service.BindBuffer(command.m_buffer.Value);
  }
}

void ResourcesDumpLayer::Post(vkBindBufferMemory2Command& command) {
  for (uint32_t i = 0; command.m_Return.Value == VK_SUCCESS && command.m_pBindInfos.Value &&
                       i < command.m_bindInfoCount.Value;
       ++i) {
    m_Service.BindBuffer(command.m_pBindInfos.Value[i].buffer);
  }
}

void ResourcesDumpLayer::Post(vkBindBufferMemory2KHRCommand& command) {
  for (uint32_t i = 0; command.m_Return.Value == VK_SUCCESS && command.m_pBindInfos.Value &&
                       i < command.m_bindInfoCount.Value;
       ++i) {
    m_Service.BindBuffer(command.m_pBindInfos.Value[i].buffer);
  }
}

void ResourcesDumpLayer::Post(vkCreateImageCommand& command) {
  if (command.m_Return.Value == VK_SUCCESS && command.m_pImage.Value &&
      command.m_pCreateInfo.Value) {
    m_Service.CreateImage(command.m_pImage.Key, command.m_device.Value, *command.m_pImage.Value,
                          *command.m_pCreateInfo.Value);
  }
}

void ResourcesDumpLayer::Pre(vkDestroyImageCommand& command) {
  m_Service.DestroyImage(command.m_image.Value);
}

void ResourcesDumpLayer::Post(vkBindImageMemoryCommand& command) {
  if (command.m_Return.Value == VK_SUCCESS) {
    m_Service.BindImage(command.m_image.Value);
  }
}

void ResourcesDumpLayer::Post(vkBindImageMemory2Command& command) {
  for (uint32_t i = 0; command.m_Return.Value == VK_SUCCESS && command.m_pBindInfos.Value &&
                       i < command.m_bindInfoCount.Value;
       ++i) {
    m_Service.BindImage(command.m_pBindInfos.Value[i].image);
  }
}

void ResourcesDumpLayer::Post(vkBindImageMemory2KHRCommand& command) {
  for (uint32_t i = 0; command.m_Return.Value == VK_SUCCESS && command.m_pBindInfos.Value &&
                       i < command.m_bindInfoCount.Value;
       ++i) {
    m_Service.BindImage(command.m_pBindInfos.Value[i].image);
  }
}

void ResourcesDumpLayer::Pre(vkCreateSwapchainKHRCommand& command) {
  if (command.m_pCreateInfo.Value) {
    m_Service.PrepareSwapchain(command.m_device.Value, *command.m_pCreateInfo.Value);
  }
}

void ResourcesDumpLayer::Post(vkCreateSwapchainKHRCommand& command) {
  if (command.m_Return.Value == VK_SUCCESS && command.m_pSwapchain.Value &&
      command.m_pCreateInfo.Value) {
    m_Service.CreateSwapchain(*command.m_pSwapchain.Value, *command.m_pCreateInfo.Value);
  }
}

void ResourcesDumpLayer::Post(vkGetSwapchainImagesKHRCommand& command) {
  if ((command.m_Return.Value != VK_SUCCESS && command.m_Return.Value != VK_INCOMPLETE) ||
      !command.m_pSwapchainImages.Value || !command.m_pSwapchainImageCount.Value) {
    return;
  }
  const uint32_t count = std::min<uint32_t>(*command.m_pSwapchainImageCount.Value,
                                            command.m_pSwapchainImages.Keys.size());
  m_Service.GetSwapchainImages(command.m_device.Value, command.m_swapchain.Value,
                               command.m_pSwapchainImages.Value,
                               command.m_pSwapchainImages.Keys.data(), count);
}

void ResourcesDumpLayer::Pre(vkDestroySwapchainKHRCommand& command) {
  m_Service.DestroySwapchain(command.m_swapchain.Value);
}

void ResourcesDumpLayer::Post(vkAcquireNextImageKHRCommand& command) {
  if ((command.m_Return.Value == VK_SUCCESS || command.m_Return.Value == VK_SUBOPTIMAL_KHR) &&
      command.m_pImageIndex.Value) {
    m_Service.AcquireSwapchainImage(command.m_swapchain.Value, *command.m_pImageIndex.Value,
                                    command.m_semaphore.Value);
  }
}

void ResourcesDumpLayer::Post(vkAcquireNextImage2KHRCommand& command) {
  if ((command.m_Return.Value == VK_SUCCESS || command.m_Return.Value == VK_SUBOPTIMAL_KHR) &&
      command.m_pImageIndex.Value && command.m_pAcquireInfo.Value) {
    m_Service.AcquireSwapchainImage(command.m_pAcquireInfo.Value->swapchain,
                                    *command.m_pImageIndex.Value,
                                    command.m_pAcquireInfo.Value->semaphore);
  }
}

void ResourcesDumpLayer::Post(vkReleaseSwapchainImagesKHRCommand& command) {
  const VkReleaseSwapchainImagesInfoKHR* info = command.m_pReleaseInfo.Value;
  if (command.m_Return.Value == VK_SUCCESS && info) {
    m_Service.ReleaseSwapchainImages(info->swapchain, info->imageIndexCount, info->pImageIndices);
  }
}

void ResourcesDumpLayer::Post(vkReleaseSwapchainImagesEXTCommand& command) {
  const VkReleaseSwapchainImagesInfoKHR* info = command.m_pReleaseInfo.Value;
  if (command.m_Return.Value == VK_SUCCESS && info) {
    m_Service.ReleaseSwapchainImages(info->swapchain, info->imageIndexCount, info->pImageIndices);
  }
}

void ResourcesDumpLayer::Post(vkCreateImageViewCommand& command) {
  if (command.m_Return.Value == VK_SUCCESS && command.m_pView.Value &&
      command.m_pCreateInfo.Value) {
    m_Service.CreateImageView(*command.m_pView.Value, *command.m_pCreateInfo.Value);
  }
}

void ResourcesDumpLayer::Pre(vkDestroyImageViewCommand& command) {
  m_Service.DestroyImageView(command.m_imageView.Value);
}

void ResourcesDumpLayer::Post(vkCreateRenderPassCommand& command) {
  if (command.m_Return.Value == VK_SUCCESS && command.m_pRenderPass.Value) {
    m_Service.CreateRenderPass(*command.m_pRenderPass.Value,
                               FinalLayouts(command.m_pCreateInfo.Value),
                               ViewMask(command.m_pCreateInfo.Value));
  }
}

void ResourcesDumpLayer::Post(vkCreateRenderPass2Command& command) {
  if (command.m_Return.Value == VK_SUCCESS && command.m_pRenderPass.Value) {
    m_Service.CreateRenderPass(*command.m_pRenderPass.Value,
                               FinalLayouts(command.m_pCreateInfo.Value),
                               ViewMask(command.m_pCreateInfo.Value));
  }
}

void ResourcesDumpLayer::Post(vkCreateRenderPass2KHRCommand& command) {
  if (command.m_Return.Value == VK_SUCCESS && command.m_pRenderPass.Value) {
    m_Service.CreateRenderPass(*command.m_pRenderPass.Value,
                               FinalLayouts(command.m_pCreateInfo.Value),
                               ViewMask(command.m_pCreateInfo.Value));
  }
}

void ResourcesDumpLayer::Pre(vkDestroyRenderPassCommand& command) {
  m_Service.DestroyRenderPass(command.m_renderPass.Value);
}

void ResourcesDumpLayer::Post(vkCreateFramebufferCommand& command) {
  if (command.m_Return.Value == VK_SUCCESS && command.m_pFramebuffer.Value &&
      command.m_pCreateInfo.Value) {
    m_Service.CreateFramebuffer(*command.m_pFramebuffer.Value, *command.m_pCreateInfo.Value);
  }
}

void ResourcesDumpLayer::Pre(vkDestroyFramebufferCommand& command) {
  m_Service.DestroyFramebuffer(command.m_framebuffer.Value);
}

void ResourcesDumpLayer::Post(vkCreateCommandPoolCommand& command) {
  if (command.m_Return.Value == VK_SUCCESS && command.m_pCommandPool.Value &&
      command.m_pCreateInfo.Value) {
    m_Service.CreateCommandPool(command.m_device.Value, *command.m_pCommandPool.Value,
                                *command.m_pCreateInfo.Value);
  }
}

void ResourcesDumpLayer::Pre(vkDestroyCommandPoolCommand& command) {
  m_Service.DestroyCommandPool(command.m_commandPool.Value);
}

void ResourcesDumpLayer::Pre(vkResetCommandPoolCommand& command) {
  m_Service.ResetCommandPool(command.m_commandPool.Value);
}

void ResourcesDumpLayer::Post(vkAllocateCommandBuffersCommand& command) {
  if (command.m_Return.Value == VK_SUCCESS && command.m_pAllocateInfo.Value &&
      command.m_pCommandBuffers.Value) {
    m_Service.AllocateCommandBuffers(command.m_device.Value, *command.m_pAllocateInfo.Value,
                                     command.m_pCommandBuffers.Value);
  }
}

void ResourcesDumpLayer::Pre(vkFreeCommandBuffersCommand& command) {
  m_Service.FreeCommandBuffers(command.m_pCommandBuffers.Value,
                               command.m_pCommandBuffers.Value ? command.m_commandBufferCount.Value
                                                               : 0);
}

void ResourcesDumpLayer::Pre(vkBeginCommandBufferCommand& command) {
  if (command.m_pBeginInfo.Value) {
    m_Service.BeginCommandBuffer(command.m_commandBuffer.Value, *command.m_pBeginInfo.Value);
  }
}

void ResourcesDumpLayer::Pre(vkResetCommandBufferCommand& command) {
  m_Service.ResetCommandBuffer(command.m_commandBuffer.Value);
}

void ResourcesDumpLayer::Pre(vkEndCommandBufferCommand& command) {
  m_Service.EndCommandBuffer(command.m_Key, command.m_commandBuffer.Value);
}

void ResourcesDumpLayer::Post(vkCreateSemaphoreCommand& command) {
  if (command.m_Return.Value == VK_SUCCESS && command.m_pSemaphore.Value &&
      command.m_pCreateInfo.Value) {
    m_Service.TrackSemaphore(command.m_device.Value, *command.m_pSemaphore.Value,
                             *command.m_pCreateInfo.Value);
  }
}

void ResourcesDumpLayer::Pre(vkDestroySemaphoreCommand& command) {
  m_Service.DestroySemaphore(command.m_semaphore.Value);
}

template <typename CommandT>
void ResourcesDumpLayer::PreSubmit(CommandT& command, SubmitSignals::Function function) {
  m_Service.PreSubmit(
      SubmittedCommandBuffers(command.m_submitCount.Value, command.m_pSubmits.Value));
  m_Service.CheckSubmission(command.m_Key, command.m_queue.Value, command.m_submitCount.Value,
                            command.m_pSubmits.Value, command.m_fence.Value);
  if (m_Service.DumpsAfterSubmission(command.m_Key, command.m_queue.Value)) {
    const char* problem = DeferSignals(
        m_SignalsDeferral, command, function,
        [this](VkSemaphore semaphore, uint64_t value) {
          return m_Service.WaitMayDependOnLaterSignals(semaphore, value);
        },
        m_Service.QueueMayWaitForLaterSignals(command.m_queue.Value));
    if (problem) {
      m_Service.CannotDeferSignals(command.m_Key, problem);
    }
  }
}

template <typename CommandT>
void ResourcesDumpLayer::PostSubmit(CommandT& command) {
  const std::optional<SubmitSignals> signals = m_SignalsDeferral.Restore();
  if (command.m_Return.Value == VK_SUCCESS) {
    m_Service.TrackSignals(command.m_queue.Value, command.m_submitCount.Value,
                           command.m_pSubmits.Value);
    m_Service.Submit(command.m_Key, command.m_queue.Value,
                     SubmittedCommandBuffers(command.m_submitCount.Value, command.m_pSubmits.Value),
                     signals ? &*signals : nullptr);
  }
}

void ResourcesDumpLayer::Pre(vkQueueSubmitCommand& command) {
  PreSubmit(command, SubmitSignals::Function::QueueSubmit);
}

void ResourcesDumpLayer::Post(vkQueueSubmitCommand& command) {
  PostSubmit(command);
}

void ResourcesDumpLayer::Pre(vkQueueSubmit2Command& command) {
  PreSubmit(command, SubmitSignals::Function::QueueSubmit2);
}

void ResourcesDumpLayer::Post(vkQueueSubmit2Command& command) {
  PostSubmit(command);
}

void ResourcesDumpLayer::Pre(vkQueueSubmit2KHRCommand& command) {
  PreSubmit(command, SubmitSignals::Function::QueueSubmit2KHR);
}

void ResourcesDumpLayer::Post(vkQueueSubmit2KHRCommand& command) {
  PostSubmit(command);
}

void ResourcesDumpLayer::Post(vkQueueBindSparseCommand& command) {
  if (command.m_Return.Value == VK_SUCCESS) {
    m_Service.TrackSignals(command.m_queue.Value, command.m_bindInfoCount.Value,
                           command.m_pBindInfo.Value);
  }
}

void ResourcesDumpLayer::Post(vkCmdPipelineBarrierCommand& command) {
  m_Service.Barriers(command.m_commandBuffer.Value, command.m_bufferMemoryBarrierCount.Value,
                     command.m_pBufferMemoryBarriers.Value, command.m_imageMemoryBarrierCount.Value,
                     command.m_pImageMemoryBarriers.Value);
  m_Service.CommandBufferCall(command.m_Key, command.m_commandBuffer.Value);
}

void ResourcesDumpLayer::Post(vkCmdPipelineBarrier2Command& command) {
  if (command.m_pDependencyInfo.Value) {
    m_Service.Barriers(command.m_commandBuffer.Value, *command.m_pDependencyInfo.Value);
  }
  m_Service.CommandBufferCall(command.m_Key, command.m_commandBuffer.Value);
}

void ResourcesDumpLayer::Post(vkCmdPipelineBarrier2KHRCommand& command) {
  if (command.m_pDependencyInfo.Value) {
    m_Service.Barriers(command.m_commandBuffer.Value, *command.m_pDependencyInfo.Value);
  }
  m_Service.CommandBufferCall(command.m_Key, command.m_commandBuffer.Value);
}

void ResourcesDumpLayer::Post(vkCmdWaitEventsCommand& command) {
  m_Service.Barriers(command.m_commandBuffer.Value, command.m_bufferMemoryBarrierCount.Value,
                     command.m_pBufferMemoryBarriers.Value, command.m_imageMemoryBarrierCount.Value,
                     command.m_pImageMemoryBarriers.Value);
  m_Service.CommandBufferCall(command.m_Key, command.m_commandBuffer.Value);
}

void ResourcesDumpLayer::Post(vkCmdWaitEvents2Command& command) {
  for (uint32_t i = 0; command.m_pDependencyInfos.Value && i < command.m_eventCount.Value; ++i) {
    m_Service.Barriers(command.m_commandBuffer.Value, command.m_pDependencyInfos.Value[i]);
  }
  m_Service.CommandBufferCall(command.m_Key, command.m_commandBuffer.Value);
}

void ResourcesDumpLayer::Post(vkCmdWaitEvents2KHRCommand& command) {
  for (uint32_t i = 0; command.m_pDependencyInfos.Value && i < command.m_eventCount.Value; ++i) {
    m_Service.Barriers(command.m_commandBuffer.Value, command.m_pDependencyInfos.Value[i]);
  }
  m_Service.CommandBufferCall(command.m_Key, command.m_commandBuffer.Value);
}

void ResourcesDumpLayer::Post(vkCmdBeginRenderPassCommand& command) {
  if (command.m_pRenderPassBegin.Value) {
    m_Service.BeginRenderPass(command.m_commandBuffer.Value, *command.m_pRenderPassBegin.Value);
  }
  m_Service.CommandBufferCall(command.m_Key, command.m_commandBuffer.Value);
}

void ResourcesDumpLayer::Post(vkCmdBeginRenderPass2Command& command) {
  if (command.m_pRenderPassBegin.Value) {
    m_Service.BeginRenderPass(command.m_commandBuffer.Value, *command.m_pRenderPassBegin.Value);
  }
  m_Service.CommandBufferCall(command.m_Key, command.m_commandBuffer.Value);
}

void ResourcesDumpLayer::Post(vkCmdBeginRenderPass2KHRCommand& command) {
  if (command.m_pRenderPassBegin.Value) {
    m_Service.BeginRenderPass(command.m_commandBuffer.Value, *command.m_pRenderPassBegin.Value);
  }
  m_Service.CommandBufferCall(command.m_Key, command.m_commandBuffer.Value);
}

void ResourcesDumpLayer::Post(vkCmdEndRenderPassCommand& command) {
  m_Service.EndRenderPass(command.m_commandBuffer.Value);
  m_Service.CommandBufferCall(command.m_Key, command.m_commandBuffer.Value);
}

void ResourcesDumpLayer::Post(vkCmdEndRenderPass2Command& command) {
  m_Service.EndRenderPass(command.m_commandBuffer.Value);
  m_Service.CommandBufferCall(command.m_Key, command.m_commandBuffer.Value);
}

void ResourcesDumpLayer::Post(vkCmdEndRenderPass2KHRCommand& command) {
  m_Service.EndRenderPass(command.m_commandBuffer.Value);
  m_Service.CommandBufferCall(command.m_Key, command.m_commandBuffer.Value);
}

void ResourcesDumpLayer::Post(vkCmdBeginRenderingCommand& command) {
  if (command.m_pRenderingInfo.Value) {
    m_Service.BeginRendering(command.m_commandBuffer.Value, *command.m_pRenderingInfo.Value);
  }
  m_Service.CommandBufferCall(command.m_Key, command.m_commandBuffer.Value);
}

void ResourcesDumpLayer::Post(vkCmdBeginRenderingKHRCommand& command) {
  if (command.m_pRenderingInfo.Value) {
    m_Service.BeginRendering(command.m_commandBuffer.Value, *command.m_pRenderingInfo.Value);
  }
  m_Service.CommandBufferCall(command.m_Key, command.m_commandBuffer.Value);
}

void ResourcesDumpLayer::Post(vkCmdEndRenderingCommand& command) {
  m_Service.EndRendering(command.m_commandBuffer.Value);
  m_Service.CommandBufferCall(command.m_Key, command.m_commandBuffer.Value);
}

void ResourcesDumpLayer::Post(vkCmdEndRenderingKHRCommand& command) {
  m_Service.EndRendering(command.m_commandBuffer.Value);
  m_Service.CommandBufferCall(command.m_Key, command.m_commandBuffer.Value);
}

void ResourcesDumpLayer::Post(vkCmdEndRendering2EXTCommand& command) {
  m_Service.EndRendering(command.m_commandBuffer.Value);
  m_Service.CommandBufferCall(command.m_Key, command.m_commandBuffer.Value);
}

void ResourcesDumpLayer::Post(vkCmdEndRendering2KHRCommand& command) {
  m_Service.EndRendering(command.m_commandBuffer.Value);
  m_Service.CommandBufferCall(command.m_Key, command.m_commandBuffer.Value);
}

void ResourcesDumpLayer::Post(vkCmdExecuteCommandsCommand& command) {
  m_Service.ExecuteCommands(command.m_commandBuffer.Value, command.m_pCommandBuffers.Value,
                            command.m_pCommandBuffers.Value ? command.m_commandBufferCount.Value
                                                            : 0);
  m_Service.CommandBufferCall(command.m_Key, command.m_commandBuffer.Value);
}

void ResourcesDumpLayer::Post(vkCmdBeginVideoCodingKHRCommand& command) {
  m_Service.BeginVideoCoding(command.m_Key, command.m_commandBuffer.Value);
}

void ResourcesDumpLayer::Post(vkCmdEndVideoCodingKHRCommand& command) {
  m_Service.EndVideoCoding(command.m_Key, command.m_commandBuffer.Value);
}

} // namespace vulkan
} // namespace gits

// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   vulkanLibrary.cpp
*
* @brief Definition of Vulkan common part library implementation.
*
*/

#include "gits.h"
#include "vulkanTools.h"
#include "vulkanStateDynamic.h"
#include "vulkanStateDynamic.h"
#include "vulkanFunctions.h"
#include "vulkanStateTracking.h"

namespace gits {
namespace Vulkan {

/**
    * @brief Constructor
    *
    * CLibrary class constructor.
    */
CLibrary::CLibrary(gits::CLibrary::state_creator_t stc)
    : gits::CLibrary(ID_VULKAN, std::move(stc)) {}

CLibrary::~CLibrary() {
  try {
    waitForAllDevices();
    destroyDeviceLevelResources();
    destroyInstanceLevelResources();
  } catch (...) {
    topmost_exception_handler("CLibrary::~CLibrary()");
  }
}

/**
    * @brief Creates Vulkan method call wrapper
    *
    * Method creates Vulkan method call wrappers based on unique
    * identifier.
    *
    * @param id Unique Vulkan method identifier.
    *
    * @exception EOperationFailed Unknown Vulkan method identifier
    *
    * @return Vulkan method call wrapper.
    */
Vulkan::CFunction* Vulkan::CLibrary::FunctionCreate(unsigned id) const {
  return Vulkan::CFunction::Create(id);
}

CLibrary& CLibrary::Get() {
  return static_cast<CLibrary&>(CGits::Instance().Library(ID_VULKAN));
}

std::set<uint64_t> CLibrary::CVulkanCommandBufferTokensBuffer::GetMappedPointers() {
  std::set<uint64_t> returnMap;

  for (auto elem : _tokensList) {
    for (auto obj : elem->GetMappedPointers()) {
      returnMap.insert((uint64_t)obj);
    }
  }
  returnMap.erase(0); // 0 is not valid pointer.
  return returnMap;
}

std::set<uint64_t> CLibrary::CVulkanCommandBufferTokensBuffer::GetMappedPointers(
    const BitRange& objRange, VulkanObjectMode objMode, const uint64_t objNumber) {
  std::set<uint64_t> returnMap;
  uint64_t renderPassCount = 0;
  uint64_t drawInRenderPass = 0;
  uint64_t blitCount = 0;
  uint64_t dispatchCount = 0;
  bool pre_recording = true;
  bool isRenderPassMode = objMode == VulkanObjectMode::MODE_VK_RENDER_PASS;
  bool isDrawMode = objMode == VulkanObjectMode::MODE_VK_DRAW;
  bool isBlitMode = objMode == VulkanObjectMode::MODE_VK_BLIT;
  bool isDispatchMode = objMode == VulkanObjectMode::MODE_VK_DISPATCH;

  for (auto elem : _tokensList) {
    if (elem->Type() & CFunction::GITS_VULKAN_END_RENDER_PASS_APITYPE) {
      drawInRenderPass = 0;
      renderPassCount++;
    } else if (elem->Type() & CFunction::GITS_VULKAN_DRAW_APITYPE) {
      drawInRenderPass++;
    } else if (elem->Type() & CFunction::GITS_VULKAN_BLIT_APITYPE) {
      blitCount++;
    } else if (elem->Type() & CFunction::GITS_VULKAN_DISPATCH_APITYPE) {
      dispatchCount++;
    }
    if ((isRenderPassMode && objRange[renderPassCount] &&
         (elem->Type() & CFunction::GITS_VULKAN_BEGIN_RENDER_PASS_APITYPE)) ||
        (isDrawMode && renderPassCount == objNumber && objRange[drawInRenderPass]) ||
        (isBlitMode && objRange[blitCount]) || (isDispatchMode && objRange[dispatchCount])) {
      pre_recording = false;
    }
    if (objMode == VulkanObjectMode::MODE_VK_QUEUE_SUBMIT ||
        objMode == VulkanObjectMode::MODE_VK_COMMAND_BUFFER ||
        (isRenderPassMode && objRange[renderPassCount]) ||
        (isDrawMode && renderPassCount == objNumber && objRange[drawInRenderPass]) ||
        (isBlitMode && objRange[blitCount]) || (isDispatchMode && objRange[dispatchCount]) ||
        (((isRenderPassMode || isDrawMode || isBlitMode || isDispatchMode) && pre_recording) &&
         (elem->Type() & CFunction::GITS_VULKAN_COMMAND_BUFFER_SET_APITYPE ||
          elem->Type() & CFunction::GITS_VULKAN_COMMAND_BUFFER_BIND_APITYPE ||
          elem->Type() & CFunction::GITS_VULKAN_COMMAND_BUFFER_PUSH_APITYPE))) {
      for (auto obj : elem->GetMappedPointers()) {
        returnMap.insert((uint64_t)obj);
      }
    }
  }
  returnMap.erase(0); // 0 is not valid pointer.
  return returnMap;
}

void CLibrary::CVulkanCommandBufferTokensBuffer::ExecAndStateTrack() {
  for (auto elem : _tokensList) {
    elem->Exec();
    elem->StateTrack();
  }
}

void CLibrary::CVulkanCommandBufferTokensBuffer::FinishRenderPass(uint64_t renderPassNumber,
                                                                  uint64_t drawNumber,
                                                                  VkCommandBuffer cmdBuffer) {
  uint64_t drawCount = 0;
  uint64_t renderPassCount = 0;
  unsigned int endRenderID = 0;
  // restoring subpasses - we need the same number of subpasses as in original VkRenderPass
  for (auto elem : _tokensList) {
    if ((elem->Type() & CFunction::GITS_VULKAN_NEXT_SUBPASS_APITYPE) && (drawCount >= drawNumber) &&
        (renderPassCount == renderPassNumber)) {
      elem->Exec();
    } else if (elem->Type() & CFunction::GITS_VULKAN_DRAW_APITYPE) {
      drawCount++;
    } else if (elem->Type() & CFunction::GITS_VULKAN_END_RENDER_PASS_APITYPE) {
      renderPassCount++;
      endRenderID = elem->Id();
    } else if (renderPassCount > renderPassNumber) {
      break;
    }
  }

  VkRenderPassBeginInfo* renderPassBeginInfo = SD()._commandbufferstates[cmdBuffer]
                                                   ->beginRenderPassesList.back()
                                                   ->renderPassBeginInfoData.Value();
  VkRenderingInfo* renderingInfo =
      SD()._commandbufferstates[cmdBuffer]->beginRenderPassesList.back()->renderingInfoData.Value();
  if (renderPassBeginInfo) {
    callvkCmdEndRenderPassByID(endRenderID, cmdBuffer);
    vkCmdEndRenderPass_SD(cmdBuffer);
  } else if (renderingInfo) {
    callvkCmdEndRenderingByID(endRenderID, cmdBuffer);
    vkCmdEndRendering_SD(cmdBuffer);
  }
}

void CLibrary::CVulkanCommandBufferTokensBuffer::FinishCommandBufferAndSubmit(
    VkCommandBuffer cmdBuffer) {
  drvVk.vkEndCommandBuffer(cmdBuffer);

  VkCommandPool cmdPool =
      SD()._commandbufferstates[cmdBuffer]->commandBufferAllocateInfoData.Value()->commandPool;
  VkSubmitInfo submitInfoNew = {
      VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr, 0, nullptr, nullptr, 1, &cmdBuffer, 0, nullptr};
  VkQueue queue =
      SD()._commandpoolstates[cmdPool]->deviceStateStore->queueStateStoreList[0]->queueHandle;
  drvVk.vkQueueSubmit(queue, 1, &submitInfoNew, VK_NULL_HANDLE);
  drvVk.vkQueueWaitIdle(queue);
  VkDevice device = SD()._commandpoolstates[cmdPool]->deviceStateStore->deviceHandle;
  drvVk.vkFreeCommandBuffers(device, cmdPool, 1, &cmdBuffer);
}

void CLibrary::CVulkanCommandBufferTokensBuffer::CreateNewCommandBuffer(Vulkan::CFunction* token,
                                                                        VkCommandBuffer cmdBuffer) {

  VkCommandPool cmdPool =
      SD()._commandbufferstates[cmdBuffer]->commandBufferAllocateInfoData.Value()->commandPool;
  VkCommandBuffer newCmdBuffer;
  VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, // VkStructureType sType;
      nullptr,                                        // const void* pNext;
      cmdPool,                                        // VkCommandPool commandPool;
      VK_COMMAND_BUFFER_LEVEL_PRIMARY,                // VkCommandBufferLevel level;
      1                                               // uint32_t commandBufferCount;
  };
  VkDevice device = SD()._commandpoolstates[cmdPool]->deviceStateStore->deviceHandle;
  drvVk.vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &newCmdBuffer);
  if (newCmdBuffer != cmdBuffer) {
    SD()._commandbufferstates[cmdBuffer].swap(SD()._commandbufferstates[newCmdBuffer]);
    SD()._commandbufferstates.erase(cmdBuffer);
    CVkCommandBuffer::AddMapping(token->CommandBuffer(), newCmdBuffer);
  }
  drvVk.vkBeginCommandBuffer(newCmdBuffer,
                             SD()._commandbufferstates[newCmdBuffer]
                                 ->beginCommandBuffer->commandBufferBeginInfoData.Value());
}

void CLibrary::CVulkanCommandBufferTokensBuffer::RestoreSettingsToSpecifiedObject(
    uint64_t objNumber, VulkanObjectMode objMode) {
  uint64_t blitCount = 0;
  uint64_t dispatchCount = 0;
  bool isRenderPassMode = objMode == VulkanObjectMode::MODE_VK_RENDER_PASS;
  bool isBlitMode = objMode == VulkanObjectMode::MODE_VK_BLIT;
  bool isDispatchMode = objMode == VulkanObjectMode::MODE_VK_DISPATCH;

  uint64_t renderPassCount = 0;
  for (auto elem : _tokensList) {
    if ((elem->Type() & CFunction::GITS_VULKAN_COMMAND_BUFFER_SET_APITYPE ||
         elem->Type() & CFunction::GITS_VULKAN_COMMAND_BUFFER_BIND_APITYPE ||
         elem->Type() & CFunction::GITS_VULKAN_COMMAND_BUFFER_PUSH_APITYPE) &&
        ((isRenderPassMode && renderPassCount < objNumber) ||
         (isBlitMode && blitCount < objNumber) || (isDispatchMode && dispatchCount < objNumber))) {
      // restoring VkCommandBuffer settings
      elem->Exec();
    } else if (elem->Type() & CFunction::GITS_VULKAN_END_RENDER_PASS_APITYPE) {
      renderPassCount++;
    } else if (elem->Type() & CFunction::GITS_VULKAN_BLIT_APITYPE) {
      blitCount++;
    } else if (elem->Type() & CFunction::GITS_VULKAN_DISPATCH_APITYPE) {
      dispatchCount++;
    }
  }
}

void CLibrary::CVulkanCommandBufferTokensBuffer::RestoreSettingsToSpecifiedDraw(
    Vulkan::CFunction* token,
    uint64_t renderPassNumber,
    uint64_t drawNumber,
    VkCommandBuffer cmdBuffer) {
  uint64_t drawCount = 0;
  uint64_t renderPassCount = 0;
  for (auto elem : _tokensList) {
    if ((elem->Type() & CFunction::GITS_VULKAN_COMMAND_BUFFER_SET_APITYPE ||
         elem->Type() & CFunction::GITS_VULKAN_COMMAND_BUFFER_BIND_APITYPE ||
         elem->Type() & CFunction::GITS_VULKAN_COMMAND_BUFFER_PUSH_APITYPE) &&
        (drawCount < drawNumber)) {
      // restoring VkCommandBuffer settings
      elem->Exec();
    } else if ((elem->Type() & CFunction::GITS_VULKAN_NEXT_SUBPASS_APITYPE) &&
               (drawCount < drawNumber) && (renderPassCount == renderPassNumber)) {
      // restoring subpasses - we need the same number of subpasses as in original VkRenderPass
      elem->Exec();
    } else if (elem->Type() & CFunction::GITS_VULKAN_DRAW_APITYPE) {
      drawCount++;
    } else if (elem->Type() & CFunction::GITS_VULKAN_END_RENDER_PASS_APITYPE) {
      renderPassCount++;
    } else if ((elem->Type() & CFunction::GITS_VULKAN_BEGIN_RENDER_PASS_APITYPE) &&
               (renderPassCount == renderPassNumber) &&
               (token->Type() & CFunction::GITS_VULKAN_DRAW_APITYPE)) {
      // Setting loadOp to LOAD, and storeOp to STORE
      VkRenderPassBeginInfo* renderPassBeginInfoPtr = SD()._commandbufferstates[cmdBuffer]
                                                          ->beginRenderPassesList.back()
                                                          ->renderPassBeginInfoData.Value();
      VkRenderingInfo* renderingInfoPtr = SD()._commandbufferstates[cmdBuffer]
                                              ->beginRenderPassesList.back()
                                              ->renderingInfoData.Value();
      if (renderPassBeginInfoPtr) {
        VkRenderPassBeginInfo renderPassBeginInfo = *renderPassBeginInfoPtr;
        renderPassBeginInfo.renderPass = SD()._commandbufferstates[cmdBuffer]
                                             ->beginRenderPassesList.back()
                                             ->renderPassStateStore->loadAndStoreRenderPassHandle;
        callvkCmdBeginRenderPassByID(elem->Id(), cmdBuffer, &renderPassBeginInfo,
                                     SD()._commandbufferstates[cmdBuffer]
                                         ->beginRenderPassesList.back()
                                         ->subpassContentsData.Value());
      } else if (renderingInfoPtr) {
        VkRenderingInfo renderingInfo = *renderingInfoPtr;
        std::vector<VkRenderingAttachmentInfo> renderingAttInfoVector;
        for (uint32_t i = 0; i < renderingInfo.colorAttachmentCount; i++) {
          VkRenderingAttachmentInfo renderingAttInfo = renderingInfo.pColorAttachments[i];
          renderingAttInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
          renderingAttInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
          renderingAttInfoVector.push_back(renderingAttInfo);
        }
        if (renderingInfo.colorAttachmentCount > 0) {
          renderingInfo.pColorAttachments = renderingAttInfoVector.data();
        }
        VkRenderingAttachmentInfo depthAttInfo;
        if (renderingInfo.pDepthAttachment != VK_NULL_HANDLE) {
          depthAttInfo = *renderingInfo.pDepthAttachment;
          depthAttInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
          depthAttInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
          renderingInfo.pDepthAttachment = &depthAttInfo;
        }
        VkRenderingAttachmentInfo stencilAttInfo;
        if (renderingInfo.pStencilAttachment != VK_NULL_HANDLE) {
          stencilAttInfo = *renderingInfo.pStencilAttachment;
          stencilAttInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
          stencilAttInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
          renderingInfo.pStencilAttachment = &stencilAttInfo;
        }
        callvkCmdBeginRenderingByID(elem->Id(), cmdBuffer, &renderingInfo);
      }
    } else if (renderPassCount > renderPassNumber) {
      break;
    }
  }
}

void CLibrary::CVulkanCommandBufferTokensBuffer::ExecAndDump(uint64_t queueSubmitNumber,
                                                             uint32_t cmdBuffBatchNumber,
                                                             uint32_t cmdBuffNumber,
                                                             VkCommandBuffer& cmdBuffer) {
  uint64_t renderPassCount = 0;
  uint64_t drawCount = 0;
  uint64_t drawInRenderPass = 0;
  uint64_t blitCount = 0;
  uint64_t dispatchCount = 0;
  for (auto elem : _tokensList) {
    cmdBuffer = CVkCommandBuffer::GetMapping(elem->CommandBuffer());
    if (Configurator::Get().vulkan.player.oneVulkanDrawPerCommandBuffer &&
        elem->Type() & CFunction::GITS_VULKAN_BEGIN_RENDER_PASS_APITYPE) {
      //  For executing each Vulkan draw in separate VkCommandBuffer we need to modify original storeOp (set it to STORE)
      elem->StateTrack();
      VkRenderPassBeginInfo* renderPassBeginInfoPtr = SD()._commandbufferstates[cmdBuffer]
                                                          ->beginRenderPassesList.back()
                                                          ->renderPassBeginInfoData.Value();
      VkRenderingInfo* renderingInfoPtr = SD()._commandbufferstates[cmdBuffer]
                                              ->beginRenderPassesList.back()
                                              ->renderingInfoData.Value();
      if (renderPassBeginInfoPtr) {
        VkRenderPassBeginInfo renderPassBeginInfo = *renderPassBeginInfoPtr;
        renderPassBeginInfo.renderPass = SD()._commandbufferstates[cmdBuffer]
                                             ->beginRenderPassesList.back()
                                             ->renderPassStateStore->storeNoLoadRenderPassHandle;
        callvkCmdBeginRenderPassByID(elem->Id(), cmdBuffer, &renderPassBeginInfo,
                                     SD()._commandbufferstates[cmdBuffer]
                                         ->beginRenderPassesList.back()
                                         ->subpassContentsData.Value());
      } else if (renderingInfoPtr) {
        VkRenderingInfo renderingInfo = *renderingInfoPtr;
        std::vector<VkRenderingAttachmentInfo> renderingAttInfoVector;
        for (uint32_t i = 0; i < renderingInfo.colorAttachmentCount; i++) {
          VkRenderingAttachmentInfo renderingAttInfo = renderingInfo.pColorAttachments[i];
          renderingAttInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
          renderingAttInfoVector.push_back(renderingAttInfo);
        }
        if (renderingInfo.colorAttachmentCount > 0) {
          renderingInfo.pColorAttachments = renderingAttInfoVector.data();
        }
        VkRenderingAttachmentInfo depthAttInfo;
        if (renderingInfo.pDepthAttachment != VK_NULL_HANDLE) {
          depthAttInfo = *renderingInfo.pDepthAttachment;
          depthAttInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
          renderingInfo.pDepthAttachment = &depthAttInfo;
        }
        VkRenderingAttachmentInfo stencilAttInfo;
        if (renderingInfo.pStencilAttachment != VK_NULL_HANDLE) {
          stencilAttInfo = *renderingInfo.pStencilAttachment;
          stencilAttInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
          renderingInfo.pStencilAttachment = &stencilAttInfo;
        }
        callvkCmdBeginRenderingByID(elem->Id(), cmdBuffer, &renderingInfo);
      }
    } else {
      elem->Exec();
      elem->StateTrack();
    }

    if (Configurator::Get().vulkan.player.oneVulkanDrawPerCommandBuffer &&
        (elem->Type() & CFunction::GITS_VULKAN_DRAW_APITYPE)) {
      ++drawInRenderPass;
      ++drawCount;
      FinishRenderPass(renderPassCount, drawCount, cmdBuffer);
      CGits::CCounter localCounter = {queueSubmitNumber, cmdBuffBatchNumber, cmdBuffNumber,
                                      renderPassCount, drawInRenderPass};
      if (localCounter == Configurator::Get().vulkan.player.captureVulkanDraws) {
        vulkanScheduleCopyRenderPasses(cmdBuffer, queueSubmitNumber, cmdBuffBatchNumber,
                                       cmdBuffNumber, renderPassCount, drawInRenderPass);
      }
      if (localCounter == Configurator::Get().vulkan.player.captureVulkanResources) {
        vulkanScheduleCopyResources(cmdBuffer, queueSubmitNumber, cmdBuffBatchNumber, cmdBuffNumber,
                                    renderPassCount, VulkanDumpingMode::VULKAN_PER_DRAW,
                                    drawInRenderPass);
      }
      FinishCommandBufferAndSubmit(cmdBuffer);
      if (!SD()._commandbufferstates[cmdBuffer]->drawImages.empty()) {
        vulkanDumpRenderPasses(cmdBuffer);
      }
      bool capturesResourcesCheck =
          !Configurator::Get().vulkan.player.captureVulkanResources.empty() &&
          (!SD()._commandbufferstates[cmdBuffer]->renderPassResourceImages.empty() ||
           !SD()._commandbufferstates[cmdBuffer]->renderPassResourceBuffers.empty());
      if (capturesResourcesCheck) {
        vulkanDumpRenderPassResources(cmdBuffer);
      }
      CreateNewCommandBuffer(elem, cmdBuffer);
      cmdBuffer = CVkCommandBuffer::GetMapping(elem->CommandBuffer());
      RestoreSettingsToSpecifiedDraw(elem, renderPassCount, drawCount, cmdBuffer);
    } else if (elem->Type() & CFunction::GITS_VULKAN_BLIT_APITYPE) {
      blitCount++;
      CGits::CCounter localCounter = {queueSubmitNumber, cmdBuffBatchNumber, cmdBuffNumber,
                                      blitCount};
      if (localCounter == Configurator::Get().vulkan.player.captureVulkanResources) {
        vulkanScheduleCopyResources(cmdBuffer, queueSubmitNumber, cmdBuffBatchNumber, cmdBuffNumber,
                                    blitCount, VulkanDumpingMode::VULKAN_PER_BLIT);
      }
      // can't finish commandBuffer on blit (e.g. barriers). Will dump resources on next draw/dispatch
    } else if (elem->Type() & CFunction::GITS_VULKAN_DISPATCH_APITYPE) {
      dispatchCount++;
      CGits::CCounter localCounter = {queueSubmitNumber, cmdBuffBatchNumber, cmdBuffNumber,
                                      dispatchCount};
      if (localCounter == Configurator::Get().vulkan.player.captureVulkanResources) {
        vulkanScheduleCopyResources(cmdBuffer, queueSubmitNumber, cmdBuffBatchNumber, cmdBuffNumber,
                                    dispatchCount, VulkanDumpingMode::VULKAN_PER_DISPATCH);
      }

      bool capturesResourcesCheck =
          !Configurator::Get().vulkan.player.captureVulkanResources.empty() &&
          (!SD()._commandbufferstates[cmdBuffer]->renderPassResourceImages.empty() ||
           !SD()._commandbufferstates[cmdBuffer]->renderPassResourceBuffers.empty());
      if (capturesResourcesCheck) {
        FinishCommandBufferAndSubmit(cmdBuffer);
        vulkanDumpRenderPassResources(cmdBuffer);
        CreateNewCommandBuffer(elem, cmdBuffer);
        cmdBuffer = CVkCommandBuffer::GetMapping(elem->CommandBuffer());
        RestoreSettingsToSpecifiedObject(dispatchCount, VulkanObjectMode::MODE_VK_DISPATCH);
      }
    }
    if (elem->Type() & CFunction::GITS_VULKAN_END_RENDER_PASS_APITYPE) {
      if (Configurator::Get().vulkan.player.oneVulkanDrawPerCommandBuffer) {
        VkRenderPassBeginInfo* renderPassBeginInfoPtr = SD()._commandbufferstates[cmdBuffer]
                                                            ->beginRenderPassesList.back()
                                                            ->renderPassBeginInfoData.Value();
        VkRenderingInfo* renderingInfoPtr = SD()._commandbufferstates[cmdBuffer]
                                                ->beginRenderPassesList.back()
                                                ->renderingInfoData.Value();
        if (renderPassBeginInfoPtr) {
          VkRenderPassBeginInfo renderPassBeginInfo = *renderPassBeginInfoPtr;
          VkRenderPass restoreRenderPassHandle =
              SD()._commandbufferstates[cmdBuffer]
                  ->beginRenderPassesList.back()
                  ->renderPassStateStore->restoreRenderPassHandle;
          if (renderPassBeginInfo.renderPass != restoreRenderPassHandle) {
            renderPassBeginInfo.renderPass = restoreRenderPassHandle;
            callvkCmdBeginRenderPassByID(getBeginRenderFunctionID(elem->Id()), cmdBuffer,
                                         &renderPassBeginInfo,
                                         SD()._commandbufferstates[cmdBuffer]
                                             ->beginRenderPassesList.back()
                                             ->subpassContentsData.Value());
            callvkCmdEndRenderPassByID(elem->Id(), cmdBuffer);
          }
        } else if (renderingInfoPtr) {
          VkRenderingInfo renderingInfo = *renderingInfoPtr;
          std::vector<VkRenderingAttachmentInfo> renderingAttInfoVector;
          bool changed = false;
          for (uint32_t i = 0; i < renderingInfo.colorAttachmentCount; i++) {
            VkRenderingAttachmentInfo renderingAttInfo = renderingInfo.pColorAttachments[i];
            if (renderingInfo.pColorAttachments[i].loadOp != VK_ATTACHMENT_LOAD_OP_LOAD) {
              renderingAttInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
              changed = true;
            }
            renderingAttInfoVector.push_back(renderingAttInfo);
          }
          if (renderingInfo.colorAttachmentCount > 0) {
            renderingInfo.pColorAttachments = renderingAttInfoVector.data();
          }
          VkRenderingAttachmentInfo depthAttInfo;
          if (renderingInfo.pDepthAttachment != VK_NULL_HANDLE) {
            depthAttInfo = *renderingInfo.pDepthAttachment;
            if (renderingInfo.pDepthAttachment->loadOp != VK_ATTACHMENT_LOAD_OP_LOAD) {
              depthAttInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
              changed = true;
            }
            renderingInfo.pDepthAttachment = &depthAttInfo;
          }
          VkRenderingAttachmentInfo stencilAttInfo;
          if (renderingInfo.pStencilAttachment != VK_NULL_HANDLE) {
            stencilAttInfo = *renderingInfo.pStencilAttachment;
            if (renderingInfo.pStencilAttachment->loadOp != VK_ATTACHMENT_LOAD_OP_LOAD) {
              stencilAttInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
              changed = true;
            }
            renderingInfo.pStencilAttachment = &stencilAttInfo;
          }
          if (changed) {
            callvkCmdBeginRenderingByID(getBeginRenderFunctionID(elem->Id()), cmdBuffer,
                                        &renderingInfo);
            callvkCmdEndRenderingByID(elem->Id(), cmdBuffer);
          }
        }
      }
      CGits::CCounter localCounter = {queueSubmitNumber, cmdBuffBatchNumber, cmdBuffNumber,
                                      renderPassCount};
      if (localCounter == Configurator::Get().vulkan.player.captureVulkanRenderPasses) {
        vulkanScheduleCopyRenderPasses(cmdBuffer, queueSubmitNumber, cmdBuffBatchNumber,
                                       cmdBuffNumber, renderPassCount);
      }
      if (localCounter == Configurator::Get().vulkan.player.captureVulkanRenderPassesResources) {
        vulkanScheduleCopyResources(cmdBuffer, queueSubmitNumber, cmdBuffBatchNumber, cmdBuffNumber,
                                    renderPassCount, VulkanDumpingMode::VULKAN_PER_RENDER_PASS);
      }
      renderPassCount++;
      drawInRenderPass = 0;
      if (Configurator::Get().vulkan.player.oneVulkanRenderPassPerCommandBuffer) {
        FinishCommandBufferAndSubmit(cmdBuffer);
        bool captureVulkanRenderPassesCheck =
            !Configurator::Get().vulkan.player.captureVulkanRenderPasses.empty() &&
            !SD()._commandbufferstates[cmdBuffer]->renderPassImages.empty();

        bool captureVulkanRenderPassesResourcesCheck =
            !Configurator::Get().vulkan.player.captureVulkanRenderPassesResources.empty() &&
            (!SD()._commandbufferstates[cmdBuffer]->renderPassResourceImages.empty() ||
             !SD()._commandbufferstates[cmdBuffer]->renderPassResourceBuffers.empty());
        if (captureVulkanRenderPassesCheck) {
          vulkanDumpRenderPasses(cmdBuffer);
        }
        if (captureVulkanRenderPassesResourcesCheck) {
          vulkanDumpRenderPassResources(cmdBuffer);
        }
        CreateNewCommandBuffer(elem, cmdBuffer);
        cmdBuffer = CVkCommandBuffer::GetMapping(elem->CommandBuffer());
        RestoreSettingsToSpecifiedObject(renderPassCount, VulkanObjectMode::MODE_VK_RENDER_PASS);
      }
    }
  }
}

void CLibrary::CVulkanCommandBufferTokensBuffer::RestoreToSpecifiedObject(
    const BitRange& objRange, VulkanObjectMode objMode) {
  uint64_t renderPassCount = 0;
  uint64_t blitCount = 0;
  uint64_t dispatchCount = 0;
  bool isRenderPassMode = objMode == VulkanObjectMode::MODE_VK_RENDER_PASS;
  bool isBlitMode = objMode == VulkanObjectMode::MODE_VK_BLIT;
  bool isDispatchMode = objMode == VulkanObjectMode::MODE_VK_DISPATCH;

  bool pre_recording = true;

  for (auto elem : _tokensList) {
    if (elem->Type() & CFunction::GITS_VULKAN_END_RENDER_PASS_APITYPE) {
      renderPassCount++;
    } else if (elem->Type() & CFunction::GITS_VULKAN_BLIT_APITYPE) {
      blitCount++;
    } else if (elem->Type() & CFunction::GITS_VULKAN_DISPATCH_APITYPE) {
      dispatchCount++;
    }
    if ((isRenderPassMode && objRange[renderPassCount] &&
         (elem->Type() & CFunction::GITS_VULKAN_BEGIN_RENDER_PASS_APITYPE)) ||
        (isBlitMode && objRange[blitCount]) || (isDispatchMode && objRange[dispatchCount])) {
      pre_recording = false;
    }
    if (pre_recording) {
      elem->Exec();
      elem->StateTrack();
    }
  }
}

void CLibrary::CVulkanCommandBufferTokensBuffer::ScheduleObject(
    void (*schedulerFunc)(Vulkan::CFunction*), const BitRange& objRange, VulkanObjectMode objMode) {
  uint64_t renderPassCount = 0;
  uint64_t blitCount = 0;
  uint64_t dispatchCount = 0;
  bool isRenderPassMode = objMode == VulkanObjectMode::MODE_VK_RENDER_PASS;
  bool isBlitMode = objMode == VulkanObjectMode::MODE_VK_BLIT;
  bool isDispatchMode = objMode == VulkanObjectMode::MODE_VK_DISPATCH;
  bool started = false;

  for (auto elem : _tokensList) {
    if (elem->Type() & CFunction::GITS_VULKAN_BLIT_APITYPE) {
      blitCount++;
    } else if (elem->Type() & CFunction::GITS_VULKAN_DISPATCH_APITYPE) {
      dispatchCount++;
    }
    if ((isRenderPassMode && objRange[renderPassCount] &&
         (elem->Type() & CFunction::GITS_VULKAN_BEGIN_RENDER_PASS_APITYPE)) ||
        (isBlitMode && objRange[blitCount]) || (isDispatchMode && objRange[dispatchCount])) {
      started = true;
    }
    if (((isRenderPassMode && objRange[renderPassCount]) || (isBlitMode && objRange[blitCount]) ||
         (isDispatchMode && objRange[dispatchCount])) &&
        started) {
      schedulerFunc(elem);
    } else if ((elem->Type() & CFunction::GITS_VULKAN_COMMAND_BUFFER_SET_APITYPE ||
                elem->Type() & CFunction::GITS_VULKAN_COMMAND_BUFFER_BIND_APITYPE ||
                elem->Type() & CFunction::GITS_VULKAN_COMMAND_BUFFER_PUSH_APITYPE) &&
               !started) {
      bool toSkip = false;
      for (auto obj : elem->GetMappedPointers()) {
        if (IsObjectToSkip(obj)) {
          toSkip = true;
          break;
        }
      }
      if (!toSkip) {
        schedulerFunc(elem);
      }
    }
    if (elem->Type() & CFunction::GITS_VULKAN_END_RENDER_PASS_APITYPE) {
      renderPassCount++;
    }
  }
  _tokensList.clear();
}
void CLibrary::CVulkanCommandBufferTokensBuffer::RestoreDraw(const uint64_t renderPassNumber,
                                                             const BitRange& drawsRange) {
  uint64_t renderPassCount = 0;
  uint64_t drawInRenderPass = 0;

  bool pre_draw = true;

  for (auto elem : _tokensList) {
    if (elem->Type() & CFunction::GITS_VULKAN_DRAW_APITYPE) {
      drawInRenderPass++;
    }
    if (renderPassCount == renderPassNumber && drawsRange[drawInRenderPass]) {
      pre_draw = false;
    }
    if (renderPassCount == renderPassNumber &&
        (elem->Type() & CFunction::GITS_VULKAN_BEGIN_RENDER_PASS_APITYPE)) {
      //  For executing each Vulkan draw in separate VkCommandBuffer we need to modify original storeOp (set it to STORE)
      elem->StateTrack();
      VkCommandBuffer cmdBuffer = CVkCommandBuffer::GetMapping(elem->CommandBuffer());
      VkRenderPassBeginInfo* renderPassBeginInfoPtr = SD()._commandbufferstates[cmdBuffer]
                                                          ->beginRenderPassesList.back()
                                                          ->renderPassBeginInfoData.Value();
      VkRenderingInfo* renderingInfoPtr = SD()._commandbufferstates[cmdBuffer]
                                              ->beginRenderPassesList.back()
                                              ->renderingInfoData.Value();
      if (renderPassBeginInfoPtr) {
        VkRenderPassBeginInfo renderPassBeginInfo = *renderPassBeginInfoPtr;
        renderPassBeginInfo.renderPass = SD()._commandbufferstates[cmdBuffer]
                                             ->beginRenderPassesList.back()
                                             ->renderPassStateStore->storeNoLoadRenderPassHandle;
        callvkCmdBeginRenderPassByID(elem->Id(), cmdBuffer, &renderPassBeginInfo,
                                     SD()._commandbufferstates[cmdBuffer]
                                         ->beginRenderPassesList.back()
                                         ->subpassContentsData.Value());
      } else if (renderingInfoPtr) {
        VkRenderingInfo renderingInfo = *renderingInfoPtr;
        std::vector<VkRenderingAttachmentInfo> renderingAttInfoVector;
        for (uint32_t i = 0; i < renderingInfo.colorAttachmentCount; i++) {
          VkRenderingAttachmentInfo renderingAttInfo = renderingInfo.pColorAttachments[i];
          renderingAttInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
          renderingAttInfoVector.push_back(renderingAttInfo);
        }
        if (renderingInfo.colorAttachmentCount > 0) {
          renderingInfo.pColorAttachments = renderingAttInfoVector.data();
        }
        VkRenderingAttachmentInfo depthAttInfo;
        if (renderingInfo.pDepthAttachment != VK_NULL_HANDLE) {
          depthAttInfo = *renderingInfo.pDepthAttachment;
          depthAttInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
          renderingInfo.pDepthAttachment = &depthAttInfo;
        }
        VkRenderingAttachmentInfo stencilAttInfo;
        if (renderingInfo.pStencilAttachment != VK_NULL_HANDLE) {
          stencilAttInfo = *renderingInfo.pStencilAttachment;
          stencilAttInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
          renderingInfo.pStencilAttachment = &stencilAttInfo;
        }
        callvkCmdBeginRenderingByID(elem->Id(), cmdBuffer, &renderingInfo);
      }
    } else if (pre_draw || (renderPassCount == renderPassNumber &&
                            (elem->Type() & CFunction::GITS_VULKAN_END_RENDER_PASS_APITYPE ||
                             elem->Type() & CFunction::GITS_VULKAN_NEXT_SUBPASS_APITYPE))) {
      elem->Exec();
      elem->StateTrack();
    }
    if (elem->Type() & CFunction::GITS_VULKAN_END_RENDER_PASS_APITYPE) {
      drawInRenderPass = 0;
      renderPassCount++;
    }
  }
}
void CLibrary::CVulkanCommandBufferTokensBuffer::ScheduleDraw(
    void (*schedulerFunc)(Vulkan::CFunction*),
    const uint64_t renderPassNumber,
    const BitRange& drawsRange) {
  uint64_t renderPassCount = 0;
  uint64_t drawInRenderPass = 0;
  bool started = false;

  for (auto elem : _tokensList) {
    if (elem->Type() & CFunction::GITS_VULKAN_DRAW_APITYPE) {
      drawInRenderPass++;
    }
    if (renderPassCount == renderPassNumber && drawsRange[drawInRenderPass]) {
      started = true;
    }
    if (renderPassCount == renderPassNumber &&
        (elem->Type() & CFunction::GITS_VULKAN_BEGIN_RENDER_PASS_APITYPE)) {
      VkCommandBuffer cmdBuffer = CVkCommandBuffer::GetMapping(elem->CommandBuffer());
      VkRenderPassBeginInfo* renderPassBeginInfoPtr = SD()._commandbufferstates[cmdBuffer]
                                                          ->beginRenderPassesList.back()
                                                          ->renderPassBeginInfoData.Value();
      VkRenderingInfo* renderingInfoPtr = SD()._commandbufferstates[cmdBuffer]
                                              ->beginRenderPassesList.back()
                                              ->renderingInfoData.Value();
      if (renderPassBeginInfoPtr) {
        VkRenderPassBeginInfo renderPassBeginInfo = *renderPassBeginInfoPtr;
        schedulevkCmdBeginRenderPassByID(elem->Id(), schedulerFunc, cmdBuffer, &renderPassBeginInfo,
                                         SD()._commandbufferstates[cmdBuffer]
                                             ->beginRenderPassesList.back()
                                             ->subpassContentsData.Value());
      } else if (renderingInfoPtr) {
        VkRenderingInfo renderingInfo = *renderingInfoPtr;
        std::vector<VkRenderingAttachmentInfo> renderingAttInfoVector;
        for (uint32_t i = 0; i < renderingInfo.colorAttachmentCount; i++) {
          VkRenderingAttachmentInfo renderingAttInfo = renderingInfo.pColorAttachments[i];
          if (renderingInfo.pColorAttachments[i].loadOp != VK_ATTACHMENT_LOAD_OP_LOAD) {
            renderingAttInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
          }
          renderingAttInfoVector.push_back(renderingAttInfo);
        }
        if (renderingInfo.colorAttachmentCount > 0) {
          renderingInfo.pColorAttachments = renderingAttInfoVector.data();
        }
        VkRenderingAttachmentInfo depthAttInfo;
        if (renderingInfo.pDepthAttachment != VK_NULL_HANDLE) {
          depthAttInfo = *renderingInfo.pDepthAttachment;
          if (renderingInfo.pDepthAttachment->loadOp != VK_ATTACHMENT_LOAD_OP_LOAD) {
            depthAttInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
          }
          renderingInfo.pDepthAttachment = &depthAttInfo;
        }
        VkRenderingAttachmentInfo stencilAttInfo;
        if (renderingInfo.pStencilAttachment != VK_NULL_HANDLE) {
          stencilAttInfo = *renderingInfo.pStencilAttachment;
          if (renderingInfo.pStencilAttachment->loadOp != VK_ATTACHMENT_LOAD_OP_LOAD) {
            stencilAttInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
          }
          renderingInfo.pStencilAttachment = &stencilAttInfo;
        }
        schedulevkCmdBeginRenderingByID(elem->Id(), schedulerFunc, cmdBuffer, &renderingInfo);
      }
    } else if (renderPassCount == renderPassNumber &&
               ((drawsRange[drawInRenderPass] && started) ||
                elem->Type() & CFunction::GITS_VULKAN_END_RENDER_PASS_APITYPE ||
                elem->Type() & CFunction::GITS_VULKAN_NEXT_SUBPASS_APITYPE)) {
      schedulerFunc(elem);
    } else if ((elem->Type() & CFunction::GITS_VULKAN_COMMAND_BUFFER_SET_APITYPE ||
                elem->Type() & CFunction::GITS_VULKAN_COMMAND_BUFFER_BIND_APITYPE ||
                elem->Type() & CFunction::GITS_VULKAN_COMMAND_BUFFER_PUSH_APITYPE) &&
               !started) {
      bool toSkip = false;
      for (auto obj : elem->GetMappedPointers()) {
        if (IsObjectToSkip(obj)) {
          toSkip = true;
          break;
        }
      }
      if (!toSkip) {
        schedulerFunc(elem);
      }
    }
    if (elem->Type() & CFunction::GITS_VULKAN_END_RENDER_PASS_APITYPE) {
      drawInRenderPass = 0;
      renderPassCount++;
    }
  }
  _tokensList.clear();
}

COnQueueSubmitEndInterface::~COnQueueSubmitEndInterface() {}

// Patches value which is found in a memory pointed to by the 'address'.
void CDeviceAddressPatcher::AddDirectAddress(VkDeviceAddress address) {
  _directAddresses.push_back(address);
}

// Patches value which is found in a memory pointed to by a found device address
// with added 'offset'. The device address is read from a memory pointed to by
// the 'address'.
void CDeviceAddressPatcher::AddIndirectAddress(VkDeviceAddress address, uint32_t offset) {
  _indirectAddresses.push_back({address, offset});
}

// - Direct Virtual Address points directly to a value which needs to be patched.
// - Indirect Virtual Address points to a memory location in which another Virtual Address
//   can be found. An offset is added to that VA to calculate a final memory from which
//   a value to be patched is read.
//
// Direct Virtual Addresses:
// -------------------------
//
//          Buffer a
//         |--------|
// dVA0 -> |  REF0  |   Buffer b
//         |--------|  |--------|
// dVA1 -------------> |  REF1  |   Buffer c
//                     |--------|  |--------|
// dVA2 -------------------------> |  REF2  |   Buffer d
//                                 |--------|  |--------|
// dVA3 -------------------------------------> |  REF3  |
//                                             |--------|
//
//
// Indirect Virtual Addresses:
// ---------------------------
//
//      Indirect buffer                       Buffer a
//         |--------|                        |--------|
// iVA0 -> |  dVA0  |      dVA0 + offset0 -> |  REF0  |   Buffer b
//         |--------|                        |--------|  |--------|
// iVA1 -> |  dVA1  |      dVA1 + offset1 -------------> |  REF1  |   Buffer c
//         |--------|                                    |--------|  |--------|
// iVA2 -> |  dVA2  |      dVA2 + offset2 -------------------------> |  REF2  |   Buffer d
//         |--------|                                                |--------|  |--------|
// iVA3 -> |  dVA3  |      dVA3 + offset3 -------------------------------------> |  REF3  |
//         |--------|                                                            |--------|
//
//
// A compute shader takes a list of the following pairs as an input:
// 1. Indirect address
// 2. Offset
//
// For each input pair, the compute shader:
// 1. Takes the indirect address (iVA) and reads a value (dVA) that is stored under the iVA.
// 2. Adds offset to the found dVA value.
// 3. Reads a value (REF) found under the calculated address (dVA + offset).
//
// In the output, the compute shader stores the following values for each input:
// 1. Calculated final address (dVA + offset).
// 2. Original REF value found at that address.
//
// This way GITS knows what value (REF) needs to be patched and where it can be found (dVA + offset).
// For each (dVA + offset) a buffer is found which is pointed to by that address. The list of
// buffers is then stored. This way, during stream replay, GITS can calculate proper addresses
// and patch/update values even when different device addresses are assigned to those buffers.
//
// In order to simplify a compute shader, direct VAs are converted to indirect VAs
// via a temporary buffer.

void CDeviceAddressPatcher::PrepareData(VkCommandBuffer commandBuffer, hash_t hash) {
  if ((hash == 0) || (Count() == 0)) {
    return;
  }

  CAutoCaller autoCaller(drvVk.vkPauseRecordingGITS, drvVk.vkContinueRecordingGITS);

  _hash = hash;
  auto count = Count();
  auto& commandBufferState = SD()._commandbufferstates[commandBuffer];
  auto device = commandBufferState->commandPoolStateStore->deviceStateStore->deviceHandle;

  VkMemoryBarrier inputMemoryBarrier = {
      VK_STRUCTURE_TYPE_MEMORY_BARRIER, // VkStructureType   sType;
      nullptr,                          // const void      * pNext;
      VK_ACCESS_MEMORY_WRITE_BIT,       // VkAccessFlags     srcAccessMask;
      VK_ACCESS_SHADER_READ_BIT         // VkAccessFlags     dstAccessMask;
  };

  VkMemoryBarrier outputMemoryBarrier = {
      VK_STRUCTURE_TYPE_MEMORY_BARRIER,                      // VkStructureType   sType;
      nullptr,                                               // const void      * pNext;
      VK_ACCESS_SHADER_READ_BIT,                             // VkAccessFlags     srcAccessMask;
      VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT // VkAccessFlags     dstAccessMask;
  };

  VkBufferMemoryBarrier outputBufferBarrier;

  VkDeviceAddress indirectBaseAddress = 0;
  if (_directAddresses.size() > 0) {
    uint32_t directAddressesSize = _directAddresses.size() * sizeof(_directAddresses[0]);
    auto indirectMemoryBufferPair = createTemporaryBuffer(
        device, directAddressesSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        commandBufferState.get(), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    mapMemoryAndCopyData(device, indirectMemoryBufferPair.first->deviceMemoryHandle, 0,
                         _directAddresses.data(), directAddressesSize);

    indirectBaseAddress =
        getBufferDeviceAddress(device, indirectMemoryBufferPair.second->bufferHandle);

    //VkMappedMemoryRange range = {
    //    VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,              // VkStructureType  sType;
    //    nullptr,                                            // const void     * pNext;
    //    indirectMemoryBufferPair.first->deviceMemoryHandle, // VkDeviceMemory   memory;
    //    0,                                                  // VkDeviceSize     offset;
    //    VK_WHOLE_SIZE                                       // VkDeviceSize     size;
    //};
    //drvVk.vkFlushMappedMemoryRanges(device, 1, &range);
  }

  std::vector<InputDataStruct> inputData;
  inputData.resize(count);

  // Prepare an input list for the compute shader with the indirect addresses and offsets.
  uint32_t s = 0;
  for (auto& addressAndOffset : _indirectAddresses) {
    inputData[s++] = {addressAndOffset.first, addressAndOffset.second, 0};
  }

  // To simplify the compute shader, all direct addresses are converted to indirect addresses by
  // storing dVAs in a buffer and providing virtual addresses of each element of that buffer instead.
  for (uint32_t a = 0; a < _directAddresses.size(); ++a) {
    inputData[s++] = {indirectBaseAddress + a * sizeof(VkDeviceAddress), 0, 0};
  }

  // Prepare a buffer with input data
  VkDeviceAddress inputDataAddress = 0;
  {
    uint32_t inputDataSize = count * sizeof(InputDataStruct);
    auto inputMemBuffPair =
        createTemporaryBuffer(device, inputDataSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                              commandBufferState.get(), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    inputDataAddress = getBufferDeviceAddress(device, inputMemBuffPair.second->bufferHandle);

    mapMemoryAndCopyData(device, inputMemBuffPair.first->deviceMemoryHandle, 0, inputData.data(),
                         inputDataSize);
  }

  // Prepare a buffer for the output
  VkDeviceAddress outputDataAddress = 0;
  {
    uint32_t outputDataSize = count * sizeof(OutputDataStruct);
    auto outputMemBuffPair =
        createTemporaryBuffer(device, outputDataSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                              commandBufferState.get(), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    outputDataAddress = getBufferDeviceAddress(device, outputMemBuffPair.second->bufferHandle);

    _device = device;
    _outputDeviceMemory = outputMemBuffPair.first->deviceMemoryHandle;
    outputBufferBarrier = {
        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, // VkStructureType   sType;
        nullptr,                                 // const void      * pNext;
        VK_ACCESS_SHADER_WRITE_BIT,              // VkAccessFlags     srcAccessMask;
        VK_ACCESS_HOST_READ_BIT,                 // VkAccessFlags     dstAccessMask;
        VK_QUEUE_FAMILY_IGNORED,                 // uint32_t          srcQueueFamilyIndex;
        VK_QUEUE_FAMILY_IGNORED,                 // uint32_t          dstQueueFamilyIndex;
        outputMemBuffPair.second->bufferHandle,  // VkBuffer          buffer;
        0,                                       // VkDeviceSize      offset;
        VK_WHOLE_SIZE                            // VkDeviceSize      size;
    };
  }

  // Dispatch the compute shader
  drvVk.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &inputMemoryBarrier, 0,
                             nullptr, 0, nullptr);
  drvVk.vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          SD().internalResources.internalPipelines[device]
                              .getPrepareDeviceAddressesForPatchingPipeline());
  VkDeviceAddress pushConstants[2] = {inputDataAddress, outputDataAddress};

  drvVk.vkCmdPushConstants(commandBuffer,
                           SD().internalResources.internalPipelines[device].getLayout(),
                           VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushConstants), pushConstants);
  drvVk.vkCmdDispatch(commandBuffer, count, 1, 1);
  drvVk.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_HOST_BIT, 0, 1,
                             &outputMemoryBarrier, 1, &outputBufferBarrier, 0, nullptr);

  if (commandBufferState->currentPipeline != VK_NULL_HANDLE) {
    drvVk.vkCmdBindPipeline(commandBuffer, commandBufferState->currentPipelineBindPoint,
                            commandBufferState->currentPipeline);
  }

  commandBufferState->queueSubmitEndMessageReceivers.push_back(this);
}

void CDeviceAddressPatcher::OnQueueSubmitEnd() {
  if (!Count()) {
    return;
  }

  std::vector<OutputDataStruct> outputData(Count());

  if (_outputDeviceMemory != VK_NULL_HANDLE) {
    CAutoCaller autoCaller(drvVk.vkPauseRecordingGITS, drvVk.vkContinueRecordingGITS);

    VkMappedMemoryRange range = {
        VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, // VkStructureType   sType;
        nullptr,                               // const void      * pNext;
        _outputDeviceMemory,                   // VkDeviceMemory    memory;
        0,                                     // VkDeviceSize      offset;
        VK_WHOLE_SIZE                          // VkDeviceSize      size;
    };

    void* src;
    drvVk.vkMapMemory(_device, _outputDeviceMemory, 0, VK_WHOLE_SIZE, 0, &src);
    drvVk.vkInvalidateMappedMemoryRanges(_device, 1, &range);

    memcpy(outputData.data(), src, outputData.size() * sizeof(outputData[0]));
  }

  std::vector<VkBufferDeviceAddressPatchGITS> streamData;
  for (auto& element : outputData) {
    // Memory location at which a value must be replaced
    CBufferDeviceAddressObjectData locationData(element.dVA);
    VkBufferDeviceAddressGITS location = {element.dVA,
                                          (VkBuffer)CGits::Instance().GetOrderedIdFromPtr(
                                              reinterpret_cast<void*>(locationData._buffer)),
                                          locationData._offset};

    // Value to be replaced during replay
    CBufferDeviceAddressObjectData patchedValueData(element.REF);
    VkBufferDeviceAddressGITS patchedValue = {
        element.REF,
        (VkBuffer)CGits::Instance().GetOrderedIdFromPtr(
            reinterpret_cast<void*>(patchedValueData._buffer)),
        patchedValueData._offset};

    streamData.push_back({location, patchedValue});
  }

  if (_hash != 0) {
    CGits::Instance().ResourceManager2().put(RESOURCE_DATA_RAW, streamData.data(),
                                             streamData.size() * sizeof(streamData[0]), _hash);
    _hash = 0;
  }

  _directAddresses.clear();
  _indirectAddresses.clear();
}

uint32_t CDeviceAddressPatcher::Count() const {
  return _directAddresses.size() + _indirectAddresses.size();
}

} //namespace Vulkan
} //namespace gits

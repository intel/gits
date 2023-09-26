// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
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
CLibrary::CLibrary(gits::CLibrary::state_creator_t stc) : gits::CLibrary(ID_VULKAN, stc) {}

CLibrary::~CLibrary() {
  waitForAllDevices();
  destroyDeviceLevelResources();
  destroyInstanceLevelResources();
}

gits::CResourceManager& gits::Vulkan::CLibrary::ProgramBinaryManager() {
  if (_progBinManager) {
    return *_progBinManager;
  }

  std::unordered_map<uint32_t, std::filesystem::path> the_map;
  the_map[RESOURCE_INDEX] = "gitsPlayerDataIndex.dat";
  the_map[RESOURCE_BUFFER] = "gitsPlayerBuffers.dat";

  auto type = Config::Get().recorder.extras.optimizations.hashType;
  const auto& ph = Config::Get().recorder.extras.optimizations.partialHash;
  _progBinManager.reset(
      new CResourceManager(the_map, Config::Get().recorder.extras.optimizations.asyncBufferWrites,
                           type, ph.enabled, ph.cutoff, ph.chunks, ph.ratio));
  return *_progBinManager;
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
    const BitRange& objRange, Config::VulkanObjectMode objMode, const uint64_t objNumber) {
  std::set<uint64_t> returnMap;
  uint64_t renderPassCount = 0;
  uint64_t drawInRenderPass = 0;
  bool pre_renderpass = true;
  bool pre_draw = true;
  bool isRenderPassMode = objMode == Config::MODE_VKRENDERPASS;
  bool isDrawMode = objMode == Config::MODE_VKDRAW;

  for (auto elem : _tokensList) {
    if (elem->Type() & CFunction::GITS_VULKAN_END_RENDERPASS_APITYPE) {
      drawInRenderPass = 0;
      renderPassCount++;
    } else if (elem->Type() & CFunction::GITS_VULKAN_DRAW_APITYPE) {
      drawInRenderPass++;
    }
    if (objRange[renderPassCount] &&
        (elem->Type() & CFunction::GITS_VULKAN_BEGIN_RENDERPASS_APITYPE)) {
      pre_renderpass = false;
    }
    if (renderPassCount == objNumber && objRange[drawInRenderPass]) {
      pre_draw = false;
    }
    if (objMode == Config::MODE_VKQUEUESUBMIT || objMode == Config::MODE_VKCOMMANDBUFFER ||
        (isRenderPassMode && objRange[renderPassCount]) ||
        (isDrawMode && renderPassCount == objNumber && objRange[drawInRenderPass]) ||
        (((isRenderPassMode && pre_renderpass) || (isDrawMode && pre_draw)) &&
         (elem->Type() & CFunction::GITS_VULKAN_CMDBUFFER_SET_APITYPE ||
          elem->Type() & CFunction::GITS_VULKAN_CMDBUFFER_BIND_APITYPE ||
          elem->Type() & CFunction::GITS_VULKAN_CMDBUFFER_PUSH_APITYPE))) {
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
  // restoring subpasses - we need the same number of subpasses as in original VkRenderPass
  for (auto elem : _tokensList) {
    if ((elem->Type() & CFunction::GITS_VULKAN_NEXT_SUBPASS_APITYPE) && (drawCount >= drawNumber) &&
        (renderPassCount == renderPassNumber)) {
      elem->Exec();
    } else if (elem->Type() & CFunction::GITS_VULKAN_DRAW_APITYPE) {
      drawCount++;
    } else if (elem->Type() & CFunction::GITS_VULKAN_END_RENDERPASS_APITYPE) {
      renderPassCount++;
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
    drvVk.vkCmdEndRenderPass(cmdBuffer);
    vkCmdEndRenderPass_SD(cmdBuffer);
  } else if (renderingInfo) {
    drvVk.vkCmdEndRendering(cmdBuffer);
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

void CLibrary::CVulkanCommandBufferTokensBuffer::CreateNewCommandBufferAndRestoreSettings(
    Vulkan::CFunction* token,
    uint64_t renderPassNumber,
    uint64_t drawNumber,
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
  uint64_t drawCount = 0;
  uint64_t renderPassCount = 0;
  for (auto elem : _tokensList) {
    if ((elem->Type() & CFunction::GITS_VULKAN_CMDBUFFER_SET_APITYPE ||
         elem->Type() & CFunction::GITS_VULKAN_CMDBUFFER_BIND_APITYPE ||
         elem->Type() & CFunction::GITS_VULKAN_CMDBUFFER_PUSH_APITYPE) &&
        (drawCount < drawNumber)) {
      // restoring VkCommandBuffer settings
      elem->Exec();
    } else if ((elem->Type() & CFunction::GITS_VULKAN_NEXT_SUBPASS_APITYPE) &&
               (drawCount < drawNumber) && (renderPassCount == renderPassNumber)) {
      // restoring subpasses - we need the same number of subpasses as in original VkRenderPass
      elem->Exec();
    } else if (elem->Type() & CFunction::GITS_VULKAN_DRAW_APITYPE) {
      drawCount++;
    } else if (elem->Type() & CFunction::GITS_VULKAN_END_RENDERPASS_APITYPE) {
      renderPassCount++;
    } else if ((elem->Type() & CFunction::GITS_VULKAN_BEGIN_RENDERPASS_APITYPE) &&
               (renderPassCount == renderPassNumber) &&
               (token->Type() & CFunction::GITS_VULKAN_DRAW_APITYPE)) {
      // Setting loadOp to LOAD, and storeOp to STORE
      VkRenderPassBeginInfo* renderPassBeginInfoPtr = SD()._commandbufferstates[newCmdBuffer]
                                                          ->beginRenderPassesList.back()
                                                          ->renderPassBeginInfoData.Value();
      VkRenderingInfo* renderingInfoPtr = SD()._commandbufferstates[newCmdBuffer]
                                              ->beginRenderPassesList.back()
                                              ->renderingInfoData.Value();
      if (renderPassBeginInfoPtr) {
        VkRenderPassBeginInfo renderPassBeginInfo = *renderPassBeginInfoPtr;
        renderPassBeginInfo.renderPass = SD()._commandbufferstates[newCmdBuffer]
                                             ->beginRenderPassesList.back()
                                             ->renderPassStateStore->loadAndStoreRenderPassHandle;
        drvVk.vkCmdBeginRenderPass(newCmdBuffer, &renderPassBeginInfo,
                                   SD()._commandbufferstates[newCmdBuffer]
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
        drvVk.vkCmdBeginRendering(newCmdBuffer, &renderingInfo);
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
  for (auto elem : _tokensList) {
    cmdBuffer = CVkCommandBuffer::GetMapping(elem->CommandBuffer());
    if (Config::Get().player.oneVulkanDrawPerCommandBuffer &&
        elem->Type() & CFunction::GITS_VULKAN_BEGIN_RENDERPASS_APITYPE) {
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
        drvVk.vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo,
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
        drvVk.vkCmdBeginRendering(cmdBuffer, &renderingInfo);
      }
    } else {
      elem->Exec();
      elem->StateTrack();
    }

    if (Config::Get().player.oneVulkanDrawPerCommandBuffer &&
        (elem->Type() & CFunction::GITS_VULKAN_DRAW_APITYPE)) {
      ++drawInRenderPass;
      ++drawCount;
      FinishRenderPass(renderPassCount, drawCount, cmdBuffer);
      CGits::CCounter localCounter = {queueSubmitNumber, cmdBuffBatchNumber, cmdBuffNumber,
                                      renderPassCount, drawInRenderPass};
      if (localCounter == Config::Get().player.captureVulkanDraws) {
        vulkanScheduleCopyRenderPasses(cmdBuffer, queueSubmitNumber, cmdBuffBatchNumber,
                                       cmdBuffNumber, renderPassCount, drawInRenderPass);
      }
      FinishCommandBufferAndSubmit(cmdBuffer);
      if (!SD()._commandbufferstates[cmdBuffer]->drawImages.empty()) {
        vulkanDumpRenderPasses(cmdBuffer);
      }
      CreateNewCommandBufferAndRestoreSettings(elem, renderPassCount, drawCount, cmdBuffer);
      cmdBuffer = CVkCommandBuffer::GetMapping(elem->CommandBuffer());
    }
    if (elem->Type() & CFunction::GITS_VULKAN_END_RENDERPASS_APITYPE) {
      if (Config::Get().player.oneVulkanDrawPerCommandBuffer) {
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
            drvVk.vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo,
                                       SD()._commandbufferstates[cmdBuffer]
                                           ->beginRenderPassesList.back()
                                           ->subpassContentsData.Value());
            drvVk.vkCmdEndRenderPass(cmdBuffer);
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
            drvVk.vkCmdBeginRendering(cmdBuffer, &renderingInfo);
            drvVk.vkCmdEndRendering(cmdBuffer);
          }
        }
      }
      CGits::CCounter localCounter = {queueSubmitNumber, cmdBuffBatchNumber, cmdBuffNumber,
                                      renderPassCount};
      if (localCounter == Config::Get().player.captureVulkanRenderPasses) {
        vulkanScheduleCopyRenderPasses(cmdBuffer, queueSubmitNumber, cmdBuffBatchNumber,
                                       cmdBuffNumber, renderPassCount);
      }
      if (localCounter == Config::Get().player.captureVulkanRenderPassesResources) {
        vulkanScheduleCopyResources(cmdBuffer, queueSubmitNumber, cmdBuffBatchNumber, cmdBuffNumber,
                                    renderPassCount);
      }
      renderPassCount++;
      drawInRenderPass = 0;
    }
  }
}

void CLibrary::CVulkanCommandBufferTokensBuffer::RestoreRenderPass(
    const BitRange& renderPassRange) {
  uint64_t renderPassCount = 0;

  bool pre_renderpass = true;

  for (auto elem : _tokensList) {
    if (elem->Type() & CFunction::GITS_VULKAN_END_RENDERPASS_APITYPE) {
      renderPassCount++;
    }
    if (renderPassRange[renderPassCount] &&
        (elem->Type() & CFunction::GITS_VULKAN_BEGIN_RENDERPASS_APITYPE)) {
      pre_renderpass = false;
    }
    if (pre_renderpass) {
      elem->Exec();
      elem->StateTrack();
    }
  }
}

void CLibrary::CVulkanCommandBufferTokensBuffer::ScheduleRenderPass(
    void (*schedulerFunc)(Vulkan::CFunction*), const BitRange& renderPassRange) {
  uint64_t renderPassCount = 0;
  bool started = false;

  for (auto elem : _tokensList) {
    if (renderPassRange[renderPassCount] &&
        (elem->Type() & CFunction::GITS_VULKAN_BEGIN_RENDERPASS_APITYPE)) {
      started = true;
    }
    if (renderPassRange[renderPassCount] && started) {
      schedulerFunc(elem);
    } else if ((elem->Type() & CFunction::GITS_VULKAN_CMDBUFFER_SET_APITYPE ||
                elem->Type() & CFunction::GITS_VULKAN_CMDBUFFER_BIND_APITYPE ||
                elem->Type() & CFunction::GITS_VULKAN_CMDBUFFER_PUSH_APITYPE) &&
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
    if (elem->Type() & CFunction::GITS_VULKAN_END_RENDERPASS_APITYPE) {
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
    if (renderPassCount == renderPassNumber && drawsRange[drawInRenderPass]) {
      pre_draw = false;
    }
    if (pre_draw || (renderPassCount == renderPassNumber &&
                     (elem->Type() & CFunction::GITS_VULKAN_END_RENDERPASS_APITYPE ||
                      elem->Type() & CFunction::GITS_VULKAN_NEXT_SUBPASS_APITYPE))) {
      elem->Exec();
      elem->StateTrack();
    }
    if (elem->Type() & CFunction::GITS_VULKAN_END_RENDERPASS_APITYPE) {
      drawInRenderPass = 0;
      renderPassCount++;
    } else if (elem->Type() & CFunction::GITS_VULKAN_DRAW_APITYPE) {
      drawInRenderPass++;
    } else if (renderPassCount == renderPassNumber &&
               (elem->Type() & CFunction::GITS_VULKAN_BEGIN_RENDERPASS_APITYPE)) {
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
        drvVk.vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo,
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
        drvVk.vkCmdBeginRendering(cmdBuffer, &renderingInfo);
      }
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
    } else if (renderPassCount == renderPassNumber &&
               (elem->Type() & CFunction::GITS_VULKAN_BEGIN_RENDERPASS_APITYPE)) {
      VkCommandBuffer cmdBuffer = CVkCommandBuffer::GetMapping(elem->CommandBuffer());
      VkRenderPassBeginInfo* renderPassBeginInfoPtr = SD()._commandbufferstates[cmdBuffer]
                                                          ->beginRenderPassesList.back()
                                                          ->renderPassBeginInfoData.Value();
      VkRenderingInfo* renderingInfoPtr = SD()._commandbufferstates[cmdBuffer]
                                              ->beginRenderPassesList.back()
                                              ->renderingInfoData.Value();
      if (renderPassBeginInfoPtr) {
        VkRenderPassBeginInfo renderPassBeginInfo = *renderPassBeginInfoPtr;
        schedulerFunc(new CvkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo,
                                                SD()._commandbufferstates[cmdBuffer]
                                                    ->beginRenderPassesList.back()
                                                    ->subpassContentsData.Value()));

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
        schedulerFunc(new CvkCmdBeginRendering(cmdBuffer, &renderingInfo));
      }
    }
    if (renderPassCount == renderPassNumber && drawsRange[drawInRenderPass]) {
      started = true;
    }
    if (renderPassCount == renderPassNumber &&
        ((drawsRange[drawInRenderPass] && started) ||
         elem->Type() & CFunction::GITS_VULKAN_END_RENDERPASS_APITYPE ||
         elem->Type() & CFunction::GITS_VULKAN_NEXT_SUBPASS_APITYPE)) {
      schedulerFunc(elem);
    } else if ((elem->Type() & CFunction::GITS_VULKAN_CMDBUFFER_SET_APITYPE ||
                elem->Type() & CFunction::GITS_VULKAN_CMDBUFFER_BIND_APITYPE ||
                elem->Type() & CFunction::GITS_VULKAN_CMDBUFFER_PUSH_APITYPE) &&
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
    if (elem->Type() & CFunction::GITS_VULKAN_END_RENDERPASS_APITYPE) {
      drawInRenderPass = 0;
      renderPassCount++;
    }
  }
  _tokensList.clear();
}

#ifndef BUILD_FOR_CCODE
// Kudos to Piotr Horodecki
bool MemoryAliasingTracker::Range::operator()(Range const& lRange, Range const& rRange) const {
  return lRange.offset < rRange.offset;
}

MemoryAliasingTracker::RangeSetType::iterator MemoryAliasingTracker::GetRange(uint64_t offset) {
  // Get an iterator to the first offset greater than given offset
  auto iterator = MemoryRanges.upper_bound({offset, 0, {}});
  // Decrement to obtain an iterator to the range which includes given offset
  iterator--;
  return iterator;
}

void MemoryAliasingTracker::SplitRange(uint64_t offset) {
  auto existingRangeIterator = GetRange(offset);
  if (existingRangeIterator->offset < offset) {
    // Create two new ranges: left range and right range by cutting existing range at offset
    Range leftRange = {existingRangeIterator->offset, offset - existingRangeIterator->offset,
                       existingRangeIterator->resources};
    Range rightRange = {offset, existingRangeIterator->size - leftRange.size, leftRange.resources};
    assert(leftRange.size > 0);
    assert(rightRange.size > 0);
    // Remove existing range
    MemoryRanges.erase(existingRangeIterator);
    // Insert two new ranges
    MemoryRanges.insert(leftRange);
    MemoryRanges.insert(rightRange);
  }
}

void MemoryAliasingTracker::AddResource(uint64_t offset,
                                        uint64_t size,
                                        std::pair<uint64_t, bool> const& resource) {
  // Split range if needed on the resource beginning.
  SplitRange(offset);

  // Split range if needed on the resource end.
  SplitRange(offset + size);

  auto beginIterator = GetRange(offset);
  auto endIterator = GetRange(offset + size);

  // Update counters in ranges occupied by the resource.
  for (auto i = beginIterator; i->offset < endIterator->offset; i++) {
    i->resources.insert(resource);
  }
}

void MemoryAliasingTracker::RemoveResource(uint64_t offset,
                                           uint64_t size,
                                           std::pair<uint64_t, bool> const& resource) {
  auto beginIterator = GetRange(offset);
  auto endIterator = GetRange(offset + size);

  // Update counters in ranges occupied by the resource.
  for (auto i = beginIterator; i->offset < endIterator->offset; i++) {
    i->resources.erase(resource);
  }

  for (auto current = beginIterator; current->offset < endIterator->offset;) {
    auto next = current;
    next++;

    if ((next->offset < endIterator->offset) && (current->resources == next->resources)) {
      Range newRange = {current->offset, current->size + next->size, current->resources};
      MemoryRanges.erase(current);
      MemoryRanges.erase(next);
      MemoryRanges.insert(newRange);

      beginIterator = GetRange(offset);
      endIterator = GetRange(offset + size);
      current = beginIterator;
    } else {
      current++;
    }
  }
}

std::set<std::pair<uint64_t, bool>> MemoryAliasingTracker::GetAliasedResourcesForResource(
    uint64_t offset, uint64_t size, std::pair<uint64_t, bool> const& resource) {
  std::set<std::pair<uint64_t, bool>> aliasedResources;

  auto beginIterator = GetRange(offset);
  auto endIterator = GetRange(offset + size);

  for (auto i = beginIterator; i->offset < endIterator->offset; ++i) {
    aliasedResources.insert(i->resources.begin(), i->resources.end());
  }

  aliasedResources.erase(resource);

  return aliasedResources;
}

MemoryAliasingTracker::MemoryAliasingTracker(uint64_t size) {
  // Add whole device memory range.
  MemoryRanges.insert({0, size, {}});
  // Add "after whole device memory empty range" (fake) with offset == size of
  // vkDeviceMemory and size == 0. This allows to use SplitRange() method in
  // case of splitting range at the end of adding resource.
  MemoryRanges.insert({size, 0, {}});
}

void MemoryAliasingTracker::AddImage(uint64_t offset, uint64_t size, VkImage image) {
  AddResource(offset, size, {(uint64_t)image, true});
}

void MemoryAliasingTracker::AddBuffer(uint64_t offset, uint64_t size, VkBuffer buffer) {
  AddResource(offset, size, {(uint64_t)buffer, false});
}

void MemoryAliasingTracker::RemoveImage(uint64_t offset, uint64_t size, VkImage image) {
  RemoveResource(offset, size, {(uint64_t)image, true});
}

void MemoryAliasingTracker::RemoveBuffer(uint64_t offset, uint64_t size, VkBuffer buffer) {
  RemoveResource(offset, size, {(uint64_t)buffer, false});
}

std::set<std::pair<uint64_t, bool>> MemoryAliasingTracker::GetAliasedResourcesForImage(
    uint64_t offset, uint64_t size, VkImage image) {
  return GetAliasedResourcesForResource(offset, size, {(uint64_t)image, true});
}

std::set<std::pair<uint64_t, bool>> MemoryAliasingTracker::GetAliasedResourcesForBuffer(
    uint64_t offset, uint64_t size, VkBuffer buffer) {
  return GetAliasedResourcesForResource(offset, size, {(uint64_t)buffer, false});
}

COnQueueSubmitEnd::~COnQueueSubmitEnd() {}

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

    VkMappedMemoryRange range = {
        VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,              // VkStructureType  sType;
        nullptr,                                            // const void     * pNext;
        indirectMemoryBufferPair.first->deviceMemoryHandle, // VkDeviceMemory   memory;
        0,                                                  // VkDeviceSize     offset;
        VK_WHOLE_SIZE                                       // VkDeviceSize     size;
    };
    drvVk.vkFlushMappedMemoryRanges(device, 1, &range);
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
  if (_hash == 0) {
    return;
  }

  std::vector<OutputDataStruct> outputData;
  outputData.resize(Count());
  std::vector<VkBufferDeviceAddressPatchGITS> binaryData;
  binaryData.reserve(Count());

  if (_outputDeviceMemory != VK_NULL_HANDLE) {
    CAutoCaller autoCaller(drvVk.vkPauseRecordingGITS, drvVk.vkContinueRecordingGITS);

    VkMappedMemoryRange range = {
        VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, // VkStructureType   sType;
        nullptr,                               // const void      * pNext;
        _outputDeviceMemory,                   // VkDeviceMemory    memory;
        0,                                     // VkDeviceSize      offset;
        VK_WHOLE_SIZE                          // VkDeviceSize      size;
    };
    drvVk.vkInvalidateMappedMemoryRanges(_device, 1, &range);
    void* src;
    drvVk.vkMapMemory(_device, _outputDeviceMemory, 0, VK_WHOLE_SIZE, 0, &src);
    memcpy(outputData.data(), src, outputData.size() * sizeof(outputData[0]));
  }

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

    binaryData.push_back({location, patchedValue});
  }

  CGits::Instance().ResourceManager().put(RESOURCE_DATA_RAW, binaryData.data(),
                                          binaryData.size() * sizeof(binaryData[0]), _hash);

  _directAddresses.clear();
  _indirectAddresses.clear();
  _hash = 0;
}

uint32_t CDeviceAddressPatcher::Count() const {
  return _directAddresses.size() + _indirectAddresses.size();
}

#endif
} //namespace Vulkan
} //namespace gits

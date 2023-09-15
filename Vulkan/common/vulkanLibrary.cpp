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

#include "vulkanLibrary.h"
#include "vulkanTools.h"
#include "gits.h"
#include "vulkanStateDynamic.h"
#include "vulkanFunctions.h"

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

std::set<uint64_t> CLibrary::CVulkanCommandBufferTokensBuffer::GetMappedPointers(
    const BitRange& objRange, Config::VulkanObjectMode objMode) {
  std::set<uint64_t> returnMap;
  uint64_t renderPassCount = 0;
  bool pre_renderpass = true;

  for (auto elem : _tokensList) {
    if (elem->Type() & CFunction::GITS_VULKAN_END_RENDERPASS_APITYPE) {
      renderPassCount++;
    }
    if (objRange[renderPassCount] &&
        (elem->Type() & CFunction::GITS_VULKAN_BEGIN_RENDERPASS_APITYPE)) {
      pre_renderpass = false;
    }
    for (auto obj : elem->GetMappedPointers()) {
      if (objMode == Config::MODE_VKQUEUESUBMIT || objMode == Config::MODE_VKCOMMANDBUFFER ||
          (objMode == Config::MODE_VKRENDERPASS &&
           (objRange[renderPassCount] ||
            (pre_renderpass && (elem->Type() & CFunction::GITS_VULKAN_CMDBUFFER_SET_APITYPE ||
                                elem->Type() & CFunction::GITS_VULKAN_CMDBUFFER_BIND_APITYPE ||
                                elem->Type() & CFunction::GITS_VULKAN_CMDBUFFER_PUSH_APITYPE))))) {
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

void CLibrary::CVulkanCommandBufferTokensBuffer::FinishCommandBufferAndRestoreSettings(
    Vulkan::CFunction* token,
    uint64_t renderPassNumber,
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

  if (token->Type() & CFunction::GITS_VULKAN_DRAW_APITYPE) {
    VkRenderPassBeginInfo* renderPassBeginInfo = SD()._commandbufferstates[cmdBuffer]
                                                     ->beginRenderPassesList.back()
                                                     ->renderPassBeginInfoData.Value();
    VkRenderingInfo* renderingInfo = SD()._commandbufferstates[cmdBuffer]
                                         ->beginRenderPassesList.back()
                                         ->renderingInfoData.Value();
    if (renderPassBeginInfo) {
      drvVk.vkCmdEndRenderPass(cmdBuffer);
    } else if (renderingInfo) {
      drvVk.vkCmdEndRendering(cmdBuffer);
    }
  }
  drvVk.vkEndCommandBuffer(cmdBuffer);

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
  VkSubmitInfo submitInfoNew = {
      VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr, 0, nullptr, nullptr, 1, &cmdBuffer, 0, nullptr};
  VkQueue queue =
      SD()._commandpoolstates[cmdPool]->deviceStateStore->queueStateStoreList[0]->queueHandle;
  drvVk.vkQueueSubmit(queue, 1, &submitInfoNew, VK_NULL_HANDLE);
  drvVk.vkQueueWaitIdle(queue);
  VkDevice device = SD()._commandpoolstates[cmdPool]->deviceStateStore->deviceHandle;
  drvVk.vkFreeCommandBuffers(device, cmdPool, 1, &cmdBuffer);
  drvVk.vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &newCmdBuffer);
  if (newCmdBuffer != cmdBuffer) {
    SD()._commandbufferstates[cmdBuffer].swap(SD()._commandbufferstates[newCmdBuffer]);
    SD()._commandbufferstates.erase(cmdBuffer);
    CVkCommandBuffer::AddMapping(token->CommandBuffer(), newCmdBuffer);
  }
  drvVk.vkBeginCommandBuffer(newCmdBuffer,
                             SD()._commandbufferstates[newCmdBuffer]
                                 ->beginCommandBuffer->commandBufferBeginInfoData.Value());
  drawCount = 0;
  renderPassCount = 0;
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
      ++drawCount;
      FinishCommandBufferAndRestoreSettings(elem, renderPassCount, drawCount, cmdBuffer);
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

#ifndef BUILD_FOR_CCODE
// Code borrowed from development team's Piotr Horodecki
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
#endif
} //namespace Vulkan
} //namespace gits

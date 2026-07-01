// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "imageLayoutService.h"
#include "stateTrackingService.h"
#include "objectState.h"

namespace gits {
namespace vulkan {

ImageLayoutService::ImageLayoutService(StateTrackingService& sts) : m_StateTracking(sts) {}

// Buffer the layout into the recording command buffer (last write wins per
// image).  It is committed to ImageState::CurrentLayout at submit time.
void ImageLayoutService::RecordImageLayout(uint64_t cbKey,
                                           uint64_t imageKey,
                                           VkImageLayout newLayout) {
  if (!imageKey) {
    return;
  }
  auto* cb = m_StateTracking.GetState<CommandBufferState>(cbKey);
  if (!cb) {
    return;
  }
  cb->ImageLayoutAfterSubmit[imageKey] = newLayout;
}

// ---------------------------------------------------------------------------
// Explicit pipeline barriers (record time)
// ---------------------------------------------------------------------------

void ImageLayoutService::OnPipelineBarrier(uint64_t cbKey,
                                           const VkImageMemoryBarrier* barriers,
                                           uint32_t count,
                                           const std::vector<uint64_t>& handleKeys) {
  if (!barriers || count == 0) {
    return;
  }
  // HandleKeys has one entry per barrier whose image != VK_NULL_HANDLE.
  uint32_t keyIdx = 0;
  for (uint32_t i = 0; i < count; ++i) {
    const VkImageMemoryBarrier& b = barriers[i];
    if (b.image != VK_NULL_HANDLE && keyIdx < handleKeys.size()) {
      const uint64_t imageKey = handleKeys[keyIdx++];
      if (b.newLayout != VK_IMAGE_LAYOUT_UNDEFINED) {
        RecordImageLayout(cbKey, imageKey, b.newLayout);
      }
    }
  }
}

void ImageLayoutService::OnPipelineBarrier2(uint64_t cbKey,
                                            const VkDependencyInfo& depInfo,
                                            const std::vector<uint64_t>& handleKeys) {
  // HandleKeys layout: [bufferBarrierKeys...][imageBarrierKeys...]
  // Each slot is consumed unconditionally (no null check in the player updater).
  uint32_t keyIdx = depInfo.bufferMemoryBarrierCount; // skip past buffer keys
  for (uint32_t i = 0; i < depInfo.imageMemoryBarrierCount && keyIdx < handleKeys.size();
       ++i, ++keyIdx) {
    const VkImageMemoryBarrier2& b = depInfo.pImageMemoryBarriers[i];
    if (b.newLayout != VK_IMAGE_LAYOUT_UNDEFINED) {
      RecordImageLayout(cbKey, handleKeys[keyIdx], b.newLayout);
    }
  }
}

// ---------------------------------------------------------------------------
// Render pass final-layout tracking (record time)
// ---------------------------------------------------------------------------

void ImageLayoutService::OnBeginRenderPass(uint64_t cbKey,
                                           uint64_t renderPassKey,
                                           uint64_t framebufferKey,
                                           const std::vector<uint64_t>& beginInfoAttachmentKeys) {
  auto* rp = m_StateTracking.GetState<RenderPassState>(renderPassKey);
  auto* fb = m_StateTracking.GetState<FramebufferState>(framebufferKey);
  if (!rp || !fb) {
    return;
  }

  ImageLayoutPairs& pairs = m_ActiveRenderPasses[cbKey];
  pairs.clear();

  const auto& finalLayouts = rp->AttachmentFinalLayouts;
  // For imageless framebuffers, AttachmentImageViewKeys is empty (pAttachments == NULL
  // at create time); fall back to the per-begin keys from VkRenderPassAttachmentBeginInfo.
  const auto& ivKeys =
      fb->AttachmentImageViewKeys.empty() ? beginInfoAttachmentKeys : fb->AttachmentImageViewKeys;
  const uint32_t count = static_cast<uint32_t>(std::min(finalLayouts.size(), ivKeys.size()));

  for (uint32_t i = 0; i < count; ++i) {
    const uint64_t ivKey = ivKeys[i];
    if (!ivKey) {
      continue;
    }
    const auto* iv = m_StateTracking.GetState<ImageViewState>(ivKey);
    if (!iv || !iv->ImageKey) {
      continue;
    }
    // Only record non-UNDEFINED final layouts - UNDEFINED means "contents discarded,
    // no specific layout guaranteed", so we leave currentLayout unchanged.
    if (finalLayouts[i] != VK_IMAGE_LAYOUT_UNDEFINED) {
      pairs.emplace_back(iv->ImageKey, finalLayouts[i]);
    }
  }
}

void ImageLayoutService::OnEndRenderPass(uint64_t cbKey) {
  const auto it = m_ActiveRenderPasses.find(cbKey);
  if (it != m_ActiveRenderPasses.end()) {
    // Buffer the implicit finalLayout transitions into the recording CB; they
    // are committed to CurrentLayout when the CB is submitted.
    for (const auto& [imageKey, finalLayout] : it->second) {
      RecordImageLayout(cbKey, imageKey, finalLayout);
    }
    m_ActiveRenderPasses.erase(it);
  }
}

void ImageLayoutService::OnResetCommandBuffer(uint64_t cbKey) {
  m_ActiveRenderPasses.erase(cbKey);
}

// ---------------------------------------------------------------------------
// Submit-time application
// ---------------------------------------------------------------------------

void ImageLayoutService::ApplyCommandBuffer(uint64_t cbKey) {
  auto* cb = m_StateTracking.GetState<CommandBufferState>(cbKey);
  if (!cb) {
    return;
  }
  for (const auto& [imageKey, layout] : cb->ImageLayoutAfterSubmit) {
    auto* imgState = m_StateTracking.GetState<ImageState>(imageKey);
    if (imgState) {
      imgState->CurrentLayout = layout;
    }
  }
}

void ImageLayoutService::OnQueueSubmit(const VkSubmitInfo* pSubmits,
                                       uint32_t submitCount,
                                       const std::vector<uint64_t>& handleKeys) {
  if (!pSubmits) {
    return;
  }
  // HandleKeys per submit: [waitSemaphores][commandBuffers][signalSemaphores].
  // Mirrors SyncStateService::OnQueueSubmit so the indices stay in lock-step.
  uint32_t keyIdx = 0;
  for (uint32_t s = 0; s < submitCount; ++s) {
    const VkSubmitInfo& info = pSubmits[s];
    for (uint32_t w = 0; w < info.waitSemaphoreCount && keyIdx < handleKeys.size(); ++w, ++keyIdx) {
    }
    for (uint32_t c = 0; c < info.commandBufferCount && keyIdx < handleKeys.size(); ++c, ++keyIdx) {
      ApplyCommandBuffer(handleKeys[keyIdx]);
    }
    for (uint32_t sg = 0; sg < info.signalSemaphoreCount && keyIdx < handleKeys.size();
         ++sg, ++keyIdx) {
    }
  }
}

void ImageLayoutService::OnQueueSubmit2(const VkSubmitInfo2* pSubmits,
                                        uint32_t submitCount,
                                        const std::vector<uint64_t>& handleKeys) {
  if (!pSubmits) {
    return;
  }
  // HandleKeys per submit: [waitSemaphoreInfos][commandBufferInfos][signalSemaphoreInfos].
  // Mirrors SyncStateService::OnQueueSubmit2 so the indices stay in lock-step.
  uint32_t keyIdx = 0;
  for (uint32_t s = 0; s < submitCount; ++s) {
    const VkSubmitInfo2& info = pSubmits[s];
    for (uint32_t w = 0; w < info.waitSemaphoreInfoCount && keyIdx < handleKeys.size();
         ++w, ++keyIdx) {
    }
    for (uint32_t c = 0; c < info.commandBufferInfoCount && keyIdx < handleKeys.size();
         ++c, ++keyIdx) {
      ApplyCommandBuffer(handleKeys[keyIdx]);
    }
    for (uint32_t sg = 0; sg < info.signalSemaphoreInfoCount && keyIdx < handleKeys.size();
         ++sg, ++keyIdx) {
    }
  }
}

void ImageLayoutService::MergeSecondary(uint64_t primaryKey, uint64_t secondaryKey) {
  auto* prim = m_StateTracking.GetState<CommandBufferState>(primaryKey);
  auto* sec = m_StateTracking.GetState<CommandBufferState>(secondaryKey);
  if (!prim || !sec) {
    return;
  }
  // Last write wins: the secondary executes at the vkCmdExecuteCommands point,
  // so a later barrier in the primary (recorded after) correctly overwrites it.
  for (const auto& [imageKey, layout] : sec->ImageLayoutAfterSubmit) {
    prim->ImageLayoutAfterSubmit[imageKey] = layout;
  }
}

} // namespace vulkan
} // namespace gits

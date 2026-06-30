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

// ---------------------------------------------------------------------------
// Explicit pipeline barriers
// ---------------------------------------------------------------------------

void ImageLayoutService::OnPipelineBarrier(const VkImageMemoryBarrier* barriers,
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
        auto* imgState = m_StateTracking.GetState<ImageState>(imageKey);
        if (imgState) {
          imgState->CurrentLayout = b.newLayout;
        }
      }
    }
  }
}

void ImageLayoutService::OnPipelineBarrier2(const VkDependencyInfo& depInfo,
                                            const std::vector<uint64_t>& handleKeys) {
  // HandleKeys layout: [bufferBarrierKeys...][imageBarrierKeys...]
  // Each slot is consumed unconditionally (no null check in the player updater).
  uint32_t keyIdx = depInfo.bufferMemoryBarrierCount; // skip past buffer keys
  for (uint32_t i = 0; i < depInfo.imageMemoryBarrierCount && keyIdx < handleKeys.size();
       ++i, ++keyIdx) {
    const VkImageMemoryBarrier2& b = depInfo.pImageMemoryBarriers[i];
    if (b.newLayout != VK_IMAGE_LAYOUT_UNDEFINED) {
      auto* imgState = m_StateTracking.GetState<ImageState>(handleKeys[keyIdx]);
      if (imgState) {
        imgState->CurrentLayout = b.newLayout;
      }
    }
  }
}

// ---------------------------------------------------------------------------
// Render pass final-layout tracking
// ---------------------------------------------------------------------------

void ImageLayoutService::OnBeginRenderPass(uint64_t cbKey,
                                           uint64_t renderPassKey,
                                           uint64_t framebufferKey) {
  auto* rp = m_StateTracking.GetState<RenderPassState>(renderPassKey);
  auto* fb = m_StateTracking.GetState<FramebufferState>(framebufferKey);
  if (!rp || !fb) {
    return;
  }

  ImageLayoutPairs& pairs = m_ActiveRenderPasses[cbKey];
  pairs.clear();

  const auto& finalLayouts = rp->AttachmentFinalLayouts;
  const auto& ivKeys = fb->AttachmentImageViewKeys;
  const uint32_t count = static_cast<uint32_t>(std::min(finalLayouts.size(), ivKeys.size()));

  for (uint32_t i = 0; i < count; ++i) {
    const uint64_t ivKey = ivKeys[i];
    if (!ivKey) {
      continue; // null slot (imageless framebuffer placeholder)
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
  ApplyPendingFinalLayouts(cbKey);
  m_ActiveRenderPasses.erase(cbKey);
}

void ImageLayoutService::OnResetCommandBuffer(uint64_t cbKey) {
  m_ActiveRenderPasses.erase(cbKey);
}

void ImageLayoutService::ApplyPendingFinalLayouts(uint64_t cbKey) {
  const auto it = m_ActiveRenderPasses.find(cbKey);
  if (it == m_ActiveRenderPasses.end()) {
    return;
  }
  for (const auto& [imageKey, finalLayout] : it->second) {
    auto* imgState = m_StateTracking.GetState<ImageState>(imageKey);
    if (imgState) {
      imgState->CurrentLayout = finalLayout;
    }
  }
}

} // namespace vulkan
} // namespace gits

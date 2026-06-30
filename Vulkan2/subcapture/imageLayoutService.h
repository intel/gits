// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "vulkanHeader2.h"

#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

namespace gits {
namespace vulkan {

class StateTrackingService;

// Tracks the current layout of VkImages as the command stream executes.
// Handles both explicit pipeline barriers and implicit render-pass final-layout
// transitions.  SubcaptureLayer::Post overrides are thin dispatchers into here
// so all layout-tracking logic lives in one place.
class ImageLayoutService {
public:
  explicit ImageLayoutService(StateTrackingService& sts);

  // ---- Explicit pipeline barriers ----------------------------------------

  // Called from Post(vkCmdPipelineBarrierCommand).
  // handleKeys: m_pImageMemoryBarriers.HandleKeys - one entry per non-null image.
  void OnPipelineBarrier(const VkImageMemoryBarrier* barriers,
                         uint32_t count,
                         const std::vector<uint64_t>& handleKeys);

  // Called from Post(vkCmdPipelineBarrier2Command/KHR) - both use VkDependencyInfo.
  // handleKeys layout: [bufferBarrierKeys...][imageBarrierKeys...]
  void OnPipelineBarrier2(const VkDependencyInfo& depInfo, const std::vector<uint64_t>& handleKeys);

  // ---- Render pass final-layout tracking ---------------------------------
  //
  // vkCmdBeginRenderPass records which images are bound as attachments so that
  // vkCmdEndRenderPass can apply the implicit finalLayout transitions.
  //
  // renderPassKey:   key of the VkRenderPass (carries attachmentFinalLayouts)
  // framebufferKey:  key of the VkFramebuffer (carries attachmentImageViewKeys)
  void OnBeginRenderPass(uint64_t cbKey, uint64_t renderPassKey, uint64_t framebufferKey);

  // Apply the finalLayout of every render-pass attachment to currentLayout and
  // clear the per-CB tracking entry.
  void OnEndRenderPass(uint64_t cbKey);

  // Called when a command buffer is reset (vkResetCommandBuffer or
  // vkBeginCommandBuffer) so stale render-pass tracking is discarded.
  void OnResetCommandBuffer(uint64_t cbKey);

private:
  // Per image key: the finalLayout to apply when the render pass ends.
  using ImageLayoutPairs = std::vector<std::pair<uint64_t, VkImageLayout>>;

  // Apply all pending (imageKey, finalLayout) pairs for the given CB and
  // remove the entry from m_ActiveRenderPasses.
  void ApplyPendingFinalLayouts(uint64_t cbKey);

  StateTrackingService& m_StateTracking;

  // cbKey ? list of (imageKey, finalLayout) pairs built at BeginRenderPass time.
  std::unordered_map<uint64_t, ImageLayoutPairs> m_ActiveRenderPasses;
};

} // namespace vulkan
} // namespace gits

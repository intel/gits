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
//
// Image layout transitions take effect on the GPU at queue-submit time, not at
// vkCmd* record time.  Applying them at record time captures stale mid-frame
// layouts when the application records the next frame's command buffers while
// the current frame is still in flight, leaving ImageState::CurrentLayout wrong
// at the subcapture cut (VUID-vkCmdDraw-None-09600).  To avoid this the per-CB
// transitions are buffered in CommandBufferState::ImageLayoutAfterSubmit at
// record time and applied to ImageState::CurrentLayout when the CB is submitted.
// Mirrors the legacy vulkanStateTracking imageLayoutAfterSubmit / vkQueueSubmit
// behaviour, and the sibling QueryPoolStateService.
class ImageLayoutService {
public:
  explicit ImageLayoutService(StateTrackingService& sts);

  // ---- Explicit pipeline barriers (record time) --------------------------

  // Called from Post(vkCmdPipelineBarrierCommand) and the sync1 event-wait path.
  // handleKeys: m_pImageMemoryBarriers.HandleKeys - one entry per non-null image.
  void OnPipelineBarrier(uint64_t cbKey,
                         const VkImageMemoryBarrier* barriers,
                         uint32_t count,
                         const std::vector<uint64_t>& handleKeys);

  // Called from Post(vkCmdPipelineBarrier2Command/KHR) and the sync2 event-wait
  // path - both use VkDependencyInfo.
  // handleKeys layout: [bufferBarrierKeys...][imageBarrierKeys...]
  void OnPipelineBarrier2(uint64_t cbKey,
                          const VkDependencyInfo& depInfo,
                          const std::vector<uint64_t>& handleKeys);

  // ---- Render pass final-layout tracking (record time) -------------------
  //
  // vkCmdBeginRenderPass records which images are bound as attachments so that
  // vkCmdEndRenderPass can buffer the implicit finalLayout transitions.
  //
  // renderPassKey:              key of the VkRenderPass (carries AttachmentFinalLayouts)
  // framebufferKey:             key of the VkFramebuffer (carries AttachmentImageViewKeys)
  // beginInfoAttachmentKeys:    image-view keys from VkRenderPassAttachmentBeginInfo
  //                             (keys[2..] of VkRenderPassBeginInfo.HandleKeys).
  //                             Used for imageless framebuffers whose AttachmentImageViewKeys
  //                             are empty because pAttachments == NULL at create time.
  void OnBeginRenderPass(uint64_t cbKey,
                         uint64_t renderPassKey,
                         uint64_t framebufferKey,
                         const std::vector<uint64_t>& beginInfoAttachmentKeys);

  // Buffer the finalLayout of every render-pass attachment into the recording
  // command buffer, fall back to the recording pool's family as EXCLUSIVE
  // owner for attachments this CB does not also touch via an explicit barrier
  // (a render pass cannot itself perform a queue-family ownership transfer),
  // and clear the per-CB render-pass tracking entry.
  void OnEndRenderPass(uint64_t cbKey);

  // Called when a command buffer is reset (vkResetCommandBuffer or
  // vkBeginCommandBuffer) so stale render-pass tracking is discarded.  The
  // buffered ImageLayoutAfterSubmit map is cleared by CommandBufferLifecycle.
  void OnResetCommandBuffer(uint64_t cbKey);

  // ---- Submit-time application -------------------------------------------

  // Walk the submit infos and apply each submitted CB's buffered layouts to
  // ImageState::CurrentLayout in submission order.  HandleKeys layout per
  // submit: [waitSemaphores...][commandBuffers...][signalSemaphores...].
  void OnQueueSubmit(const VkSubmitInfo* pSubmits,
                     uint32_t submitCount,
                     const std::vector<uint64_t>& handleKeys);
  void OnQueueSubmit2(const VkSubmitInfo2* pSubmits,
                      uint32_t submitCount,
                      const std::vector<uint64_t>& handleKeys);

  // Fold a secondary CB's buffered layouts into the primary (vkCmdExecuteCommands)
  // so they are applied when the primary is submitted.  Last write wins.
  void MergeSecondary(uint64_t primaryKey, uint64_t secondaryKey);

  // Apply one command buffer's buffered layouts to the tracked images.
  void ApplyCommandBuffer(uint64_t cbKey);

  // ---- Present-time completion --------------------------------------------

  // Called from SubcaptureLayer::Post(vkQueuePresentKHRCommand&) for each
  // image with a successful (VK_SUCCESS/VK_SUBOPTIMAL_KHR) per-image present
  // result. vkQueuePresentKHR performs the acquire half of a queue-family
  // ownership transfer implicitly on the presentation engine's behalf, so
  // there is no acquire-half command buffer for ApplyCommandBuffer to route
  // through. This resolves imageKey's pending transfer directly - a no-op
  // unless the image is EXCLUSIVE and currently ExclusiveOwnershipPending.
  void CompletePendingTransferOnPresent(uint64_t imageKey, uint32_t presentQueueFamily);

private:
  // Per image key: the finalLayout to apply when the render pass ends.
  using ImageLayoutPairs = std::vector<std::pair<uint64_t, VkImageLayout>>;

  // Buffer newLayout for imageKey into cbKey's ImageLayoutAfterSubmit map.
  void RecordImageLayout(uint64_t cbKey, uint64_t imageKey, VkImageLayout newLayout);

  void RecordExclusiveOwner(uint64_t cbKey, uint64_t imageKey, uint32_t ownerFamily);
  void RecordExclusivePending(uint64_t cbKey, uint64_t imageKey);
  // Permanently taint imageKey as having mixed/partial queue-family ownership
  // (see NoteExclusiveQueueFamilyTransfer) - independent of, and never
  // overwritten by, RecordExclusiveOwner/RecordExclusivePending's per-image
  // "last write wins" map so a later whole-image update in the same CB cannot
  // erase the taint.
  void RecordExclusiveMixed(uint64_t cbKey, uint64_t imageKey);
  void NoteExclusiveQueueFamilyUse(uint64_t cbKey, uint64_t imageKey);
  void NoteExclusiveQueueFamilyTransfer(uint64_t cbKey,
                                        uint64_t imageKey,
                                        uint32_t srcFamily,
                                        uint32_t dstFamily,
                                        const VkImageSubresourceRange& range);

  StateTrackingService& m_StateTracking;

  // cbKey -> list of (imageKey, finalLayout) pairs built at BeginRenderPass time.
  std::unordered_map<uint64_t, ImageLayoutPairs> m_ActiveRenderPasses;
};

} // namespace vulkan
} // namespace gits

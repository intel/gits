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
#include "gpuReadbackHelper.h" // for AspectMaskForFormat

namespace gits {
namespace vulkan {

namespace {
// Whether range covers every mip, layer and aspect of img, i.e. a barrier
// using it transfers/uses the whole image rather than a subset of it.
bool CoversWholeImage(const ImageState& img, const VkImageSubresourceRange& range) {
  const uint32_t levelCount = (range.levelCount == VK_REMAINING_MIP_LEVELS)
                                  ? img.MipLevels - range.baseMipLevel
                                  : range.levelCount;
  const uint32_t layerCount = (range.layerCount == VK_REMAINING_ARRAY_LAYERS)
                                  ? img.ArrayLayers - range.baseArrayLayer
                                  : range.layerCount;
  if (range.baseMipLevel != 0 || levelCount != img.MipLevels) {
    return false;
  }
  if (range.baseArrayLayer != 0 || layerCount != img.ArrayLayers) {
    return false;
  }
  return range.aspectMask == AspectMaskForFormat(img.Format, img.Disjoint);
}
} // namespace

ImageLayoutService::ImageLayoutService(StateTrackingService& sts) : m_StateTracking(sts) {}

void ImageLayoutService::RecordExclusiveOwner(uint64_t cbKey,
                                              uint64_t imageKey,
                                              uint32_t ownerFamily) {
  if (!imageKey || ownerFamily == UINT32_MAX) {
    return;
  }
  auto* img = m_StateTracking.GetState<ImageState>(imageKey);
  if (!img || img->SharingMode != VK_SHARING_MODE_EXCLUSIVE) {
    return;
  }
  auto* cb = m_StateTracking.GetState<CommandBufferState>(cbKey);
  if (!cb) {
    return;
  }
  cb->ExclusiveOwnerAfterSubmit[imageKey] = ExclusiveOwnerUpdate{ownerFamily, /*Pending=*/false};
}

// Mark imageKey as mid queue-family-ownership-transfer for this CB: no family
// may legally read it once this CB is submitted, until a later submit records
// the matching acquire (RecordExclusiveOwner) for the same image.
void ImageLayoutService::RecordExclusivePending(uint64_t cbKey, uint64_t imageKey) {
  if (!imageKey) {
    return;
  }
  auto* img = m_StateTracking.GetState<ImageState>(imageKey);
  if (!img || img->SharingMode != VK_SHARING_MODE_EXCLUSIVE) {
    return;
  }
  auto* cb = m_StateTracking.GetState<CommandBufferState>(cbKey);
  if (!cb) {
    return;
  }
  cb->ExclusiveOwnerAfterSubmit[imageKey] = ExclusiveOwnerUpdate{UINT32_MAX, /*Pending=*/true};
}

void ImageLayoutService::RecordExclusiveMixed(uint64_t cbKey, uint64_t imageKey) {
  if (!imageKey) {
    return;
  }
  auto* img = m_StateTracking.GetState<ImageState>(imageKey);
  if (!img || img->SharingMode != VK_SHARING_MODE_EXCLUSIVE) {
    return;
  }
  auto* cb = m_StateTracking.GetState<CommandBufferState>(cbKey);
  if (!cb) {
    return;
  }
  cb->ExclusiveOwnerMixedAfterSubmit.insert(imageKey);
}

void ImageLayoutService::NoteExclusiveQueueFamilyUse(uint64_t cbKey, uint64_t imageKey) {
  auto* cb = m_StateTracking.GetState<CommandBufferState>(cbKey);
  if (!cb) {
    return;
  }
  auto* pool = m_StateTracking.GetState<CommandPoolState>(cb->PoolKey);
  if (!pool || pool->QueueFamilyIndex == UINT32_MAX) {
    return;
  }
  RecordExclusiveOwner(cbKey, imageKey, pool->QueueFamilyIndex);
}

// A queue-family ownership transfer is split across two barriers submitted on
// two different queues: a release (recorded on the source family's CB) and a
// matching acquire (recorded on the destination family's CB) - see the Vulkan
// spec's "Queue Family Ownership Transfer" section. This CB's recording pool
// tells us which side we are looking at:
//  - pool family == dstFamily: this is the acquire, so the image becomes
//    legally owned by dstFamily once this CB is submitted.
//  - otherwise (pool family == srcFamily, the release, or an indeterminate
//    pool): ownership is relinquished but not yet acquired, so no family may
//    read the image until the matching acquire is submitted - mark it
//    pending instead of guessing an owner.
//
// Ownership transfers apply per subresourceRange, so a valid stream may move
// only some mips/layers/aspects while the rest stay on the other family.
// Without per-subresource tracking we cannot represent that split, so a
// range that is not the whole image permanently taints it as mixed-ownership
// instead of guessing a single owner - see RecordExclusiveMixed.
void ImageLayoutService::NoteExclusiveQueueFamilyTransfer(uint64_t cbKey,
                                                          uint64_t imageKey,
                                                          uint32_t srcFamily,
                                                          uint32_t dstFamily,
                                                          const VkImageSubresourceRange& range) {
  if (srcFamily == VK_QUEUE_FAMILY_IGNORED || dstFamily == VK_QUEUE_FAMILY_IGNORED ||
      srcFamily == dstFamily) {
    return;
  }
  auto* img = m_StateTracking.GetState<ImageState>(imageKey);
  if (!img || img->SharingMode != VK_SHARING_MODE_EXCLUSIVE) {
    return;
  }
  if (!CoversWholeImage(*img, range)) {
    RecordExclusiveMixed(cbKey, imageKey);
    return;
  }
  auto* cb = m_StateTracking.GetState<CommandBufferState>(cbKey);
  if (!cb) {
    return;
  }
  auto* pool = m_StateTracking.GetState<CommandPoolState>(cb->PoolKey);
  if (pool && pool->QueueFamilyIndex == dstFamily) {
    RecordExclusiveOwner(cbKey, imageKey, dstFamily);
  } else {
    RecordExclusivePending(cbKey, imageKey);
  }
}

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
      // Exactly one of these applies: a real ownership transfer decides
      // pending-vs-acquired itself; anything else (no transfer) falls back to
      // this CB's own family.  Calling both would let the fallback overwrite
      // a release's pending state with this CB's (source) family, which is
      // exactly the illegal "still readable on the source" bug being fixed.
      if (b.srcQueueFamilyIndex != VK_QUEUE_FAMILY_IGNORED &&
          b.dstQueueFamilyIndex != VK_QUEUE_FAMILY_IGNORED &&
          b.srcQueueFamilyIndex != b.dstQueueFamilyIndex) {
        NoteExclusiveQueueFamilyTransfer(cbKey, imageKey, b.srcQueueFamilyIndex,
                                         b.dstQueueFamilyIndex, b.subresourceRange);
      } else {
        NoteExclusiveQueueFamilyUse(cbKey, imageKey);
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
    const uint64_t imageKey = handleKeys[keyIdx];
    if (b.newLayout != VK_IMAGE_LAYOUT_UNDEFINED) {
      RecordImageLayout(cbKey, imageKey, b.newLayout);
    }
    // See OnPipelineBarrier: exactly one of these applies per barrier.
    if (b.srcQueueFamilyIndex != VK_QUEUE_FAMILY_IGNORED &&
        b.dstQueueFamilyIndex != VK_QUEUE_FAMILY_IGNORED &&
        b.srcQueueFamilyIndex != b.dstQueueFamilyIndex) {
      NoteExclusiveQueueFamilyTransfer(cbKey, imageKey, b.srcQueueFamilyIndex,
                                       b.dstQueueFamilyIndex, b.subresourceRange);
    } else {
      NoteExclusiveQueueFamilyUse(cbKey, imageKey);
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
    // are committed to CurrentLayout when the CB is submitted.  A render pass
    // cannot itself perform a queue-family ownership transfer (the spec
    // requires srcQueueFamilyIndex == dstQueueFamilyIndex for barriers
    // recorded inside one), so also fall back to this CB's own family as
    // EXCLUSIVE owner - the same fallback OnPipelineBarrier applies for
    // non-transfer barriers.  Without this, an attachment whose only touch in
    // the CB is the render pass (no explicit barrier) would keep a stale or
    // unknown owner.
    for (const auto& [imageKey, finalLayout] : it->second) {
      RecordImageLayout(cbKey, imageKey, finalLayout);
      NoteExclusiveQueueFamilyUse(cbKey, imageKey);
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
  for (const auto& [imageKey, update] : cb->ExclusiveOwnerAfterSubmit) {
    auto* imgState = m_StateTracking.GetState<ImageState>(imageKey);
    if (imgState) {
      imgState->ExclusiveOwnerFamily = update.Family;
      imgState->ExclusiveOwnershipPending = update.Pending;
    }
  }
  // Sticky: once observed, never cleared - see RecordExclusiveMixed.
  for (uint64_t imageKey : cb->ExclusiveOwnerMixedAfterSubmit) {
    auto* imgState = m_StateTracking.GetState<ImageState>(imageKey);
    if (imgState) {
      imgState->ExclusiveOwnershipMixed = true;
    }
  }
}

// ---------------------------------------------------------------------------
// Present-time completion
// ---------------------------------------------------------------------------

// Mirrors exactly what ApplyCommandBuffer() does for an ExclusiveOwnerAfterSubmit
// entry a normal acquire barrier would have produced, applied directly since
// there is no command buffer to buffer it through - see the declaration
// comment in imageLayoutService.h.
void ImageLayoutService::CompletePendingTransferOnPresent(uint64_t imageKey,
                                                          uint32_t presentQueueFamily) {
  if (!imageKey || presentQueueFamily == UINT32_MAX) {
    return;
  }
  auto* img = m_StateTracking.GetState<ImageState>(imageKey);
  if (!img || img->SharingMode != VK_SHARING_MODE_EXCLUSIVE || !img->ExclusiveOwnershipPending) {
    return;
  }
  img->ExclusiveOwnerFamily = presentQueueFamily;
  img->ExclusiveOwnershipPending = false;
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
  for (const auto& [imageKey, update] : sec->ExclusiveOwnerAfterSubmit) {
    prim->ExclusiveOwnerAfterSubmit[imageKey] = update;
  }
  prim->ExclusiveOwnerMixedAfterSubmit.insert(sec->ExclusiveOwnerMixedAfterSubmit.begin(),
                                              sec->ExclusiveOwnerMixedAfterSubmit.end());
}

} // namespace vulkan
} // namespace gits

// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandsCustom.h"
#include "dispatchTableAuto.h"

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

namespace gits {
namespace vulkan {

class PlayerManager;

// ---------------------------------------------------------------------------
// RestoreContentService (player side)
//
// Owns the transient staging resources used to upload state-restore buffer and
// image contents during replay.  The recorder emits a per-device manifest
// (RestoreContentManifest) followed by exactly one RestoreContentData token
// per manifest entry; this service turns that stream into GPU work:
//
//   * On the manifest it queries live free device memory (VK_EXT_memory_budget,
//     falling back to the total host-visible heap), sizes a single reusable
//     staging buffer (bounded by an 80 MB cap, a fraction of the available
//     memory, and the total footprint of the resources), and packs the
//     resources into batches that each fit inside one staging buffer.  The
//     pipeline depth is reduced when a slot is a large fraction of available
//     memory so the combined staging footprint stays within budget.
//   * As Data tokens arrive it copies the bytes into the current batch's
//     staging slot; when a batch fills it records copy commands and submits
//     them, cycling through a small ring of staging slots so GPU upload of one
//     batch overlaps the host fill of the next.
//   * Once it has consumed one data token per manifest entry it flushes the
//     final batch, waits for all in-flight work, and destroys every staging
//     object.  No separate end token is needed.
//
// No staging object ever appears in the stream, so the subcapture stays small
// and the batching adapts to whatever platform replays it.
// ---------------------------------------------------------------------------
class RestoreContentService {
public:
  explicit RestoreContentService(PlayerManager& player) : m_Player(player) {}
  ~RestoreContentService();

  RestoreContentService(const RestoreContentService&) = delete;
  RestoreContentService& operator=(const RestoreContentService&) = delete;

  void Manifest(const RestoreContentManifestCommand& command);
  void OnData(const RestoreContentDataCommand& command);

private:
  struct ResourceDesc {
    bool IsImage{false};
    uint64_t DstKey{0};
    VkDeviceSize Size{0};
    VkFormat Format{VK_FORMAT_UNDEFINED};
    VkImageLayout FinalLayout{VK_IMAGE_LAYOUT_UNDEFINED};
    VkImageAspectFlags AspectMask{0};
    std::vector<VkBufferImageCopy> Regions;
    size_t BatchIdx{0};
    VkDeviceSize BaseOffset{0};
    bool Received{false};
  };

  // One reusable staging slot in the pipeline ring.
  struct Slot {
    VkBuffer Buffer{VK_NULL_HANDLE};
    VkDeviceMemory Memory{VK_NULL_HANDLE};
    void* Mapped{nullptr};
    VkFence Fence{VK_NULL_HANDLE};
    VkCommandBuffer Cb{VK_NULL_HANDLE};
    bool InFlight{false};
  };

  struct Session {
    uint64_t DeviceKey{0};
    VkDevice Device{VK_NULL_HANDLE};
    VkPhysicalDevice PhysDevice{VK_NULL_HANDLE};
    VkQueue Queue{VK_NULL_HANDLE};
    VkCommandPool Pool{VK_NULL_HANDLE};
    VkDeviceLevelDispatchTable* Dt{nullptr};
    std::vector<ResourceDesc> Resources;
    VkDeviceSize StagingSize{0};
    size_t NumBatches{0};
    std::vector<Slot> Slots;
    size_t CurrentBatch{SIZE_MAX};
    // Number of data tokens (== resources) consumed so far.  When it reaches
    // Resources.size() the final batch is flushed and the staging torn down.
    size_t Processed{0};
    bool Valid{false};
  };

  // Wait for (and recycle) the slot that will hold batchIdx, so its previous
  // in-flight submission has finished before we overwrite its staging bytes.
  void BeginBatch(Session& session, size_t batchIdx);
  // Record + submit the copies for every received resource of batchIdx.
  void FlushBatch(Session& session, size_t batchIdx);
  void DestroySession(Session& session);

  // Allocate `ring` staging slots of session.StagingSize bytes.  On any failure
  // it tears down whatever it created and returns false so the caller can retry
  // with a smaller ring.
  bool BuildSlots(Session& session,
                  size_t ring,
                  uint32_t memType,
                  const VkBufferCreateInfo& probeCi);

  Slot& SlotFor(Session& session, size_t batchIdx) {
    return session.Slots[batchIdx % session.Slots.size()];
  }

  PlayerManager& m_Player;
  std::unordered_map<uint64_t, std::unique_ptr<Session>> m_Sessions;
};

} // namespace vulkan
} // namespace gits

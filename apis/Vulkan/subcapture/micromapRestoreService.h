// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "objectState.h"

#include <cstddef>
#include <cstdint>
#include <unordered_set>
#include <vector>

namespace gits {
namespace vulkan {

class StateTrackingService;

// Restore-time opacity micromap content: replays the analyzer's reduced micromap chain, giving
// every micromap it needs a live handle again and destroying the resurrected ones afterwards.
// A friend of StateTrackingService because it re-enters its restore recursion (RestoreOne) and
// both reads and writes pass state. The record-time counterpart is MicromapStateService - the
// two communicate only through StateTrackingService.
class MicromapRestoreService {
public:
  explicit MicromapRestoreService(StateTrackingService& sts);

  // Claim every live micromap's storage buffer, then replay the reduced micromap chain. Must run
  // before StateTrackingService::RestoreAccelerationStructureContents, since a micromap has to be
  // built before a build that names it (VUID-vkCmdBuildAccelerationStructuresKHR-micromap-11632).
  void RestoreContents();

  // Destroy the micromaps RestoreContents resurrected and release the addresses their relocated
  // storage buffers reserved. Must run after
  // StateTrackingService::RestoreAccelerationStructureContents, because any retained build in it
  // may name one.
  void DestroyResurrected();

  // Flag a micromap's storage buffer as content-restored so RestoreBufferContents leaves it
  // alone. Micromap storage is opaque, so a raw byte copy over it is never a valid fallback and
  // no restore path may forget the claim.
  void MarkBackingContentRestored(uint64_t micromapKey);

private:
  // Replay the reduced micromap chain in order: each build as a build-from-inputs, each copy
  // verbatim. Private because RestoreContents' null check on the analysis results guards it.
  void RestoreChain();

  // Give a micromap a live handle again for the rest of the restore, re-creating it (and a
  // relocated storage buffer, if the application freed that too) when needed. Enrolls whatever
  // the application destroyed for teardown. False when no handle could be produced.
  bool Resurrect(uint64_t micromapKey, uint64_t deviceKey, uint64_t physDevKey);

  // Re-create a Destroyed micromap whose storage buffer the application also destroyed, on a
  // dedicated buffer at a freshly reserved address under the caller-supplied synthetic keys.
  // Relocating is safe because no micromap address is pinned. False if no address could be
  // reserved (nothing emitted).
  bool EmitRelocatedCreate(uint64_t deviceKey,
                           uint64_t physDevKey,
                           const MicromapState& state,
                           uint64_t bufKey,
                           uint64_t memKey);

  // The micromap counterpart of StateTrackingService::EmitAccelerationStructureRebuildBytes - no
  // update mode, no ppBuildRangeInfos, and only data and triangleArray to relocate.
  void EmitRebuildBytes(uint64_t deviceKey,
                        uint64_t physDevKey,
                        uint64_t queueKey,
                        uint64_t poolKey,
                        const std::vector<char>& commandBytes,
                        const std::vector<CapturedBuildInputBuffer>& capturedInputs,
                        uint64_t logMicromapKey,
                        const std::unordered_set<uint64_t>& keepDstMicromapKeys);

  // Emit a stored vkCmdCopyMicromapEXT in a one-shot command buffer, patching its CB key.
  void EmitCopyReplay(uint64_t deviceKey,
                      uint64_t queueKey,
                      uint64_t poolKey,
                      const std::vector<char>& commandBytes);

  // Micromaps RestoreContents brought back, destroyed again once the builds naming them have
  // replayed. BufferKey/MemoryKey are set only when a relocated storage buffer was needed too.
  struct ResurrectedMicromap {
    uint64_t MicromapKey{};
    uint64_t DeviceKey{};
    uint64_t BufferKey{};
    uint64_t MemoryKey{};
  };
  std::vector<ResurrectedMicromap> m_ResurrectedMicromaps;

  // Address reservations held by EmitRelocatedCreate, released in DestroyResurrected. SIZE_MAX
  // when none was taken, which must stay distinct from 0 - that would release everything.
  size_t m_ReservationMark{SIZE_MAX};

  StateTrackingService& m_Sts;
};

} // namespace vulkan
} // namespace gits

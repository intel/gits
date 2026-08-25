// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "vulkanHeader2.h"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace gits {
namespace vulkan {

// Tracks, during replay, which events currently carry a pending signal.  An
// event becomes pending once a host vkSetEvent, or a vkCmdSetEvent (2/2KHR)
// recorded into a command buffer that has since been submitted, has been
// replayed; it stays pending until it is reset (or destroyed).  This is the
// player-side analog of the legacy SD()._eventstates[event]->eventUsed flag
// (vulkanStateTracking.h:2448-2454; applied post-submit from
// CommandBufferState::eventStatesAfterSubmit, vulkanPlayerRunWrap.h:3120-3230)
// used to guard the event-status catch-up loop so the player never spins
// forever waiting on an event that will not be (re)signalled in this replay
// range (e.g. one whose SET state was only established by state restore, with
// no in-range vkSetEvent/vkCmdSetEvent to reproduce it). Keyed by the live
// VkEvent/VkCommandBuffer/VkCommandPool handles. Thread-safe.
//
// Most titles never touch VkEvent at all, so ClearCommandBuffer/
// ApplyCommandBuffer/MergeCommandBuffer - called unconditionally from the very
// hot vkBeginCommandBuffer/vkQueueSubmit*/vkCmdExecuteCommands paths - take a
// relaxed atomic fast path out before touching the mutex or any map as long as
// no event command has ever been recorded (RecordCmdEvent/MarkPending flip it
// on permanently). This keeps those hot paths free of locking/hashing
// overhead for the common case where nothing here is ever actually used.
class EventPendingSignalService {
public:
  void MarkPending(VkEvent event);
  void ClearPending(VkEvent event);
  bool IsPending(VkEvent event);

  // vkCmdSetEvent/vkCmdResetEvent (2/2KHR) recorded into a command buffer only
  // take effect once that command buffer is submitted, so buffer the net
  // per-event effect here (last write wins) instead of touching the pending
  // set directly. Mirrors legacy CommandBufferState::eventStatesAfterSubmit.
  void RecordCmdEvent(VkCommandBuffer commandBuffer, VkEvent event, bool signaled);

  // Folds a secondary command buffer's buffered net effects into the primary
  // that executed it via vkCmdExecuteCommands (the secondary's commands only
  // take effect through the primary once the primary is submitted). Mirrors
  // legacy vkCmdExecuteCommands_SD folding eventStatesAfterSubmit into the
  // primary (vulkanStateTracking.h:4244-4247).
  void MergeCommandBuffer(VkCommandBuffer primary, VkCommandBuffer secondary);

  // Discards any buffered vkCmdSetEvent/vkCmdResetEvent effects for a command
  // buffer that is about to be (re-)recorded. Keeps pool membership, since the
  // buffer itself is not being freed. Mirrors legacy vkResetCommandBuffer_SD
  // clearing eventStatesAfterSubmit (vulkanStateTracking.h:2572-2588).
  void ClearCommandBuffer(VkCommandBuffer commandBuffer);

  // Applies a submitted command buffer's buffered vkCmdSetEvent/vkCmdResetEvent
  // net effects to the pending set. A no-op if nothing was recorded for it.
  void ApplyCommandBuffer(VkCommandBuffer commandBuffer);

  // Registers which pool a command buffer was allocated from, so a whole-pool
  // reset/destroy can find every command buffer it owns. Mirrors legacy
  // CCommandPoolState::commandBufferStateStoreList.
  void TrackCommandBuffer(VkCommandBuffer commandBuffer, VkCommandPool commandPool);

  // Fully forgets a command buffer (buffered effects + pool membership) when
  // it is individually freed. Mirrors legacy vkFreeCommandBuffers_SD.
  void UntrackCommandBuffer(VkCommandBuffer commandBuffer);

  // Clears buffered effects for every command buffer currently allocated from
  // the pool, keeping their pool membership (the buffers themselves are not
  // freed by a pool reset). Mirrors legacy vkResetCommandPool_SD.
  void ResetCommandPool(VkCommandPool commandPool);

  // Fully forgets every command buffer allocated from the pool (buffered
  // effects + pool membership) and drops the pool itself, since destroying a
  // pool implicitly frees all command buffers allocated from it. Mirrors
  // legacy vkDestroyCommandPool_SD.
  void UntrackCommandPool(VkCommandPool commandPool);

private:
  std::mutex m_Mutex;
  std::atomic<bool> m_HasEventActivity{false};
  std::unordered_set<uint64_t> m_PendingEvents;
  std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>> m_CommandBufferEventStates;
  std::unordered_map<uint64_t, uint64_t> m_CommandBufferPool;
  std::unordered_map<uint64_t, std::unordered_set<uint64_t>> m_CommandPoolBuffers;
};

} // namespace vulkan
} // namespace gits

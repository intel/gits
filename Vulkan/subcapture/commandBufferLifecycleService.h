// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "objectState.h"
#include "vulkanHeader2.h"

#include <cstdint>
#include <vector>

namespace gits {
namespace vulkan {

class StateTrackingService;

// Forward declarations for command types used in the interface.
// Must match the 'class' keyword used in commandsAuto.h.
class vkAllocateCommandBuffersCommand;
class vkBeginCommandBufferCommand;
class vkEndCommandBufferCommand;

// Tracks the lifecycle of VkCommandBuffer objects: allocation, begin/end/reset,
// pool reset, and handle-dependency accumulation for pre-range commands.
//
// Extracted from SubcaptureLayer to consolidate all CB state mutations in one place
// and eliminate the duplicated "clear CB fields" logic across reset paths.
class CommandBufferLifecycleService {
public:
  explicit CommandBufferLifecycleService(StateTrackingService& sts);

  // Create one CommandBufferState per CB, each backed by a synthetic single-CB
  // vkAllocateCommandBuffers command (avoids N allocation re-emission at restore).
  void OnAllocate(vkAllocateCommandBuffersCommand& command);

  // Remove state entries for freed command buffers.
  void OnFree(const std::vector<uint64_t>& cbKeys);

  // Transition CB to recording state; encodes the begin command for restore.
  void OnBegin(vkBeginCommandBufferCommand& command);

  // Transition CB to executable state; encodes the end command for restore.
  void OnEnd(vkEndCommandBufferCommand& command);

  // Reset a single CB to initial state.
  void OnReset(uint64_t cbKey);

  // Reset all CBs belonging to the given command pool.
  void OnResetPool(uint64_t poolKey);
  void OnDestroyPool(uint64_t poolKey);

  // Add a single handle dependency to a recording CB.
  // No-op if the CB is not currently in recording state or the key is zero.
  void TrackHandleDependency(uint64_t cbKey, uint64_t handleKey);

  // Add multiple handle dependencies to a recording CB, skipping zero keys.
  void TrackHandleDependencies(uint64_t cbKey, const std::vector<uint64_t>& handleKeys);

private:
  // Zero out all mutable tracking fields on a CB state.
  static void ClearState(CommandBufferState& state);

  StateTrackingService& m_StateTracking;
};

} // namespace vulkan
} // namespace gits

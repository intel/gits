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
#include <mutex>
#include <unordered_set>

namespace gits {
namespace vulkan {

// Tracks, during replay, which fences currently carry a pending signal.  A fence
// becomes pending once a submit/acquire/sparse-bind that signals it has been
// replayed and stays pending until it is reset (or destroyed).  This is the
// player-side analog of the legacy SD()._fencestates[fence]->fenceUsed flag
// (vulkanPlayerRunWrap.h:1000-1004,1422) used to guard the fence-status / wait
// catch-up so the player never blocks on a fence that will not be signalled in
// this replay range (e.g. a subcapture frame-pacing fence whose signalling
// submit lies before the cut).  Keyed by the live VkFence handle so callers can
// query it directly.  Thread-safe.
class FencePendingSignalService {
public:
  void MarkPending(VkFence fence);
  void ClearPending(VkFence fence);
  bool IsPending(VkFence fence);

private:
  std::mutex m_Mutex;
  std::unordered_set<uint64_t> m_PendingFences;
};

} // namespace vulkan
} // namespace gits

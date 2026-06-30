// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <cstdint>
#include <string>

namespace gits {
namespace vulkan {

// Tracks the current frame counter and decides when state restore should be
// triggered.  Mirrors DirectX SubcaptureRange in design: a frame string such
// as "5" or "3-6" is parsed at construction; IsRestorePoint() returns true
// exactly once, on the frame immediately before the subcapture start frame
// (i.e. after Present N-1 is observed).
//
// "Frame 1" in GITS convention is the first frame after state restore ends,
// so a range of "1" means trimming mode - state restore fires immediately
// before any present is observed (m_CurrentFrame == 1 at construction, so the
// >= check in IsRestorePoint() is satisfied on the first present).
class SubcaptureRange {
public:
  // Parses the frames string from configuration.
  // An empty or "-" string means subcapture is disabled.
  explicit SubcaptureRange(const std::string& framesStr);

  // Called after each vkQueuePresentKHR.  Advances the frame counter.
  void FrameEnd();

  // Returns true exactly once: on the Present call that precedes the first
  // frame of the requested subcapture range.  After it returns true once it
  // always returns false (state restore should only be triggered once).
  bool IsRestorePoint() const;

  // Returns true while the current frame is within [startFrame_, endFrame_].
  bool InRange() const;

  // Returns true if subcapture is enabled at all.
  bool IsEnabled() const {
    return m_Enabled;
  }

private:
  bool m_Enabled{false};
  uint32_t m_StartFrame{1};
  uint32_t m_EndFrame{1};
  uint32_t m_CurrentFrame{1}; // 1-based number of the frame currently being
                              // rendered; advanced to the next frame after each
                              // present (FrameEnd). Starts at 1 (frame 1 is in
                              // progress before the first present).
  mutable bool m_RestoreFired{false};
};

} // namespace vulkan
} // namespace gits

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

namespace gits::opengl2 {

// Parses a frame range ("N" or "N-M") and tracks the 1-based frame being replayed.
class SubcaptureRange {
public:
  explicit SubcaptureRange(const std::string& frames);

  void FrameEnd() {
    ++m_CurrentFrame;
  }

  bool InRange() const {
    return m_CurrentFrame >= m_StartFrame && m_CurrentFrame <= m_EndFrame;
  }

  uint32_t StartFrame() const {
    return m_StartFrame;
  }

  uint32_t CurrentFrame() const {
    return m_CurrentFrame;
  }

private:
  uint32_t m_StartFrame{1};
  uint32_t m_EndFrame{1};
  uint32_t m_CurrentFrame{1};
};

} // namespace gits::opengl2

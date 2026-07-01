// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "subcaptureRange.h"
#include "exception.h"
#include "log.h"

#include <string>

namespace gits {
namespace vulkan {

SubcaptureRange::SubcaptureRange(const std::string& framesStr) {
  if (framesStr.empty() || framesStr == "-") {
    return;
  }

  try {
    const auto pos = framesStr.find('-');
    if (pos != std::string::npos) {
      m_StartFrame = static_cast<uint32_t>(std::stoi(framesStr.substr(0, pos)));
      m_EndFrame = static_cast<uint32_t>(std::stoi(framesStr.substr(pos + 1)));
    } else {
      m_StartFrame = static_cast<uint32_t>(std::stoi(framesStr));
      m_EndFrame = m_StartFrame;
    }
  } catch (...) {
    throw Exception("Vulkan subcapture: invalid frame range '" + framesStr + "'");
  }

  m_Enabled = true;
  LOG_INFO << "Vulkan subcapture enabled for frames " << m_StartFrame << "-" << m_EndFrame;
}

void SubcaptureRange::FrameEnd() {
  ++m_CurrentFrame;
}

bool SubcaptureRange::IsRestorePoint() const {
  if (!m_Enabled || m_RestoreFired) {
    return false;
  }
  // m_CurrentFrame is the 1-based number of the frame currently being rendered.
  // State restore must be captured at the start of m_StartFrame, i.e. on the
  // present that completes frame (m_StartFrame - 1).
  // Using ">=" together with the m_RestoreFired latch above also covers the
  // trimming case (m_StartFrame == 1): there is no earlier frame to restore
  // from, so the condition is satisfied on the very first present and restore
  // fires once before any frame is recorded. For m_StartFrame >= 2 the counter
  // passes through every value, so ">=" first becomes true exactly at
  // m_CurrentFrame == m_StartFrame - 1, identical to the previous "==" check.
  if (m_CurrentFrame >= m_StartFrame - 1) {
    m_RestoreFired = true;
    return true;
  }
  return false;
}

bool SubcaptureRange::InRange() const {
  if (!m_Enabled) {
    return false;
  }
  return m_CurrentFrame >= m_StartFrame && m_CurrentFrame <= m_EndFrame;
}

} // namespace vulkan
} // namespace gits

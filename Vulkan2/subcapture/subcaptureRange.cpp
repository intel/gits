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
    throw Exception("Vulkan2 subcapture: invalid frame range '" + framesStr + "'");
  }

  m_Enabled = true;
  LOG_INFO << "Vulkan2 subcapture enabled for frames " << m_StartFrame << "-" << m_EndFrame;
}

void SubcaptureRange::FrameEnd() {
  ++m_CurrentFrame;
}

bool SubcaptureRange::IsRestorePoint() const {
  if (!m_Enabled || m_RestoreFired) {
    return false;
  }
  // Trimming mode: start frame is 1, so restore before any frame is presented.
  // General case: restore after Present of frame (startFrame - 1).
  // m_CurrentFrame is incremented by FrameEnd *after* the present, so at the
  // moment of the present call m_CurrentFrame == startFrame - 1.
  if (m_CurrentFrame == m_StartFrame - 1) {
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

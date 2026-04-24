// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "subcaptureRange.h"
#include "configurator.h"
#include "exception.h"

#include <string>

namespace gits {
namespace DirectX {

SubcaptureRange::SubcaptureRange() {
  const gits::Configuration& config = Configurator::Get();

  if (!config.directx.features.subcapture.enabled ||
      config.directx.features.subcapture.executionSerialization) {
    return;
  }

  const std::string& frames = Configurator::Get().directx.features.subcapture.frames;
  size_t pos = frames.find("-");
  try {
    if (pos != std::string::npos) {
      m_StartFrame = std::stoi(frames.substr(0, pos));
      m_EndFrame = std::stoi(frames.substr(pos + 1));
    } else {
      m_StartFrame = std::stoi(frames);
      m_EndFrame = m_StartFrame;
    }
  } catch (...) {
    throw Exception("Invalid subcapture range: '" +
                    Configurator::Get().directx.features.subcapture.frames + "'");
  }

  std::string commandListExecutions = config.directx.features.subcapture.commandListExecutions;
  if (!commandListExecutions.empty()) {
    size_t pos = commandListExecutions.find("-");
    if (pos != std::string::npos) {
      m_ExecutionRangeStart = std::stoi(commandListExecutions.substr(0, pos));
      m_ExecutionRangeEnd = std::stoi(commandListExecutions.substr(pos + 1));
    } else {
      m_ExecutionRangeStart = m_ExecutionRangeEnd = std::stoi(commandListExecutions);
    }
  } else if (m_StartFrame == 1) {
    m_TrimmingMode = true;
    m_InFrameRange = true;
  }
}

void SubcaptureRange::FrameEnd(bool stateRestore) {
  m_ExecutionCount = 0;
  m_ZeroOrFirstFrame = false;

  if (!stateRestore) {
    ++m_CurrentFrame;
  }

  if (m_CurrentFrame >= m_StartFrame && m_CurrentFrame <= m_EndFrame) {
    m_InFrameRange = true;
  } else {
    m_InFrameRange = false;
  }
}

bool SubcaptureRange::IsFrameRangeStart(bool stateRestore) {
  if (stateRestore) {
    return false;
  }
  return m_CurrentFrame == m_StartFrame - 1;
}

void SubcaptureRange::ExecutionStart() {
  ++m_ExecutionCount;
  m_InsideExecution = true;
}

void SubcaptureRange::ExecutionEnd() {
  m_InsideExecution = false;
}

bool SubcaptureRange::IsExecutionRangeStart() {
  if (m_ZeroOrFirstFrame) {
    return false;
  }
  if (!m_ExecutionRangeStart || !m_InFrameRange) {
    return false;
  }
  return m_ExecutionCount == m_ExecutionRangeStart - 1;
}

bool SubcaptureRange::InRange() {
  if (m_ZeroOrFirstFrame && !m_TrimmingMode) {
    return false;
  }
  if (!m_ExecutionRangeStart || !m_InFrameRange) {
    return m_InFrameRange;
  }
  if (!m_InsideExecution && m_ExecutionCount == m_ExecutionRangeEnd) {
    return false;
  }
  return m_ExecutionCount >= m_ExecutionRangeStart && m_ExecutionCount <= m_ExecutionRangeEnd;
}

bool SubcaptureRange::CommandListSubcapture() {
  return m_ExecutionRangeStart;
}

} // namespace DirectX
} // namespace gits

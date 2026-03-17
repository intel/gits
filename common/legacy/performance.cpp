// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   performance.cpp
 *
 * @brief Definitions of function calls performance.
 *
 */
#include "performance.h"

namespace gits {

void FrameTimeSheet::AddFrameTime(uint64_t time) {
  ++m_FrameNumber;
  double timeSec = time * 0.000'000'001;
  m_FrameTimes.emplace_back(m_FrameNumber, timeSec);
}

void FrameTimeSheet::OutputTimeData(std::ostream& stream) {
  stream << "Frame#,Time[sec]\n";
  for (auto& [frameNumber, time] : m_FrameTimes) {
    stream << frameNumber << "," << time << "\n";
  }
}

} // namespace gits

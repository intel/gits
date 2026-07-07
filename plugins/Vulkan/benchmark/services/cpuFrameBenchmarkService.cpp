// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "cpuFrameBenchmarkService.h"
#include "log.h"

#include <fstream>

namespace gits {
namespace vulkan {

CpuFrameBenchmarkService::CpuFrameBenchmarkService(const BenchmarkConfig& cfg,
                                                   gits::MessageBus& msgBus)
    : m_Cfg(cfg), m_MsgBus(msgBus) {
  if (cfg.IsCapture) {
    m_SubscriptionId = m_MsgBus.subscribe({PUBLISHER_RECORDER, TOPIC_STREAM_SAVED},
                                          [this](Topic t, const MessagePtr& m) { WriteResults(); });
  } else {
    m_SubscriptionId = m_MsgBus.subscribe({PUBLISHER_PLAYER, TOPIC_PROGRAM_EXIT},
                                          [this](Topic t, const MessagePtr& m) { WriteResults(); });
  }
}

CpuFrameBenchmarkService::~CpuFrameBenchmarkService() {
  m_MsgBus.unsubscribe(m_SubscriptionId);
}

void CpuFrameBenchmarkService::OnStart() {
  if (!m_Start) {
    return;
  }
  m_Start = false;
  m_StartTime = std::chrono::steady_clock::now();
}

void CpuFrameBenchmarkService::OnPostPresent() {
  m_PresentTimes.push_back(std::chrono::steady_clock::now());
}

void CpuFrameBenchmarkService::WriteResults() {
  const auto toSeconds = [](std::chrono::steady_clock::duration d) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(d).count() * 0.000'000'001;
  };

  // CSV: one row per counted present. Frame 1's time is the instance-creation ->
  // first-present interval (0 if no start baseline was captured); every later
  // frame's time is the present-to-present delta.
  std::ofstream stream(m_Cfg.Output);
  GITS_ASSERT(stream.good(), "CpuFrameBenchmarkService - failed to create file: " + m_Cfg.Output);
  stream << "Frame#,Time[sec]\n";
  for (size_t i = 0; i < m_PresentTimes.size(); ++i) {
    double durationSec;
    if (i == 0) {
      durationSec = m_Start ? 0.0 : toSeconds(m_PresentTimes[0] - m_StartTime);
    } else {
      durationSec = toSeconds(m_PresentTimes[i] - m_PresentTimes[i - 1]);
    }
    stream << (i + 1) << "," << durationSec << "\n";
  }

  // Average FPS over the steady-state window, excluding the first WarmupFrames
  // presents. Anchor at the WarmupFrames-th present so the measured window is
  // t_N - t_anchor, spanning exactly measuredFrames present-to-present intervals.
  const size_t totalFrames = m_PresentTimes.size();
  const size_t warmup = m_Cfg.WarmupFrames;
  if (totalFrames > warmup) {
    // WarmupFrames-th present (1-based); with no warmup, anchor at the first present.
    const size_t anchorIdx = (warmup == 0) ? 0 : warmup - 1;
    const size_t measuredFrames = totalFrames - warmup;
    const double windowSec = toSeconds(m_PresentTimes.back() - m_PresentTimes[anchorIdx]);
    if (windowSec > 0.0) {
      const double fps = static_cast<double>(measuredFrames) / windowSec;
      LOG_INFO << "Benchmark: FPS: " << fps << " (" << measuredFrames << " frames in "
               << windowSec * 1e3 << "ms, excluding " << warmup
               << " warmup frames). Frame times written to " << m_Cfg.Output;
      return;
    }
  }

  LOG_INFO << "Benchmark: FPS: N/A (stream too short: " << totalFrames << " frames, warmup "
           << warmup << "). Frame times written to " << m_Cfg.Output;
}

} // namespace vulkan
} // namespace gits

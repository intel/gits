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

namespace gits::opengl {

CpuFrameBenchmarkService::CpuFrameBenchmarkService(const BenchmarkConfig& cfg,
                                                   gits::MessageBus& msgBus)
    : m_Cfg(cfg), m_MsgBus(msgBus) {
  if (cfg.IsCapture) {
    // The recorder drops the frames presented while it closes, before it saves the stream. It
    // closes when a plugin asks it to and at program exit.
    const auto stop = [this](Topic, const MessagePtr&) {
      const std::lock_guard<std::mutex> lock(m_Mutex);
      m_Stopped = true;
    };
    m_SubscriptionIds.push_back(m_MsgBus.subscribe({PUBLISHER_PLUGIN, TOPIC_CLOSE_RECORDER}, stop));
    m_SubscriptionIds.push_back(m_MsgBus.subscribe({PUBLISHER_RECORDER, TOPIC_PROGRAM_EXIT}, stop));
    m_SubscriptionIds.push_back(
        m_MsgBus.subscribe({PUBLISHER_RECORDER, TOPIC_STREAM_SAVED},
                           [this](Topic, const MessagePtr&) { WriteResults(); }));
  } else {
    m_SubscriptionIds.push_back(
        m_MsgBus.subscribe({PUBLISHER_PLAYER, TOPIC_PROGRAM_EXIT},
                           [this](Topic, const MessagePtr&) { WriteResults(); }));
  }
}

CpuFrameBenchmarkService::~CpuFrameBenchmarkService() {
  for (const unsigned id : m_SubscriptionIds) {
    m_MsgBus.unsubscribe(id);
  }
}

void CpuFrameBenchmarkService::OnStart() {
  const std::lock_guard<std::mutex> lock(m_Mutex);
  if (!m_Start) {
    return;
  }
  m_Start = false;
  m_StartTime = std::chrono::steady_clock::now();
}

void CpuFrameBenchmarkService::OnFrameEnd() {
  const std::lock_guard<std::mutex> lock(m_Mutex);
  if (!m_Stopped) {
    m_FrameEndTimes.push_back(std::chrono::steady_clock::now());
  }
}

void CpuFrameBenchmarkService::WriteResults() {
  const std::lock_guard<std::mutex> lock(m_Mutex);
  m_Stopped = true;
  const auto toSeconds = [](std::chrono::steady_clock::duration d) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(d).count() * 0.000'000'001;
  };

  // CSV: one row per counted swap. Frame 1's time is the context-creation-to-first-swap interval
  // (0 if no start baseline was captured); every later frame's time is the swap-to-swap delta.
  std::ofstream stream(m_Cfg.Output);
  GITS_ASSERT(stream.good(), "CpuFrameBenchmarkService - failed to create file: " + m_Cfg.Output);
  stream << "Frame#,Time[sec]\n";
  for (size_t i = 0; i < m_FrameEndTimes.size(); ++i) {
    double durationSec;
    if (i == 0) {
      durationSec = m_Start ? 0.0 : toSeconds(m_FrameEndTimes[0] - m_StartTime);
    } else {
      durationSec = toSeconds(m_FrameEndTimes[i] - m_FrameEndTimes[i - 1]);
    }
    stream << (i + 1) << "," << durationSec << "\n";
  }

  // Average FPS over the steady-state window, excluding the first WarmupFrames swaps. Anchor at
  // the WarmupFrames-th swap so the measured window spans exactly measuredFrames swap-to-swap
  // intervals.
  const size_t totalFrames = m_FrameEndTimes.size();
  const size_t warmup = m_Cfg.WarmupFrames;
  if (totalFrames > warmup) {
    // WarmupFrames-th swap (1-based); with no warmup, anchor at the first swap, which leaves the
    // first frame out of the window.
    const size_t anchorIdx = (warmup == 0) ? 0 : warmup - 1;
    const size_t measuredFrames = totalFrames - 1 - anchorIdx;
    const double windowSec = toSeconds(m_FrameEndTimes.back() - m_FrameEndTimes[anchorIdx]);
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

} // namespace gits::opengl

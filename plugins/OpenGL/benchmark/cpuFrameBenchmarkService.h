// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "config.h"
#include "messageBus.h"

#include <chrono>
#include <mutex>
#include <vector>

namespace gits::opengl {

// Measures per-frame (swap-to-swap) CPU frame times and writes them to a .csv file. The average
// FPS is computed over a steady-state window that excludes the first WarmupFrames swaps. Results
// are flushed when the stream is saved (capture) or when the player exits (playback), via the
// MessageBus; under the recorder, frames stop being counted once it is asked to close. The
// recorder calls its layers from every application thread, so all members are guarded by
// m_Mutex.
class CpuFrameBenchmarkService {
public:
  CpuFrameBenchmarkService(const BenchmarkConfig& cfg, gits::MessageBus& msgBus);
  ~CpuFrameBenchmarkService();
  CpuFrameBenchmarkService(const CpuFrameBenchmarkService&) = delete;
  CpuFrameBenchmarkService& operator=(const CpuFrameBenchmarkService&) = delete;

  void OnStart();
  void OnFrameEnd();

private:
  void WriteResults();

private:
  const BenchmarkConfig m_Cfg;
  gits::MessageBus& m_MsgBus;
  std::mutex m_Mutex;
  // Start baseline, captured once at the first context creation. Used only to give CSV frame 1
  // a context-creation-to-first-swap duration; it never enters the FPS window, which is anchored
  // at a swap.
  bool m_Start{true};
  std::chrono::steady_clock::time_point m_StartTime{};
  // One timestamp per counted swap. The frame time of swap i (i >= 1) is
  // m_FrameEndTimes[i] - m_FrameEndTimes[i - 1].
  std::vector<std::chrono::steady_clock::time_point> m_FrameEndTimes;
  // Set once the recorder stops recording frames.
  bool m_Stopped{};
  std::vector<unsigned> m_SubscriptionIds;
};

} // namespace gits::opengl

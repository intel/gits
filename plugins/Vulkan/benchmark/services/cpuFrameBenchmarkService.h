// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "../config.h"
#include "messageBus.h"

#include <chrono>
#include <vector>

namespace gits {
namespace vulkan {

// Measures per-frame (present-to-present) CPU frame times and writes them to a
// .csv file. The average FPS is computed over a steady-state window that
// excludes the first WarmupFrames presents. Results are flushed when the stream
// is saved (capture) or when the player exits (playback), via the MessageBus.
class CpuFrameBenchmarkService {
public:
  CpuFrameBenchmarkService(const BenchmarkConfig& cfg, gits::MessageBus& msgBus);
  ~CpuFrameBenchmarkService();
  CpuFrameBenchmarkService(const CpuFrameBenchmarkService&) = delete;
  CpuFrameBenchmarkService& operator=(const CpuFrameBenchmarkService&) = delete;

  void OnStart();
  void OnPostPresent();

private:
  void WriteResults();

private:
  const BenchmarkConfig m_Cfg;
  gits::MessageBus& m_MsgBus;
  // Start baseline, captured once at the first vkCreateInstance. Used ONLY to
  // give CSV frame 1 an instance-creation -> first-present duration; it never
  // enters the FPS window (which is anchored at a present).
  bool m_Start{true};
  std::chrono::steady_clock::time_point m_StartTime{};
  // One steady_clock timestamp per counted present. The per-frame time of
  // present i (i >= 2) is m_PresentTimes[i] - m_PresentTimes[i - 1].
  std::vector<std::chrono::steady_clock::time_point> m_PresentTimes;
  unsigned m_SubscriptionId{};
};

} // namespace vulkan
} // namespace gits

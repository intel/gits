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
#include "taskScheduler.h"

#include <atomic>
#include <chrono>
#include <vector>

namespace gits {
namespace DirectX {

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
  unsigned m_FrameNumber{};
  bool m_Start{true};
  std::chrono::steady_clock::time_point m_PrevTimePoint{};
  std::vector<std::pair<unsigned, double>> m_FrameTimes;
  unsigned m_SubscriptionId{};
};

} // namespace DirectX
} // namespace gits

// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "../config.h"
#include "taskScheduler.h"
#include "messageBus.h"

#include <chrono>
#include <atomic>

namespace gits {
namespace DirectX {

class CpuFrameBenchmarkService {
public:
  CpuFrameBenchmarkService(const BenchmarkConfig& cfg, gits::MessageBus& msgBus);
  ~CpuFrameBenchmarkService();

  void onStart();
  void onPostPresent();

private:
  void writeResults();

private:
  const BenchmarkConfig cfg_;
  gits::MessageBus& msgBus_;
  unsigned frameNumber_{};
  bool start_{true};
  std::chrono::steady_clock::time_point prevTimePoint_{};
  std::vector<std::pair<unsigned, double>> frameTimes_;
  unsigned subscriptionId_{};
};

} // namespace DirectX
} // namespace gits

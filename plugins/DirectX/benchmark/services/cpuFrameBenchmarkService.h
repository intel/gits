// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "../config.h"
#include "taskScheduler.h"

#include <fstream>
#include <chrono>

namespace gits {
namespace DirectX {

class CpuFrameBenchmarkService {
public:
  CpuFrameBenchmarkService(const BenchmarkConfig::CpuFrameBenchmarkConfig& cfg);
  void onPreExecute();
  void onPostPresent();

private:
  void writeResult(size_t frameNumber, double cpuTime);

private:
  const BenchmarkConfig::CpuFrameBenchmarkConfig cfg_;
  size_t frameNumber_{};
  std::chrono::steady_clock::time_point prevTimePoint{};
  bool firstExecute_ = true;
  std::ofstream fileStream_;
  TaskScheduler scheduler_;
};

} // namespace DirectX
} // namespace gits

// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "cpuFrameBenchmarkService.h"
#include "benchmarkUtils.h"

namespace gits {
namespace DirectX {

gits::DirectX::CpuFrameBenchmarkService::CpuFrameBenchmarkService(
    const BenchmarkConfig::CpuFrameBenchmarkConfig& cfg)
    : cfg_(cfg), scheduler_("cpu-frame-benchmark") {}

void CpuFrameBenchmarkService::onPreExecute() {
  if (!firstExecute_) {
    return;
  }

  firstExecute_ = false;
  prevTimePoint = std::chrono::steady_clock::now();
}

void CpuFrameBenchmarkService::onPostPresent() {
  if (!cfg_.enabled) {
    return;
  }

  ++frameNumber_;
  const double durationSec = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                 std::chrono::steady_clock::now() - prevTimePoint)
                                 .count() *
                             0.000'000'001;
  prevTimePoint = std::chrono::steady_clock::now();
  const auto frameNumber = frameNumber_;
  scheduler_.schedule(
      [this, frameNumber, durationSec]() { writeResult(frameNumber, durationSec); });
}

void CpuFrameBenchmarkService::writeResult(size_t frameNumber, double cpuTime) {
  if (!fileStream_.is_open()) {
    fileStream_.open(cfg_.output);
    if (!fileStream_.good()) {
      logAndThrow("Failed to create file: " + cfg_.output);
    }
    fileStream_ << "Frame #,Time [sec]\n";
  }

  fileStream_ << frameNumber << "," << cpuTime << "\n";
  fileStream_.flush();
}

} // namespace DirectX
} // namespace gits

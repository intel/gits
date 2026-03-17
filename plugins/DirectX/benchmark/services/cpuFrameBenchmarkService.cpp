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
namespace DirectX {

CpuFrameBenchmarkService::CpuFrameBenchmarkService(const BenchmarkConfig& cfg,
                                                   gits::MessageBus& msgBus)
    : cfg_(cfg), msgBus_(msgBus) {
  if (cfg.isCapture) {
    subscriptionId_ = msgBus_.subscribe({PUBLISHER_RECORDER, TOPIC_STREAM_SAVED},
                                        [this](Topic t, const MessagePtr& m) { writeResults(); });
  } else {
    subscriptionId_ = msgBus_.subscribe({PUBLISHER_PLAYER, TOPIC_PROGRAM_EXIT},
                                        [this](Topic t, const MessagePtr& m) { writeResults(); });
  }
}

void CpuFrameBenchmarkService::onStart() {
  if (!start_) {
    return;
  }
  start_ = false;
  prevTimePoint_ = std::chrono::steady_clock::now();
}

CpuFrameBenchmarkService::~CpuFrameBenchmarkService() {
  msgBus_.unsubscribe(subscriptionId_);
}

void CpuFrameBenchmarkService::onPostPresent() {
  ++frameNumber_;
  double durationSec = std::chrono::duration_cast<std::chrono::nanoseconds>(
                           std::chrono::steady_clock::now() - prevTimePoint_)
                           .count() *
                       0.000'000'001;
  prevTimePoint_ = std::chrono::steady_clock::now();
  frameTimes_.emplace_back(frameNumber_, durationSec);
}

void CpuFrameBenchmarkService::writeResults() {

  std::ofstream stream(cfg_.output);
  GITS_ASSERT(stream.good(), "CpuFrameBenchmarkService - failed to create file: " + cfg_.output);
  stream << "Frame#,Time[sec]\n";
  for (auto& [frameNumber, duration] : frameTimes_) {
    stream << frameNumber << "," << duration << "\n";
  }
}

} // namespace DirectX
} // namespace gits

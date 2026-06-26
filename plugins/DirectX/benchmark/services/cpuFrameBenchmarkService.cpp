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
    : m_Cfg(cfg), m_MsgBus(msgBus) {
  if (cfg.IsCapture) {
    m_SubscriptionId = m_MsgBus.subscribe({PUBLISHER_RECORDER, TOPIC_STREAM_SAVED},
                                          [this](Topic t, const MessagePtr& m) { WriteResults(); });
  } else {
    m_SubscriptionId = m_MsgBus.subscribe({PUBLISHER_PLAYER, TOPIC_PROGRAM_EXIT},
                                          [this](Topic t, const MessagePtr& m) { WriteResults(); });
  }
}

void CpuFrameBenchmarkService::OnStart() {
  if (!m_Start) {
    return;
  }
  m_Start = false;
  m_PrevTimePoint = std::chrono::steady_clock::now();
}

CpuFrameBenchmarkService::~CpuFrameBenchmarkService() {
  m_MsgBus.unsubscribe(m_SubscriptionId);
}

void CpuFrameBenchmarkService::OnPostPresent() {
  ++m_FrameNumber;
  double durationSec = std::chrono::duration_cast<std::chrono::nanoseconds>(
                           std::chrono::steady_clock::now() - m_PrevTimePoint)
                           .count() *
                       0.000'000'001;
  m_PrevTimePoint = std::chrono::steady_clock::now();
  m_FrameTimes.emplace_back(m_FrameNumber, durationSec);
}

void CpuFrameBenchmarkService::WriteResults() {
  std::ofstream stream(m_Cfg.Output);
  GITS_ASSERT(stream.good(), "CpuFrameBenchmarkService - failed to create file: " + m_Cfg.Output);
  stream << "Frame#,Time[sec]\n";
  for (auto& [frameNumber, duration] : m_FrameTimes) {
    stream << frameNumber << "," << duration << "\n";
  }
}

} // namespace DirectX
} // namespace gits

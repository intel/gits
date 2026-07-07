// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "layer.h"

namespace gits {
namespace vulkan {

BenchmarkLayer::BenchmarkLayer(const BenchmarkConfig& cfg, gits::MessageBus& msgBus)
    : Layer("Benchmark"), m_Cfg(cfg), m_CpuFrameBenchmarkService(cfg, msgBus) {}

void BenchmarkLayer::Pre(vkCreateInstanceCommand& command) {
  (void)command;
  // Capture the start baseline for CSV frame 1 only (does not affect FPS).
  m_CpuFrameBenchmarkService.OnStart();
}

void BenchmarkLayer::Post(vkQueuePresentKHRCommand& command) {
  // Count only frames that actually presented: skip commands the player elided
  // (e.g. renderOffscreen) and unsuccessful presents.  VK_SUBOPTIMAL_KHR still
  // represents a completed present, so it is counted.
  if (command.m_Skip ||
      (command.m_Return.Value != VK_SUCCESS && command.m_Return.Value != VK_SUBOPTIMAL_KHR)) {
    return;
  }
  m_CpuFrameBenchmarkService.OnPostPresent();
}

} // namespace vulkan
} // namespace gits

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
  // A present elided by offscreen replay still ends a frame, so m_Skip is not checked.
  // VK_SUBOPTIMAL_KHR still presented a frame. The present recorded after state restore is not an
  // application frame.
  if (IsStateRestoreKey(command.m_Key) ||
      (command.m_Return.Value != VK_SUCCESS && command.m_Return.Value != VK_SUBOPTIMAL_KHR)) {
    return;
  }
  m_CpuFrameBenchmarkService.OnPostPresent();
}

} // namespace vulkan
} // namespace gits

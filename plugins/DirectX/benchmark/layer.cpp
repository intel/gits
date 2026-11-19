// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "layer.h"
#include "pluginUtils.h"
#include "keyUtils.h"

namespace gits {
namespace DirectX {

BenchmarkLayer::BenchmarkLayer(const BenchmarkConfig& cfg, gits::MessageBus& msgBus)
    : Layer("Benchmark"), m_Cfg(cfg), m_CpuFrameBenchmarkService(cfg, msgBus) {}

void BenchmarkLayer::Pre(CreateDXGIFactoryCommand& command) {
  m_CpuFrameBenchmarkService.OnStart();
}

void BenchmarkLayer::Pre(CreateDXGIFactory1Command& command) {
  m_CpuFrameBenchmarkService.OnStart();
}

void BenchmarkLayer::Pre(CreateDXGIFactory2Command& command) {
  m_CpuFrameBenchmarkService.OnStart();
}

void BenchmarkLayer::Post(IDXGISwapChainPresentCommand& command) {
  // Present can return a success code such as DXGI_STATUS_OCCLUDED (locked desktop). The frame is
  // complete, so count it.
  if (command.Skip || FAILED(command.m_Result.Value) || command.m_Flags.Value & DXGI_PRESENT_TEST ||
      IsStateRestoreKey(command.Key)) {
    return;
  }

  m_CpuFrameBenchmarkService.OnPostPresent();
}

void BenchmarkLayer::Post(IDXGISwapChain1Present1Command& command) {
  if (command.Skip || FAILED(command.m_Result.Value) ||
      command.m_PresentFlags.Value & DXGI_PRESENT_TEST || IsStateRestoreKey(command.Key)) {
    return;
  }

  m_CpuFrameBenchmarkService.OnPostPresent();
}

} // namespace DirectX
} // namespace gits

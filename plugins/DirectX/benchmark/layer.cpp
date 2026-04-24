// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layer.h"
#include "pluginUtils.h"
#include "keyUtils.h"

namespace gits {
namespace DirectX {

BenchmarkLayer::BenchmarkLayer(const BenchmarkConfig& cfg, gits::MessageBus& msgBus)
    : Layer("Benchmark"), cfg_(cfg), cpuFrameBenchmarkService_(cfg, msgBus) {}

void BenchmarkLayer::Pre(CreateDXGIFactoryCommand& command) {
  cpuFrameBenchmarkService_.onStart();
}

void BenchmarkLayer::Pre(CreateDXGIFactory1Command& command) {
  cpuFrameBenchmarkService_.onStart();
}

void BenchmarkLayer::Pre(CreateDXGIFactory2Command& command) {
  cpuFrameBenchmarkService_.onStart();
}

void BenchmarkLayer::Post(IDXGISwapChainPresentCommand& c) {
  if (c.Skip || c.m_Result.Value != S_OK || c.m_Flags.Value & DXGI_PRESENT_TEST ||
      IsStateRestoreKey(c.Key)) {
    return;
  }

  cpuFrameBenchmarkService_.onPostPresent();
}

void BenchmarkLayer::Post(IDXGISwapChain1Present1Command& c) {
  if (c.Skip || c.m_Result.Value != S_OK || c.m_PresentFlags.Value & DXGI_PRESENT_TEST ||
      IsStateRestoreKey(c.Key)) {
    return;
  }

  cpuFrameBenchmarkService_.onPostPresent();
}

} // namespace DirectX
} // namespace gits

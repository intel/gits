// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layer.h"
#include "pluginUtils.h"
#include "benchmarkUtils.h"

namespace gits {
namespace DirectX {

BenchmarkLayer::BenchmarkLayer(CGits& gits, const BenchmarkConfig& cfg)
    : Layer("Benchmark"),
      gits_(gits),
      cfg_(cfg),
      cpuFrameBenchmarkService_(cfg.cpuFrameBenchmarkConfig) {
  initializeLog(&gits_);
}

void BenchmarkLayer::pre(ID3D12CommandQueueExecuteCommandListsCommand& command) {
  cpuFrameBenchmarkService_.onPreExecute();
}

void BenchmarkLayer::post(IDXGISwapChainPresentCommand& c) {
  if (c.skip || c.result_.value != S_OK || c.Flags_.value & DXGI_PRESENT_TEST ||
      c.key & Command::stateRestoreKeyMask) {
    return;
  }

  cpuFrameBenchmarkService_.onPostPresent();
}

void BenchmarkLayer::post(IDXGISwapChain1Present1Command& c) {
  if (c.skip || c.result_.value != S_OK || c.PresentFlags_.value & DXGI_PRESENT_TEST ||
      c.key & Command::stateRestoreKeyMask) {
    return;
  }

  cpuFrameBenchmarkService_.onPostPresent();
}

} // namespace DirectX
} // namespace gits

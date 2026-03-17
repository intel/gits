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

void BenchmarkLayer::pre(CreateDXGIFactoryCommand& command) {
  cpuFrameBenchmarkService_.onStart();
}

void BenchmarkLayer::pre(CreateDXGIFactory1Command& command) {
  cpuFrameBenchmarkService_.onStart();
}

void BenchmarkLayer::pre(CreateDXGIFactory2Command& command) {
  cpuFrameBenchmarkService_.onStart();
}

void BenchmarkLayer::post(IDXGISwapChainPresentCommand& c) {
  if (c.skip || c.result_.value != S_OK || c.Flags_.value & DXGI_PRESENT_TEST ||
      isStateRestoreKey(c.key)) {
    return;
  }

  cpuFrameBenchmarkService_.onPostPresent();
}

void BenchmarkLayer::post(IDXGISwapChain1Present1Command& c) {
  if (c.skip || c.result_.value != S_OK || c.PresentFlags_.value & DXGI_PRESENT_TEST ||
      isStateRestoreKey(c.key)) {
    return;
  }

  cpuFrameBenchmarkService_.onPostPresent();
}

} // namespace DirectX
} // namespace gits

// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "config.h"
#include "services/cpuFrameBenchmarkService.h"

namespace gits {

class CGits;

namespace DirectX {

class BenchmarkLayer : public Layer {
public:
  BenchmarkLayer(CGits& gits, const BenchmarkConfig& cfg);
  ~BenchmarkLayer() = default;

  void pre(ID3D12CommandQueueExecuteCommandListsCommand& command) override;
  void post(IDXGISwapChainPresentCommand& command) override;
  void post(IDXGISwapChain1Present1Command& command) override;

private:
  CGits& gits_;
  BenchmarkConfig cfg_;
  CpuFrameBenchmarkService cpuFrameBenchmarkService_;
};

} // namespace DirectX
} // namespace gits

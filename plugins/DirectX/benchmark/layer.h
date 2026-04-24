// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "config.h"
#include "messageBus.h"
#include "services/cpuFrameBenchmarkService.h"

namespace gits {
namespace DirectX {

class BenchmarkLayer : public Layer {
public:
  BenchmarkLayer(const BenchmarkConfig& cfg, gits::MessageBus& msgBus);
  ~BenchmarkLayer() = default;

  void Pre(CreateDXGIFactoryCommand& command) override;
  void Pre(CreateDXGIFactory1Command& command) override;
  void Pre(CreateDXGIFactory2Command& command) override;
  void Post(IDXGISwapChainPresentCommand& command) override;
  void Post(IDXGISwapChain1Present1Command& command) override;

private:
  BenchmarkConfig cfg_;
  CpuFrameBenchmarkService cpuFrameBenchmarkService_;
};

} // namespace DirectX
} // namespace gits

// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"

namespace gits {
namespace DirectX {

struct HelloPluginConfig {
  bool printFrames = false;
  bool printGPUSubmissions = false;
};

class HelloPluginLayer : public Layer {
public:
  HelloPluginLayer(const HelloPluginConfig& cfg);
  ~HelloPluginLayer() = default;

  void Post(IDXGISwapChainPresentCommand& c) override;
  void Post(IDXGISwapChain1Present1Command& c) override;
  void Post(ID3D12CommandQueueExecuteCommandListsCommand& c) override;

private:
  void newFrame();

  HelloPluginConfig cfg_;
};

} // namespace DirectX
} // namespace gits

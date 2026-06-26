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
  bool PrintFrames = false;
  bool PrintGpuSubmissions = false;
};

class HelloPluginLayer : public Layer {
public:
  HelloPluginLayer(const HelloPluginConfig& cfg);
  ~HelloPluginLayer() = default;

  void Post(IDXGISwapChainPresentCommand& command) override;
  void Post(IDXGISwapChain1Present1Command& command) override;
  void Post(ID3D12CommandQueueExecuteCommandListsCommand& command) override;

private:
  void NewFrame();

  HelloPluginConfig m_Cfg;
};

} // namespace DirectX
} // namespace gits

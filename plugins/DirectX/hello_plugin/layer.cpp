// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "layer.h"
#include "log.h"

namespace gits {
namespace DirectX {

HelloPluginLayer::HelloPluginLayer(const HelloPluginConfig& cfg)
    : Layer("HelloPlugin"), m_Cfg(cfg) {}

void HelloPluginLayer::Post(IDXGISwapChainPresentCommand& command) {
  NewFrame();
}

void HelloPluginLayer::Post(IDXGISwapChain1Present1Command& command) {
  NewFrame();
}

void HelloPluginLayer::Post(ID3D12CommandQueueExecuteCommandListsCommand& command) {
  static unsigned gpuSubmissionNum = 0;
  if (!m_Cfg.PrintGpuSubmissions) {
    return;
  }
  LOG_INFO << "HelloPlugin - GPU Submission: " << ++gpuSubmissionNum;
}

void HelloPluginLayer::NewFrame() {
  static unsigned frameNum = 0;
  if (!m_Cfg.PrintFrames) {
    return;
  }
  LOG_INFO << "HelloPlugin - Frame: " << ++frameNum;
}

} // namespace DirectX
} // namespace gits

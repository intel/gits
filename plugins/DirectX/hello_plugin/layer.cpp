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
    : Layer("HelloPlugin"), cfg_(cfg) {}

void HelloPluginLayer::post(IDXGISwapChainPresentCommand& c) {
  newFrame();
}

void HelloPluginLayer::post(IDXGISwapChain1Present1Command& c) {
  newFrame();
}

void HelloPluginLayer::post(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  static unsigned gpuSubmissionNum = 0;
  if (!cfg_.printGPUSubmissions) {
    return;
  }
  LOG_INFO << "HelloPlugin - GPU Submission: " << ++gpuSubmissionNum;
}

void HelloPluginLayer::newFrame() {
  static unsigned frameNum = 0;
  if (!cfg_.printFrames) {
    return;
  }
  LOG_INFO << "HelloPlugin - Frame: " << ++frameNum;
}

} // namespace DirectX
} // namespace gits

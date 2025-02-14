// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "layer.h"
#include "pluginUtils.h"

namespace gits {
namespace DirectX {

HelloPluginLayer::HelloPluginLayer(CGits& gits, const HelloPluginConfig& cfg)
    : Layer("HelloPlugin"), gits_(gits), cfg_(cfg) {}

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
  log(gits_, "GPU Submission: ", ++gpuSubmissionNum);
}

void HelloPluginLayer::newFrame() {
  static unsigned frameNum = 0;
  if (!cfg_.printFrames) {
    return;
  }
  log(gits_, "Frame: ", ++frameNum);
}

} // namespace DirectX
} // namespace gits

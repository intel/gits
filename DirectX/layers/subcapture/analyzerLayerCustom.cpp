// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "analyzerLayerAuto.h"
#include "config.h"
#include "gits.h"
#include "configurationLib.h"

#include <fstream>
#include <sstream>

namespace gits {
namespace DirectX {

void AnalyzerLayer::post(IDXGISwapChainPresentCommand& c) {
  if (c.Flags_.value & DXGI_PRESENT_TEST || c.key & Command::stateRestoreKeyMask) {
    return;
  }
  analyzerService_.present(c.key);
}

void AnalyzerLayer::post(IDXGISwapChain1Present1Command& c) {
  if (c.PresentFlags_.value & DXGI_PRESENT_TEST || c.key & Command::stateRestoreKeyMask) {
    return;
  }
  analyzerService_.present(c.key);
}

void AnalyzerLayer::post(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  analyzerService_.executeCommandLists(c.key, c.object_.key, c.ppCommandLists_.keys);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListResetCommand& c) {
  analyzerService_.commandListReset(c.object_.key);
}

void AnalyzerLayer::post(ID3D12CommandQueueWaitCommand& c) {
  analyzerService_.commandQueueWait(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
}

void AnalyzerLayer::post(ID3D12CommandQueueSignalCommand& c) {
  analyzerService_.commandQueueSignal(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
}

void AnalyzerLayer::post(ID3D12FenceSignalCommand& c) {
  analyzerService_.fenceSignal(c.key, c.object_.key, c.Value_.value);
}

void AnalyzerLayer::post(ID3D12DeviceCreateFenceCommand& c) {
  analyzerService_.fenceSignal(c.key, c.ppFence_.key, c.InitialValue_.value);
}

void AnalyzerLayer::post(ID3D12Device3EnqueueMakeResidentCommand& c) {
  analyzerService_.fenceSignal(c.key, c.pFenceToSignal_.key, c.FenceValueToSignal_.value);
}

void AnalyzerLayer::post(ID3D12CommandQueueCopyTileMappingsCommand& c) {
  analyzerService_.copyTileMappings(c.key, c.object_.key);
}

void AnalyzerLayer::post(ID3D12CommandQueueUpdateTileMappingsCommand& c) {
  analyzerService_.updateTileMappings(c.key, c.object_.key);
}

} // namespace DirectX
} // namespace gits

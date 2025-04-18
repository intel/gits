// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "layerAuto.h"
#include "analyzerService.h"

namespace gits {
namespace DirectX {

class AnalyzerLayer : public Layer {
public:
  AnalyzerLayer() : Layer("Analyzer") {}

  void post(IDXGISwapChainPresentCommand& c) override;
  void post(IDXGISwapChain1Present1Command& c) override;
  void post(ID3D12CommandQueueExecuteCommandListsCommand& c) override;
  void post(ID3D12CommandQueueWaitCommand& c) override;
  void post(ID3D12CommandQueueSignalCommand& c) override;
  void post(ID3D12FenceSignalCommand& c) override;
  void post(ID3D12DeviceCreateFenceCommand& c) override;
  void post(ID3D12Device3EnqueueMakeResidentCommand& c) override;
  void post(ID3D12CommandQueueCopyTileMappingsCommand& c) override;
  void post(ID3D12CommandQueueUpdateTileMappingsCommand& c) override;
  %for interface in interfaces:
  %for function in interface.functions:
  %if interface.name.startswith('ID3D12GraphicsCommandList') and not function.name.startswith('SetName'):
  void post(${interface.name}${function.name}Command& c) override;
  %endif
  %endfor
  %endfor

private:
  AnalyzerService analyzerService_;
};

} // namespace DirectX
} // namespace gits

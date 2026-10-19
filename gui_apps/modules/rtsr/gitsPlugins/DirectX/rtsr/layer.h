// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "performanceTracker.h"
#include "pipelineTracker.h"
#include "rtsrPluginConfig.h"
#include "rtsr/shared/client/rtsrClient.h"

#include <memory>

namespace gits {

class CGits;

namespace DirectX {

// GITS DirectX layer implementing the RTSR client. The layer tracks the stream's
// root signatures and pipeline states itself, serializes them, and talks to the
// RTSR server directly; nothing is forwarded to RTSR's own layer DLL.
//
// When no server answers the handshake the layer stays disabled and every hook
// is a no-op, so playback and capture proceed unaffected.
class RtsrLayer : public Layer {
public:
  // 'gits' and 'playback' only select the clock the frame time is measured on,
  // see PerformanceTracker.
  RtsrLayer(const rtsr::PluginConfig& config, CGits* gits, bool playback);
  ~RtsrLayer();

  // Root signatures have to be tracked because a pipeline's state cannot be
  // serialized without the description of the signature it was built with.
  void Post(ID3D12DeviceCreateRootSignatureCommand& c) override;

  // Harvesting.
  void Post(ID3D12DeviceCreateGraphicsPipelineStateCommand& c) override;
  void Post(ID3D12DeviceCreateComputePipelineStateCommand& c) override;
  void Post(ID3D12Device2CreatePipelineStateCommand& c) override;
  void Post(ID3D12PipelineLibraryLoadGraphicsPipelineCommand& c) override;
  void Post(ID3D12PipelineLibraryLoadComputePipelineCommand& c) override;
  void Post(ID3D12PipelineLibrary1LoadPipelineCommand& c) override;

  // Metadata and lifetime.
  void Post(ID3D12ObjectSetNameCommand& c) override;
  void Post(IUnknownReleaseCommand& c) override;

  // Binds pipelines whose creation was deferred by a player layer, and swaps in
  // a replacement when the server has installed one.
  void Pre(ID3D12DeviceCreateCommandListCommand& c) override;
  void Pre(ID3D12GraphicsCommandListSetPipelineStateCommand& c) override;
  void Pre(ID3D12GraphicsCommandListResetCommand& c) override;

  // Performance measurement. These bracket the GPU work of each bound pipeline
  // with timestamp queries and are no-ops until the server asks for timings.
  void Post(ID3D12DeviceCreateCommandListCommand& c) override;
  void Post(ID3D12GraphicsCommandListResetCommand& c) override;
  void Post(ID3D12GraphicsCommandListSetPipelineStateCommand& c) override;
  void Pre(ID3D12GraphicsCommandListCloseCommand& c) override;
  void Post(ID3D12CommandQueueExecuteCommandListsCommand& c) override;
  void Post(IDXGISwapChainPresentCommand& c) override;
  void Post(IDXGISwapChain1Present1Command& c) override;

private:
  bool m_Enabled{false};
  std::unique_ptr<rtsr::RtsrClient> m_Client;
  std::unique_ptr<PipelineTracker> m_Tracker;
  // Declared last, and torn down first, because its worker thread reports
  // through the client and reads the tracker.
  std::unique_ptr<PerformanceTracker> m_Performance;
};

} // namespace DirectX
} // namespace gits

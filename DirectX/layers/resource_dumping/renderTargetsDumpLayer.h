// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "renderTargetsDump.h"
#include "bit_range.h"
#include "resourceStateTracker.h"

#include <unordered_map>
#include <map>
#include <set>
#include <d3d12.h>

namespace gits {
namespace DirectX {

class RenderTargetsDumpLayer : public Layer {
public:
  RenderTargetsDumpLayer();
  void post(ID3D12DeviceCreateRenderTargetViewCommand& c) override;
  void post(ID3D12DeviceCreateDepthStencilViewCommand& c) override;
  void post(ID3D12DeviceCopyDescriptorsSimpleCommand& c) override;
  void post(ID3D12DeviceCopyDescriptorsCommand& c) override;
  void post(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c) override;
  void post(ID3D12GraphicsCommandListDrawInstancedCommand& c) override;
  void post(ID3D12GraphicsCommandListDrawIndexedInstancedCommand& c) override;
  void post(ID3D12CommandQueueExecuteCommandListsCommand& c) override;
  void post(ID3D12CommandQueueWaitCommand& c) override;
  void post(ID3D12CommandQueueSignalCommand& c) override;
  void post(ID3D12FenceSignalCommand& c) override;
  void post(ID3D12DeviceCreateFenceCommand& c) override;
  void post(ID3D12Device3EnqueueMakeResidentCommand& c) override;
  void post(IDXGISwapChainPresentCommand& c) override;
  void post(IDXGISwapChain1Present1Command& c) override;

private:
  std::wstring dumpPath_;
  RenderTargetsDump resourceDump_;
  BitRange frameRange_;
  BitRange drawRange_;
  unsigned drawCount_{};
  unsigned executeCount_{};
  std::unordered_map<unsigned, unsigned> drawCountByCommandList_;
  bool stateRestorePhase_{};
  ResourceStateTracker resourceStateTracker_;

  struct RenderTarget {
    unsigned slot;
    ID3D12Resource* resource;
    unsigned resourceKey;
    bool isDesc{};
    D3D12_RENDER_TARGET_VIEW_DESC desc{};
  };
  struct DepthStencil {
    ID3D12Resource* resource;
    unsigned resourceKey;
    bool isDesc{};
    D3D12_DEPTH_STENCIL_VIEW_DESC desc{};
  };
  std::map<std::pair<unsigned, unsigned>, RenderTarget> renderTargetsByDescriptorHandle_;
  std::map<std::pair<unsigned, unsigned>, DepthStencil> depthStencilsByDescriptorHandle_;
  std::unordered_map<unsigned, std::vector<RenderTarget>> renderTargetsByCommandList_;
  std::unordered_map<unsigned, DepthStencil> depthStencilByCommandList_;

private:
  void onDraw(ID3D12GraphicsCommandList* commandList, unsigned commandListKey);
  void dumpRenderTarget(ID3D12GraphicsCommandList* commandList,
                        RenderTarget& renderTarget,
                        unsigned frame,
                        unsigned commandListDraw,
                        unsigned frameDraw);
  void dumpDepthStencil(ID3D12GraphicsCommandList* commandList,
                        DepthStencil& depthStencil,
                        unsigned frame,
                        unsigned commandListDraw,
                        unsigned frameDraw);
  template <typename Descriptors>
  void copyDescriptors(Descriptors& descriptors,
                       unsigned srcHeapKey,
                       unsigned srcHeapIndex,
                       unsigned destHeapKey,
                       unsigned destHeapIndex);
};

} // namespace DirectX
} // namespace gits

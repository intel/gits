// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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
  ~RenderTargetsDumpLayer();
  void Post(StateRestoreBeginCommand& c) override;
  void Post(StateRestoreEndCommand& c) override;
  void Post(ID3D12DeviceCreateRenderTargetViewCommand& c) override;
  void Post(ID3D12DeviceCreateDepthStencilViewCommand& c) override;
  void Post(ID3D12DeviceCopyDescriptorsSimpleCommand& c) override;
  void Post(ID3D12DeviceCopyDescriptorsCommand& c) override;
  void Post(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c) override;
  void Post(ID3D12GraphicsCommandListDrawInstancedCommand& c) override;
  void Post(ID3D12GraphicsCommandListDrawIndexedInstancedCommand& c) override;
  void Post(ID3D12CommandQueueExecuteCommandListsCommand& c) override;
  void Post(ID3D12CommandQueueWaitCommand& c) override;
  void Post(ID3D12CommandQueueSignalCommand& c) override;
  void Post(ID3D12FenceSignalCommand& c) override;
  void Post(ID3D12DeviceCreateFenceCommand& c) override;
  void Post(ID3D12Device3EnqueueMakeResidentCommand& c) override;
  void Post(IDXGISwapChainPresentCommand& c) override;
  void Post(IDXGISwapChain1Present1Command& c) override;

private:
  std::wstring m_DumpPath;
  RenderTargetsDump m_ResourceDump;
  BitRange m_FrameRange;
  BitRange m_DrawRange;
  bool m_DryRun{};
  unsigned m_DrawCount{};
  unsigned m_ExecuteCount{};
  std::unordered_map<unsigned, unsigned> m_DrawCountByCommandList;
  ResourceStateTracker m_ResourceStateTracker;
  unsigned m_CurrentFrame{1};

  struct RenderTarget {
    unsigned slot;
    ID3D12Resource* resource;
    unsigned ResourceKey;
    bool isDesc{};
    D3D12_RENDER_TARGET_VIEW_DESC desc{};
  };
  struct DepthStencil {
    ID3D12Resource* resource;
    unsigned ResourceKey;
    bool isDesc{};
    D3D12_DEPTH_STENCIL_VIEW_DESC desc{};
  };
  std::map<std::pair<unsigned, unsigned>, RenderTarget> m_RenderTargetsByDescriptorHandle;
  std::map<std::pair<unsigned, unsigned>, DepthStencil> m_DepthStencilsByDescriptorHandle;
  std::unordered_map<unsigned, std::vector<RenderTarget>> m_RenderTargetsByCommandList;
  std::unordered_map<unsigned, DepthStencil> m_DepthStencilByCommandList;

  struct DryRunInfo {
    std::map<unsigned, std::set<unsigned>> drawsWithTextureByFrame;
  } m_DryRunInfo;

private:
  void OnDraw(ID3D12GraphicsCommandList* commandList, unsigned commandListKey);
  void DumpRenderTarget(ID3D12GraphicsCommandList* commandList,
                        RenderTarget& renderTarget,
                        unsigned frame,
                        unsigned commandListDraw,
                        unsigned frameDraw);
  void DumpDepthStencil(ID3D12GraphicsCommandList* commandList,
                        DepthStencil& depthStencil,
                        unsigned frame,
                        unsigned commandListDraw,
                        unsigned frameDraw);
  template <typename Descriptors>
  void CopyDescriptors(Descriptors& descriptors,
                       unsigned srcHeapKey,
                       unsigned srcHeapIndex,
                       unsigned destHeapKey,
                       unsigned destHeapIndex);
};

} // namespace DirectX
} // namespace gits

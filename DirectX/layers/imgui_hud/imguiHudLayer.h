// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"

#include <vector>
#include <d3d12.h>
#include <wrl/client.h>
#include <mutex>
#include <unordered_set>

namespace gits {

class CGits;

namespace DirectX {

struct FrameContext {
  UINT64 fenceValue = {};
  Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
  ID3D12Resource* rtResource = {};
  D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = {};
};

class ImGuiHUDLayer : public Layer {
public:
  ImGuiHUDLayer();
  ~ImGuiHUDLayer();

  void Post(IDXGISwapChainGetBufferCommand& c) override;
  void Pre(IUnknownReleaseCommand& c) override;
  void Pre(ID3D12CommandQueueExecuteCommandListsCommand& c) override;
  void Post(IDXGIFactoryCreateSwapChainCommand& c) override;
  void Post(IDXGIFactory2CreateSwapChainForHwndCommand& c) override;
  void Post(IDXGIFactory2CreateSwapChainForCoreWindowCommand& c) override;
  void Post(IDXGIFactory2CreateSwapChainForCompositionCommand& c) override;
  void Pre(IDXGISwapChainPresentCommand& command) override;
  void Post(IDXGISwapChainPresentCommand& command) override;
  void Pre(IDXGISwapChain1Present1Command& command) override;
  void Post(IDXGISwapChain1Present1Command& command) override;
  void Post(IDXGISwapChainResizeBuffersCommand& command) override;
  void Post(IDXGISwapChain3ResizeBuffers1Command& command) override;

private:
  bool CreateFrameContext(unsigned bufferCount);
  bool InitializeResources(IUnknown* device, IDXGISwapChain* swapChain);
  void InitializeImGui(DXGI_FORMAT format);
  void OnPrePresent();
  void WaitForCurrentFrame();
  void WaitForFrame(unsigned bufferIndex);
  void Present();

private:
  bool m_Initialized = false;

  std::mutex m_Mutex;
  std::unordered_set<unsigned> m_BackBufferKeys;

  std::vector<FrameContext> m_FrameContext = {};
  Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence = nullptr;
  Microsoft::WRL::ComPtr<ID3D12Device> m_Device = nullptr;
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_RtvDescHeap = nullptr;
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_SrvDescHeap = nullptr;
  Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_CommandQueue = nullptr;
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_CommandList = nullptr;
  Microsoft::WRL::ComPtr<IDXGISwapChain3> m_SwapChain = nullptr;

  UINT64 m_FenceValue = 0;
  bool m_FirstExecuteInFrame = true;

  void* m_Window = nullptr;
  bool m_ResizeBuffersWarning = false;
};

} // namespace DirectX
} // namespace gits

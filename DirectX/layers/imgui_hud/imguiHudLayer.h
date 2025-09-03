// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"

#include <vector>
#include <d3d12.h>
#include <wrl/client.h>

namespace gits {

class CGits;

namespace DirectX {

struct FrameContext {
  Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
  Microsoft::WRL::ComPtr<ID3D12Resource> rtResource = {};
  D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = {};
};

class ImGuiHUDLayer : public Layer {
public:
  ImGuiHUDLayer();
  ~ImGuiHUDLayer();

  void post(IDXGIFactoryCreateSwapChainCommand& c) override;
  void post(IDXGIFactory2CreateSwapChainForHwndCommand& c) override;
  void post(IDXGIFactory2CreateSwapChainForCoreWindowCommand& c) override;
  void post(IDXGIFactory2CreateSwapChainForCompositionCommand& c) override;
  void pre(IDXGISwapChainPresentCommand& command) override;
  void pre(IDXGISwapChainResizeBuffersCommand& command) override;
  void post(IDXGISwapChainResizeBuffersCommand& command) override;
  void pre(IDXGISwapChain3ResizeBuffers1Command& command) override;
  void post(IDXGISwapChain3ResizeBuffers1Command& command) override;

private:
  bool createFrameContext(unsigned bufferCount);
  bool initializeResources(IUnknown* device, IDXGISwapChain* swapChain);
  void initializeImGui(DXGI_FORMAT format);

private:
  bool initialized_ = false;

  std::vector<FrameContext> frameContext_ = {};
  Microsoft::WRL::ComPtr<ID3D12Fence> fence_ = nullptr;
  Microsoft::WRL::ComPtr<ID3D12Device> device_ = nullptr;
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescHeap_ = nullptr;
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescHeap_ = nullptr;
  Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_ = nullptr;
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_ = nullptr;
  Microsoft::WRL::ComPtr<IDXGISwapChain3> swapChain_ = nullptr;

  UINT frameIndex_ = 0;
  UINT64 fenceValue_ = 0;

  void* window_ = nullptr;
  bool resizeBuffersWarning_ = false;
};

} // namespace DirectX
} // namespace gits

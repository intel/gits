// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "iunknownWrapper.h"

#include <unordered_map>
#include <vector>
#include <mutex>

namespace gits {
namespace DirectX {

class InterceptorCustomizationLayer : public Layer {
public:
  InterceptorCustomizationLayer() : Layer("InterceptorCustomization") {}

  void post(IUnknownReleaseCommand& command) override;
  void post(IDXGIFactoryCreateSwapChainCommand& command) override;
  void post(IDXGIFactory2CreateSwapChainForHwndCommand& command) override;
  void post(IDXGIFactory2CreateSwapChainForCoreWindowCommand& command) override;
  void post(IDXGIFactory2CreateSwapChainForCompositionCommand& command) override;
  void post(IDXGIFactoryMediaCreateSwapChainForCompositionSurfaceHandleCommand& command) override;
  void pre(IDXGISwapChainResizeBuffersCommand& command) override;
  void post(IDXGISwapChainResizeBuffersCommand& command) override;
  void pre(IDXGISwapChain3ResizeBuffers1Command& command) override;
  void post(IDXGISwapChain3ResizeBuffers1Command& command) override;
  void pre(D3D12CreateDeviceCommand& command) override;
  void post(D3D12CreateDeviceCommand& command) override;

private:
  void acquireSwapChainBuffers(IDXGISwapChain* swapChain, unsigned swapChainKey);
  void releaseSwapChainBuffers(unsigned swapChainKey);
  std::unordered_map<unsigned, std::vector<IUnknownWrapper*>> swapChainBuffers_;
  std::mutex mutex_;
};

} // namespace DirectX
} // namespace gits

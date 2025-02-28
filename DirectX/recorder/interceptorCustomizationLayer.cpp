// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "interceptorCustomizationLayer.h"
#include "wrapperUtils.h"
#include "captureManager.h"
#include "objectInfos.h"
#include "intelExtensionsWrappers.h"
#include "gits.h"

#include <wrl/client.h>

namespace gits {
namespace DirectX {

void InterceptorCustomizationLayer::acquireSwapChainBuffers(IDXGISwapChain* swapChain,
                                                            unsigned swapChainKey) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<IUnknownWrapper*>& swapChainBuffers = swapChainBuffers_[swapChainKey];

  DXGI_SWAP_CHAIN_DESC desc{};
  HRESULT hr = swapChain->GetDesc(&desc);
  GITS_ASSERT(hr == S_OK);

  for (unsigned i = 0; i < desc.BufferCount; ++i) {
    ID3D12Resource* buffer = nullptr;
    hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&buffer));
    if (hr == S_OK) {
      wrapObject(IID_PPV_ARGS(&buffer));
      swapChainBuffers.push_back(reinterpret_cast<IUnknownWrapper*>(buffer));
    }
  }
}

void InterceptorCustomizationLayer::releaseSwapChainBuffers(unsigned swapChainKey) {
  std::vector<IUnknownWrapper*>& swapChainBuffers = swapChainBuffers_[swapChainKey];
  CaptureManager& manager = CaptureManager::get();
  for (IUnknownWrapper* bufferWrapper : swapChainBuffers) {
    ID3D12Resource* buffer = bufferWrapper->getWrappedObject<ID3D12Resource>();
    buffer->Release();
    manager.removeWrapper(bufferWrapper);
  }
  swapChainBuffers.clear();
}

void InterceptorCustomizationLayer::post(IUnknownReleaseCommand& c) {
  if (c.result_.value == 1) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = swapChainBuffers_.find(c.object_.key);
    if (it != swapChainBuffers_.end()) {
      c.object_.value->AddRef();
      releaseSwapChainBuffers(c.object_.key);
      c.object_.value->Release();
    }
  }
}

void InterceptorCustomizationLayer::post(IDXGIFactoryCreateSwapChainCommand& c) {
  if (c.result_.value == S_OK) {
    acquireSwapChainBuffers(*c.ppSwapChain_.value, c.ppSwapChain_.key);
  }
}

void InterceptorCustomizationLayer::post(IDXGIFactory2CreateSwapChainForHwndCommand& c) {
  if (c.result_.value == S_OK) {
    acquireSwapChainBuffers(*c.ppSwapChain_.value, c.ppSwapChain_.key);
  }
}

void InterceptorCustomizationLayer::post(IDXGIFactory2CreateSwapChainForCoreWindowCommand& c) {
  if (c.result_.value == S_OK) {
    acquireSwapChainBuffers(*c.ppSwapChain_.value, c.ppSwapChain_.key);
  }
}

void InterceptorCustomizationLayer::post(IDXGIFactory2CreateSwapChainForCompositionCommand& c) {
  if (c.result_.value == S_OK) {
    acquireSwapChainBuffers(*c.ppSwapChain_.value, c.ppSwapChain_.key);
  }
}

void InterceptorCustomizationLayer::post(
    IDXGIFactoryMediaCreateSwapChainForCompositionSurfaceHandleCommand& c) {
  if (c.result_.value == S_OK) {
    acquireSwapChainBuffers(*c.ppSwapChain_.value, c.ppSwapChain_.key);
  }
}

void InterceptorCustomizationLayer::pre(IDXGISwapChainResizeBuffersCommand& c) {
  std::lock_guard<std::mutex> lock(mutex_);
  // Resizing the swapchain requires all the buffers to have been previously released (we should not track them anymore)
  // https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-resizebuffers
  releaseSwapChainBuffers(c.object_.key);
}

void InterceptorCustomizationLayer::post(IDXGISwapChainResizeBuffersCommand& c) {
  if (c.result_.value == S_OK) {
    acquireSwapChainBuffers(c.object_.value, c.object_.key);
  }
}

void InterceptorCustomizationLayer::pre(IDXGISwapChain3ResizeBuffers1Command& c) {
  std::lock_guard<std::mutex> lock(mutex_);
  // Resizing the swapchain requires all the buffers to have been previously released (we should not track them anymore)
  // https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-resizebuffers
  releaseSwapChainBuffers(c.object_.key);
}

void InterceptorCustomizationLayer::post(IDXGISwapChain3ResizeBuffers1Command& c) {
  if (c.result_.value == S_OK) {
    acquireSwapChainBuffers(c.object_.value, c.object_.key);
  }
}

void InterceptorCustomizationLayer::pre(D3D12CreateDeviceCommand& command) {
  Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
  if (command.pAdapter_.value) {
    command.pAdapter_.value->QueryInterface(IID_PPV_ARGS(&adapter));
  }
  if (!adapter) {
    Microsoft::WRL::ComPtr<IDXGIFactory1> factory;
    HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
    GITS_ASSERT(hr == S_OK);
    hr = factory->EnumAdapters(0, &adapter);
    GITS_ASSERT(hr == S_OK);
  }

  DXGI_ADAPTER_DESC desc{};
  HRESULT hr = adapter->GetDesc(&desc);
  GITS_ASSERT(hr == S_OK);

  CaptureManager::get().loadIntelExtension(desc.VendorId, desc.DeviceId);
}

void InterceptorCustomizationLayer::post(D3D12CreateDeviceCommand& command) {
  CaptureManager::get().interceptXessFunctions();
}

} // namespace DirectX
} // namespace gits

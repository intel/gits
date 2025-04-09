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

void InterceptorCustomizationLayer::post(IDXGISwapChainGetBufferCommand& c) {
  if (c.result_.value == S_OK) {
    std::lock_guard<std::mutex> lock(mutex_);
    swapChainByBufferKey_[c.ppSurface_.key] = c.object_.key;
    buffersBySwapChainKey_[c.object_.key].push_back(static_cast<IUnknown*>(*c.ppSurface_.value));
  }
}

void InterceptorCustomizationLayer::pre(IUnknownReleaseCommand& c) {
  // Handle swap chain buffers shared reference count
  // Needs to be done in pre layer, because objects need to be alive for removeWrapper call

  c.object_.value->AddRef();
  auto result = c.object_.value->Release();
  if (result != 1) {
    return;
  }

  std::lock_guard<std::mutex> lock(mutex_);
  auto itSwapChain = swapChainByBufferKey_.find(c.object_.key);
  if (itSwapChain == swapChainByBufferKey_.end()) {
    return;
  }
  auto itBuffers = buffersBySwapChainKey_.find(itSwapChain->second);
  if (itBuffers == buffersBySwapChainKey_.end()) {
    return;
  }

  for (IUnknown* bufferObject : itBuffers->second) {
    // Buffer which is currently beeing released is removed in IUnknownWrapper::Release
    if (bufferObject == c.object_.value) {
      continue;
    }
    auto* bufferWrapper = CaptureManager::get().findWrapper(bufferObject);
    swapChainByBufferKey_.erase(bufferWrapper->getKey());
    CaptureManager::get().removeWrapper(bufferWrapper);
  }
  swapChainByBufferKey_.erase(itSwapChain);
  buffersBySwapChainKey_.erase(itBuffers);
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

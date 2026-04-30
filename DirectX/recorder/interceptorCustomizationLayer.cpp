// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "interceptorCustomizationLayer.h"
#include "wrapperUtils.h"
#include "captureManager.h"
#include "intelExtensionsWrappers.h"
#include "log.h"

#include <wrl/client.h>

namespace gits {
namespace DirectX {

void InterceptorCustomizationLayer::Post(IDXGISwapChainGetBufferCommand& c) {
  if (c.m_Result.Value == S_OK) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_SwapChainByBufferKey[c.m_ppSurface.Key] = c.m_Object.Key;
    m_BuffersBySwapChainKey[c.m_Object.Key].insert(static_cast<IUnknown*>(*c.m_ppSurface.Value));
  }
}

void InterceptorCustomizationLayer::Pre(IUnknownReleaseCommand& c) {
  // Handle swap chain buffers shared reference count
  // Needs to be done in pre layer, because objects need to be alive for removeWrapper call

  c.m_Object.Value->AddRef();
  auto result = c.m_Object.Value->Release();
  if (result != 1) {
    return;
  }

  std::lock_guard<std::mutex> lock(m_Mutex);
  auto itSwapChain = m_SwapChainByBufferKey.find(c.m_Object.Key);
  if (itSwapChain == m_SwapChainByBufferKey.end()) {
    return;
  }
  auto itBuffers = m_BuffersBySwapChainKey.find(itSwapChain->second);
  if (itBuffers == m_BuffersBySwapChainKey.end()) {
    return;
  }

  for (IUnknown* bufferObject : itBuffers->second) {
    // Buffer which is currently beeing released is removed in IUnknownWrapper::Release
    if (bufferObject == c.m_Object.Value) {
      continue;
    }
    auto* bufferWrapper = CaptureManager::get().findWrapper(bufferObject);
    GITS_ASSERT(bufferWrapper != nullptr, "Can't remove a nonexistent buffer wrapper.");
    m_SwapChainByBufferKey.erase(bufferWrapper->GetKey());
    CaptureManager::get().removeWrapper(bufferWrapper);
  }
  m_SwapChainByBufferKey.erase(itSwapChain);
  m_BuffersBySwapChainKey.erase(itBuffers);
}

void InterceptorCustomizationLayer::Pre(D3D12CreateDeviceCommand& command) {
  Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
  if (command.m_pAdapter.Value) {
    command.m_pAdapter.Value->QueryInterface(IID_PPV_ARGS(&adapter));
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

void InterceptorCustomizationLayer::Post(D3D12CreateDeviceCommand& command) {
  CaptureManager::get().interceptXessFunctions();
  CaptureManager::get().interceptXellFunctions();
  CaptureManager::get().interceptXefgFunctions();
}

} // namespace DirectX
} // namespace gits

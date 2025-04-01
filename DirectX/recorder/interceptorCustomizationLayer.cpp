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

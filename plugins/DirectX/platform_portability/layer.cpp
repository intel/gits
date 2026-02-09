// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "layer.h"
#include "log.h"
#include <wrl/client.h>
#include <igdext.h>

namespace gits {
namespace DirectX {
PlatformPortabilityLayer::~PlatformPortabilityLayer() {}

void PlatformPortabilityLayer::pre(
    ID3D12DeviceRemovedExtendedDataSettingsSetAutoBreadcrumbsEnablementCommand& c) {
  c.skip = true;
}
void PlatformPortabilityLayer::pre(
    ID3D12DeviceRemovedExtendedDataSettingsSetPageFaultEnablementCommand& c) {
  c.skip = true;
}
void PlatformPortabilityLayer::pre(
    ID3D12DeviceRemovedExtendedDataSettingsSetWatsonDumpEnablementCommand& c) {
  c.skip = true;
}
void PlatformPortabilityLayer::pre(
    ID3D12DeviceRemovedExtendedDataSettings1SetBreadcrumbContextEnablementCommand& c) {
  c.skip = true;
}
void PlatformPortabilityLayer::pre(
    ID3D12DeviceRemovedExtendedDataSettings2UseMarkersOnlyAutoBreadcrumbsCommand& c) {
  c.skip = true;
}
void PlatformPortabilityLayer::pre(ID3D12SDKConfigurationSetSDKVersionCommand& c) {
  c.skip = true;
}

void PlatformPortabilityLayer::pre(INTC_D3D12_CreateDeviceExtensionContextCommand& c) {
  c.skip = true;
}

void PlatformPortabilityLayer::pre(INTC_D3D12_CreateDeviceExtensionContext1Command& c) {
  c.skip = true;
}

void PlatformPortabilityLayer::pre(INTC_D3D12_SetApplicationInfoCommand& c) {
  c.skip = true;
}

void PlatformPortabilityLayer::pre(INTC_D3D12_SetFeatureSupportCommand& c) {
  c.skip = true;
}

void PlatformPortabilityLayer::pre(INTC_D3D12_CreatePlacedResourceCommand& c) {
  c.skip = true;
  if (!c.pHeap_.value) {
    c.result_.value = E_INVALIDARG;
    return;
  }

  // Get device from heap
  Microsoft::WRL::ComPtr<ID3D12Device> pDevice;
  HRESULT hr = c.pHeap_.value->GetDevice(IID_PPV_ARGS(&pDevice));
  if (FAILED(hr) || !pDevice) {
    return;
  }

  // The pDesc_ should already be a standard D3D12_RESOURCE_DESC pointer
  auto* pDesc = reinterpret_cast<const D3D12_RESOURCE_DESC*>(c.pDesc_.value);
  if (!pDesc) {
    return;
  }

  // Forward to standard D3D12 CreatePlacedResource
  hr = pDevice->CreatePlacedResource(c.pHeap_.value, c.HeapOffset_.value, pDesc,
                                     static_cast<D3D12_RESOURCE_STATES>(c.InitialState_.value),
                                     c.pOptimizedClearValue_.value, c.riid_.value,
                                     c.ppvResource_.value);

  GITS_ASSERT(SUCCEEDED(hr), "Failed to create placed resource");

  if (SUCCEEDED(hr)) {
    c.result_.value = hr;
  }
}

} // namespace DirectX
} // namespace gits

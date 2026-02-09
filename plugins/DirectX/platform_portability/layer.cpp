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
    LOG_ERROR << "PlatformPortability - pHeap is null";
    c.result_.value = E_INVALIDARG;
    return;
  }

  // Get device from heap
  Microsoft::WRL::ComPtr<ID3D12Device> pDevice;
  HRESULT hr = c.pHeap_.value->GetDevice(IID_PPV_ARGS(&pDevice));
  if (FAILED(hr) || !pDevice) {
    LOG_ERROR << "PlatformPortability - GetDevice failed with HRESULT: 0x" << std::hex << hr
              << std::dec;
    c.result_.value = FAILED(hr) ? hr : E_FAIL;
    return;
  }

  // Extract D3D12_RESOURCE_DESC from Intel extended structure
  // The first member of INTC_D3D12_RESOURCE_DESC_0001 is a pointer to D3D12_RESOURCE_DESC
  if (!c.pDesc_.value) {
    LOG_ERROR << "PlatformPortability - Resource descriptor is null";
    c.result_.value = E_INVALIDARG;
    return;
  }

  auto** ppDesc = reinterpret_cast<D3D12_RESOURCE_DESC**>(c.pDesc_.value);
  auto* pDesc = *ppDesc;

  if (!pDesc) {
    LOG_ERROR << "PlatformPortability - D3D12_RESOURCE_DESC pointer is null";
    c.result_.value = E_INVALIDARG;
    return;
  }

  // Validate descriptor
  if (pDesc->Dimension > D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
    LOG_ERROR << "PlatformPortability - Invalid resource dimension: " << pDesc->Dimension;
    c.result_.value = E_INVALIDARG;
    return;
  }

  // Forward to standard D3D12 CreatePlacedResource
  hr = pDevice->CreatePlacedResource(c.pHeap_.value, c.HeapOffset_.value, pDesc,
                                     static_cast<D3D12_RESOURCE_STATES>(c.InitialState_.value),
                                     c.pOptimizedClearValue_.value, c.riid_.value,
                                     c.ppvResource_.value);

  if (FAILED(hr)) {
    LOG_ERROR << "PlatformPortability - CreatePlacedResource failed with HRESULT: 0x" << std::hex
              << hr << std::dec;
    LOG_ERROR << "  Heap: " << c.pHeap_.value << ", Offset: " << c.HeapOffset_.value;
    LOG_ERROR << "  Dimension: " << pDesc->Dimension << ", Format: " << pDesc->Format;
    LOG_ERROR << "  Width: " << pDesc->Width << ", Height: " << pDesc->Height;
    LOG_ERROR << "  Flags: 0x" << std::hex << pDesc->Flags << std::dec;
  }

  c.result_.value = hr;
}

} // namespace DirectX
} // namespace gits

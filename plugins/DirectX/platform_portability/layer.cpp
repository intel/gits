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

void PlatformPortabilityLayer::Pre(
    ID3D12DeviceRemovedExtendedDataSettingsSetAutoBreadcrumbsEnablementCommand& c) {
  c.Skip = true;
}
void PlatformPortabilityLayer::Pre(
    ID3D12DeviceRemovedExtendedDataSettingsSetPageFaultEnablementCommand& c) {
  c.Skip = true;
}
void PlatformPortabilityLayer::Pre(
    ID3D12DeviceRemovedExtendedDataSettingsSetWatsonDumpEnablementCommand& c) {
  c.Skip = true;
}
void PlatformPortabilityLayer::Pre(
    ID3D12DeviceRemovedExtendedDataSettings1SetBreadcrumbContextEnablementCommand& c) {
  c.Skip = true;
}
void PlatformPortabilityLayer::Pre(
    ID3D12DeviceRemovedExtendedDataSettings2UseMarkersOnlyAutoBreadcrumbsCommand& c) {
  c.Skip = true;
}
void PlatformPortabilityLayer::Pre(ID3D12SDKConfigurationSetSDKVersionCommand& c) {
  c.Skip = true;
}

void PlatformPortabilityLayer::Pre(INTC_D3D12_CreateDeviceExtensionContextCommand& c) {
  c.Skip = true;
}

void PlatformPortabilityLayer::Pre(INTC_D3D12_CreateDeviceExtensionContext1Command& c) {
  c.Skip = true;
}

void PlatformPortabilityLayer::Pre(INTC_D3D12_SetApplicationInfoCommand& c) {
  c.Skip = true;
}

void PlatformPortabilityLayer::Pre(INTC_D3D12_SetFeatureSupportCommand& c) {
  c.Skip = true;
}

void PlatformPortabilityLayer::Pre(INTC_D3D12_CreatePlacedResourceCommand& c) {
  c.Skip = true;

  if (!c.m_pHeap.Value) {
    LOG_ERROR << "PlatformPortability - pHeap is null";
    c.m_Result.Value = E_INVALIDARG;
    return;
  }

  // Get device from heap
  Microsoft::WRL::ComPtr<ID3D12Device> pDevice;
  HRESULT hr = c.m_pHeap.Value->GetDevice(IID_PPV_ARGS(&pDevice));
  if (FAILED(hr) || !pDevice) {
    LOG_ERROR << "PlatformPortability - GetDevice failed with HRESULT: 0x" << std::hex << hr
              << std::dec;
    c.m_Result.Value = FAILED(hr) ? hr : E_FAIL;
    return;
  }

  // Extract D3D12_RESOURCE_DESC from Intel extended structure
  // The first member of INTC_D3D12_RESOURCE_DESC_0001 is a pointer to D3D12_RESOURCE_DESC
  if (!c.m_pDesc.Value) {
    LOG_ERROR << "PlatformPortability - Resource descriptor is null";
    c.m_Result.Value = E_INVALIDARG;
    return;
  }

  auto** ppDesc = reinterpret_cast<D3D12_RESOURCE_DESC**>(c.m_pDesc.Value);
  auto* pDesc = *ppDesc;

  if (!pDesc) {
    LOG_ERROR << "PlatformPortability - D3D12_RESOURCE_DESC pointer is null";
    c.m_Result.Value = E_INVALIDARG;
    return;
  }

  // Validate descriptor
  if (pDesc->Dimension > D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
    LOG_ERROR << "PlatformPortability - Invalid resource dimension: " << pDesc->Dimension;
    c.m_Result.Value = E_INVALIDARG;
    return;
  }

  // Forward to standard D3D12 CreatePlacedResource
  hr = pDevice->CreatePlacedResource(c.m_pHeap.Value, c.m_HeapOffset.Value, pDesc,
                                     static_cast<D3D12_RESOURCE_STATES>(c.m_InitialState.Value),
                                     c.m_pOptimizedClearValue.Value, c.m_riid.Value,
                                     c.m_ppvResource.Value);

  if (FAILED(hr)) {
    LOG_ERROR << "PlatformPortability - CreatePlacedResource failed with HRESULT: 0x" << std::hex
              << hr << std::dec;
    LOG_ERROR << "  Heap: " << c.m_pHeap.Value << ", Offset: " << c.m_HeapOffset.Value;
    LOG_ERROR << "  Dimension: " << pDesc->Dimension << ", Format: " << pDesc->Format;
    LOG_ERROR << "  Width: " << pDesc->Width << ", Height: " << pDesc->Height;
    LOG_ERROR << "  Flags: 0x" << std::hex << pDesc->Flags << std::dec;
  }

  c.m_Result.Value = hr;
}

} // namespace DirectX
} // namespace gits

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
    ID3D12DeviceRemovedExtendedDataSettingsSetAutoBreadcrumbsEnablementCommand& command) {
  command.Skip = true;
}
void PlatformPortabilityLayer::Pre(
    ID3D12DeviceRemovedExtendedDataSettingsSetPageFaultEnablementCommand& command) {
  command.Skip = true;
}
void PlatformPortabilityLayer::Pre(
    ID3D12DeviceRemovedExtendedDataSettingsSetWatsonDumpEnablementCommand& command) {
  command.Skip = true;
}
void PlatformPortabilityLayer::Pre(
    ID3D12DeviceRemovedExtendedDataSettings1SetBreadcrumbContextEnablementCommand& command) {
  command.Skip = true;
}
void PlatformPortabilityLayer::Pre(
    ID3D12DeviceRemovedExtendedDataSettings2UseMarkersOnlyAutoBreadcrumbsCommand& command) {
  command.Skip = true;
}
void PlatformPortabilityLayer::Pre(ID3D12SDKConfigurationSetSDKVersionCommand& command) {
  command.Skip = true;
}

void PlatformPortabilityLayer::Pre(D3D12GetDebugInterfaceCommand& command) {
  command.Skip = true;
}

void PlatformPortabilityLayer::Pre(ID3D12DebugEnableDebugLayerCommand& command) {
  command.Skip = true;
}

void PlatformPortabilityLayer::Pre(DXGIGetDebugInterface1Command& command) {
  command.Skip = true;
}

void PlatformPortabilityLayer::Pre(IDXGIInfoQueueSetBreakOnSeverityCommand& command) {
  command.Skip = true;
}

void PlatformPortabilityLayer::Pre(INTC_D3D12_CreateDeviceExtensionContextCommand& command) {
  command.Skip = true;
}

void PlatformPortabilityLayer::Pre(INTC_D3D12_CreateDeviceExtensionContext1Command& command) {
  command.Skip = true;
}

void PlatformPortabilityLayer::Pre(INTC_D3D12_SetApplicationInfoCommand& command) {
  command.Skip = true;
}

void PlatformPortabilityLayer::Pre(INTC_D3D12_SetFeatureSupportCommand& command) {
  command.Skip = true;
}

void PlatformPortabilityLayer::Pre(INTC_D3D12_CreatePlacedResourceCommand& command) {
  command.Skip = true;

  if (!command.m_pHeap.Value) {
    LOG_ERROR << "PlatformPortability - pHeap is null";
    command.m_Result.Value = E_INVALIDARG;
    return;
  }

  // Get device from heap
  Microsoft::WRL::ComPtr<ID3D12Device> pDevice;
  HRESULT hr = command.m_pHeap.Value->GetDevice(IID_PPV_ARGS(&pDevice));
  if (FAILED(hr) || !pDevice) {
    LOG_ERROR << "PlatformPortability - GetDevice failed with HRESULT: 0x" << std::hex << hr
              << std::dec;
    command.m_Result.Value = FAILED(hr) ? hr : E_FAIL;
    return;
  }

  // Extract D3D12_RESOURCE_DESC from Intel extended structure
  // The first member of INTC_D3D12_RESOURCE_DESC_0001 is a pointer to D3D12_RESOURCE_DESC
  if (!command.m_pDesc.Value) {
    LOG_ERROR << "PlatformPortability - Resource descriptor is null";
    command.m_Result.Value = E_INVALIDARG;
    return;
  }

  auto** ppDesc = reinterpret_cast<D3D12_RESOURCE_DESC**>(command.m_pDesc.Value);
  auto* pDesc = *ppDesc;

  if (!pDesc) {
    LOG_ERROR << "PlatformPortability - D3D12_RESOURCE_DESC pointer is null";
    command.m_Result.Value = E_INVALIDARG;
    return;
  }

  // Validate descriptor
  if (pDesc->Dimension > D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
    LOG_ERROR << "PlatformPortability - Invalid resource dimension: " << pDesc->Dimension;
    command.m_Result.Value = E_INVALIDARG;
    return;
  }

  // Forward to standard D3D12 CreatePlacedResource
  hr = pDevice->CreatePlacedResource(
      command.m_pHeap.Value, command.m_HeapOffset.Value, pDesc,
      static_cast<D3D12_RESOURCE_STATES>(command.m_InitialState.Value),
      command.m_pOptimizedClearValue.Value, command.m_riid.Value, command.m_ppvResource.Value);

  if (FAILED(hr)) {
    LOG_ERROR << "PlatformPortability - CreatePlacedResource failed with HRESULT: 0x" << std::hex
              << hr << std::dec;
    LOG_ERROR << "  Heap: " << command.m_pHeap.Value << ", Offset: " << command.m_HeapOffset.Value;
    LOG_ERROR << "  Dimension: " << pDesc->Dimension << ", Format: " << pDesc->Format;
    LOG_ERROR << "  Width: " << pDesc->Width << ", Height: " << pDesc->Height;
    LOG_ERROR << "  Flags: 0x" << std::hex << pDesc->Flags << std::dec;
  }

  command.m_Result.Value = hr;
}

} // namespace DirectX
} // namespace gits

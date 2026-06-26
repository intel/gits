// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"

namespace gits {

class CGits;

namespace DirectX {

class PlatformPortabilityLayer : public Layer {
public:
  PlatformPortabilityLayer() : Layer("PlatformPortability") {}
  ~PlatformPortabilityLayer();

  void Pre(
      ID3D12DeviceRemovedExtendedDataSettingsSetAutoBreadcrumbsEnablementCommand& command) override;
  void Pre(ID3D12DeviceRemovedExtendedDataSettingsSetPageFaultEnablementCommand& command) override;
  void Pre(ID3D12DeviceRemovedExtendedDataSettingsSetWatsonDumpEnablementCommand& command) override;
  void Pre(ID3D12DeviceRemovedExtendedDataSettings1SetBreadcrumbContextEnablementCommand& command)
      override;
  void Pre(ID3D12DeviceRemovedExtendedDataSettings2UseMarkersOnlyAutoBreadcrumbsCommand& command)
      override;
  void Pre(ID3D12SDKConfigurationSetSDKVersionCommand& command) override;

  void Pre(D3D12GetDebugInterfaceCommand& command) override;
  void Pre(ID3D12DebugEnableDebugLayerCommand& command) override;
  void Pre(DXGIGetDebugInterface1Command& command) override;
  void Pre(IDXGIInfoQueueSetBreakOnSeverityCommand& command) override;

  void Pre(INTC_D3D12_CreateDeviceExtensionContextCommand& command) override;
  void Pre(INTC_D3D12_CreateDeviceExtensionContext1Command& command) override;
  void Pre(INTC_D3D12_SetApplicationInfoCommand& command) override;
  void Pre(INTC_D3D12_SetFeatureSupportCommand& command) override;
  void Pre(INTC_D3D12_CreatePlacedResourceCommand& command) override;
};

} // namespace DirectX
} // namespace gits

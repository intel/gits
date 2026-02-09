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

  void pre(ID3D12DeviceRemovedExtendedDataSettingsSetAutoBreadcrumbsEnablementCommand& c) override;
  void pre(ID3D12DeviceRemovedExtendedDataSettingsSetPageFaultEnablementCommand& c) override;
  void pre(ID3D12DeviceRemovedExtendedDataSettingsSetWatsonDumpEnablementCommand& c) override;
  void pre(
      ID3D12DeviceRemovedExtendedDataSettings1SetBreadcrumbContextEnablementCommand& c) override;
  void pre(
      ID3D12DeviceRemovedExtendedDataSettings2UseMarkersOnlyAutoBreadcrumbsCommand& c) override;
  void pre(ID3D12SDKConfigurationSetSDKVersionCommand& c) override;

  void pre(INTC_D3D12_CreateDeviceExtensionContextCommand& command) override;
  void pre(INTC_D3D12_CreateDeviceExtensionContext1Command& command) override;
  void pre(INTC_D3D12_SetApplicationInfoCommand& command) override;
  void pre(INTC_D3D12_SetFeatureSupportCommand& command) override;
  void pre(INTC_D3D12_CreatePlacedResourceCommand& c) override;
};

} // namespace DirectX
} // namespace gits

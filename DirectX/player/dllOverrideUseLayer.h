// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"

#include <filesystem>

namespace gits {
namespace DirectX {

class PlayerManager;

class DllOverrideUseLayer : public Layer {
public:
  DllOverrideUseLayer(PlayerManager& manager);
  ~DllOverrideUseLayer() = default;

  void pre(DllContainerMetaCommand& command) override;

  void pre(D3D12CreateDeviceCommand& command) override;
  void pre(D3D12GetDebugInterfaceCommand& command) override;
  void pre(D3D12CreateRootSignatureDeserializerCommand& command) override;
  void pre(D3D12CreateVersionedRootSignatureDeserializerCommand& command) override;
  void pre(D3D12EnableExperimentalFeaturesCommand& command) override;
  void pre(D3D12GetInterfaceCommand& command) override;
  void pre(D3D12SerializeRootSignatureCommand& command) override;
  void pre(D3D12SerializeVersionedRootSignatureCommand& command) override;
  void pre(ID3D12SDKConfiguration1CreateDeviceFactoryCommand& command) override;

  void pre(xessGetVersionCommand& command) override;
  void pre(xessD3D12CreateContextCommand& command) override;
  void pre(xellGetVersionCommand& command) override;
  void pre(xellD3D12CreateContextCommand& command) override;
  void pre(xefgSwapChainGetVersionCommand& command) override;
  void pre(xefgSwapChainD3D12CreateContextCommand& command) override;

private:
  void loadAgilitySDK();
  void loadXess();
  void loadXell();
  void loadXefg();

  PlayerManager& manager_;
  bool useAddressPinning_{};
  std::filesystem::path dllOverridesDirectory_;
  const std::string dllOverridesRelativePath_ = "D3D12\\overrides\\D3D12";
  bool agilitySDKLoaded_{};
  bool xessLoaded_{};
  bool xellLoaded_{};
  bool xefgLoaded_{};
  bool agilitySDKOverrideUsed_{};
};

} // namespace DirectX
} // namespace gits

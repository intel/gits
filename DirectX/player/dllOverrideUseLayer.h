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
#include <string>

namespace gits {
namespace DirectX {

class PlayerManager;

class DllOverrideUseLayer : public Layer {
public:
  DllOverrideUseLayer(PlayerManager& manager);
  ~DllOverrideUseLayer() = default;

  void Pre(DllContainerMetaCommand& command) override;

  void Pre(D3D12CreateDeviceCommand& command) override;
  void Pre(D3D12GetDebugInterfaceCommand& command) override;
  void Pre(D3D12CreateRootSignatureDeserializerCommand& command) override;
  void Pre(D3D12CreateVersionedRootSignatureDeserializerCommand& command) override;
  void Pre(D3D12EnableExperimentalFeaturesCommand& command) override;
  void Pre(D3D12GetInterfaceCommand& command) override;
  void Pre(D3D12SerializeRootSignatureCommand& command) override;
  void Pre(D3D12SerializeVersionedRootSignatureCommand& command) override;
  void Pre(ID3D12SDKConfiguration1CreateDeviceFactoryCommand& command) override;

  void Pre(xessGetVersionCommand& command) override;
  void Pre(xessD3D12CreateContextCommand& command) override;
  void Pre(xellGetVersionCommand& command) override;
  void Pre(xellD3D12CreateContextCommand& command) override;
  void Pre(xefgSwapChainGetVersionCommand& command) override;
  void Pre(xefgSwapChainD3D12CreateContextCommand& command) override;

private:
  void LoadAgilitySdk();
  void LoadXess();
  void LoadXell();
  void LoadXefg();

  PlayerManager& m_Manager;
  bool m_UseAddressPinning{};
  std::filesystem::path m_DllOverridesDirectory;
  const std::string m_DllOverridesRelativePath = "D3D12\\overrides\\D3D12";
  bool m_AgilitySdkLoaded{};
  bool m_XessLoaded{};
  bool m_XellLoaded{};
  bool m_XefgLoaded{};
  bool m_AgilitySdkOverrideUsed{};
};

} // namespace DirectX
} // namespace gits

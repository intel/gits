// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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

  void pre(D3D12CreateDeviceCommand& command) override;
  void pre(DllContainerMetaCommand& command) override;
  void pre(xessGetVersionCommand& command) override;
  void pre(xessD3D12CreateContextCommand& command) override;

private:
  PlayerManager& manager_;
  bool useAddressPinning_{};
  std::filesystem::path dllOverridesDirectory_ = "D3D12\\overrides\\D3D12";
  bool agilitySDKLoaded_{};
  bool xessLoaded_{};
};

} // namespace DirectX
} // namespace gits

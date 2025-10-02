// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "dllOverrideUseLayer.h"
#include "config.h"
#include "log2.h"
#include "PlayerManager.h"

namespace gits {
namespace DirectX {
DllOverrideUseLayer::DllOverrideUseLayer(PlayerManager& manager)
    : Layer("DllOverrideUse"), manager_(manager) {
  useAddressPinning_ = Configurator::Get().directx.player.addressPinning == AddressPinningMode::USE;
  dllOverridesDirectory_ =
      Configurator::Get().common.player.applicationPath.parent_path() / dllOverridesRelativePath_;
  try {
    std::filesystem::create_directories(dllOverridesDirectory_);
  } catch (const std::filesystem::filesystem_error& e) {
    LOG_ERROR << "Failed to create dll overrides directory: " << e.what();
  }
}

void DllOverrideUseLayer::pre(DllContainerMetaCommand& c) {
  {
    std::ofstream file(dllOverridesDirectory_ / c.dllName_.value, std::ios::binary);
    if (!file.is_open()) {
      LOG_ERROR << "Could not open dll override file for writing";
      return;
    }
    file.write(reinterpret_cast<const char*>(c.dllData_.value), c.dllData_.size);
    file.close();
    if (file.bad()) {
      LOG_ERROR << "Write to dll override file failed";
      return;
    }
  }

  if (Configurator::Get().directx.player.enableAgilitySDKDllAppOverride && !agilitySDKLoaded_ &&
      !useAddressPinning_ && wcscmp(c.dllName_.value, L"D3D12Core.dll") == 0) {
    LOG_INFO << "Agility SDK - Applying app override";
    if (manager_.loadAgilitySdk(dllOverridesRelativePath_)) {
      agilitySDKLoaded_ = true;
    }
  }

  if (Configurator::Get().directx.player.enableXessDllAppOverride && !xessLoaded_ &&
      wcscmp(c.dllName_.value, L"libxess.dll") == 0) {
    LOG_INFO << "Xess - Applying app override";
    if (manager_.getXessService().loadXess(dllOverridesDirectory_ / "libxess.dll")) {
      xessLoaded_ = true;
    }
  }
}

void DllOverrideUseLayer::pre(D3D12CreateDeviceCommand& c) {
  loadAgilitySDK();
}

void DllOverrideUseLayer::pre(D3D12GetDebugInterfaceCommand& c) {
  loadAgilitySDK();
}

void DllOverrideUseLayer::pre(D3D12CreateRootSignatureDeserializerCommand& c) {
  loadAgilitySDK();
}

void DllOverrideUseLayer::pre(D3D12CreateVersionedRootSignatureDeserializerCommand& c) {
  loadAgilitySDK();
}

void DllOverrideUseLayer::pre(D3D12EnableExperimentalFeaturesCommand& c) {
  loadAgilitySDK();
}

void DllOverrideUseLayer::pre(D3D12GetInterfaceCommand& c) {
  loadAgilitySDK();
}

void DllOverrideUseLayer::pre(D3D12SerializeRootSignatureCommand& c) {
  loadAgilitySDK();
}

void DllOverrideUseLayer::pre(D3D12SerializeVersionedRootSignatureCommand& c) {
  loadAgilitySDK();
}

void DllOverrideUseLayer::pre(xessGetVersionCommand& c) {
  loadXess();
}

void DllOverrideUseLayer::pre(xessD3D12CreateContextCommand& c) {
  loadXess();
}

void DllOverrideUseLayer::loadAgilitySDK() {
  if (!agilitySDKLoaded_) {
    if (manager_.loadAgilitySdk("D3D12")) {
      agilitySDKLoaded_ = true;
    }
  }
}

void DllOverrideUseLayer::loadXess() {
  if (!xessLoaded_) {
    if (manager_.getXessService().loadXess("D3D12\\libxess.dll")) {
      xessLoaded_ = true;
    }
  }
}

} // namespace DirectX
} // namespace gits

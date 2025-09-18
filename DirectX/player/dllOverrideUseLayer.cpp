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
  try {
    std::filesystem::create_directories(dllOverridesDirectory_);
  } catch (const std::filesystem::filesystem_error& e) {
    LOG_ERROR << "Failed to create dll overrides directory: " << e.what();
  }
}
void DllOverrideUseLayer::pre(D3D12CreateDeviceCommand& c) {
  if (!agilitySDKLoaded_) {
    manager_.loadAgilitySdk("D3D12");
    agilitySDKLoaded_ = true;
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

  if (Configurator::Get().directx.player.enableAgilitySDKDllAppOverride && !useAddressPinning_ &&
      wcscmp(c.dllName_.value, L"D3D12Core.dll") == 0) {
    LOG_INFO << "Agility SDK - Applying app override";
    GITS_ASSERT(!agilitySDKLoaded_);
    manager_.loadAgilitySdk(dllOverridesDirectory_);
    agilitySDKLoaded_ = true;
  }

  if (Configurator::Get().directx.player.enableXessDllAppOverride &&
      wcscmp(c.dllName_.value, L"libxess.dll") == 0) {
    LOG_INFO << "Xess - Applying app override";
    GITS_ASSERT(!xessLoaded_);
    manager_.getXessService().loadXess(dllOverridesDirectory_ / "libxess.dll");
    xessLoaded_ = true;
  }
}

void DllOverrideUseLayer::pre(xessGetVersionCommand& c) {
  if (!xessLoaded_) {
    manager_.getXessService().loadXess("D3D12\\libxess.dll");
    xessLoaded_ = true;
  }
}

void DllOverrideUseLayer::pre(xessD3D12CreateContextCommand& c) {
  if (!xessLoaded_) {
    manager_.getXessService().loadXess("D3D12\\libxess.dll");
    xessLoaded_ = true;
  }
}

} // namespace DirectX
} // namespace gits

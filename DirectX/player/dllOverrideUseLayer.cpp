// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "dllOverrideUseLayer.h"
#include "playerManager.h"
#include "log.h"
#include "configurator.h"

#include <fstream>

namespace gits {
namespace DirectX {
DllOverrideUseLayer::DllOverrideUseLayer(PlayerManager& manager)
    : Layer("DllOverrideUse"), m_Manager(manager) {
  m_UseAddressPinning =
      Configurator::Get().directx.player.addressPinning == AddressPinningMode::USE;
  m_DllOverridesDirectory =
      Configurator::Get().common.player.applicationPath.parent_path() / m_DllOverridesRelativePath;
  try {
    std::filesystem::create_directories(m_DllOverridesDirectory);
  } catch (const std::filesystem::filesystem_error& e) {
    LOG_ERROR << "Failed to create dll overrides directory: " << e.what();
  }
}

void DllOverrideUseLayer::Pre(DllContainerMetaCommand& c) {
  {
    std::ofstream file(m_DllOverridesDirectory / c.m_dllName.Value, std::ios::binary);
    if (!file.is_open()) {
      LOG_ERROR << "Could not open dll override file for writing";
      return;
    }
    file.write(reinterpret_cast<const char*>(c.m_dllData.Value), c.m_dllData.Size);
    file.close();
    if (file.bad()) {
      LOG_ERROR << "Write to dll override file failed";
      return;
    }
  }

  if (Configurator::Get().directx.player.enableAgilitySDKDllAppOverride && !m_AgilitySdkLoaded &&
      !m_UseAddressPinning && wcscmp(c.m_dllName.Value, L"D3D12Core.dll") == 0) {
    LOG_INFO << "Agility SDK - Applying app override";
    if (m_Manager.LoadAgilitySdk(m_DllOverridesRelativePath)) {
      m_AgilitySdkLoaded = true;
      m_AgilitySdkOverrideUsed = true;
    }
  }

  if (Configurator::Get().directx.player.enableXessDllAppOverride && !m_XessLoaded &&
      wcscmp(c.m_dllName.Value, L"libxess.dll") == 0) {
    LOG_INFO << "Xess - Applying app override";
    if (m_Manager.GetXessService().LoadXess(m_DllOverridesDirectory / "libxess.dll")) {
      m_XessLoaded = true;
    }
  }

  if (Configurator::Get().directx.player.enableXellDllAppOverride && !m_XellLoaded &&
      wcscmp(c.m_dllName.Value, L"libxell.dll") == 0) {
    LOG_INFO << "Xell - Applying app override";
    if (m_Manager.GetXessService().LoadXell(m_DllOverridesDirectory / "libxell.dll")) {
      m_XellLoaded = true;
    }
  }

  if (Configurator::Get().directx.player.enableXefgDllAppOverride && !m_XefgLoaded &&
      wcscmp(c.m_dllName.Value, L"libxess_fg.dll") == 0) {
    LOG_INFO << "XeSS FG - Applying app override";
    if (m_Manager.GetXessService().LoadXefg(m_DllOverridesDirectory / "libxess_fg.dll")) {
      m_XefgLoaded = true;
    }
  }
}

void DllOverrideUseLayer::Pre(D3D12CreateDeviceCommand& c) {
  LoadAgilitySdk();
}

void DllOverrideUseLayer::Pre(D3D12GetDebugInterfaceCommand& c) {
  LoadAgilitySdk();
}

void DllOverrideUseLayer::Pre(D3D12CreateRootSignatureDeserializerCommand& c) {
  LoadAgilitySdk();
}

void DllOverrideUseLayer::Pre(D3D12CreateVersionedRootSignatureDeserializerCommand& c) {
  LoadAgilitySdk();
}

void DllOverrideUseLayer::Pre(D3D12EnableExperimentalFeaturesCommand& c) {
  LoadAgilitySdk();
}

void DllOverrideUseLayer::Pre(D3D12GetInterfaceCommand& c) {
  LoadAgilitySdk();
}

void DllOverrideUseLayer::Pre(D3D12SerializeRootSignatureCommand& c) {
  LoadAgilitySdk();
}

void DllOverrideUseLayer::Pre(D3D12SerializeVersionedRootSignatureCommand& c) {
  LoadAgilitySdk();
}

void DllOverrideUseLayer::Pre(ID3D12SDKConfiguration1CreateDeviceFactoryCommand& c) {
  if (m_AgilitySdkOverrideUsed) {
    c.m_SDKPath.Value = const_cast<char*>(m_DllOverridesRelativePath.c_str());
  } else {
    c.m_SDKPath.Value = const_cast<char*>(
        "D3D12"); // This is unsafe but we need the value member to not be const to do a memcpy on it
    // Nothing should modify the value after this point so we accept this
    HMODULE d3d12CoreDll = LoadLibrary("D3D12\\D3D12Core.dll");
    if (d3d12CoreDll) {
      UINT sdkVersion = *reinterpret_cast<UINT*>(GetProcAddress(d3d12CoreDll, "D3D12SDKVersion"));
      c.m_SDKVersion.Value = sdkVersion;
      FreeLibrary(d3d12CoreDll);
    }
  }
}

void DllOverrideUseLayer::Pre(xessGetVersionCommand& c) {
  LoadXess();
}

void DllOverrideUseLayer::Pre(xessD3D12CreateContextCommand& c) {
  LoadXess();
}

void DllOverrideUseLayer::Pre(xellGetVersionCommand& c) {
  LoadXell();
}

void DllOverrideUseLayer::Pre(xellD3D12CreateContextCommand& c) {
  LoadXell();
}

void DllOverrideUseLayer::Pre(xefgSwapChainGetVersionCommand& c) {
  LoadXefg();
}

void DllOverrideUseLayer::Pre(xefgSwapChainD3D12CreateContextCommand& c) {
  LoadXefg();
}

void DllOverrideUseLayer::LoadAgilitySdk() {
  if (!m_AgilitySdkLoaded) {
    if (m_Manager.LoadAgilitySdk("D3D12")) {
      m_AgilitySdkLoaded = true;
    }
  }
}

void DllOverrideUseLayer::LoadXess() {
  if (!m_XessLoaded) {
    if (m_Manager.GetXessService().LoadXess("D3D12\\libxess.dll")) {
      m_XessLoaded = true;
    }
  }
}

void DllOverrideUseLayer::LoadXell() {
  if (!m_XellLoaded) {
    if (m_Manager.GetXessService().LoadXell("D3D12\\libxell.dll")) {
      m_XellLoaded = true;
    }
  }
}

void DllOverrideUseLayer::LoadXefg() {
  if (!m_XefgLoaded) {
    if (m_Manager.GetXessService().LoadXefg("D3D12\\libxess_fg.dll")) {
      m_XefgLoaded = true;
    }
  }
}

} // namespace DirectX
} // namespace gits

// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "intelExtensionsService.h"
#include "gits.h"
#include "configurationLib.h"
#include "to_string/toStr.h"

#include <map>
#include <setupapi.h>
#include <ntddvdeo.h>
#include <wrl.h>

namespace gits {
namespace DirectX {

IntelExtensionsService::~IntelExtensionsService() {
  if (intelExtensionLoaded_) {
    INTC_UnloadExtensionsLibrary();
  }
}

void IntelExtensionsService::loadIntelExtensions(IDXGIAdapter1* adapter) {
  // Intel Extensions DLL is part of the Intel GPU driver and needs to be loaded after the driver DLLs
  // EnumAdapters will load the driver DLLs and is necessary in order to call INTC_LoadExtensionsLibrary
  GITS_ASSERT(adapter != nullptr);

  if (intelExtensionLoaded_) {
    return;
  }

  DXGI_ADAPTER_DESC1 desc{};
  HRESULT hr = adapter->GetDesc1(&desc);
  GITS_ASSERT(hr == S_OK);

  // If the extensions cannot be loaded (e.g. not an Intel GPU) just print a warning
  hr = INTC_LoadExtensionsLibrary(false, desc.VendorId, desc.DeviceId);
  if (FAILED(hr)) {
    Log(WARN) << "Failed to load Intel Extensions (" << toStr(desc.Description) << ")";
    return;
  }
  intelExtensionLoaded_ = true;
  Log(INFO) << "Loaded Intel Extensions (" << toStr(desc.Description) << ")";
}

void IntelExtensionsService::setApplicationInfo() {

  if (!intelExtensionLoaded_ || applicationNameSet_) {
    return;
  }

  const auto parseVersion = [](std::string version) -> INTCAppInfoVersion {
    std::stringstream ss(version);
    std::string temp;
    std::vector<uint32_t> parts;
    while (std::getline(ss, temp, '.')) {
      parts.push_back(std::stoi(temp));
    }
    GITS_ASSERT(parts.size() == 3);
    return {parts[0], parts[1], parts[2]};
  };

  std::string appName;
  std::string appVersion = "0.0.0";
  std::string engineName;
  std::string engineVersion = "0.0.0";

  INTCExtensionAppInfo1 appInfo{};

  const auto& appInfoConfigOverride = Configurator::Get().directx.player.applicationInfoOverride;
  if (appInfoConfigOverride.enabled) {
    appName = appInfoConfigOverride.applicationName;

    appVersion = appInfoConfigOverride.applicationVersion;
    appInfo.ApplicationVersion = parseVersion(appVersion);

    engineName = appInfoConfigOverride.engineName;
    const std::wstring engineNameW(engineName.begin(), engineName.end());
    appInfo.pEngineName = engineNameW.c_str();

    engineVersion = appInfoConfigOverride.engineVersion;
    appInfo.EngineVersion = parseVersion(engineVersion);
  } else {
    appName = CGits::Instance().FilePlayer().GetApplicationName();
  }
  const std::wstring appNameW(appName.begin(), appName.end());
  appInfo.pApplicationName = appNameW.c_str();

  HRESULT hr = INTC_D3D12_SetApplicationInfo(&appInfo);
  if (hr != S_OK) {
    Log(ERR) << "INTC_D3D12_SetApplicationInfo failed - Application name is not set.";
  } else {
    Log(INFO) << "INTC_D3D12_SetApplicationInfo - Application: \"" << appName << "\" ("
              << appVersion << "), Engine: \"" << engineName << "\" (" << engineVersion << ")";
    applicationNameSet_ = true;
  }
}

} // namespace DirectX
} // namespace gits

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

void IntelExtensionsService::loadIntelExtensions() {
  if (intelExtensionLoaded_) {
    return;
  }

  // Intel Extensions DLL is part of the Intel GPU driver and needs to be loaded after the driver DLLs
  Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
  Microsoft::WRL::ComPtr<IDXGIFactory6> factory;
  HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));
  GITS_ASSERT(hr == S_OK);

  // EnumAdapters will load the driver DLLs and is necessary in order to call INTC_LoadExtensionsLibrary
  // This code assumes that the main adapter is the Intel GPU
  hr = factory->EnumAdapters1(0, &adapter);
  GITS_ASSERT(hr == S_OK);

  DXGI_ADAPTER_DESC1 desc{};
  hr = adapter->GetDesc1(&desc);
  GITS_ASSERT(hr == S_OK);

  std::wstring descriptionW = desc.Description;
  std::string description(descriptionW.begin(), descriptionW.end());

  // If the extensions cannot be loaded (e.g. not an Intel GPU) just print a warning
  hr = INTC_LoadExtensionsLibrary(false, desc.VendorId, desc.DeviceId);
  if (FAILED(hr)) {
    Log(WARN) << "Failed to load Intel Extensions (" << description << ")";
    return;
  }
  intelExtensionLoaded_ = true;
  Log(INFO) << "Loaded Intel Extensions (" << description << ")";
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

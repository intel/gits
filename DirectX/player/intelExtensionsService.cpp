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

namespace gits {
namespace DirectX {

IntelExtensionsService::~IntelExtensionsService() {
  if (intelExtensionLoaded_) {
    INTC_UnloadExtensionsLibrary();
  }
}

void IntelExtensionsService::loadIntelExtensions(const uint32_t& vendorID,
                                                 const uint32_t& deviceID) {
  if (intelExtensionLoaded_) {
    return;
  }

  HRESULT result = INTC_LoadExtensionsLibrary(false, vendorID, deviceID);
  if (FAILED(result)) {
    Log(WARN) << "INTC_LoadExtensionsLibrary failed.";
    return;
  }
  intelExtensionLoaded_ = true;
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
    Log(ERR) << "INTC_D3D12_SetApplicationInfo failed - application name is not set.";
  } else {
    Log(INFO) << "INTC_D3D12_SetApplicationInfo - application: \"" << appName << "\" ("
              << appVersion << "), engine: \"" << engineName << "\" (" << engineVersion << ")";
    applicationNameSet_ = true;
  }
}

} // namespace DirectX
} // namespace gits

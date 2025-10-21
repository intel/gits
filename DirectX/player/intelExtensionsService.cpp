// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "intelExtensionsService.h"
#include "gits.h"
#include "log.h"
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
    LOG_WARNING << "Failed to load Intel Extensions (" << toStr(desc.Description) << ")";
    return;
  }
  intelExtensionLoaded_ = true;
  LOG_INFO << "Loaded Intel Extensions (" << toStr(desc.Description) << ")";
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

  std::string appName = CGits::Instance().FilePlayer().GetApplicationName();
  std::string appVersion = "0.0.0";
  std::string engineName;
  std::string engineVersion = "0.0.0";

  const auto& appInfoConfigOverride = Configurator::Get().directx.player.applicationInfoOverride;
  if (appInfoConfigOverride.enabled) {
    appName = appInfoConfigOverride.applicationName;

    appVersion = appInfoConfigOverride.applicationVersion;
    appInfo_.ApplicationVersion = parseVersion(appVersion);

    engineName = appInfoConfigOverride.engineName;
    engineName_ = std::wstring(engineName.begin(), engineName.end());
    appInfo_.pEngineName = engineName_.c_str();

    engineVersion = appInfoConfigOverride.engineVersion;
    appInfo_.EngineVersion = parseVersion(engineVersion);
  }
  appName_ = std::wstring(appName.begin(), appName.end());
  appInfo_.pApplicationName = appName_.c_str();

  HRESULT hr = INTC_D3D12_SetApplicationInfo(&appInfo_);
  if (hr != S_OK) {
    LOG_ERROR << "INTC_D3D12_SetApplicationInfo failed - Application name is not set.";
  } else {
    LOG_INFO << "INTC_D3D12_SetApplicationInfo - Application: \"" << appName << "\" (" << appVersion
             << "), Engine: \"" << engineName << "\" (" << engineVersion << ")";
    applicationNameSet_ = true;
  }
}

const INTCExtensionAppInfo1& IntelExtensionsService::getAppInfo() const {
  return appInfo_;
}

} // namespace DirectX
} // namespace gits

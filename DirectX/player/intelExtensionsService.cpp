// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "intelExtensionsService.h"
#include "streamHeader.h"
#include "to_string/toStr.h"
#include "configurator.h"
#include "log.h"

#include <map>
#include <setupapi.h>
#include <ntddvdeo.h>
#include <wrl.h>

namespace gits {
namespace DirectX {

IntelExtensionsService::~IntelExtensionsService() {
  if (m_IntelExtensionLoaded) {
    INTC_UnloadExtensionsLibrary();
  }
}

void IntelExtensionsService::LoadIntelExtensions(IDXGIAdapter1* adapter) {
  // Intel Extensions DLL is part of the Intel GPU driver and needs to be loaded after the driver DLLs
  // EnumAdapters will load the driver DLLs and is necessary in order to call INTC_LoadExtensionsLibrary
  GITS_ASSERT(adapter != nullptr);

  if (m_IntelExtensionLoaded) {
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
  m_IntelExtensionLoaded = true;
  LOG_INFO << "Loaded Intel Extensions (" << toStr(desc.Description) << ")";
}

void IntelExtensionsService::SetApplicationInfo() {

  if (!m_IntelExtensionLoaded || m_ApplicationNameSet) {
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

  std::string appName = stream::StreamHeader::Get().GetApplicationName();
  std::string appVersion = "0.0.0";
  std::string engineName;
  std::string engineVersion = "0.0.0";

  const auto& appInfoConfigOverride = Configurator::Get().directx.player.applicationInfoOverride;
  if (appInfoConfigOverride.enabled) {
    appName = appInfoConfigOverride.applicationName;

    appVersion = appInfoConfigOverride.applicationVersion;
    m_AppInfo.ApplicationVersion = parseVersion(appVersion);

    engineName = appInfoConfigOverride.engineName;
    m_EngineName = std::wstring(engineName.begin(), engineName.end());
    m_AppInfo.pEngineName = m_EngineName.c_str();

    engineVersion = appInfoConfigOverride.engineVersion;
    m_AppInfo.EngineVersion = parseVersion(engineVersion);
  }
  m_AppName = std::wstring(appName.begin(), appName.end());
  m_AppInfo.pApplicationName = m_AppName.c_str();

  HRESULT hr = INTC_D3D12_SetApplicationInfo(&m_AppInfo);
  if (hr != S_OK) {
    LOG_ERROR << "INTC_D3D12_SetApplicationInfo failed - Application name is not set.";
  } else {
    LOG_INFO << "INTC_D3D12_SetApplicationInfo - Application: \"" << appName << "\" (" << appVersion
             << "), Engine: \"" << engineName << "\" (" << engineVersion << ")";
    m_ApplicationNameSet = true;
  }
}

const INTCExtensionAppInfo1& IntelExtensionsService::GetAppInfo() const {
  return m_AppInfo;
}

} // namespace DirectX
} // namespace gits

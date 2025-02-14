// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "intelExtensionsService.h"
#include "gits.h"

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
    Log(ERR) << "INTC_LoadExtensionsLibrary failed.";
    return;
  }
  intelExtensionLoaded_ = true;
}

void IntelExtensionsService::setApplicationName(const std::string& appName) {

  if (!intelExtensionLoaded_ || applicationNameSet_) {
    return;
  }

  std::wstring nameW(appName.begin(), appName.end());
  INTCExtensionAppInfo1 appInfo{};
  appInfo.pApplicationName = nameW.c_str();
  HRESULT hr = INTC_D3D12_SetApplicationInfo(&appInfo);
  if (hr != S_OK) {
    Log(ERR) << "INTC_D3D12_SetApplicationInfo failed - application name is not set.";
  } else {
    Log(INFO) << "INTC_D3D12_SetApplicationInfo - application name set to " << appName;
    applicationNameSet_ = true;
  }
}

} // namespace DirectX
} // namespace gits

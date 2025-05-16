// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "adapterService.h"
#include "to_string/toStr.h"

namespace gits {
namespace DirectX {

void AdapterService::loadAdapters() {
  const std::unordered_map<std::string, unsigned> adapterMap = {
      {"", 0}, {"intel", 0x8086}, {"amd", 0x1002}, {"nvidia", 0x10de}};

  const auto& adapterOverride = Configurator::Get().directx.player.adapterOverride;
  std::string adapterVendor = adapterOverride.vendor;
  for (char& c : adapterVendor) {
    c = std::tolower(c);
  }
  if (adapterMap.count(adapterVendor) == 0) {
    Log(WARN) << "AdapterOverride - Unknown vendor: " << adapterOverride.vendor;
    return;
  }

  Microsoft::WRL::ComPtr<IDXGIFactory6> factory;
  CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));
  GITS_ASSERT(factory);

  Log(INFO) << "Adapters:";
  unsigned vendorId = adapterMap.at(adapterVendor);
  unsigned adapterIndex = 0;
  unsigned adapterFromVendorIndex = 0;
  unsigned adapterOverrideIndex = 0;
  bool overrideFound = false;
  Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
  while (SUCCEEDED(factory->EnumAdapters1(adapterIndex, &adapter))) {
    // Set the DXGIAdapter at index 0 by default
    if (!adapter_) {
      adapter_ = adapter;
    }

    DXGI_ADAPTER_DESC1 adapterDesc{};
    HRESULT hr = adapter->GetDesc1(&adapterDesc);
    GITS_ASSERT(hr == S_OK);

    Log(INFO) << "  (" << adapterIndex << ")" << std::hex << std::setfill('0') << " VendorId = 0x"
              << std::setw(4) << adapterDesc.VendorId << " DeviceId = 0x" << std::setw(4)
              << adapterDesc.DeviceId << " Description = " << std::setw(4)
              << toStr(adapterDesc.Description);

    // Check if the DXGIAdapter is the one we want to override
    bool isMatch = vendorId == 0 && adapterIndex == adapterOverride.index;
    if (adapterDesc.VendorId == vendorId) {
      isMatch = adapterFromVendorIndex == adapterOverride.index;
      ++adapterFromVendorIndex;
    }
    if (isMatch) {
      overrideFound = true;
      adapter_ = adapter;
      adapterOverrideIndex = adapterIndex;
    }

    ++adapterIndex;
  }

  // Print out information about the overriden DXGIAdapter (if the option was set)
  if (adapterOverride.enabled) {
    if (overrideFound) {
      Log(INFO) << "AdapterOverride - Adapter override found at index " << adapterOverrideIndex;
    } else {
      Log(WARN)
          << "AdapterOverride - Adapter override not found, will use default adapter (index 0)";
    }
  }
}

bool AdapterService::isAdapterOverride() const {
  return Configurator::Get().directx.player.adapterOverride.enabled;
}

IDXGIAdapter1* AdapterService::getAdapter() const {
  return adapter_.Get();
}

void AdapterService::setCaptureAdapterLuid(unsigned key, LUID captureLuid) {
  captureLuids_[key] = captureLuid;
}

void AdapterService::setCurrentAdapterLuid(unsigned key, LUID currentLuid) {
  auto it = captureLuids_.find(key);
  GITS_ASSERT(it != captureLuids_.end());
  luidsByCaptureLuid_[it->second] = currentLuid;
  captureLuids_.erase(it);
}

LUID AdapterService::getCurrentLuid(LUID captureLuid) {
  if (luidsByCaptureLuid_.empty()) {
    return LUID{0};
  }
  auto it = luidsByCaptureLuid_.find(captureLuid);
  GITS_ASSERT(it != luidsByCaptureLuid_.end());
  return it->second;
}

} // namespace DirectX
} // namespace gits

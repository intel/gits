// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "adapterService.h"
#include "to_string/toStr.h"
#include "log.h"
#include "configurator.h"

namespace gits {
namespace DirectX {

void AdapterService::LoadAdapters() {
  const std::unordered_map<std::string, unsigned> adapterMap = {
      {"", 0}, {"intel", 0x8086}, {"amd", 0x1002}, {"nvidia", 0x10de}};

  const auto& adapterOverride = Configurator::Get().directx.player.adapterOverride;
  std::string adapterVendor = adapterOverride.vendor;
  for (char& c : adapterVendor) {
    c = std::tolower(c);
  }
  if (adapterMap.count(adapterVendor) == 0) {
    LOG_WARNING << "AdapterOverride - Unknown vendor: " << adapterOverride.vendor;
    return;
  }

  Microsoft::WRL::ComPtr<IDXGIFactory6> factory;
  CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));
  GITS_ASSERT(factory);

  LOG_INFO << "Adapters:";
  unsigned vendorId = adapterMap.at(adapterVendor);
  unsigned adapterIndex = 0;
  unsigned adapterFromVendorIndex = 0;
  unsigned adapterOverrideIndex = 0;
  bool overrideFound = false;
  Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
  while (SUCCEEDED(factory->EnumAdapters1(adapterIndex, &adapter))) {
    // Set the DXGIAdapter at index 0 by default
    if (!m_Adapter) {
      m_Adapter = adapter;
    }

    DXGI_ADAPTER_DESC1 adapterDesc{};
    HRESULT hr = adapter->GetDesc1(&adapterDesc);
    GITS_ASSERT(hr == S_OK);

    LOG_INFO << "  (" << adapterIndex << ")" << std::hex << std::setfill(L'0') << " VendorId = 0x"
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
      m_Adapter = adapter;
      adapterOverrideIndex = adapterIndex;
    }

    ++adapterIndex;
  }

  // Print out information about the overriden DXGIAdapter (if the option was set)
  if (adapterOverride.enabled) {
    if (overrideFound) {
      LOG_INFO << "AdapterOverride - Adapter override found at index " << adapterOverrideIndex;
    } else {
      LOG_WARNING
          << "AdapterOverride - Adapter override not found, will use default adapter (index 0)";
    }
  }
}

bool AdapterService::IsAdapterOverride() const {
  return Configurator::Get().directx.player.adapterOverride.enabled;
}

IDXGIAdapter1* AdapterService::GetAdapter() const {
  return m_Adapter.Get();
}

void AdapterService::SetCaptureAdapterLuid(unsigned key, LUID captureLuid) {
  m_CaptureLuids[key] = captureLuid;
}

void AdapterService::SetCurrentAdapterLuid(unsigned key, LUID currentLuid) {
  auto it = m_CaptureLuids.find(key);
  GITS_ASSERT(it != m_CaptureLuids.end());
  m_LuidsByCaptureLuid[it->second] = currentLuid;
  m_CaptureLuids.erase(it);
}

LUID AdapterService::GetCurrentLuid(LUID captureLuid) {
  if (m_LuidsByCaptureLuid.empty()) {
    return LUID{0};
  }
  auto it = m_LuidsByCaptureLuid.find(captureLuid);
  GITS_ASSERT(it != m_LuidsByCaptureLuid.end());
  return it->second;
}

} // namespace DirectX
} // namespace gits

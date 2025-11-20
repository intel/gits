// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "layer.h"
#include "log.h"

namespace gits {
namespace DirectX {

template <typename DescType>
void spoofAdapterDesc(AdapterSpoofConfig& cfg, DescType* pDesc) {
  if (!pDesc) {
    return;
  }

  auto wStrDescription = std::wstring(cfg.description.begin(), cfg.description.end());
  auto size = sizeof(WCHAR) * std::min(wStrDescription.length(), size_t(127));
  memset(pDesc->Description, 0, sizeof(pDesc->Description)); // Zero the description
  memcpy(pDesc->Description, wStrDescription.c_str(), size); // Copy the new description
  pDesc->VendorId = cfg.vendorId;
  pDesc->DeviceId = cfg.deviceId;

  std::ostringstream oss;
  oss << std::hex << std::setfill('0') << " VendorId = 0x" << std::setw(4) << cfg.vendorId
      << " DeviceId = 0x" << std::setw(4) << cfg.deviceId << " Description = " << std::setw(4)
      << cfg.description;

  LOG_INFO << "AdapterSpoof - Adapter spoofed to: " << oss.str();
}

AdapterSpoofLayer::AdapterSpoofLayer(const AdapterSpoofConfig& cfg)
    : Layer("AdapterSpoof"), cfg_(cfg) {}

void AdapterSpoofLayer::post(IDXGIAdapterGetDescCommand& c) {
  spoofAdapterDesc(cfg_, c.pDesc_.value);
}

void AdapterSpoofLayer::post(IDXGIAdapter1GetDesc1Command& c) {
  spoofAdapterDesc(cfg_, c.pDesc_.value);
}

void AdapterSpoofLayer::post(IDXGIAdapter2GetDesc2Command& c) {
  spoofAdapterDesc(cfg_, c.pDesc_.value);
}

void AdapterSpoofLayer::post(IDXGIAdapter4GetDesc3Command& c) {
  spoofAdapterDesc(cfg_, c.pDesc_.value);
}

} // namespace DirectX
} // namespace gits

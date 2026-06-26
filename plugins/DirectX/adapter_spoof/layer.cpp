// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "layer.h"
#include "log.h"

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <sstream>

namespace gits {
namespace DirectX {

template <typename DescType>
void SpoofAdapterDesc(const AdapterSpoofConfig& cfg, DescType* desc) {
  if (!desc) {
    return;
  }

  auto wStrDescription = std::wstring(cfg.Description.begin(), cfg.Description.end());
  auto copySize = sizeof(WCHAR) * std::min(wStrDescription.length(), size_t(127));
  std::memset(desc->Description, 0, sizeof(desc->Description));
  std::memcpy(desc->Description, wStrDescription.c_str(), copySize);
  desc->VendorId = cfg.VendorId;
  desc->DeviceId = cfg.DeviceId;

  std::ostringstream oss;
  oss << std::hex << std::setfill('0') << " VendorId = 0x" << std::setw(4) << cfg.VendorId
      << " DeviceId = 0x" << std::setw(4) << cfg.DeviceId << " Description = " << std::setw(4)
      << cfg.Description;

  LOG_INFO << "AdapterSpoof - Adapter spoofed to: " << oss.str();
}

AdapterSpoofLayer::AdapterSpoofLayer(const AdapterSpoofConfig& cfg)
    : Layer("AdapterSpoof"), m_Cfg(cfg) {}

void AdapterSpoofLayer::Post(IDXGIAdapterGetDescCommand& command) {
  SpoofAdapterDesc(m_Cfg, command.m_pDesc.Value);
}

void AdapterSpoofLayer::Post(IDXGIAdapter1GetDesc1Command& command) {
  SpoofAdapterDesc(m_Cfg, command.m_pDesc.Value);
}

void AdapterSpoofLayer::Post(IDXGIAdapter2GetDesc2Command& command) {
  SpoofAdapterDesc(m_Cfg, command.m_pDesc.Value);
}

void AdapterSpoofLayer::Post(IDXGIAdapter4GetDesc3Command& command) {
  SpoofAdapterDesc(m_Cfg, command.m_pDesc.Value);
}

} // namespace DirectX
} // namespace gits

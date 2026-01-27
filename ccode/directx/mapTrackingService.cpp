// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "directx/mapTrackingService.h"

namespace directx {

MapTrackingService& MapTrackingService::Get() {
  static MapTrackingService s_Instance;
  return s_Instance;
}

void MapTrackingService::MapResource(unsigned resourceKey,
                                     unsigned subresourceIndex,
                                     std::uintptr_t captureAddress,
                                     void** currentAddress) {
  if (!currentAddress) {
    return;
  }
  m_MappedData[captureAddress] = MappedInfo{*currentAddress, resourceKey};
  m_MappedDataByResource[resourceKey].insert(captureAddress);
}

void MapTrackingService::DestroyResource(unsigned resourceKey) {
  auto itAddresses = m_MappedDataByResource.find(resourceKey);
  if (itAddresses == m_MappedDataByResource.end()) {
    return;
  }
  for (std::uintptr_t address : itAddresses->second) {
    auto it = m_MappedData.find(address);
    if (it != m_MappedData.end() && it->second.resourceKey == resourceKey) {
      m_MappedData.erase(it);
    }
  }
  m_MappedDataByResource.erase(resourceKey);
}

void* MapTrackingService::GetCurrentAddress(std::uintptr_t captureAddress) {
  auto it = m_MappedData.find(captureAddress);
  if (it == m_MappedData.end()) {
    return nullptr;
  }
  return it->second.currentAddress;
}

} // namespace directx

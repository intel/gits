// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "mapTrackingService.h"
#include "log.h"

namespace gits {
namespace DirectX {

void MapTrackingService::MapResource(unsigned ResourceKey,
                                     unsigned subresourceIndex,
                                     void* captureAddress,
                                     void** currentAddress) {
  if (!currentAddress) {
    return;
  }
  m_MappedData[captureAddress] = MappedInfo{*currentAddress, ResourceKey};
}

void MapTrackingService::DestroyResource(unsigned ResourceKey) {

  auto itAddresses = m_MappedDataByResource.find(ResourceKey);
  if (itAddresses == m_MappedDataByResource.end()) {
    return;
  }
  for (void* address : itAddresses->second) {
    auto it = m_MappedData.find(address);
    GITS_ASSERT(it != m_MappedData.end());
    if (it->second.ResourceKey == ResourceKey) {
      m_MappedData.erase(it);
    }
  }
  m_MappedDataByResource.erase(ResourceKey);
}

void* MapTrackingService::GetCurrentAddress(void* captureAddress) {

  auto it = m_MappedData.find(captureAddress);
  GITS_ASSERT(it != m_MappedData.end());

  return it->second.CurrentAddress;
}

} // namespace DirectX
} // namespace gits

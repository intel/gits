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

void MapTrackingService::MapResource(unsigned resourceKey,
                                     unsigned subresourceIndex,
                                     void* captureAddress,
                                     void** currentAddress) {
  if (!currentAddress) {
    return;
  }
  m_MappedData[captureAddress] = MappedInfo{*currentAddress, resourceKey};
}

void MapTrackingService::DestroyResource(unsigned resourceKey) {

  auto itAddresses = m_MappedDataByResource.find(resourceKey);
  if (itAddresses == m_MappedDataByResource.end()) {
    return;
  }
  for (void* address : itAddresses->second) {
    auto it = m_MappedData.find(address);
    GITS_ASSERT(it != m_MappedData.end());
    if (it->second.ResourceKey == resourceKey) {
      m_MappedData.erase(it);
    }
  }
  m_MappedDataByResource.erase(resourceKey);
}

void* MapTrackingService::GetCurrentAddress(void* captureAddress) {

  auto it = m_MappedData.find(captureAddress);
  GITS_ASSERT(it != m_MappedData.end());

  return it->second.CurrentAddress;
}

} // namespace DirectX
} // namespace gits

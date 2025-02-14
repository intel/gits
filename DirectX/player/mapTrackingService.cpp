// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "mapTrackingService.h"
#include "gits.h"

namespace gits {
namespace DirectX {

void MapTrackingService::mapResource(unsigned resourceKey,
                                     unsigned subresourceIndex,
                                     void* captureAddress,
                                     void** currentAddress) {
  if (!currentAddress) {
    return;
  }
  mappedData_[captureAddress] = MappedInfo{*currentAddress, resourceKey};
}

void MapTrackingService::destroyResource(unsigned resourceKey) {

  auto itAddresses = mappedDataByResource_.find(resourceKey);
  if (itAddresses == mappedDataByResource_.end()) {
    return;
  }
  for (void* address : itAddresses->second) {
    auto it = mappedData_.find(address);
    GITS_ASSERT(it != mappedData_.end());
    if (it->second.resourceKey == resourceKey) {
      mappedData_.erase(it);
    }
  }
  mappedDataByResource_.erase(resourceKey);
}

void* MapTrackingService::getCurrentAddress(void* captureAddress) {

  auto it = mappedData_.find(captureAddress);
  GITS_ASSERT(it != mappedData_.end());

  return it->second.currentAddress;
}

} // namespace DirectX
} // namespace gits

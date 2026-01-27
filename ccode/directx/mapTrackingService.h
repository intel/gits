// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "directx/directx.h"
#include <unordered_map>
#include <unordered_set>

namespace directx {

class MapTrackingService {
public:
  static MapTrackingService& Get();

  void MapResource(unsigned resourceKey,
                   unsigned subresourceIndex,
                   std::uintptr_t captureAddress,
                   void** currentAddress);
  void DestroyResource(unsigned resourceKey);
  void* GetCurrentAddress(std::uintptr_t captureAddress);

private:
  MapTrackingService() = default;
  ~MapTrackingService() = default;

  // Prevent copying and assignment
  MapTrackingService(const MapTrackingService&) = delete;
  MapTrackingService& operator=(const MapTrackingService&) = delete;

  struct MappedInfo {
    void* currentAddress;
    unsigned resourceKey;
  };
  std::unordered_map<std::uintptr_t, MappedInfo> m_MappedData;
  std::unordered_map<unsigned, std::unordered_set<std::uintptr_t>> m_MappedDataByResource;
};

} // namespace directx

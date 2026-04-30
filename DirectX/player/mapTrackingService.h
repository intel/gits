// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace gits {
namespace DirectX {

class MapTrackingService {
public:
  void MapResource(unsigned resourceKey,
                   unsigned subresourceIndex,
                   void* captureAddress,
                   void** currentAddress);
  void DestroyResource(unsigned resourceKey);
  void* GetCurrentAddress(void* captureAddress);

private:
  struct MappedInfo {
    void* CurrentAddress{};
    unsigned ResourceKey{};
  };
  std::unordered_map<void*, MappedInfo> m_MappedData;
  std::unordered_map<unsigned, std::unordered_set<void*>> m_MappedDataByResource;
};

} // namespace DirectX
} // namespace gits

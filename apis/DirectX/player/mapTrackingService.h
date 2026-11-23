// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace gits {
namespace DirectX {

class MapTrackingService {
public:
  void MapResource(ObjectKey resourceKey,
                   unsigned subresourceIndex,
                   void* captureAddress,
                   void** currentAddress);
  void DestroyResource(ObjectKey resourceKey);
  void* GetCurrentAddress(void* captureAddress);

private:
  struct MappedInfo {
    void* CurrentAddress{};
    ObjectKey ResourceKey{};
  };
  std::unordered_map<void*, MappedInfo> m_MappedData;
  std::unordered_map<ObjectKey, std::unordered_set<void*>> m_MappedDataByResource;
};

} // namespace DirectX
} // namespace gits

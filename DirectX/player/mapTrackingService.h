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
  void mapResource(unsigned resourceKey,
                   unsigned subresourceIndex,
                   void* captureAddress,
                   void** currentAddress);
  void destroyResource(unsigned resourceKey);
  void* getCurrentAddress(void* captureAddress);

private:
  struct MappedInfo {
    void* currentAddress;
    unsigned resourceKey;
  };
  std::unordered_map<void*, MappedInfo> mappedData_;
  std::unordered_map<unsigned, std::unordered_set<void*>> mappedDataByResource_;
};

} // namespace DirectX
} // namespace gits

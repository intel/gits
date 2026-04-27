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

class StateTrackingService;

class MapStateService {
public:
  MapStateService(StateTrackingService& stateService) : m_StateService(stateService) {}
  void MapResource(unsigned resourceKey, unsigned subresourceIndex, void* captureAddress) {
    m_MappedDataBySubresource[resourceKey][subresourceIndex] = captureAddress;
  }
  void DestroyResource(unsigned resourceKey) {
    m_MappedDataBySubresource.erase(resourceKey);
  }
  void RestoreMapState();

private:
  StateTrackingService& m_StateService;
  std::unordered_map<unsigned, std::unordered_map<unsigned, void*>> m_MappedDataBySubresource;
  std::unordered_set<unsigned> m_RestoredResources;
};

} // namespace DirectX
} // namespace gits

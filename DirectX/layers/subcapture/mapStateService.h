// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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
  MapStateService(StateTrackingService& stateService) : stateService_(stateService) {}
  void mapResource(unsigned resourceKey, unsigned subresourceIndex, void* captureAddress) {
    mappedDataBySubresource_[resourceKey][subresourceIndex] = captureAddress;
  }
  void destroyResource(unsigned resourceKey) {
    mappedDataBySubresource_.erase(resourceKey);
  }
  void restoreMapState();

private:
  StateTrackingService& stateService_;
  std::unordered_map<unsigned, std::unordered_map<unsigned, void*>> mappedDataBySubresource_;
  std::unordered_set<unsigned> restoredResources_;
};

} // namespace DirectX
} // namespace gits

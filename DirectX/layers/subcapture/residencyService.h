// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <unordered_map>

namespace gits {
namespace DirectX {

class StateTrackingService;

class ResidencyService {
public:
  ResidencyService(StateTrackingService& stateService) : stateService_(stateService) {}
  void createNotResident(const unsigned key, const unsigned deviceKey);
  void makeResident(const std::vector<unsigned>& keys, const unsigned deviceKey);
  void evict(const std::vector<unsigned>& keys, const unsigned deviceKey);
  void destroyObject(const unsigned key);
  void restoreResidency();

private:
  struct ResidencyInfo {
    unsigned residencyCount{};
    unsigned deviceKey{};
    bool createdNotResident{};
  };
  StateTrackingService& stateService_;
  std::unordered_map<unsigned, ResidencyInfo> residency_;
};

} // namespace DirectX
} // namespace gits

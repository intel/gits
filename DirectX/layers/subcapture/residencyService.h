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

namespace gits {
namespace DirectX {

class StateTrackingService;

class ResidencyService {
public:
  ResidencyService(StateTrackingService& stateService) : m_StateService(stateService) {}
  void CreateNotResident(GITSKey key, GITSKey deviceKey);
  void MakeResident(const std::vector<GITSKey>& keys, GITSKey deviceKey);
  void Evict(const std::vector<GITSKey>& keys, GITSKey deviceKey);
  void DestroyObject(GITSKey key);
  void RestoreResidency();

private:
  struct ResidencyInfo {
    unsigned ResidencyCount{};
    GITSKey DeviceKey{};
    bool CreatedNotResident{};
  };
  StateTrackingService& m_StateService;
  std::unordered_map<GITSKey, ResidencyInfo> m_Residency;
};

} // namespace DirectX
} // namespace gits

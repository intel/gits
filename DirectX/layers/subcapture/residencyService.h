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
  ResidencyService(StateTrackingService& stateService) : m_StateService(stateService) {}
  void CreateNotResident(unsigned key, unsigned deviceKey);
  void MakeResident(const std::vector<unsigned>& keys, unsigned deviceKey);
  void Evict(const std::vector<unsigned>& keys, unsigned deviceKey);
  void DestroyObject(unsigned key);
  void RestoreResidency();

private:
  struct ResidencyInfo {
    unsigned ResidencyCount{};
    unsigned DeviceKey{};
    bool CreatedNotResident{};
  };
  StateTrackingService& m_StateService;
  std::unordered_map<unsigned, ResidencyInfo> m_Residency;
};

} // namespace DirectX
} // namespace gits

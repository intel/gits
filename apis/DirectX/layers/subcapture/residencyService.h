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
  void CreateNotResident(ObjectKey key, ObjectKey deviceKey);
  void MakeResident(const std::vector<ObjectKey>& keys, ObjectKey deviceKey);
  void Evict(const std::vector<ObjectKey>& keys, ObjectKey deviceKey);
  void DestroyObject(ObjectKey key);
  void RestoreResidency();

private:
  struct ResidencyInfo {
    unsigned ResidencyCount{};
    ObjectKey DeviceKey{};
    bool CreatedNotResident{};
  };
  StateTrackingService& m_StateService;
  std::unordered_map<ObjectKey, ResidencyInfo> m_Residency;
};

} // namespace DirectX
} // namespace gits

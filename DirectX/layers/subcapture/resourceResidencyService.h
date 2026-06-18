// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <unordered_set>

namespace gits {
namespace DirectX {

class StateTrackingService;

class ResourceResidencyService {
public:
  ResourceResidencyService(StateTrackingService& stateService, unsigned deviceKey)
      : m_StateService(stateService), m_DeviceKey(deviceKey) {}
  void AddResource(unsigned resourceKey);
  void RecordMakeResident();
  void RecordEvict();

private:
  StateTrackingService& m_StateService;
  unsigned m_DeviceKey{};
  std::unordered_set<unsigned> m_ResidencyKeys;
};

} // namespace DirectX
} // namespace gits

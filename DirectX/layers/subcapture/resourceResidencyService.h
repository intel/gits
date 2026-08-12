// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"

#include <unordered_set>

namespace gits {
namespace DirectX {

class StateTrackingService;

class ResourceResidencyService {
public:
  ResourceResidencyService(StateTrackingService& stateService, GITSKey deviceKey)
      : m_StateService(stateService), m_DeviceKey(deviceKey) {}
  void AddResource(GITSKey resourceKey);
  void RecordMakeResident();
  void RecordEvict();

private:
  StateTrackingService& m_StateService;
  GITSKey m_DeviceKey{};
  std::unordered_set<GITSKey> m_ResidencyKeys;
};

} // namespace DirectX
} // namespace gits

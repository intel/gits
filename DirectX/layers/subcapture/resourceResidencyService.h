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
#include <vector>

namespace gits {
namespace DirectX {

class StateTrackingService;

class ResourceResidencyService {
public:
  ResourceResidencyService(StateTrackingService& stateService, ObjectKey deviceKey)
      : m_StateService(stateService), m_DeviceKey(deviceKey) {}
  void AddResource(ObjectKey resourceKey);
  void AddResources(const std::vector<ObjectKey>& resourceKeys);
  void RecordMakeResident();
  void RecordEvict();

private:
  StateTrackingService& m_StateService;
  ObjectKey m_DeviceKey{};
  std::unordered_set<ObjectKey> m_ResidencyKeys;
};

} // namespace DirectX
} // namespace gits

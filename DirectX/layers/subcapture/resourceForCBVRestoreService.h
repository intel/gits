// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "command.h"

#include <memory>
#include <unordered_map>
#include <set>

namespace gits {
namespace DirectX {

class StateTrackingService;

class ResourceForCBVRestoreService {
public:
  ResourceForCBVRestoreService(StateTrackingService& stateService) : m_StateService(stateService) {}
  void AddResourceCreationCommand(unsigned resourceKey, unsigned heapKey, Command* creationCommand);
  bool RestoreResourceObject(unsigned resourceKey);
  void ReleaseResources();
  bool ResourceRestored(unsigned resourceKey);

private:
  struct ResourceForCBVRestoreInfo {
    std::unique_ptr<Command> CreationCommand;
    unsigned HeapKey{};
  };

  StateTrackingService& m_StateService;
  std::unordered_map<unsigned, ResourceForCBVRestoreInfo> m_ResourceCreationInfo;
  std::set<unsigned> m_RestoredResourceObjects;
};

} // namespace DirectX
} // namespace gits

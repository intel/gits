// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"

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
  void AddResourceCreationCommand(GITSKey resourceKey, GITSKey heapKey, Command* creationCommand);
  bool RestoreResourceObject(GITSKey resourceKey);
  void ReleaseResources();
  bool ResourceRestored(GITSKey resourceKey);

private:
  struct ResourceForCBVRestoreInfo {
    std::unique_ptr<Command> CreationCommand;
    GITSKey HeapKey{};
  };

  StateTrackingService& m_StateService;
  std::unordered_map<GITSKey, ResourceForCBVRestoreInfo> m_ResourceCreationInfo;
  std::set<GITSKey> m_RestoredResourceObjects;
};

} // namespace DirectX
} // namespace gits

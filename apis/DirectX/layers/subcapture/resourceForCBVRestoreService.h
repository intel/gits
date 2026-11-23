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
  void AddResourceCreationCommand(ObjectKey resourceKey,
                                  ObjectKey heapKey,
                                  Command* creationCommand);
  bool RestoreResourceObject(ObjectKey resourceKey);
  void ReleaseResources();
  bool ResourceRestored(ObjectKey resourceKey);

private:
  struct ResourceForCBVRestoreInfo {
    std::unique_ptr<Command> CreationCommand;
    ObjectKey HeapKey{};
  };

  StateTrackingService& m_StateService;
  std::unordered_map<ObjectKey, ResourceForCBVRestoreInfo> m_ResourceCreationInfo;
  std::set<ObjectKey> m_RestoredResourceObjects;
};

} // namespace DirectX
} // namespace gits

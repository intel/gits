// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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
  ResourceForCBVRestoreService(StateTrackingService& stateService) : stateService_(stateService) {}
  void addResourceCreationCommand(unsigned resourceKey, unsigned heapKey, Command* command);
  bool restoreResourceObject(unsigned resourceKey);
  void releaseResources();
  bool resourceRestored(unsigned key);

private:
  struct ResourceForCBVRestoreInfo {
    std::unique_ptr<Command> creationCommand;
    unsigned heapKey{};
  };

  StateTrackingService& stateService_;
  std::unordered_map<unsigned, ResourceForCBVRestoreInfo> resourceCreationInfo_;
  std::set<unsigned> restoredResourceObjects_;
};

} // namespace DirectX
} // namespace gits

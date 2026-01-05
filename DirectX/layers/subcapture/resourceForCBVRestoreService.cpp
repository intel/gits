// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "resourceForCBVRestoreService.h"
#include "stateTrackingService.h"
#include "commandWritersFactory.h"
#include "commandWritersCustom.h"

namespace gits {
namespace DirectX {

void ResourceForCBVRestoreService::addResourceCreationCommand(unsigned resourceKey,
                                                              unsigned heapKey,
                                                              Command* command) {
  auto& info = resourceCreationInfo_[resourceKey];
  info.heapKey = heapKey;
  info.creationCommand.reset(command);
}

bool ResourceForCBVRestoreService::restoreResourceObject(unsigned resourceKey) {
  if (restoredResourceObjects_.find(resourceKey) != restoredResourceObjects_.end()) {
    return true;
  }

  auto infoIt = resourceCreationInfo_.find(resourceKey);
  if (infoIt == resourceCreationInfo_.end()) {
    return false;
  }

  ObjectState* heapState = stateService_.getState(infoIt->second.heapKey);
  if (!heapState || !heapState->restored) {
    return false;
  }

  stateService_.getRecorder().record(createCommandWriter(infoIt->second.creationCommand.get()));
  restoredResourceObjects_.insert(infoIt->first);
  resourceCreationInfo_.erase(infoIt);

  return true;
}

void ResourceForCBVRestoreService::releaseResources() {
  for (unsigned key : restoredResourceObjects_) {
    IUnknownReleaseCommand c;
    c.key = stateService_.getUniqueCommandKey();
    c.object_.key = key;
    stateService_.getRecorder().record(new IUnknownReleaseWriter(c));
  }
}

bool ResourceForCBVRestoreService::resourceRestored(unsigned key) {
  auto it = restoredResourceObjects_.find(key);
  return it != restoredResourceObjects_.end();
}

} // namespace DirectX
} // namespace gits

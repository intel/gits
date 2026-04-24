// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "resourceForCBVRestoreService.h"
#include "stateTrackingService.h"
#include "commandSerializersFactory.h"
#include "commandSerializersCustom.h"

namespace gits {
namespace DirectX {

void ResourceForCBVRestoreService::AddResourceCreationCommand(unsigned resourceKey,
                                                              unsigned heapKey,
                                                              Command* creationCommand) {
  auto& info = m_ResourceCreationInfo[resourceKey];
  info.HeapKey = heapKey;
  info.CreationCommand.reset(creationCommand);
}

bool ResourceForCBVRestoreService::RestoreResourceObject(unsigned resourceKey) {
  if (m_RestoredResourceObjects.find(resourceKey) != m_RestoredResourceObjects.end()) {
    return true;
  }

  auto infoIt = m_ResourceCreationInfo.find(resourceKey);
  if (infoIt == m_ResourceCreationInfo.end()) {
    return false;
  }

  ObjectState* heapState = m_StateService.GetState(infoIt->second.HeapKey);
  if (!heapState || !heapState->Restored) {
    return false;
  }

  m_StateService.GetRecorder().Record(
      *createCommandSerializer(infoIt->second.CreationCommand.get()));
  m_RestoredResourceObjects.insert(infoIt->first);
  m_ResourceCreationInfo.erase(infoIt);

  return true;
}

void ResourceForCBVRestoreService::ReleaseResources() {
  for (unsigned key : m_RestoredResourceObjects) {
    IUnknownReleaseCommand c;
    c.Key = m_StateService.GetUniqueCommandKey();
    c.m_Object.Key = key;
    m_StateService.GetRecorder().Record(IUnknownReleaseSerializer(c));
  }
}

bool ResourceForCBVRestoreService::ResourceRestored(unsigned resourceKey) {
  auto it = m_RestoredResourceObjects.find(resourceKey);
  return it != m_RestoredResourceObjects.end();
}

} // namespace DirectX
} // namespace gits

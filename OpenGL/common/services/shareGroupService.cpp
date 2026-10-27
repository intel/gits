// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "shareGroupService.h"

namespace gits::opengl {

ShareGroupService& ShareGroupService::Get() {
  // Meyer's singleton - same rationale as ContextHandleMapService::Get()/
  // ObjectNameMapService::Get(): no MessageBus/OrderingRecorder ownership here, so none of
  // CaptureManager's static-destruction-order hazards apply.
  static ShareGroupService instance;
  return instance;
}

uint64_t ShareGroupService::GroupForLocked(const void* context) {
  if (!context) {
    return 0;
  }
  auto it = m_ContextToGroup.find(context);
  if (it != m_ContextToGroup.end()) {
    return it->second;
  }
  const uint64_t group = m_NextGroup++;
  m_ContextToGroup[context] = group;
  return group;
}

void ShareGroupService::RegisterNewContext(const void* newContext, const void* shareContext) {
  if (!newContext) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  const uint64_t group = shareContext ? GroupForLocked(shareContext) : m_NextGroup++;
  m_ContextToGroup[newContext] = group;
  m_CapturedContexts.insert(newContext);
}

void ShareGroupService::MergeGroups(const void* hglrc1, const void* hglrc2) {
  if (!hglrc1 || !hglrc2) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  const uint64_t group1 = GroupForLocked(hglrc1);
  const uint64_t group2 = GroupForLocked(hglrc2);
  if (group1 == group2) {
    return;
  }
  // Repoint every context currently in hglrc2's group onto hglrc1's - see this class's header
  // comment for why this does not retroactively reconcile any objects already registered
  // independently under either group before the merge. The context count here is always small
  // (one entry per context the process has ever created), so a linear scan is fine - this is a
  // low-frequency path (one call per wglShareLists call, not per GL call).
  for (auto& [context, group] : m_ContextToGroup) {
    if (group == group2) {
      group = group1;
    }
  }
}

uint64_t ShareGroupService::GroupFor(const void* context) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  return GroupForLocked(context);
}

bool ShareGroupService::IsCreationCaptured(const void* context) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  return m_CapturedContexts.count(context) != 0;
}

} // namespace gits::opengl

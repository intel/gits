// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "contextHandleMapService.h"

#include "log.h"

namespace gits::opengl2 {

ContextHandleMapService& ContextHandleMapService::Get() {
  // Meyer's singleton: unlike CaptureManager, this service does not itself publish to
  // MessageBus or own an OrderingRecorder, so it has none of the static-destruction-order
  // hazards that forced CaptureManager into a heap-allocated, never-destroyed pattern (see
  // recorder/captureManager.cpp). The player has no equivalent lifecycle hazard either.
  static ContextHandleMapService instance;
  return instance;
}

void ContextHandleMapService::Register(HGLRC handle, GITSKey key) {
  if (!handle) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_HandleToKey[handle] = key;
  m_WarnedMissingHandles.erase(handle);
}

GITSKey ContextHandleMapService::GetKey(HGLRC handle) {
  if (!handle) {
    return 0;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_HandleToKey.find(handle);
  if (it != m_HandleToKey.end()) {
    return it->second;
  }
  if (m_WarnedMissingHandles.insert(handle).second) {
    LOG_WARNING << "ContextHandleMapService: HGLRC " << handle
                << " has no registered key (never created, or created before recording "
                   "started)";
  }
  return 0;
}

void ContextHandleMapService::Erase(HGLRC handle) {
  if (!handle) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_HandleToKey.erase(handle);
  m_WarnedMissingHandles.erase(handle);
}

void ContextHandleMapService::SetHandle(GITSKey key, HGLRC handle) {
  if (key == 0 || !handle) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_KeyToHandle[key] = handle;
}

HGLRC ContextHandleMapService::GetHandle(GITSKey key) {
  if (key == 0) {
    return nullptr;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_KeyToHandle.find(key);
  if (it != m_KeyToHandle.end()) {
    return it->second;
  }
  if (m_WarnedMissingKeys.insert(key).second) {
    LOG_WARNING << "ContextHandleMapService: GITSKey " << key
                << " has no registered HGLRC (never created during this replay, or the "
                   "creating command has not been replayed yet)";
  }
  return nullptr;
}

} // namespace gits::opengl2

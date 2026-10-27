// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "handleMapService.h"

#include "log.h"

namespace gits::opengl {

template <typename HandleT>
HandleMapService<HandleT>& HandleMapService<HandleT>::Get() {
  // Meyer's singleton: unlike CaptureManager, this service does not itself publish to
  // MessageBus or own an OrderingRecorder, so it has none of the static-destruction-order
  // hazards that forced CaptureManager into a heap-allocated, never-destroyed pattern (see
  // recorder/captureManager.cpp). The player has no equivalent lifecycle hazard either. One
  // static instance per HandleT instantiation, exactly like the two pre-merge classes each had
  // their own single static instance.
  static HandleMapService instance;
  return instance;
}

template <typename HandleT>
void HandleMapService<HandleT>::Register(HandleT handle, GITSKey key, uint64_t shareGroup) {
  if (!handle) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  const HandleKey handleKey{shareGroup, handle};
  m_HandleToKey[handleKey] = key;
  m_WarnedMissingHandles.erase(handleKey);
}

template <typename HandleT>
GITSKey HandleMapService<HandleT>::GetKey(HandleT handle, uint64_t shareGroup) {
  if (!handle) {
    return 0;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  const HandleKey handleKey{shareGroup, handle};
  auto it = m_HandleToKey.find(handleKey);
  if (it != m_HandleToKey.end()) {
    return it->second;
  }
  if constexpr (kWarnMissingKey<HandleT>) {
    if (m_WarnedMissingHandles.insert(handleKey).second) {
      LOG_WARNING << HandleTypeInfo<HandleT>::kServiceName << ": "
                  << HandleTypeInfo<HandleT>::kTypeName << " " << handle
                  << " has no registered key in share group " << shareGroup
                  << " (never created, or created before recording started)";
    }
  }
  return 0;
}

template <typename HandleT>
void HandleMapService<HandleT>::Erase(HandleT handle, uint64_t shareGroup) {
  if (!handle) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  const HandleKey handleKey{shareGroup, handle};
  m_HandleToKey.erase(handleKey);
  m_WarnedMissingHandles.erase(handleKey);
}

template <typename HandleT>
void HandleMapService<HandleT>::SetHandle(GITSKey key, HandleT handle) {
  if (key == 0 || !handle) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_KeyToHandle[key] = handle;
}

template <typename HandleT>
HandleT HandleMapService<HandleT>::GetHandle(GITSKey key) {
  if (key == 0) {
    return nullptr;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_KeyToHandle.find(key);
  if (it != m_KeyToHandle.end()) {
    return it->second;
  }
  if (m_WarnedMissingKeys.insert(key).second) {
    LOG_WARNING << HandleTypeInfo<HandleT>::kServiceName << ": GITSKey " << key
                << " has no registered " << HandleTypeInfo<HandleT>::kTypeName
                << " (never created during this replay, or the creating command has not been "
                   "replayed yet)";
  }
  return nullptr;
}

// Explicit instantiations for every mapped-handle type today (see codegen/handle_lifecycle.py's
// MAPPED_HANDLE_LIFECYCLES) - matches the two `extern template class` declarations in
// handleMapService.h. A third mapped-handle type needs a third line here.
template class HandleMapService<HGLRC>;
template class HandleMapService<GLsync>;
template class HandleMapService<HPBUFFERARB>;
template class HandleMapService<HPBUFFEREXT>;
template class HandleMapService<GLXContext>;
template class HandleMapService<XVisualInfo*>;

} // namespace gits::opengl

// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "handleMapService.h"

namespace gits {
namespace vulkan {

void HandleMapService::SetKey(uint64_t handle, GITSKey key) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_HandleToKey[handle] = key;
}

GITSKey HandleMapService::GetKey(uint64_t handle) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_HandleToKey.find(handle);
  GITS_ASSERT(it != m_HandleToKey.end());
  return it->second;
}

bool HandleMapService::HasKey(uint64_t handle) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  return m_HandleToKey.find(handle) != m_HandleToKey.end();
}

void HandleMapService::SetHandle(GITSKey key, uint64_t handle) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_KeyToHandle[key] = handle;
}

uint64_t HandleMapService::GetHandle(GITSKey key) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_KeyToHandle.find(key);
  GITS_ASSERT(it != m_KeyToHandle.end());
  return it->second;
}

} // namespace vulkan
} // namespace gits

// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "handleMapService.h"

#include "log.h"

namespace gits {
namespace vulkan {

void HandleMapService::SetKey(uint64_t handle, GITSKey key) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_HandleToKey[handle] = key;
  // Maintain audit trail so GetKey / GetKeyLenient can classify misses as
  // use-after-destroy vs never-registered.
  m_EverRegistered.insert(handle);
  m_RemovedHandles.erase(handle);
}

GITSKey HandleMapService::GetKey(uint64_t handle) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_HandleToKey.find(handle);
  if (it == m_HandleToKey.end()) {
    // GetKey is used at sites that are supposed to hold a locally-guaranteed
    // invariant (output-handle path, HasKey-then-GetKey guarded lookups, ...).
    // A miss here is therefore a real bug; classify it before asserting so
    // the crash log says what kind.  Codegen-generated CollectHandleKeys must
    // call GetKeyLenient instead (see header).
    const bool wasRegistered = m_EverRegistered.count(handle) != 0;
    auto removedIt = m_RemovedHandles.find(handle);
    if (removedIt != m_RemovedHandles.end()) {
      LOG_ERROR << "HandleMapService::GetKey: handle=0x" << std::hex << handle << std::dec
                << " missing -- previously REMOVED (last key=" << removedIt->second
                << "). Use-after-destroy at a strict GetKey site.";
    } else if (wasRegistered) {
      LOG_ERROR << "HandleMapService::GetKey: handle=0x" << std::hex << handle << std::dec
                << " missing -- was registered earlier but is gone without a "
                   "RemoveHandle call (unexpected -- map mutated elsewhere?).";
    } else {
      LOG_ERROR << "HandleMapService::GetKey: handle=0x" << std::hex << handle << std::dec
                << " missing -- NEVER registered. The vkCreate* for this handle "
                   "was not intercepted (untracked extension path or garbage value).";
    }
  }
  GITS_ASSERT(it != m_HandleToKey.end());
  return it->second;
}

GITSKey HandleMapService::GetKeyLenient(uint64_t handle) {
  if (!handle) {
    return 0;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_HandleToKey.find(handle);
  if (it != m_HandleToKey.end()) {
    return it->second;
  }

  // Miss: warn once per unique handle and return 0.  Treating the slot as
  // VK_NULL_HANDLE matches what the driver does for fields it ignores (e.g.
  // renderPass on a graphics pipeline library link pipeline).  If the slot is
  // actually load-bearing the downstream API call will fail in both original
  // capture and replay, with the warning logged here pointing the way.
  if (m_LenientWarned.insert(handle).second) {
    auto removedIt = m_RemovedHandles.find(handle);
    if (removedIt != m_RemovedHandles.end()) {
      LOG_WARNING << "HandleMapService::GetKeyLenient: handle=0x" << std::hex << handle << std::dec
                  << " was previously destroyed (last key=" << removedIt->second
                  << "). Recording as VK_NULL_HANDLE. Expected for fields the driver "
                     "ignores (e.g. renderPass on GPL link pipelines); investigate if "
                     "the field is supposed to be consumed.";
    } else if (m_EverRegistered.count(handle) != 0) {
      LOG_WARNING << "HandleMapService::GetKeyLenient: handle=0x" << std::hex << handle << std::dec
                  << " was registered earlier but is no longer mapped (no "
                     "RemoveHandle on record). Recording as VK_NULL_HANDLE.";
    } else {
      LOG_WARNING << "HandleMapService::GetKeyLenient: handle=0x" << std::hex << handle << std::dec
                  << " was never registered. Recording as VK_NULL_HANDLE. "
                     "If this field is actually consumed, the vkCreate* was missed "
                     "(untracked extension path) or the field holds garbage.";
    }
  }
  return 0;
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

uint64_t HandleMapService::TryGetHandle(GITSKey key) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_KeyToHandle.find(key);
  return it != m_KeyToHandle.end() ? it->second : 0;
}

GITSKey HandleMapService::TryGetKey(uint64_t handle) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_HandleToKey.find(handle);
  return it != m_HandleToKey.end() ? it->second : 0;
}

void HandleMapService::RemoveHandle(uint64_t handle) {
  if (!handle) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_HandleToKey.find(handle);
  if (it == m_HandleToKey.end()) {
    return;
  }
  const GITSKey removedKey = it->second;
  // Remember the handle (and the key it had) so a later GetKey / GetKeyLenient
  // miss can report a precise use-after-destroy.  No per-call log spam.
  m_RemovedHandles[handle] = removedKey;
  // Remove the reverse mapping only if it still points at this handle.
  // SetHandle / SetKey are populated independently and the player side may
  // register a key->handle entry that should not be wiped just because the
  // capture side dropped the forward mapping for an unrelated handle that
  // happens to share the same key value.
  auto reverseIt = m_KeyToHandle.find(removedKey);
  if (reverseIt != m_KeyToHandle.end() && reverseIt->second == handle) {
    m_KeyToHandle.erase(reverseIt);
  }
  m_HandleToKey.erase(it);
}

} // namespace vulkan
} // namespace gits

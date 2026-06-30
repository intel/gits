// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "arguments.h"
#include "tools.h"

#include <unordered_map>
#include <unordered_set>
#include <mutex>

namespace gits {
namespace vulkan {

class HandleMapService : public gits::noncopyable {
public:
  static HandleMapService& Get() {
    static HandleMapService instance;
    return instance;
  }

  void SetKey(uint64_t handle, GITSKey key);
  GITSKey GetKey(uint64_t handle);
  // Lenient variant used by codegen-generated CollectHandleKeys /
  // CollectPNextHandleKeys walks of VkXxxCreateInfo handle members.  Returns
  // the key for handle, or 0 (which the player translates to VK_NULL_HANDLE)
  // when the handle is not registered.  Emits a single LOG_WARNING per unique
  // handle classifying the miss as use-after-destroy or never-registered.
  //
  // Some struct fields are conditionally consumed by the driver depending on
  // sibling flags / pNext chain (e.g. VkGraphicsPipelineCreateInfo::renderPass
  // is ignored for graphics pipeline library link pipelines).  Applications
  // commonly leave these fields set to a stale value the driver never derefs;
  // we must not assert in that case.  Misses for fields the driver actually
  // consumes will surface as pipeline-creation failures (in original capture
  // and replay alike) plus the warning logged here.
  GITSKey GetKeyLenient(uint64_t handle);
  bool HasKey(uint64_t handle);
  void SetHandle(GITSKey key, uint64_t handle);
  uint64_t GetHandle(GITSKey key);
  // Returns the player-side handle for key, or 0 if the key is not mapped.
  // Use instead of GetHandle when the object may not have been restored
  // (e.g. a resource destroyed before the subcapture range).
  uint64_t TryGetHandle(GITSKey key);
  // Returns the key for handle, or 0 if the handle is not mapped.
  // Use instead of GetKey when the handle may not be registered.
  GITSKey TryGetKey(uint64_t handle);
  // Erase a handle's mapping in both directions.  Must be invoked from every
  // vkDestroy* / vkFree* wrapper so that a subsequent vkCreate* call which
  // happens to be returned the same VkXxx handle by the driver does NOT inherit
  // the destroyed object's GITSKey via UpdateOutputHandle's "reuse existing
  // key" path -- that key collision silently overwrites m_States entries in
  // the subcapture state tracker and causes restored linked pipelines to be
  // bound against the wrong dependency version (see comments in
  // handleArgumentUpdaters.h::UpdateOutputHandle).  No-op for an unknown
  // handle (idempotent / safe on validation errors).
  void RemoveHandle(uint64_t handle);

private:
  HandleMapService() = default;
  ~HandleMapService() = default;

  std::mutex m_Mutex;
  std::unordered_map<uint64_t, GITSKey> m_HandleToKey;
  std::unordered_map<GITSKey, uint64_t> m_KeyToHandle;

  // Audit trail used by GetKey / GetKeyLenient to classify map misses
  // (use-after-destroy vs never-registered) in warnings and assertion
  // messages.  Cost is bounded by the number of distinct Vulkan objects
  // created during the run.
  std::unordered_set<uint64_t> m_EverRegistered;
  // Handles that were explicitly removed via RemoveHandle, mapped to the key
  // they had at the time of removal, so we can pinpoint the destroy event.
  std::unordered_map<uint64_t, GITSKey> m_RemovedHandles;
  // Dedup set for GetKeyLenient warnings: at most one warning per unique
  // handle for the lifetime of the process.
  std::unordered_set<uint64_t> m_LenientWarned;
};

} // namespace vulkan
} // namespace gits

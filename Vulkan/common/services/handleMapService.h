// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "arguments.h"
#include "handleTypeIndexAuto.h"
#include "tools.h"

#include <array>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <mutex>

namespace gits {
namespace vulkan {

// Maps between GITSKeys (stream-stable object ids) and live Vulkan handles.
//
// The recorder direction (handle -> key) is bucketed per Vulkan object type.
// The Vulkan spec does NOT guarantee that non-dispatchable handle *values* are
// unique across object types: a driver may legitimately hand out the same
// numeric value for, say, a VkBuffer and a VkSampler that are alive at the same
// time.  A single global handle->key map would then let one object's
// registration clobber another's.  Keying lookups by (object-type bucket,
// handle value) removes that collision.  The bucket index is a compile-time
// constant per handle type (HandleTypeIndex<T>()), so there is no per-call type
// dispatch and lookups stay O(1).
//
// The player direction (key -> handle) stays a single map because GITSKeys are
// globally unique by construction.
class HandleMapService : public gits::noncopyable {
public:
  static HandleMapService& Get() {
    static HandleMapService instance;
    return instance;
  }

  // ---- Recorder direction: handle -> key, bucketed per handle type T. ----

  template <typename T>
  void SetKey(T handle, GITSKey key) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    SetKeyLocked(HandleTypeIndex<T>(), reinterpret_cast<uint64_t>(handle), key);
  }

  template <typename T>
  GITSKey GetKey(T handle) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return GetKeyLocked(HandleTypeIndex<T>(), reinterpret_cast<uint64_t>(handle));
  }

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
  template <typename T>
  GITSKey GetKeyLenient(T handle) {
    if (!handle) {
      return 0;
    }
    std::lock_guard<std::mutex> lock(m_Mutex);
    return GetKeyLenientLocked(HandleTypeIndex<T>(), reinterpret_cast<uint64_t>(handle));
  }

  // Overload for objecttype-tagged uint64 handles (e.g.
  // VkDebugUtilsObjectNameInfoEXT::objectHandle) whose concrete type is not
  // known statically at the call site.  All buckets are searched.  Only rare
  // debug-label/tag paths use this, so the linear scan is irrelevant to
  // performance; being informational only, a cross-type value collision at
  // worst attaches a debug name to the wrong object.
  GITSKey GetKeyLenient(uint64_t handle);

  template <typename T>
  bool HasKey(T handle) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    const auto& bucket = m_ByType[HandleTypeIndex<T>()];
    return bucket.HandleToKey.find(reinterpret_cast<uint64_t>(handle)) != bucket.HandleToKey.end();
  }

  // Returns the key for handle, or 0 if the handle is not mapped.
  // Use instead of GetKey when the handle may not be registered.
  template <typename T>
  GITSKey TryGetKey(T handle) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    const auto& bucket = m_ByType[HandleTypeIndex<T>()];
    auto it = bucket.HandleToKey.find(reinterpret_cast<uint64_t>(handle));
    return it != bucket.HandleToKey.end() ? it->second : 0;
  }

  // Erase a handle's mapping.  Must be invoked from every vkDestroy* / vkFree*
  // wrapper so that a subsequent vkCreate* call which happens to be returned
  // the same VkXxx handle by the driver does NOT inherit the destroyed object's
  // GITSKey via UpdateOutputHandle's "reuse existing key" path -- that key
  // collision silently overwrites m_States entries in the subcapture state
  // tracker and causes restored linked pipelines to be bound against the wrong
  // dependency version (see comments in
  // handleArgumentUpdaters.h::UpdateOutputHandle).  No-op for an unknown handle
  // (idempotent / safe on validation errors).
  template <typename T>
  void RemoveHandle(T handle) {
    if (!handle) {
      return;
    }
    std::lock_guard<std::mutex> lock(m_Mutex);
    RemoveHandleLocked(HandleTypeIndex<T>(), reinterpret_cast<uint64_t>(handle));
  }

  // ---- Player direction: key -> handle (GITSKeys are globally unique). ----

  void SetHandle(GITSKey key, uint64_t handle);
  uint64_t GetHandle(GITSKey key);
  // Returns the player-side handle for key, or 0 if the key is not mapped.
  // Use instead of GetHandle when the object may not have been restored
  // (e.g. a resource destroyed before the subcapture range).
  uint64_t TryGetHandle(GITSKey key);

private:
  HandleMapService() = default;
  ~HandleMapService() = default;

  // Per-object-type handle->key store plus the audit trail used to classify
  // GetKey / GetKeyLenient misses (use-after-destroy vs never-registered).
  // Cost is bounded by the number of distinct Vulkan objects created per type.
  struct HandleBucket {
    std::unordered_map<uint64_t, GITSKey> HandleToKey;
    std::unordered_set<uint64_t> EverRegistered;
    // Handles removed via RemoveHandle, mapped to the key they had at removal.
    std::unordered_map<uint64_t, GITSKey> RemovedHandles;
    // Dedup set for GetKeyLenient warnings: at most one warning per unique
    // handle for the lifetime of the process.
    std::unordered_set<uint64_t> LenientWarned;
  };

  void SetKeyLocked(std::size_t typeIndex, uint64_t handle, GITSKey key);
  GITSKey GetKeyLocked(std::size_t typeIndex, uint64_t handle);
  GITSKey GetKeyLenientLocked(std::size_t typeIndex, uint64_t handle);
  void RemoveHandleLocked(std::size_t typeIndex, uint64_t handle);

  std::mutex m_Mutex;
  std::array<HandleBucket, kHandleTypeCount> m_ByType;
  std::unordered_map<GITSKey, uint64_t> m_KeyToHandle;
  // Dedup set for the type-erased GetKeyLenient(uint64_t) overload, which has no
  // per-type bucket to record its one-shot warnings in.
  std::unordered_set<uint64_t> m_TypeErasedLenientWarned;
};

} // namespace vulkan
} // namespace gits

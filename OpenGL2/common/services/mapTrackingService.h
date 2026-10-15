// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// Recorder-side bookkeeping for the "update on unmap only" buffer-map capture mechanism - see
// docs/development/opengl2-buffer-map-tracking-plan.md's Decisions 1-5. Naming
// (StoreData/TakeData) mirrors Vulkan/recorder/mapTrackingService.h's StoreData/RemoveData,
// scoped down to the single relevant subset: registering an active mapped range at
// glMapBufferRange time, and consuming it back at glUnmapBuffer time. Deliberately NOT built:
// Vulkan's continuous dirty-page tracking, external memory, persistent mapping, or
// SnapshotAllMapped - see the plan doc's Non-goals.
//
// Lives here (common/services/, linked by both the recorder and the player) for location
// consistency with ContextHandleMapService/ObjectNameMapService, even though only the recorder
// side has a caller today: the player's MappedDataMetaRunner performs its own map/write/unmap
// directly against the real driver in one call (see player/include/mappedDataMetaRunner.h) and
// never needs to track an in-progress mapping across separate calls the way the recorder does.

#include "glTypes.h"

#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

namespace gits::opengl2 {

// A currently-active glMapBufferRange mapping, keyed by the real, process-local buffer name
// and share group (not a GITSKey - this is purely an in-process bookkeeping record between the
// map and unmap calls on the same buffer, never itself serialized). Pointer/Offset/Length/
// Access are exactly glMapBufferRange's own parameters (Pointer is its return value) - see the
// plan doc's Decision 1 for why only glMapBufferRange/glUnmapBuffer are tracked at all.
//
// Share-group identity (see common/services/shareGroupService.h and
// modern-opengl-backend-architecture.md's share-group decision record): a GLuint buffer name
// is unique only within a share group, not process-globally - two independent, non-sharing
// contexts can each have a buffer named 1, and without this, concurrently mapping buffer 1 in
// both would let one context's TakeData() consume the other's tracked range. Reuses the same
// ShareGroupService identity as ObjectNameMapService, keyed the same way (see that class's own
// header comment for why only this "name as bookkeeping key" direction needs it at all).
struct MappedRange {
  void* Pointer{nullptr};
  GLintptr Offset{0};
  GLsizeiptr Length{0};
  GLbitfield Access{0};
};

class MapTrackingService {
public:
  static MapTrackingService& Get();

  // Registers a just-succeeded glMapBufferRange call's result for `buffer` (the real name
  // resolved from the mapped target - see the plan doc's Decision 4) within `shareGroup` (see
  // this header's own comment above). Overwrites any previous entry for the same
  // (shareGroup, buffer) without warning: GL forbids mapping an already-mapped buffer again,
  // so a real driver call already enforces there is at most one active mapping per buffer - a
  // stale leftover entry here would only be reachable if a prior unmap's TakeData was somehow
  // skipped, which should not happen given CustomWrapper_glUnmapBuffer's own call shape (see
  // mapTrackingWrappers.cpp).
  void StoreData(uint64_t shareGroup, GLuint buffer, const MappedRange& range);

  // Consumes (looks up and removes) the tracked mapping for `buffer` within `shareGroup`,
  // called from CustomWrapper_glUnmapBuffer right before the real glUnmapBuffer call - see the
  // plan doc's Decision 5. Returns std::nullopt if `buffer` has no tracked mapping in that
  // share group (glMapBufferRange was never called for it, or it targets a buffer/target this
  // plan does not track - e.g. no buffer was bound at map time). "Take", not "peek": a mapping
  // is valid for at most one matching unmap.
  std::optional<MappedRange> TakeData(uint64_t shareGroup, GLuint buffer);

private:
  MapTrackingService() = default;

  struct Key {
    uint64_t ShareGroup;
    GLuint Buffer;
    bool operator==(const Key& other) const {
      return ShareGroup == other.ShareGroup && Buffer == other.Buffer;
    }
  };
  struct KeyHash {
    size_t operator()(const Key& key) const {
      return std::hash<uint64_t>()(key.ShareGroup) ^ (std::hash<GLuint>()(key.Buffer) << 1);
    }
  };

  std::mutex m_Mutex;
  std::unordered_map<Key, MappedRange, KeyHash> m_MappedBuffers;
};

} // namespace gits::opengl2

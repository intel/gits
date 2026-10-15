// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// Recorder-side buffer-map tracking: remembers every currently-mapped buffer range and, at
// unmap, captures the bytes the application wrote through the mapped pointer as a
// MappedDataMetaCommand - see docs/development/opengl2-buffer-map-tracking-plan.md's
// Decisions 1-6. Deliberately NOT built: Vulkan's continuous dirty-page tracking, external
// memory, persistent mapping, or SnapshotAllMapped - see the plan doc's Non-goals.
//
// Lives under recorder/ rather than common/services/, mirroring Vulkan and DirectX: capturing a
// mapped range needs the recorder's own real-driver dispatch table and CaptureManager, neither of
// which the recorder+player-shared services may depend on. The player needs no counterpart -
// MappedDataMetaRunner performs its whole map/write/unmap against the real driver in one call and
// never tracks an in-progress mapping.

#include "glTypes.h"

#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <unordered_set>

namespace gits::opengl2 {

class MapTrackingService {
public:
  static MapTrackingService& Get();

  // Registers a just-succeeded glMapBufferRange so the matching unmap can capture whatever the
  // application writes through `mappedPointer`. Does nothing when the mapped target has no
  // resolvable buffer bound - see the plan doc's Decision 4.
  void OnMapBufferRange(
      GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access, void* mappedPointer);

  // Performs the application-visible glUnmapBuffer and returns its result unchanged, snapshotting
  // the tracked range before or after that call as the mapping's access flags require. Emits a
  // MappedDataMetaCommand only for a write-capable mapping whose unmap succeeded - see the plan
  // doc's Decision 5.
  GLboolean UnmapBufferAndCapture(GLenum target);

private:
  MapTrackingService() = default;

  // A currently-active glMapBufferRange mapping. Pointer/Offset/Length/Access are exactly that
  // call's own parameters, Pointer being its return value.
  struct MappedRange {
    void* Pointer{nullptr};
    GLintptr Offset{0};
    GLsizeiptr Length{0};
    GLbitfield Access{0};
  };

  // A GLuint buffer name is unique only within a share group, not process-globally: two
  // independent, non-sharing contexts can each have a buffer named 1, and without the share
  // group here one context's unmap could consume the other's tracked range. Reuses the same
  // ShareGroupService identity as ObjectNameMapService (see common/services/shareGroupService.h
  // and modern-opengl-backend-architecture.md's share-group decision record).
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

  // Asks the real driver which buffer is bound to `target`, the only binding state that can
  // never drift from what the application actually did - see the plan doc's Decision 4. Returns
  // 0, warning once per target, for a target this service does not track or one with nothing
  // bound.
  GLuint ResolveBoundBuffer(GLenum target);

  void ReportUnsupportedCoherentMapping(GLuint buffer);

  // Overwrites any previous entry for the same (share group, buffer) without warning: GL
  // forbids mapping an already-mapped buffer, so the driver itself already enforces that at
  // most one mapping per buffer is live.
  void StoreMappedRange(uint64_t shareGroup, GLuint buffer, const MappedRange& range);

  // "Take", not "peek": a tracked mapping is valid for at most one matching unmap. Returns
  // std::nullopt when the buffer has no tracked mapping in that share group.
  std::optional<MappedRange> TakeMappedRange(uint64_t shareGroup, GLuint buffer);

  std::mutex m_MappedBuffersMutex;
  std::unordered_map<Key, MappedRange, KeyHash> m_MappedBuffers;

  // Warn-once state for the gaps Decision 4 names (unrecognized target, nothing bound) and the
  // unsupported coherent mapping Phase 0 diagnoses. Map/unmap is a low-frequency path, so a
  // plain mutex is fine, mirroring ObjectNameMapService's own warn-once convention.
  std::mutex m_WarnedTargetsMutex;
  std::unordered_set<GLenum> m_WarnedUnrecognizedTargets;
  std::unordered_set<GLenum> m_WarnedNoBufferBound;
  std::once_flag m_WarnedCoherentMapping;
};

} // namespace gits::opengl2

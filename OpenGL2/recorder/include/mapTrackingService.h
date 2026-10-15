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

  // --- Extensions beyond the original glMapBufferRange/glUnmapBuffer pair -------------------
  //
  // Everything below extends the same mechanism (Decisions 1-6 above) to additional map/unmap
  // entry points a real target application was found to call instead of (or alongside)
  // glMapBufferRange/glUnmapBuffer - see the plan doc's Decision 1 "Revisit when" clause, which
  // calls exactly this out as an additive extension, not a rework.

  // Registers a just-succeeded glMapBuffer/glMapBufferARB (target-addressed, implicit
  // whole-buffer range - neither function takes an explicit offset/length or a GL_MAP_*_BIT
  // bitfield) so the matching glUnmapBuffer/glUnmapBufferARB can capture whatever the
  // application writes. `access` is gl.xml's BufferAccessARB enumerant
  // (GL_READ_ONLY/GL_WRITE_ONLY/GL_READ_WRITE), not glMapBufferRange's GL_MAP_*_BIT bitfield -
  // translated internally (see TranslateWholeBufferAccessToBits) into the same MappedRange
  // representation OnMapBufferRange above uses. Does nothing when the mapped target has no
  // resolvable buffer bound - see Decision 4.
  void OnMapBuffer(GLenum target, GLenum access, void* mappedPointer);

  // Core and EXT DSA maps provide the buffer name directly. Whole-buffer maps query
  // GL_BUFFER_SIZE; range maps already provide their offset, length, and access flags.
  void OnMapNamedBuffer(GLuint buffer, GLenum access, void* mappedPointer);
  void OnMapNamedBufferRange(
      GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access, void* mappedPointer);

  // Registers a just-succeeded glMapNamedBufferEXT (DSA, implicit whole-buffer range) - like
  // OnMapBuffer above, but the buffer name is already given directly by the caller, so no
  // target-to-bound-buffer resolution is needed at all (Decision 4 sidestepped entirely, exactly
  // as that decision's own text anticipated).
  void OnMapNamedBufferEXT(GLuint buffer, GLenum access, void* mappedPointer);

  // Performs the application-visible glUnmapBufferARB and returns its result unchanged.
  // A separate entry point must call its own, specific real
  // dispatch-table entry: glUnmapBuffer and glUnmapBufferARB are functionally identical but
  // distinct driver exports, and a driver is not guaranteed to resolve both.
  GLboolean UnmapBufferARBAndCapture(GLenum target);
  GLboolean UnmapNamedBufferAndCapture(GLuint buffer);

  // Performs the application-visible glUnmapNamedBufferEXT and returns its result unchanged -
  // same Decision-5 snapshot behavior as UnmapBufferAndCapture above, but DSA-addressed like
  // OnMapNamedBufferEXT above (no target resolution needed).
  GLboolean UnmapNamedBufferEXTAndCapture(GLuint buffer);

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
  // ShareGroupService identity as ObjectNameMapService (see shareGroupService.h and
  // modern-opengl-backend-architecture.md's share-group decision record).
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

  // Translates glMapBuffer/glMapBufferARB/glMapNamedBufferEXT's own `access` parameter - one of
  // GL_READ_ONLY/GL_WRITE_ONLY/GL_READ_WRITE (gl.xml's BufferAccessARB group - NOT a bitfield,
  // unlike glMapBufferRange's GL_MAP_*_BIT `access`) - into the GL_MAP_READ_BIT/GL_MAP_WRITE_BIT
  // bitfield convention MappedRange::Access already uses. An access value outside the three
  // legal enumerants (an application bug gl.xml itself cannot rule out) maps to 0 (neither
  // bit) - UnmapBufferAndCapture's own branching already treats "neither bit set" as "nothing
  // to snapshot", the same fallback every other unrecognized-input path in this service uses.
  static GLbitfield TranslateWholeBufferAccessToBits(GLenum access);

  // Shared tail of OnMapBuffer/OnMapNamedBufferEXT: both map the entire buffer implicitly (no
  // explicit length), so each separately queries it (via its own target/DSA-appropriate
  // GL_BUFFER_SIZE getter) before reaching this common StoreMappedRange call. Unlike
  // OnMapBufferRange/OnMapNamedBufferRangeEXT, this never checks GL_MAP_COHERENT_BIT: `access`
  // here is the BufferAccessARB enum group, not MapBufferAccessMask, so the whole-buffer map
  // family has no coherent-mapping concept of its own at all.
  void StoreWholeBufferMap(
      uint64_t shareGroup, GLuint buffer, GLenum access, void* mappedPointer, GLsizeiptr length);

  // Shared Decision-5 branching, used by every *new* unmap entry point below
  // UnmapBufferAndCapture (which predates this helper and is left as its own, separately
  // -reviewed implementation rather than retrofitted onto it, to avoid any behavior change to
  // that already-shipped, already-tested path).
  //
  // `callerName` is used only for this function's own LOG_WARNING messages, so a warning always
  // names the specific entry point (e.g. "glUnmapBufferARB") that actually observed the
  // problem. `realUnmap` performs the one real, app-visible unmap call - whichever specific
  // driver entry point matches the caller - and returns its GLboolean result, exactly as the
  // app's own call would. `realReadOnlyRemap` performs the write-only fallback's temporary
  // read-only re-map of [offset, offset+length) and returns the mapped pointer, or nullptr on
  // failure; `realInternalUnmap` performs the second, purely-internal unmap that follows a
  // successful remap. Its result is checked, but never propagated to the app. `realReadSubData`
  // reads the same range after a failed remap, returning false if the getter is unavailable.
  GLboolean SnapshotBufferAndUnmap(
      const char* callerName,
      GLuint buffer,
      uint64_t shareGroup,
      const std::function<GLboolean()>& realUnmap,
      const std::function<void*(GLintptr, GLsizeiptr)>& realReadOnlyRemap,
      const std::function<GLboolean()>& realInternalUnmap,
      const std::function<bool(GLintptr, GLsizeiptr, void*)>& realReadSubData);

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

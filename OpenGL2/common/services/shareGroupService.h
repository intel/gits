// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "glTypes.h"

#include <cstdint>
#include <mutex>
#include <unordered_map>

namespace gits::opengl2 {

// Identifies which GL "share group" (see wglShareLists/wglCreateContextAttribsARB's
// hShareContext) the calling thread's current HGLRC belongs to, so ObjectNameMapService/
// MapTrackingService can bucket GL object names by share group instead of by GLObjectKind
// alone - see modern-opengl-backend-architecture.md's share-group decision record. GL object
// names (GLuint) are unique only within a share group, not process-globally: two independent,
// non-sharing contexts can each have a buffer/texture named 1, and VAO/FBO names are
// context-local even between two contexts that otherwise share everything else - bucketing
// ObjectNameMapService/MapTrackingService only by kind would let a second, non-sharing
// context's name 1 silently overwrite the first context's mapping.
//
// Process-local, real-HGLRC-keyed bookkeeping - deliberately NOT stream-serialized, unlike
// ContextHandleMapService's GITSKey. The recorder and player each maintain their own,
// independent ShareGroupService instance over their own process-local HGLRC values (real,
// captured-process contexts on the recorder side; real, replay-process contexts on the player
// side) - nothing here needs to match numerically between the two sides, only to be
// self-consistent within one side's own lifetime: the GL objects' actual cross-process identity
// already flows through GITSKey via ObjectNameMapService/MapTrackingService's existing
// Register/GetKey/SetHandle/GetHandle calls. This service only partitions each side's own
// name->key (or name->data) maps so two same-process, non-sharing contexts' identically-named
// objects don't collide - it plays no role in matching a recorded name to its replayed one.
//
// Scope decision (see modern-opengl-backend-architecture.md's own copy of this note): supports
// the common initial-sharing case (wglCreateContextAttribsARB's hShareContext at context-
// creation time, handled by RegisterNewContext()) and wglShareLists called before either
// context has created any GL objects of its own (the typical, immediately-after-creation use,
// handled by MergeGroups()). Calling wglShareLists to merge two contexts that have *already*
// independently created same-numbered objects is a known, out-of-scope case: MergeGroups()
// repoints one context's *future* lookups onto the other's existing bucket, but does not
// retroactively reconcile or move any objects the two contexts already registered
// independently before the merge - those remain split across what are now, incorrectly, two
// different buckets for the same logical share group. This is a real, documented limitation,
// not a silent one: a real application scenario that hits it would see ObjectNameMapService's
// existing "no registered key/name" warnings for the pre-merge objects, not silently wrong
// results.
class ShareGroupService {
public:
  static ShareGroupService& Get();

  // Call once, right after a context-creating command (wglCreateContext/
  // wglCreateContextAttribsARB) succeeds, with the newly-created context and its (possibly
  // null) share context - see codegen/handle_lifecycle.py's CONTEXT_SHARE_PARAM. A null
  // `newContext` is a no-op (failed create). If `shareContext` is null or not yet registered
  // (e.g. an app that shares against a context created before recording/replay attached its
  // hooks), `newContext` starts a fresh group of its own; otherwise it joins `shareContext`'s
  // existing group.
  void RegisterNewContext(HGLRC newContext, HGLRC shareContext);

  // Call after a successful wglShareLists(hglrc1, hglrc2) - see
  // codegen/handle_lifecycle.py's SHARE_LISTS_COMMANDS. Merges hglrc2's group into hglrc1's
  // (each is lazily registered as a fresh, singleton group first if never seen). See this
  // class's own header comment for the exact, documented scope limitation on merging contexts
  // that already have independently-created objects.
  void MergeGroups(HGLRC hglrc1, HGLRC hglrc2);

  // Returns the share-group identity for `context` (typically the current thread's real
  // wglGetCurrentContext()), lazily registering it as a new, singleton group if never seen. A
  // null context returns 0 (a degraded-fidelity fallback bucket for the "no current context"
  // case, which is itself undefined behavior at the GL application level - not something this
  // service can meaningfully partition further).
  uint64_t GroupFor(HGLRC context);

private:
  ShareGroupService() = default;

  uint64_t GroupForLocked(HGLRC context);

  std::mutex m_Mutex;
  std::unordered_map<HGLRC, uint64_t> m_ContextToGroup;
  uint64_t m_NextGroup{1};
};

} // namespace gits::opengl2

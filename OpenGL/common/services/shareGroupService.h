// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// The recorder groups its contexts for ObjectNameMapService's/MapTrackingService's name->key
// lookups (via recorder/include/handleArgumentUpdaters.h's CurrentShareGroup()/
// GroupIdentityFor()), registering them in recorderWrappersAuto.cpp.mako's context-creating
// and wglShareLists handling. The player groups its replay contexts the same way, from its
// context-creating and wglShareLists runners, for BindlessHandleMapService: a bindless handle,
// unlike a GITSKey, is only unique within its share group.

#include "glTypes.h"

#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace gits::opengl {

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
// Contexts are the real HGLRC or GLXContext values, passed as `const void*` so one service serves
// both window systems.
//
// Process-local, real-HGLRC-keyed bookkeeping - deliberately NOT stream-serialized, unlike
// ContextHandleMapService's GITSKey. Scoped to the HGLRC values of the process it runs in -
// nothing here needs to match numerically between the recorder's and the player's contexts: the
// GL objects' actual cross-process identity already flows through GITSKey via
// ObjectNameMapService/MapTrackingService's existing Register/GetKey/SetHandle/GetHandle calls.
// This service only partitions each process's own maps so two same-process, non-sharing
// contexts' identically-named objects or handles don't collide - it plays no role in matching a
// recorded name to its replayed one.
//
// Scope decision (see modern-opengl-backend-architecture.md's own copy of this note): supports
// the common initial-sharing case (wglCreateContextAttribsARB's hShareContext at context-
// creation time, handled by RegisterNewContext()) and wglShareLists called before either
// context has created any GL objects of its own (the typical, immediately-after-creation use).
// wglShareLists merging two contexts that have *already* independently created same-numbered
// objects (e.g. a dummy context used briefly before being shared with the real one) is a known,
// accepted out-of-scope case: MergeGroups() repoints one context's future lookups onto the
// other's existing bucket, but does not retroactively reconcile or move any objects the two
// contexts already registered independently before the merge. Revisit only with a concrete
// repro that actually exercises wglShareLists this way - a real target application's captured
// stream's "Texture has no registered key" warnings were suspected to be this, but a live,
// instrumented repro showed wglShareLists is never called in that stream at all (all its
// contexts join their share group via wglCreateContextAttribsARB's hShareContext instead,
// which RegisterNewContext() already handles correctly), so this gap was not the cause there.
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
  void RegisterNewContext(const void* newContext, const void* shareContext);

  // Call after a successful wglShareLists(hglrc1, hglrc2) - see
  // codegen/handle_lifecycle.py's SHARE_LISTS_COMMANDS. Merges hglrc2's group into hglrc1's
  // (each is lazily registered as a fresh, singleton group first if never seen). Does not
  // retroactively reconcile objects either context already registered independently before
  // this call - see this class's own header comment.
  void MergeGroups(const void* hglrc1, const void* hglrc2);

  // Returns the share-group identity for `context` (typically the current thread's real
  // wglGetCurrentContext()), lazily registering it as a new, singleton group if never seen. A
  // null context returns 0 (a degraded-fidelity fallback bucket for the "no current context"
  // case, which is itself undefined behavior at the GL application level - not something this
  // service can meaningfully partition further).
  uint64_t GroupFor(const void* context);

  // True if `context` was passed to RegisterNewContext(), i.e. its creation was captured, so
  // every object created on it went through a captured command.
  bool IsCreationCaptured(const void* context);

private:
  ShareGroupService() = default;

  uint64_t GroupForLocked(const void* context);

  std::mutex m_Mutex;
  std::unordered_map<const void*, uint64_t> m_ContextToGroup;
  std::unordered_set<const void*> m_CapturedContexts;
  uint64_t m_NextGroup{1};
};

} // namespace gits::opengl

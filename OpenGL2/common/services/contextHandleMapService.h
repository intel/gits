// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "arguments.h"
#include "glTypes.h"

#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace gits::opengl2 {

// HGLRC <-> GITSKey, and nothing else. See modern-opengl-backend-architecture.md's "Real
// key-based mapping for handles GITS creates" decision: HGLRC is the one native handle type
// OpenGL2 fully owns the lifecycle of today (wglCreateContext(ARB) creates it,
// wglDeleteContext destroys it), so - unlike HDC (see TypeFacts.is_opaque_handle) - it earns
// a real mapping instead of being captured as an opaque, unremapped value.
//
// Lives here (common/services/, linked by both the recorder and the player) rather than
// under recorder/ because it now serves both directions of the same identity map: the
// recorder direction (HGLRC -> GITSKey, Register/GetKey/Erase) and the player direction
// (GITSKey -> HGLRC, SetHandle/GetHandle added for the player skeleton - see
// docs/development/opengl2-player-skeleton-plan.md's Decision 5). This mirrors how Vulkan's
// single HandleMapService already serves both directions from one shared-location class -
// splitting the two directions into unrelated classes would duplicate the "one handle type,
// one mutex" structure for no benefit, since the recorder and player processes never run
// simultaneously against the same instance anyway.
//
// Deliberately NOT a generalized, Vulkan-style HandleMapService: no per-type bucketing (there
// is only one type), no audit trail beyond a single "already warned" set per direction. Per
// modern-opengl-backend-architecture.md's "Keep abstractions backend-local until reuse is
// demonstrated", this only grows into something more general once a second mapped-handle
// type actually shows up.
//
// Register() deliberately takes an already-minted key rather than minting one itself (unlike
// this class's original recorder-only version): CaptureManager - the sole key minter - is a
// recorder-only concept, and this service now also has to build cleanly for the player, which
// has no CaptureManager at all. The recorder mints via CaptureManager::CreateHandleKey() and
// passes the result in (see recorder/include/handleArgumentUpdaters.h's UpdateOutputHandle),
// mirroring how Vulkan's HandleMapService::SetKey also takes an already-minted key rather than
// depending on CaptureManager itself.
//
// Low-frequency path (one call per context create/use/destroy, not per GL call), so a plain
// mutex is fine - contrast with recorder/unsupportedCommand.cpp's tbb::concurrent_unordered_set,
// which specifically avoids a mutex because it runs on every disabled GL call.
class ContextHandleMapService {
public:
  static ContextHandleMapService& Get();

  // Registers a newly-minted GITSKey for a newly-created HGLRC. Called only for the
  // handle-creating commands (wglCreateContext/wglCreateContextAttribsARB) - see
  // recorder/include/handleArgumentUpdaters.h's UpdateOutputHandle. A null handle (a failed
  // create) is not registered.
  void Register(HGLRC handle, GITSKey key);

  // Looks up the key for an existing HGLRC (wglMakeCurrent, wglDeleteContext,
  // wglShareLists, ...). Returns 0 (a never-valid key) and logs a single warning per
  // unique never-seen handle if it is not registered - see handleArgumentUpdaters.h's
  // UpdateHandle. A null handle returns 0 silently (e.g. wglMakeCurrent(hdc, nullptr) to
  // detach the current context is a normal, expected call).
  GITSKey GetKey(HGLRC handle);

  // Removes a destroyed context's mapping (wglDeleteContext) so a later create that
  // happens to reuse the same HGLRC value mints a fresh key instead of silently reusing
  // the destroyed context's identity. No-op for an unmapped/null handle. Recorder-only;
  // the player has no corresponding call - see the player skeleton plan's Non-goals.
  void Erase(HGLRC handle);

  // ---- Player direction: GITSKey -> live HGLRC, established as the stream is replayed. ----

  // Registers the live HGLRC created by a real wglCreateContext(ARB) call under the key
  // that was already decoded onto the same field - see player/include/
  // handleArgumentUpdaters.h's RegisterCreatedHandle. A zero key or null handle is a no-op.
  void SetHandle(GITSKey key, HGLRC handle);

  // Looks up the live HGLRC for an existing key (every other HGLRC-typed use) - see
  // player/include/handleArgumentUpdaters.h's ResolveHandle. Returns nullptr and logs a
  // single warning per unique never-seen key if it is not registered. A zero key returns
  // nullptr silently (mirrors GetKey's null-handle handling above).
  HGLRC GetHandle(GITSKey key);

private:
  ContextHandleMapService() = default;

  std::mutex m_Mutex;
  std::unordered_map<HGLRC, GITSKey> m_HandleToKey;
  std::unordered_set<HGLRC> m_WarnedMissingHandles;
  std::unordered_map<GITSKey, HGLRC> m_KeyToHandle;
  std::unordered_set<GITSKey> m_WarnedMissingKeys;
};

} // namespace gits::opengl2

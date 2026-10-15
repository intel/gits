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

#include <cstdint>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace gits::opengl2 {

// HandleT <-> GITSKey, and nothing else - the ONE generic implementation for every "mapped
// handle" type (see intermediates.py's _MAPPED_HANDLE_BASE_TYPES / TypeFacts.is_mapped_handle):
// an opaque, GITS-owned-lifecycle pointer typedef (HGLRC, GLsync, ...) that is NOT a GLuint-
// named object, so ObjectNameMapService's per-namespace bucketing does not apply to it - see
// codegen/handle_lifecycle.py's MappedHandleLifecycle for the Python-side data this class's two
// instantiations below are driven by.
//
// HISTORY: this used to be two hand-written, structurally-identical classes -
// ContextHandleMapService (HGLRC) and, once GLsync arrived and turned out to need the exact
// same shape a second time, SyncHandleMapService (GLsync) - each a full copy of the other's
// Register/GetKey/Erase/SetHandle/GetHandle bodies, differing only in the handle's C++ type and
// the type name embedded in a couple of log messages. That duplication was deliberate at the
// time ContextHandleMapService was the only mapped-handle service that existed ("Keep
// abstractions backend-local until reuse is demonstrated" - see
// modern-opengl-backend-architecture.md's decision record), on the theory that a second,
// same-shaped class was simpler to review/modify independently than a shared template with no
// second user yet. Once GLsync actually became that second user, the theory's own premise
// ("no reuse demonstrated yet") stopped holding, and the copy became exactly the risk that
// decision record was written to avoid for everything else: a bug fix or behavior change in one
// class's Register()/GetKey()/etc. silently not applying to the other's identical body unless a
// reviewer remembers to update both. This template is that promised generalization - see this
// class's own two aliases below, and MappedHandleLifecycle's own docstring
// (codegen/handle_lifecycle.py) for the matching Python-side generalization and the concrete
// checklist for adding a third mapped-handle type.
//
// Serves both directions of the same identity map, same as Vulkan's own HandleMapService
// (common/services/handleMapService.h under Vulkan/ - a DIFFERENT, unrelated class in a
// different namespace, despite the identical file/class name; see this class's own "Deliberately
// NOT Vulkan's own HandleMapService shape" note below): the recorder direction (HandleT ->
// GITSKey, Register/GetKey/Erase) and the player direction (GITSKey -> HandleT, SetHandle/
// GetHandle, added for the player skeleton - see docs/development/opengl2-player-skeleton-
// plan.md's Decision 5). The recorder and player processes never run simultaneously against the
// same instance, so there is no reason to split the two directions into unrelated classes.
//
// Deliberately NOT Vulkan's own HandleMapService shape (a single non-template class bucketed by
// a compile-time-constant HandleTypeIndex<T>() into one std::array<HandleBucket, N> - see that
// class's own header comment, Vulkan/common/services/handleMapService.h): Vulkan's version
// exists to solve a problem this backend does not have - non-dispatchable Vulkan handle
// *values* are not guaranteed unique across object types, so two different handle types sharing
// one flat map could collide. HGLRC and GLsync (and any future OpenGL2 mapped-handle type) are
// each already a distinct C++ pointer typedef with no such cross-type collision risk (a
// std::unordered_map<HGLRC, GITSKey> and a std::unordered_map<GLsync, GITSKey> are two
// completely independent maps by construction, one per template instantiation - no bucket index
// or type-erasure needed to keep them apart). A plain `template <typename HandleT> class
// HandleMapService` - one map pair per instantiation, exactly mirroring what the two now-merged
// classes already did - is the simpler, sufficient tool for this backend's actual problem;
// Vulkan's own bucketed design is not "the established template/generic-service convention this
// codebase always uses" so much as the right tool for Vulkan's own, different constraint.
//
// Register() deliberately takes an already-minted key rather than minting one itself (unlike
// this class's original, recorder-only precedent): CaptureManager - the sole key minter - is a
// recorder-only concept, and this service also has to build cleanly for the player, which has
// no CaptureManager at all. The recorder mints via CaptureManager::CreateHandleKey() and passes
// the result in (see recorder/include/handleArgumentUpdaters.h's UpdateOutputHandle), mirroring
// how Vulkan's own HandleMapService::SetKey also takes an already-minted key rather than
// depending on CaptureManager itself.
//
// Low-frequency (HGLRC) or hot (GLsync - commonly created/waited-on once per frame per streamed
// resource, so a single capture can issue tens of thousands of glClientWaitSync calls) - a
// plain mutex is used uniformly for both today anyway: a lock + unordered_map lookup is
// sub-microsecond on any modern platform, nowhere near what would make a capture/replay tool's
// own bookkeeping the bottleneck next to the real GL driver call it wraps. Contrast with
// recorder/unsupportedCommand.cpp's tbb::concurrent_unordered_set, which specifically avoids a
// mutex because it runs on every disabled GL call - a genuinely hotter path than even GLsync's.
// If a mutex ever measurably shows up in a profile for some future HandleT, a sharded lock or a
// lock-free map would be the next step for that instantiation specifically - not attempted here
// without a measured need.
//
// Recorder-direction (HandleT -> GITSKey) share-group scoping: Register()/GetKey()/Erase() below
// all take a `shareGroup` parameter (default 0) because NOT every mapped-handle type's own
// value is unique process-wide. HGLRC is: a context handle IS the very identity
// ShareGroupService::GroupFor() keys its own group lookup off, so scoping it by its own group
// would be a chicken-and-egg problem - every HGLRC call site passes the default 0, degenerating
// this to the original flat, process-global map. GLsync is NOT: the legacy OpenGL backend
// already classifies it as a share-group-scoped "shared" object (OpenGL/common/include/
// mapping.h's `OGL_SYNC_MAP` -> `SharedGLObjectTraits`, bucketed per
// `GetCurrentContextSharedGroupId()`), because a GL implementation is free to hand out a
// GLsync token that is only unique within a share group (e.g. a small per-share-group index
// cast to a pointer) rather than a genuinely process-wide-unique heap address - two
// independent, non-sharing contexts' live fences can then carry the exact same opaque value at
// the same time. GLsync call sites (recorder/include/handleArgumentUpdaters.h's
// HandleShareGroupFor()) pass the real CurrentShareGroup() instead, so two such fences land in
// different map entries instead of one silently overwriting the other's key - which would
// otherwise serialize a later glClientWaitSync/glWaitSync/glDeleteSync against the WRONG fence.
// The player direction (SetHandle/GetHandle, GITSKey -> HandleT) needs no such parameter: a
// GITSKey is already minted uniquely per registered object (CaptureManager::CreateHandleKey())
// and so never collides across share groups on its own, mirroring ObjectNameMapService's own
// "only the forward, name-keyed direction needs the share group" precedent exactly (see that
// class's own header comment).
template <typename HandleT>
class HandleMapService {
public:
  static HandleMapService& Get();

  // Registers a newly-minted GITSKey for a newly-created handle, in the given share group (see
  // this class's own header comment on the `shareGroup` parameter - 0/default for HGLRC, the
  // real CurrentShareGroup() for GLsync). Called only for this handle type's own creating
  // command(s) (see codegen/handle_lifecycle.py's MappedHandleLifecycle.creating_commands) -
  // see recorder/include/handleArgumentUpdaters.h's UpdateOutputHandle. A null handle (a
  // failed create) is not registered.
  void Register(HandleT handle, GITSKey key, uint64_t shareGroup = 0);

  // Looks up the key for an existing handle in the given share group. Returns 0 (a never-valid
  // key) and logs a single warning per unique never-seen (shareGroup, handle) pair if it is not
  // registered - see handleArgumentUpdaters.h's UpdateHandle. A null handle returns 0 silently
  // (e.g. wglMakeCurrent(hdc, nullptr) to detach the current context is a normal, expected
  // call; no enabled GLsync-typed field ever passes a null `sync` as meaningful GL usage
  // either, but treating it as "missing" would be a spurious warning, not a real bug report -
  // nothing could ever have registered handle 0 in the first place, since Register() above
  // already rejects it).
  GITSKey GetKey(HandleT handle, uint64_t shareGroup = 0);

  // Removes a destroyed handle's mapping in the given share group (see
  // codegen/handle_lifecycle.py's MappedHandleLifecycle.param_to_erase_on_success) so a later
  // create that happens to reuse the same HandleT value (in the same or a different share
  // group) mints a fresh key instead of silently reusing the destroyed object's identity.
  // No-op for an unmapped/null handle. Recorder-only; the player has no corresponding call -
  // see the player skeleton plan's Non-goals.
  void Erase(HandleT handle, uint64_t shareGroup = 0);

  // ---- Player direction: GITSKey -> live HandleT, established as the stream is replayed. ----

  // Registers the live handle created by a real handle-creating call under the key that was
  // already decoded onto the same field - see player/include/handleArgumentUpdaters.h's
  // UpdateOutputHandle. A zero key or null handle is a no-op.
  void SetHandle(GITSKey key, HandleT handle);

  // Looks up the live handle for an existing key (every other use of this handle type).
  // Returns a null handle and logs a single warning per unique never-seen key if it is not
  // registered - see player/include/handleArgumentUpdaters.h's UpdateHandle. A zero key
  // returns a null handle silently (mirrors GetKey's null-handle handling above).
  HandleT GetHandle(GITSKey key);

private:
  HandleMapService() = default;

  // Recorder-direction (handle -> key) map key: see this class's own header comment on why
  // HandleT alone is not always enough - a share-group-scoped type's HandleT value is only
  // unique within its own share group, mirroring ObjectNameMapService::NameKey exactly (same
  // struct shape, same rationale, one level up from GLuint names to whole mapped-handle
  // values). HGLRC's own call sites always pass shareGroup 0, so this degenerates back to a
  // plain HandleT-keyed lookup for that instantiation with no behavior change.
  struct HandleKey {
    uint64_t ShareGroup;
    HandleT Handle;
    bool operator==(const HandleKey& other) const {
      return ShareGroup == other.ShareGroup && Handle == other.Handle;
    }
  };
  struct HandleKeyHash {
    size_t operator()(const HandleKey& key) const {
      return std::hash<uint64_t>()(key.ShareGroup) ^ (std::hash<HandleT>()(key.Handle) << 1);
    }
  };

  std::mutex m_Mutex;
  std::unordered_map<HandleKey, GITSKey, HandleKeyHash> m_HandleToKey;
  std::unordered_set<HandleKey, HandleKeyHash> m_WarnedMissingHandles;
  // Player-direction (key -> handle) maps stay keyed by GITSKey alone - see this class's own
  // header comment on why that direction needs no share-group parameter at all.
  std::unordered_map<GITSKey, HandleT> m_KeyToHandle;
  std::unordered_set<GITSKey> m_WarnedMissingKeys;
};

// Human-readable names embedded in HandleMapService<HandleT>'s log messages, matching this
// type's own class alias below verbatim (e.g. "ContextHandleMapService: HGLRC <value> has no
// registered key ...") - specialized once per mapped-handle type. Adding a third mapped-handle
// type means adding a third specialization here (see codegen/handle_lifecycle.py's
// MappedHandleLifecycle docstring for the rest of that checklist).
template <typename HandleT>
struct HandleTypeInfo;

template <>
struct HandleTypeInfo<HGLRC> {
  static constexpr const char* kServiceName = "ContextHandleMapService";
  static constexpr const char* kTypeName = "HGLRC";
};

template <>
struct HandleTypeInfo<GLsync> {
  static constexpr const char* kServiceName = "SyncHandleMapService";
  static constexpr const char* kTypeName = "GLsync";
};

template <>
struct HandleTypeInfo<HPBUFFERARB> {
  static constexpr const char* kServiceName = "PbufferHandleMapService";
  static constexpr const char* kTypeName = "HPBUFFERARB";
};

template <>
struct HandleTypeInfo<HPBUFFEREXT> {
  static constexpr const char* kServiceName = "PbufferEXTHandleMapService";
  static constexpr const char* kTypeName = "HPBUFFEREXT";
};

// Readable, backward-compatible names for the two mapped-handle types today - every existing
// hand-written call site (handleArgumentUpdaters.h, recorder/mapTrackingWrappers.cpp,
// recorderWrappersAuto.cpp.mako's erase_service lookup, ...) keeps referring to these exact
// names, unaware they are now the same template underneath. A future third mapped-handle type
// gets a third alias here, matching whatever name its own call sites read most naturally as.
using ContextHandleMapService = HandleMapService<HGLRC>;
using SyncHandleMapService = HandleMapService<GLsync>;
using PbufferHandleMapService = HandleMapService<HPBUFFERARB>;
using PbufferEXTHandleMapService = HandleMapService<HPBUFFEREXT>;

// Method bodies live in handleMapService.cpp, explicitly instantiated there for exactly these
// two types - this extern template declaration suppresses any *implicit* instantiation from
// this header, keeping one compiled copy of each method per handle type, matching a plain
// non-template class's usual header/.cpp split. A third mapped-handle type needs a third
// `extern template class HandleMapService<NewHandleT>;` line here, and a matching
// `template class HandleMapService<NewHandleT>;` in handleMapService.cpp.
extern template class HandleMapService<HGLRC>;
extern template class HandleMapService<GLsync>;
extern template class HandleMapService<HPBUFFERARB>;
extern template class HandleMapService<HPBUFFEREXT>;

} // namespace gits::opengl2

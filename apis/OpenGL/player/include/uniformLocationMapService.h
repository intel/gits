// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// Lives under player/ rather than common/services/, mirroring MapTrackingService's own
// recorder/ placement (see recorder/include/mapTrackingService.h's header comment) for the
// same reason in the opposite direction: every method below is only ever called from player-
// side code (ReplayCustomizationLayer/commandRunnersAuto.cpp.mako - see this class's own
// method comments for each call site) and only ever needs player-side identity
// (CurrentContext()'s live, replay-time context - player/include/handleArgumentUpdaters.h). The
// recorder has no counterpart: a captured stream already records whichever location the
// recording-time driver returned, so there is nothing for the recorder itself to remap.

#include "arguments.h" // GITSKey
#include "glTypes.h"

#include <cstdint>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace gits::opengl {

// Player-only: remaps a captured glGetUniformLocation() return value to its real, replay-time
// location before any later glUniform*(location, ...) call uses it - see the legacy OpenGL
// backend's own equivalent
// (OpenGLLegacy/common/include/playerRunWrap.h's HandleUniformLocationMapping()/
// CGLUniformLocation), which this is a deliberately scoped-down analogue of.
//
// A uniform's location is driver/link-dependent - re-linking the exact same shader source on a
// different driver, or even the same driver on a different run, is not guaranteed to assign the
// same integer location to the same uniform. Replaying the captured location value verbatim
// (what this backend did before this fix) can silently set the wrong uniform, or one that
// doesn't exist at all, on a replay-time driver whose linker assigned different locations.
//
// Keying note: m_Locations is keyed by (program's stream-stable GITSKey, captured location) -
// NOT the replay-time live program GLuint. An earlier version of this class keyed by the live
// GLuint directly; that collided across independent, non-sharing contexts, since a GL object
// name is only unique *within its own share group* (see ShareGroupService's own header
// comment and ObjectNameMapService's own header comment on the same point) - a driver is free
// to (and in practice routinely does) hand out live program handle `1` to the first program
// created in each of several independent, non-sharing contexts. Two such programs are
// different objects with entirely unrelated uniform layouts, but a GLuint-keyed map would
// treat them as the same map entry, so the later context's Register() call would silently
// overwrite - and the earlier context's subsequent glUniform*() calls would then silently
// resolve against - the wrong program's locations. Keying by GITSKey instead avoids this
// entirely: a GITSKey is minted once per created object from a single global counter
// (CaptureManager::CreateHandleKey(), recorder-side) and is never itself reused, and
// crucially, is never even scoped to a share group in the first place - it already is the
// stable, collision-free identity ObjectNameMapService itself uses (see that class's own
// header comment: "a GITSKey is already created uniquely per registered object ... and so
// never collides across share groups on its own"). `command.m_program.Key` (populated purely
// by argumentCoders.h's Decode(), untouched by UpdateHandle() - see
// player/include/handleArgumentUpdaters.h, which only ever writes `.Value`) is available at
// every call site below for exactly this reason, and using it here is simply reusing the same
// identity the object-name-mapping system already established, rather than introducing a
// second, less precise one.
//
// PurgeProgram() below remains necessary regardless of this keying change - not to prevent a
// reused-GLuint collision (a GITSKey is never reused, so that specific failure mode this
// method's own comment previously worried about cannot happen with this keying) but as
// ordinary cleanup hygiene: a deleted program's locations should not linger in this map
// forever, and the deferred-purge-while-current logic below (a program can remain a fully
// legal glUniform*() target after glDeleteProgram, until it stops being current everywhere -
// see the GL spec's deferred-deletion rule) is unaffected by the keying change and still
// applies unchanged.
//
// Scope decision (documented further in modern-opengl-backend-architecture.md): only exact
// locations returned by glGetUniformLocation(ARB) are remapped, keyed by (program identity,
// captured location). GL_ARB_shader_objects' glGetUniformLocationARB/glUseProgramObjectARB
// share the core commands' tracking in ReplayCustomizationLayer; its `programObj` handle maps
// through the same Program bucket as a core program name (see object_name_lifecycle.py's
// HANDLE_ARB_CLASSES). The legacy backend's own
// CGLUniformLocation additionally remaps whole *uniform arrays* (see playerRunWrap.h's
// HandleUniformLocationMapping array branch), since an application commonly computes array
// element locations itself (`glGetUniformLocation(prog, "arr[0]") + i`). The same is done here
// from a UniformArrayMetaCommand (uniformArrayMetaTypes.h), which carries the array's
// capture-time size: UniformArrayMetaRunner registers each element both drivers keep active,
// and maps elements only the capture driver kept active to -1, which GL ignores. Any other
// (program, location) pair Resolve() never saw registered falls back to the raw captured value
// with a one-time warning, rather than guessing.
//
// Also owns simple "current program" tracking (SetCurrentProgram/GetCurrentProgram): classic,
// non-DSA glUniform*() calls apply to whichever program is currently in use
// (glUseProgram-bound), not to an explicit parameter, so Resolve() needs to know which
// program's map to consult. GL_CURRENT_PROGRAM is genuine per-context state - unlike a GL
// object *name* (share-group-scoped - see ShareGroupService's own header comment), the
// glUseProgram *binding point* is per-context even for two contexts in the same share group,
// since program/shader objects are shareable but which one is bound for use is not (see the
// GL spec's "Shared Objects and Multiple Contexts" - program/shader objects are shared,
// GL_CURRENT_PROGRAM is not). Tracked here keyed by the real, replay-time context (see
// player/include/handleArgumentUpdaters.h's CurrentContext()) rather than a single
// process-global value, so a replay that runs multiple GL contexts - sharing or not,
// concurrently on different threads or serially on one - resolves each glUniform*() call
// against the program actually bound on its own context, not whichever context last called
// glUseProgram anywhere in the process.
class UniformLocationMapService {
public:
  static UniformLocationMapService& Get();

  // Called by ReplayCustomizationLayer (player/replayCustomizationLayer.cpp) right after a
  // real glGetUniformLocation(ARB) call returns, with the program's
  // stream-stable GITSKey (command.m_program.Key - see this class's own header comment for why
  // this, not the live replay-time handle, is the correct identity to key by),
  // the captured (recorder-time) location this call's Command originally decoded, and the real,
  // replay-time location the driver just returned. A captured location of -1 (glGetUniformLocation's
  // documented "no such active uniform" return) is not registered - Resolve() already treats an
  // unregistered (program, location) pair as an identity fallback, and -1 has no location to
  // remap to any live uniform in any case. Deliberately not context-keyed: a program's
  // GITSKey is shared-object identity (see ShareGroupService's own header comment), not
  // per-context state, so a given program's locations are unambiguous regardless of which
  // context looked them up.
  void Register(GITSKey programKey, GLint capturedLocation, GLint replayLocation);

  // Called by the generic per-command Run() for every enabled *classic* (non-DSA)
  // glUniform*(location, ...) command's `location` parameter (a GLint parameter literally
  // named `location` on a command with no explicit `program` parameter of its own - see
  // commandRunnersAuto.cpp.mako's is_uniform_location_param/is_explicit_program_uniform_command)
  // before the real call, substituting the captured location for its replay-time equivalent
  // under `context`'s current program (see GetUniformProgram()) - callers pass CurrentContext()
  // (handleArgumentUpdaters.h), the real context this call is about to run against. Falls back
  // to `capturedLocation` unchanged - with a one-time warning per (program, location) pair - if
  // this exact location was never registered via Register() (see this class's own header
  // comment for when that happens).
  GLint Resolve(const void* context, GLint capturedLocation);

  // Called by the generic per-command Run() for every enabled glProgramUniform*(program,
  // location, ...) DSA command's `location` parameter instead of Resolve() above - these take
  // their target program explicitly rather than relying on whichever program happens to be
  // glUseProgram-bound at replay time (GL_CURRENT_PROGRAM), so the caller passes the target
  // program's already-decoded GITSKey (command.m_program.Key) directly (see
  // object_name_lifecycle.py's ('glProgramUniform*', 'program') LOOKUP entries, which make
  // sure that same field's live `.Value` is itself replay-correct before the real call, via
  // UpdateHandle - unrelated to this GITSKey-keyed lookup) instead of going through
  // GetCurrentProgram(). Resolving these against whatever program is current instead would
  // silently use the wrong program's location map if a different one happens to be bound at
  // replay time. Same fallback/warning behavior as Resolve() otherwise - both share the same
  // lookup, just keyed by a different, explicitly-supplied program identity.
  GLint ResolveForProgram(GITSKey programKey, GLint capturedLocation);

  // Captured locations of `programKey` by the replay location they resolve to, for subcapture
  // state restore. Locations queried before and after a relink can resolve to the same one.
  std::unordered_map<GLint, std::vector<GLint>> CapturedLocations(GITSKey programKey) const;

  // Called by ReplayCustomizationLayer for glUseProgram after a successful real call,
  // with the same `context` CurrentContext() would return at that point (the context
  // glUseProgram was just called against) - see this class's own header comment for why
  // "current program" must be tracked per-context rather than passed explicitly (classic
  // glUniform* calls have no program parameter of their own).
  //
  // Also the trigger point for completing a deferred PurgeProgram(): the program `context` was
  // bound to *before* this call (if any) is about to stop being current in `context` - if that
  // outgoing program is in m_PendingDeletion and this was its last remaining current context,
  // it is purged now (see PurgeProgram()'s own comment).
  //
  // `programKey` is the program's stream-stable GITSKey (command.m_program.Key), not its live
  // replay-time handle - see this class's own header comment. The caller
  // (ReplayCustomizationLayer) is responsible for only calling this when the real driver actually
  // switched to the
  // requested program (queried via glGetIntegerv(GL_CURRENT_PROGRAM, ...) after the real
  // call, not inferred from the call's own success/failure) - glUseProgram leaves
  // GL_CURRENT_PROGRAM unchanged on a failed link/validate, and tracking the requested program
  // regardless of outcome would desynchronize this map from the driver's real state.
  void SetCurrentProgram(const void* context, GITSKey programKey);

  GITSKey GetCurrentProgram(const void* context) const;

  // Called by ReplayCustomizationLayer after GL bound `pipelineKey` on `context`.
  void BindProgramPipeline(const void* context, GITSKey pipelineKey);
  // Called by ReplayCustomizationLayer after GL made `programKey` the active program of
  // `pipelineKey`.
  void SetActiveProgram(GITSKey pipelineKey, GITSKey programKey);
  // Called by the glDeleteProgramPipelines(EXT) runners after the real call, which also unbinds
  // the pipeline.
  void DeleteProgramPipeline(GITSKey pipelineKey);
  // The program classic glUniform*() calls on `context` set uniforms of: its current program, or
  // with none, the active program of its bound program pipeline. Resolve() resolves against it.
  GITSKey GetUniformProgram(const void* context) const;

  // Called by glDeleteProgramRunner's Run() (commandRunnersAuto.cpp.mako, generated from
  // any resolve_object_name_params entry whose ObjectNameOverride is
  // (kind=GLObjectKind.PROGRAM, role=ObjectNameRole.DELETE) - a structural, kind/role-driven
  // rule, not a hardcoded command-name check, so a future second command sharing that same
  // (kind, role) pair would automatically also purge here) with the program's already-decoded
  // GITSKey, before the real glDeleteProgram call.
  //
  // Per the GL spec's deferred-deletion rule, a program still bound as current (in ANY
  // context, not just the one this glDeleteProgram call happens to run against) remains a
  // fully legal target for further glUniform*() calls until it is no longer current anywhere -
  // GL only actually reclaims the name at that point. An earlier version of this method purged
  // m_Locations unconditionally right here instead, which broke exactly that legal window: any
  // glUniform*() call against the still-current "deleted" program between this call and the
  // program actually becoming non-current would find its m_Locations entries already gone and
  // fall back to the raw captured location.
  //
  // Fixed by checking IsCurrentInAnyContext() first: if the program is current in at least one
  // tracked context right now, its entries are left alone and `liveProgram` is instead recorded
  // in m_PendingDeletion; SetCurrentProgram() below re-checks every outgoing current program
  // against m_PendingDeletion on every glUseProgram() call and actually purges the moment one
  // drops to zero remaining contexts. If the program is not current anywhere already (the
  // common case - most deletes happen well after the app itself has moved on), this purges
  // immediately exactly as before. The active program of a program pipeline counts as current
  // too, so SetActiveProgram() and DeleteProgramPipeline() re-check the program they release.
  //
  // Residual gap, deliberately left as a documented limitation rather than fully closed here:
  // a context being destroyed (wglDeleteContext) does not, on its own, update
  // m_CurrentProgramByContext or re-check m_PendingDeletion - only a later SetCurrentProgram()
  // call does. A pending-deletion program whose *only* remaining current context is torn down
  // without that context ever calling glUseProgram to something else first stays in
  // m_PendingDeletion (and keeps its m_Locations entries) for the rest of the replay process,
  // matching ContextHandleMapService's own established "player-side cleanup on destroy is not
  // load-bearing within a single replay" precedent (see that service's own Delete() comment) -
  // this is an extremely narrow real-world case (an app tearing down a context while its
  // deleted-but-still-current program is never made non-current first) that, unlike the
  // immediate-purge bug this fix addresses, has no currently tested sample exercising it.
  void PurgeProgram(GITSKey programKey);

private:
  UniformLocationMapService() = default;

  // All must be called with m_Mutex already held - internal helpers shared by PurgeProgram(),
  // SetCurrentProgram() and the pipeline methods, see their own comments above.
  // Also counts the active program of any pipeline.
  bool IsCurrentInAnyContextLocked(GITSKey programKey) const;
  void ErasePurgeProgramLocked(GITSKey programKey);
  GITSKey ActiveProgramLocked(GITSKey pipelineKey) const;
  // Completes a deferred PurgeProgram() of `programKey` once nothing keeps it current; called
  // when a context or a pipeline stops referencing it.
  void ReleaseProgramLocked(GITSKey programKey);

  struct Key {
    GITSKey ProgramKey;
    GLint CapturedLocation;
    bool operator==(const Key& other) const {
      return ProgramKey == other.ProgramKey && CapturedLocation == other.CapturedLocation;
    }
  };
  struct KeyHash {
    size_t operator()(const Key& key) const {
      return std::hash<GITSKey>()(key.ProgramKey) ^
             (static_cast<size_t>(static_cast<uint32_t>(key.CapturedLocation)) << 1);
    }
  };

  mutable std::mutex m_Mutex;
  std::unordered_map<Key, GLint, KeyHash> m_Locations;
  std::unordered_set<Key, KeyHash> m_WarnedMissing;
  // Per-context "current program" (see this class's own header comment for why per-context,
  // not process-global or per-share-group), tracked by the program's GITSKey rather than its
  // live handle (see this class's own header comment). A context never seen before (not yet
  // in the map) has no current program - GetCurrentProgram() returns key 0 (mirrors
  // GL_CURRENT_PROGRAM's own "no program in use" value 0, and object_name_lifecycle.py's
  // "zero name -> zero key" convention) for it, same as a fresh real GL context would report.
  std::unordered_map<const void*, GITSKey> m_CurrentProgramByContext;
  // Per-context program pipeline binding, and the active program of each pipeline, by GITSKey.
  std::unordered_map<const void*, GITSKey> m_PipelineByContext;
  std::unordered_map<GITSKey, GITSKey> m_ActiveProgramByPipeline;
  // Programs glDeleteProgram has already run against but that were still current in at least
  // one context or active in a pipeline at that point - see PurgeProgram()'s own comment.
  // Removed once nothing keeps the program current any more.
  std::unordered_set<GITSKey> m_PendingDeletion;
};

} // namespace gits::opengl

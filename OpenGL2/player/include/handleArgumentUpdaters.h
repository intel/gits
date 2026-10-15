// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// Player-side counterpart of recorder/include/handleArgumentUpdaters.h, resolving the
// opposite direction (GITSKey -> live handle, instead of handle -> GITSKey) - see
// docs/development/opengl2-player-skeleton-plan.md's Decision 5 and
// modern-opengl-backend-architecture.md's "Real key-based mapping for handles GITS creates".
// Hand-written, not generated, for the same reason the recorder's own copy is: no command's
// param/return-value shape here needs anything commandRunnersAuto.cpp.mako couldn't already
// express with a plain call - see that template's own header comment for the generated side's
// `UpdateHandle(command.m_${p.name})` call sites, which resolve against
// UpdateHandle/UpdateOutputHandle's HandleArgument<HandleT> template below via ordinary C++
// template argument deduction, with no per-type codegen branching needed on either side.
//
// Named identically to the recorder-side copy's UpdateHandle/UpdateOutputHandle (see
// docs/development/opengl2-layer-architecture-alignment-plan.md's Phase A) - mirrors Vulkan's
// own convention exactly (Vulkan/recorder/handleArgumentUpdaters.h and
// Vulkan/player/handleArgumentUpdaters.h both use the same two names, differing only by which
// manager type resolution runs against). Previously named ResolveHandle/RegisterCreatedHandle/
// ResolveHandleArray/RegisterCreatedHandleArray here - purely a rename, no behavior change;
// the old names read reasonably in isolation but, unlike Vulkan's shared naming, gave no hint
// that this is the same key<->handle protocol as the recorder side, which made it unclear
// whether OpenGL2 actually implements Vulkan's pattern at all (it does).
//
// UpdateHandle/UpdateOutputHandle below used to be two hand-copied, non-templated overload
// pairs (one for HandleArgument<HGLRC>, one for HandleArgument<GLsync>) before GLsync's own
// arrival prompted generalizing HandleMapService itself into a template (see
// common/services/handleMapService.h's own header comment) - now a single
// `template <typename HandleT>` pair covers every mapped-handle type uniformly, present and
// future, with no new overload needed per type.
//
// The GLuint overloads below are plain, non-templated overloads taking an explicit
// GLObjectKind parameter, for the same reason the recorder-side copy's GLuint overloads are -
// see that file's header comment.

#include "arguments.h"
#include "glTypes.h"
#include "handleMapService.h"
#include "objectNameMapService.h"
#include "openglDriversAuto.h"

#include <atomic>

namespace gits::opengl2 {

// Returns the real, replay-time HGLRC current on this thread, by calling the real
// wglGetCurrentContext() directly rather than reading any decoded Command state - player-side
// counterpart of recorder/include/handleArgumentUpdaters.h's CurrentShareGroup() (see that
// function's own comment for why calling the real driver directly, instead of tracking this
// backend's own notion of "current context", is safe and current at any point a wrapper/Run()
// executes). Used by UniformLocationMapService's callers (commandRunnersAuto.cpp.mako's
// glUseProgram hook and its generic is_uniform_location_param resolve hook) to key "current
// program" per real context instead of process-globally - see that service's own header
// comment for why program-in-use is per-context GL state, not per-share-group or process-wide.
inline HGLRC CurrentContext() {
  auto realFunction = player::g_DispatchTable.wglGetCurrentContext.load(std::memory_order_acquire);
  return realFunction ? realFunction() : nullptr;
}

// Existing-handle resolution before a real call: substitute the decoded Key's live handle for
// Value (still whatever Decode() zero-initialized it to) - wglMakeCurrent, wglDeleteContext,
// wglShareLists, glIsSync, glClientWaitSync, glWaitSync, glDeleteSync, etc. Generic over every
// mapped-handle type (HGLRC, GLsync, ...) via HandleMapService<HandleT> - see that class's own
// header comment (common/services/handleMapService.h) for why one template, not one hand-copied
// overload per type, is now the right shape here. A zero key (e.g. wglMakeCurrent's hglrc when
// detaching) is left at a null Value without a lookup or warning - it is a normal value, not a
// missing registration.
template <typename HandleT>
inline void UpdateHandle(HandleArgument<HandleT>& arg) {
  if (arg.Key == 0) {
    return;
  }
  arg.Value = HandleMapService<HandleT>::Get().GetHandle(arg.Key);
}

// After a real handle-creating call succeeds (wglCreateContext(ARB), glFenceSync, ...):
// register the newly-created live handle under the key that was already decoded onto this same
// field (see handle_lifecycle.py's HANDLE_CREATING_COMMANDS, which recorderWrappersAuto.cpp.
// mako's own UpdateOutputHandle call consults for the same purpose). Generic over every
// mapped-handle type, same rationale as UpdateHandle above. A null Value (a failed create) is
// left unregistered.
template <typename HandleT>
inline void UpdateOutputHandle(HandleArgument<HandleT>& arg) {
  if (arg.Value) {
    HandleMapService<HandleT>::Get().SetHandle(arg.Key, arg.Value);
  }
}

// A deliberately, permanently invalid GL object name - no real Gen/Create call will ever hand
// this out - substituted below for a captured name whose own create call the recorder never
// observed (recorded as UNMAPPED_OBJECT_NAME_KEY; see this header's own comment on
// UpdateHandle(HandleArgument<GLuint>&, GLObjectKind) for when that happens). Matches the
// legacy OpenGL backend's own identical fallback (OpenGL/common/include/mapping.h's
// CMappedArgument<...>::GetMapping(): "LOG_WARNING << \"Couldn't map object name\" ...; return
// ... 1000000") - same value, same rationale: replaying the raw, capture-time name unchanged
// risks silently aliasing an unrelated, valid object that happens to already have that same
// numeric name at replay time (a real object, e.g. a texture the game legitimately created
// with name 774 for something else entirely), which is far more dangerous than a real call
// simply failing (GL_INVALID_VALUE/no-op) against a name so large no Gen call would plausibly
// have produced it.
inline constexpr GLuint kUnmappedObjectName = 1000000;

// Existing-name resolution before a real call: substitute the decoded Key's live, player-local
// name for Value - glBindTexture/glBindBuffer/glUseProgram/glBindAttribLocation/
// glGetUniformLocation's object-name parameter, and glDeleteProgram/glDeleteShader's
// about-to-be-destroyed name. Only the key is decoded, so Value starts at 0. A zero key (e.g.
// glBindTexture(target, 0) to unbind) replays name 0 without a lookup or warning, mirroring
// UpdateHandle(HandleArgument<HGLRC>&)'s zero-key handling above exactly.
// UNMAPPED_OBJECT_NAME_KEY means the recorder itself never saw this name's own create call (see
// ObjectNameMapService::GetKey's "no registered key (never created, or created before
// recording started)" warning) - substitute kUnmappedObjectName; see that constant's own
// comment for why.
inline void UpdateHandle(HandleArgument<GLuint>& arg, GLObjectKind kind) {
  if (arg.Key == UNMAPPED_OBJECT_NAME_KEY) {
    arg.Value = kUnmappedObjectName;
    return;
  }
  arg.Value = arg.Key == 0 ? 0 : ObjectNameMapService::Get().GetHandle(kind, arg.Key);
}

// After a real glCreateProgram/glCreateShader call succeeds: register the newly-created live
// name under the key that was already decoded onto this same field (see
// object_name_lifecycle.py's CREATE role). A zero Value (a failed create) is left unregistered.
inline void UpdateOutputHandle(HandleArgument<GLuint>& arg, GLObjectKind kind) {
  if (arg.Value != 0) {
    ObjectNameMapService::Get().SetHandle(kind, arg.Key, arg.Value);
  }
}

// Player-side counterpart of the recorder's UpdateHandleIfKnown/ConfirmImplicitCreate (see
// their header comments for exactly which commands use this and why): resolves an already-known
// key normally, but for a key this replay has never seen before - not a stream gap, since the
// recorder only mints a key here when it confirmed a real implicit create at capture time (see
// object_name_lifecycle.py's ObjectNameRole.LOOKUP_OR_CREATE) - creates a fresh, replay-time
// name through `gen`, the recorded bind's own family's Gen entry point (an APPLE vertex array
// name is not valid for core glBindVertexArray, nor the reverse), and registers it, exactly as
// UpdateOutputHandle would for an ordinary Gen call's result. An unresolved `gen` leaves the
// name 0, as if that Gen call itself had failed. Uses TryGetHandle (not GetHandle) so this
// expected, first-ever-reference miss never logs GetHandle's "no registered name" warning - see
// TryGetHandle's own header comment (objectNameMapService.h). A zero key (unbind) and
// UNMAPPED_OBJECT_NAME_KEY (a bind the recorder saw rejected) are handled exactly as by the
// plain UpdateHandle overload above.
template <typename GenFn>
inline void UpdateHandleOrCreate(HandleArgument<GLuint>& arg, GLObjectKind kind, GenFn gen) {
  if (arg.Key == 0 || arg.Key == UNMAPPED_OBJECT_NAME_KEY) {
    UpdateHandle(arg, kind);
    return;
  }
  ObjectNameMapService& service = ObjectNameMapService::Get();
  GLuint liveName = service.TryGetHandle(kind, arg.Key);
  if (liveName == 0) {
    if (gen != nullptr) {
      gen(1, &liveName);
    }
    service.SetHandle(kind, arg.Key, liveName);
  }
  arg.Value = liveName;
}

// Existing-name resolution for an array of GL object names (glDeleteTextures' `textures`, and
// any other LOOKUP/DELETE-role array field - see commandRunnersAuto.cpp.mako's own comment)
// before a real call - substitutes each element's live, player-local name for Value from its
// decoded Key, mirroring the scalar UpdateHandle above applied elementwise. Value and Key are
// always the same length (see arguments.h's HandleArrayArgument<T>).
//
// Deliberately NOT given the scalar UpdateHandle's kUnmappedObjectName substitution above: a
// per-element Key of 0 (the recorder never observed that element's own create call) already
// resolves to Value 0 here unconditionally, via GetHandle(kind, 0)'s own documented "a zero key
// returns 0 silently" behavior (objectNameMapService.h) - never the dangerous raw capture-time
// name the scalar overload used to leave in place. A 0 element is already the GL-defined safe
// no-op for every current caller of this array overload (glDeleteTextures/glDeleteBuffers/...
// silently ignore a 0 entry; a 0 in a multi-bind array unbinds that unit) - strictly safer than
// substituting kUnmappedObjectName, which would risk a real GL_INVALID_VALUE where GL itself
// already defines a well-behaved no-op.
inline void UpdateHandleArray(HandleArrayArgument<GLuint>& arg, GLObjectKind kind) {
  for (size_t i = 0; i < arg.Key.size(); ++i) {
    arg.Value[i] = ObjectNameMapService::Get().GetHandle(kind, arg.Key[i]);
  }
}

// After a real glGenTextures/glGenBuffers call: register each newly-created live name under
// the key that was already decoded onto the same element (see object_name_lifecycle.py's
// CREATE role on an array field), mirroring the scalar UpdateOutputHandle above applied
// elementwise.
inline void UpdateOutputHandleArray(HandleArrayArgument<GLuint>& arg, GLObjectKind kind) {
  for (size_t i = 0; i < arg.Value.size(); ++i) {
    ObjectNameMapService::Get().SetHandle(kind, arg.Key[i], arg.Value[i]);
  }
}

} // namespace gits::opengl2

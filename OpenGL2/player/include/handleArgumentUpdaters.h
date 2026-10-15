// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// Player-side counterpart of recorder/include/handleArgumentUpdaters.h, resolving the
// opposite direction (GITSKey -> live HGLRC, instead of HGLRC -> GITSKey) - see
// docs/development/opengl2-player-skeleton-plan.md's Decision 5 and
// modern-opengl-backend-architecture.md's "Real key-based mapping for handles GITS creates".
// Hand-written, not generated, for the same reason the recorder's own copy is: HGLRC is the
// only HandleArgument<T> instantiation ContextHandleMapService needs, so there is nothing for
// codegen to loop over there.
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
// The GLuint overloads below are plain, non-templated overloads taking an explicit
// GLObjectKind parameter, for the same reason the recorder-side copy's GLuint overloads are -
// see that file's header comment.

#include "arguments.h"
#include "contextHandleMapService.h"
#include "glTypes.h"
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

// Existing-handle resolution before a real call: substitute the decoded Key's live handle
// for Value (still whatever Decode() zero-initialized it to) - wglMakeCurrent,
// wglDeleteContext, wglShareLists, etc. A zero key (e.g. wglMakeCurrent's hglrc when
// detaching) is left at a null Value without a lookup or warning - it is a normal value, not
// a missing registration.
inline void UpdateHandle(HandleArgument<HGLRC>& arg) {
  if (arg.Key == 0) {
    return;
  }
  arg.Value = ContextHandleMapService::Get().GetHandle(arg.Key);
}

// After a real wglCreateContext(ARB) call succeeds: register the newly-created live handle
// under the key that was already decoded onto this same field (see handle_lifecycle.py's
// HANDLE_CREATING_COMMANDS, which recorderWrappersAuto.cpp.mako's own UpdateOutputHandle call
// consults for the same purpose). A null Value (a failed create) is left unregistered.
inline void UpdateOutputHandle(HandleArgument<HGLRC>& arg) {
  if (arg.Value) {
    ContextHandleMapService::Get().SetHandle(arg.Key, arg.Value);
  }
}

// Existing-name resolution before a real call: substitute the decoded Key's live, player-local
// name for Value - glBindTexture/glBindBuffer/glUseProgram/glBindAttribLocation/
// glGetUniformLocation's object-name parameter, and glDeleteProgram/glDeleteShader's
// about-to-be-destroyed name. A zero key is left at a zero Value without a lookup or warning,
// mirroring UpdateHandle(HandleArgument<HGLRC>&)'s zero-key handling above exactly.
inline void UpdateHandle(HandleArgument<GLuint>& arg, GLObjectKind kind) {
  if (arg.Key == 0) {
    return;
  }
  arg.Value = ObjectNameMapService::Get().GetHandle(kind, arg.Key);
}

// After a real glCreateProgram/glCreateShader call succeeds: register the newly-created live
// name under the key that was already decoded onto this same field (see
// object_name_lifecycle.py's CREATE role). A zero Value (a failed create) is left unregistered.
inline void UpdateOutputHandle(HandleArgument<GLuint>& arg, GLObjectKind kind) {
  if (arg.Value != 0) {
    ObjectNameMapService::Get().SetHandle(kind, arg.Key, arg.Value);
  }
}

// Existing-name resolution for an array of GL object names (glDeleteTextures' `textures`)
// before a real call - substitutes each element's live, player-local name for Value from its
// decoded Key, mirroring the scalar UpdateHandle above applied elementwise. Value and Key are
// always the same length (see arguments.h's HandleArrayArgument<T>).
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

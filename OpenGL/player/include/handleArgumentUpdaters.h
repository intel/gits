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
// docs/development/opengl-player-skeleton-plan.md's Decision 5 and
// modern-opengl-backend-architecture.md's "Real key-based mapping for handles GITS creates".
// Hand-written, not generated, for the same reason the recorder's own copy is: no command's
// param/return-value shape here needs anything commandRunnersAuto.cpp.mako couldn't already
// express with a plain call - see that template's own header comment for the generated side's
// `UpdateHandle(command.m_${p.name})` call sites, which resolve against
// UpdateHandle/UpdateOutputHandle's HandleArgument<HandleT> template below via ordinary C++
// template argument deduction, with no per-type codegen branching needed on either side.
//
// Named identically to the recorder-side copy's UpdateHandle/UpdateOutputHandle (see
// docs/development/opengl-layer-architecture-alignment-plan.md's Phase A) - mirrors Vulkan's
// own convention exactly (Vulkan/recorder/handleArgumentUpdaters.h and
// Vulkan/player/handleArgumentUpdaters.h both use the same two names, differing only by which
// manager type resolution runs against). Previously named ResolveHandle/RegisterCreatedHandle/
// ResolveHandleArray/RegisterCreatedHandleArray here - purely a rename, no behavior change;
// the old names read reasonably in isolation but, unlike Vulkan's shared naming, gave no hint
// that this is the same key<->handle protocol as the recorder side, which made it unclear
// whether OpenGL actually implements Vulkan's pattern at all (it does).
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
#include "command.h"
#include "glTypes.h"
#include "handleMapService.h"
#include "log.h"
#include "objectNameMapService.h"
#include "openglDriversAuto.h"

#include <atomic>
#include <cstddef>
#include <mutex>
#include <string>
#include <type_traits>
#include <unordered_set>

namespace gits::opengl {

// Returns the real, replay-time HGLRC (GLXContext elsewhere) current on this thread, by calling
// the real wglGetCurrentContext() (glXGetCurrentContext()) directly rather than reading any
// decoded Command state - player-side counterpart of recorder/include/handleArgumentUpdaters.h's
// CurrentShareGroup() (see that function's own comment for why calling the real driver directly,
// instead of tracking this backend's own notion of "current context", is safe and current at any
// point a wrapper/Run() executes). Used by UniformLocationMapService's callers (commandRunnersAuto.cpp.mako's
// glUseProgram hook and its generic is_uniform_location_param resolve hook) to key "current
// program" per real context instead of process-globally - see that service's own header
// comment for why program-in-use is per-context GL state, not per-share-group or process-wide.
#if defined(_WIN32)
inline HGLRC CurrentContext() {
  auto realFunction = player::g_DispatchTable.wglGetCurrentContext.load(std::memory_order_acquire);
  return realFunction ? realFunction() : nullptr;
}
#else
inline GLXContext CurrentContext() {
  auto realFunction = player::g_DispatchTable.glXGetCurrentContext.load(std::memory_order_acquire);
  return realFunction ? realFunction() : nullptr;
}
#endif

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

// Skips `command`'s real call, which references an object name whose create call the recorder
// never saw (UNMAPPED_OBJECT_NAME_KEY). No replay-time name stands in for it safely: every
// nonzero name can belong to a live object, and a legacy bind implicitly creates any name it is
// given, so a substitute could alias an object or make the rejected call succeed. Warns once per
// command.
inline void SkipUnmappedObjectNameCall(Command& command, const char* commandName) {
  command.m_Skip = true;
  static std::mutex mutex;
  static std::unordered_set<std::string> warned;
  std::lock_guard<std::mutex> lock(mutex);
  if (warned.insert(commandName).second) {
    LOG_WARNING << commandName
                << " references an object name that was not created during recording; calls "
                   "doing so are skipped";
  }
}

// Existing-name resolution before a real call: substitute the decoded Key's live, player-local
// name for Value - glBindTexture/glBindBuffer/glUseProgram/glBindAttribLocation/
// glGetUniformLocation's object-name parameter, and glDeleteProgram/glDeleteShader's
// about-to-be-destroyed name. Only the key is decoded, so Value starts at 0. A zero key (e.g.
// glBindTexture(target, 0) to unbind) replays name 0 without a lookup or warning, mirroring
// UpdateHandle(HandleArgument<HGLRC>&)'s zero-key handling above exactly.
// UNMAPPED_OBJECT_NAME_KEY means the recorder itself never saw this name's own create call (see
// ObjectNameMapService::GetKey's "no registered key (never created, or created before
// recording started)" warning): Value stays 0 and false is returned, for the caller to skip the
// call with SkipUnmappedObjectNameCall.
[[nodiscard]] inline bool UpdateHandle(HandleArgument<GLuint>& arg, GLObjectKind kind) {
  if (arg.Key == UNMAPPED_OBJECT_NAME_KEY) {
    arg.Value = 0;
    return false;
  }
  arg.Value = arg.Key == 0 ? 0 : ObjectNameMapService::Get().GetHandle(kind, arg.Key);
  return true;
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
// plain UpdateHandle overload above, including its return value.
//
// Where `gen` yields no name although the call itself is legal - glGenQueries while another
// query is active, on some drivers - an unused name is picked instead, with `isObject` (the
// family's glIs* entry point), for the call to create exactly as it did at capture.
template <typename GenFn, typename IsObjectFn = std::nullptr_t>
[[nodiscard]] inline bool UpdateHandleOrCreate(HandleArgument<GLuint>& arg,
                                               GLObjectKind kind,
                                               GenFn gen,
                                               IsObjectFn isObject = nullptr) {
  if (arg.Key == 0 || arg.Key == UNMAPPED_OBJECT_NAME_KEY) {
    return UpdateHandle(arg, kind);
  }
  ObjectNameMapService& service = ObjectNameMapService::Get();
  GLuint liveName = service.TryGetHandle(kind, arg.Key);
  if (liveName == 0) {
    if (gen != nullptr) {
      gen(1, &liveName);
    }
    if constexpr (!std::is_same_v<IsObjectFn, std::nullptr_t>) {
      if (liveName == 0 && isObject != nullptr) {
        liveName =
            service.UnusedName(kind, [isObject](GLuint name) { return isObject(name) != 0; });
      }
    }
    service.SetHandle(kind, arg.Key, liveName);
  }
  arg.Value = liveName;
  return true;
}

// Existing-name resolution for an array of GL object names (glDeleteTextures' `textures`, and
// any other LOOKUP/DELETE-role array field - see commandRunnersAuto.cpp.mako's own comment)
// before a real call - substitutes each element's live, player-local name for Value from its
// decoded Key, mirroring the scalar UpdateHandle above applied elementwise. Value and Key are
// always the same length (see arguments.h's HandleArrayArgument<T>).
//
// Each element is resolved like the scalar UpdateHandle: key 0 is a captured zero name and
// replays as 0, and UNMAPPED_OBJECT_NAME_KEY (a lookup-array name the recorder never saw
// created) makes this return false, for the caller to skip the whole call rather than unbind
// that element or alias another object through it. The recorder never writes that key into
// delete-role arrays: GL ignores unused names there, so a miss is key 0 and replays as a no-op
// 0 entry.
[[nodiscard]] inline bool UpdateHandleArray(HandleArrayArgument<GLuint>& arg, GLObjectKind kind) {
  bool mapped = true;
  for (size_t i = 0; i < arg.Key.size(); ++i) {
    if (arg.Key[i] == UNMAPPED_OBJECT_NAME_KEY) {
      arg.Value[i] = 0;
      mapped = false;
      continue;
    }
    arg.Value[i] = ObjectNameMapService::Get().GetHandle(kind, arg.Key[i]);
  }
  return mapped;
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

} // namespace gits::opengl

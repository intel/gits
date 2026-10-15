// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// Hand-written, not generated: no command's param/return-value shape here needs anything
// recorderWrappersAuto.cpp.mako couldn't already express with a plain call - see that
// template's own header comment. Mirrors the shape of Vulkan's handleArgumentUpdaters.h
// (UpdateHandle for a lookup, UpdateOutputHandle for a mint-and-register). UpdateHandle/
// UpdateOutputHandle for HandleArgument<HandleT> below are a `template <typename HandleT>`
// pair, generic over every mapped-handle type (HGLRC, GLsync, ...) via
// HandleMapService<HandleT> (see common/services/handleMapService.h) - they used to be two
// hand-copied, non-templated overload pairs (one for HGLRC, one for GLsync) before GLsync's
// own arrival prompted generalizing HandleMapService itself into a template; a third
// mapped-handle type needs zero new overloads here, only a new HandleMapService<T> alias.
//
// The GLuint overloads below (UpdateHandle/UpdateOutputHandle for a single GL object name,
// UpdateHandleArray/UpdateOutputHandleArray for an array of them) are plain, non-templated
// overloads taking an explicit GLObjectKind parameter, NOT folded into the single-argument
// `template <typename HandleT> UpdateHandle(HandleArgument<HandleT>&)` pair above - see
// docs/development/opengl2-object-name-mapping-plan.md: every GL object-name namespace
// (texture, buffer, program, shader, ...) is the same C++ type, GLuint, so a template
// specialized purely on T could never distinguish HandleArgument<HandleT> (already handled
// above, for a real mapped-handle type like HGLRC/GLsync - no GLObjectKind involved) from
// HandleArgument<GLuint> (always needs one) - and more directly, a create/lookup call always
// needs to know *which* namespace it's creating/looking up in, which is a per-call-site fact
// (see object_name_lifecycle.py's OBJECT_NAME_PARAM_OVERRIDES), not something inferable from T
// alone the way it is for a genuinely mapped handle type. Plain overloads taking an explicit
// GLObjectKind parameter make this explicit at every call site (recorderWrappersAuto.cpp.mako)
// instead of hiding it behind template deduction; arity (one arg vs. two) keeps the two
// UpdateHandle/UpdateOutputHandle families from ever colliding at a call site.

#include "arguments.h"
#include "captureManager.h"
#include "glTypes.h"
#include "handleMapService.h"
#include "objectNameMapService.h"
#include "openglDriversAuto.h"
#include "shareGroupService.h"

namespace gits::opengl2 {

// The share group of whichever HGLRC is current on this thread right now, per the real
// driver - see shareGroupService.h's own header comment for why GL object names must be
// bucketed by this instead of by GLObjectKind alone. Queried directly from the
// real driver rather than tracked separately here, mirroring recorder/clientMemoryCapture.cpp's
// precedent of calling real GL/WGL functions directly from recorder helpers to introspect state
// that isn't itself a captured command parameter (see that file's QueryBoundBufferQuiet).
// Safe to call at any point a GL wrapper runs: a context, once made current on a thread, stays
// current until that thread's own next wglMakeCurrent call, so this always reflects the
// context the in-flight wrapped call is actually being made against.
inline uint64_t CurrentShareGroup() {
  auto realFunction = g_DispatchTable.wglGetCurrentContext.load(std::memory_order_acquire);
  return ShareGroupService::Get().GroupFor(realFunction ? realFunction() : nullptr);
}

// The `shareGroup` argument HandleMapService<HandleT>::Register/GetKey/Erase should use for
// `handle`'s own recorder-direction map - see that class's own header comment
// (common/services/handleMapService.h) for the full rationale. The generic (unspecialized)
// template returns the default, process-global bucket (0), correct for HGLRC: a context handle
// IS the identity ShareGroupService::GroupFor() itself keys off, so scoping it by its own group
// is a chicken-and-egg problem. The GLsync overload below returns the real CurrentShareGroup()
// instead, since GLsync tokens are only guaranteed unique within a share group (see
// handleMapService.h's own comment on why, and the legacy OpenGL backend's identical
// OGL_SYNC_MAP/SharedGLObjectTraits classification, OpenGL/common/include/mapping.h). Ordinary
// overloading (not a class template specialization) picks the right one at every UpdateHandle/
// UpdateOutputHandle call site below purely from `arg.Value`'s own C++ type - a third
// share-group-scoped mapped-handle type would need only its own overload here, no changes to
// UpdateHandle/UpdateOutputHandle themselves.
template <typename HandleT>
inline uint64_t HandleShareGroupFor(HandleT /*handle*/) {
  return 0;
}

inline uint64_t HandleShareGroupFor(GLsync /*handle*/) {
  return CurrentShareGroup();
}

// The bucketing identity ObjectNameMapService should use for `kind`'s recorder-direction
// (name -> key) map - see that service's own header comment on why a GLuint name is only
// unique within *some* scope narrower than the whole process, and ShareGroupService's header
// comment for the GL spec's own "Shared Objects and Multiple Contexts" rule this implements.
// That rule's list of container objects never shared between contexts - not even two contexts
// in the same share group that share every other object type (buffers, textures, programs,
// shaders, renderbuffers, samplers, ...) - is exactly {framebuffer, program pipeline, query,
// transform feedback, vertex array}: bucketing any of these five by share group like every
// other kind would let two sharing contexts' independently-created object of the same name
// collide in the same map bucket. All five are instead bucketed by the current context's own
// raw identity (its live HGLRC value, always distinct across live contexts, sharing or not)
// rather than by CurrentShareGroup()'s share-group id, which two sharing contexts would
// otherwise have in common. Every other kind continues to use the share group, since those
// object types genuinely are shared and must collide-check across the whole group.
inline uint64_t GroupIdentityFor(GLObjectKind kind) {
  if (kind == GLObjectKind::Framebuffer || kind == GLObjectKind::VertexArray ||
      kind == GLObjectKind::ProgramPipeline || kind == GLObjectKind::TransformFeedback ||
      kind == GLObjectKind::Query) {
    auto realFunction = g_DispatchTable.wglGetCurrentContext.load(std::memory_order_acquire);
    return reinterpret_cast<uint64_t>(realFunction ? realFunction() : nullptr);
  }
  return CurrentShareGroup();
}

// Existing-handle lookup: wglMakeCurrent, wglDeleteContext (the handle being destroyed - see
// handle_lifecycle.py's HANDLE_PARAM_TO_ERASE_ON_SUCCESS, which erases separately after this
// runs), wglShareLists, wglGetCurrentContext's return value, glIsSync, glClientWaitSync,
// glWaitSync, glDeleteSync's `sync` parameter, etc. Generic over every mapped-handle type
// (HGLRC, GLsync, ...) via HandleMapService<HandleT> (see that class's own header comment,
// common/services/handleMapService.h, for why one template - not one hand-copied overload per
// type - is now the right shape here). A null handle (e.g. wglMakeCurrent's hglrc when
// detaching) is left at key 0 without a lookup or warning - it is a normal value, not a missing
// registration; no enabled GLsync-typed field ever passes a null `sync` as meaningful GL usage
// either, but treating it as "missing" would be a spurious warning, not a real bug report.
template <typename HandleT>
inline void UpdateHandle(HandleArgument<HandleT>& arg) {
  if (!arg.Value) {
    return;
  }
  arg.Key = HandleMapService<HandleT>::Get().GetKey(arg.Value, HandleShareGroupFor(arg.Value));
}

// New-handle mint: only for a handle-creating command's return value (wglCreateContext(ARB),
// glFenceSync(APPLE) - see handle_lifecycle.py's HANDLE_CREATING_COMMANDS) - every other field
// of this handle type is an existing handle and uses UpdateHandle above. A null handle (a
// failed create) is left unregistered. Generic over every mapped-handle type, same rationale as
// UpdateHandle above. The key is minted here (via CaptureManager, a recorder-only concept)
// rather than inside HandleMapService<HandleT>::Register itself, since that service also has to
// build cleanly for the player - see common/services/handleMapService.h's header comment.
template <typename HandleT>
inline void UpdateOutputHandle(HandleArgument<HandleT>& arg) {
  if (!arg.Value) {
    return;
  }
  arg.Key = CaptureManager::Get().CreateHandleKey();
  HandleMapService<HandleT>::Get().Register(arg.Value, arg.Key, HandleShareGroupFor(arg.Value));
}

// Existing-name lookup for a single GL object name (glBindTexture/glBindBuffer/glUseProgram/
// glBindAttribLocation/glGetUniformLocation's object-name parameter, and
// glDeleteProgram/glDeleteShader's about-to-be-destroyed name - see object_name_lifecycle.py's
// LOOKUP/DELETE roles, both of which need this same pre-call lookup). A zero name (e.g.
// glBindTexture(target, 0) to unbind) is left at key 0 without a lookup or warning, mirroring
// UpdateHandle(HandleArgument<HGLRC>&)'s null-handle handling above exactly.
inline void UpdateHandle(HandleArgument<GLuint>& arg, GLObjectKind kind) {
  if (arg.Value == 0) {
    return;
  }
  arg.Key = ObjectNameMapService::Get().GetKey(kind, GroupIdentityFor(kind), arg.Value);
}

// New-name create for a single GL object name (glCreateProgram/glCreateShader's return value -
// see object_name_lifecycle.py's CREATE role). A zero name (a failed create) is left
// unregistered, mirroring UpdateOutputHandle(HandleArgument<HGLRC>&)'s null-handle handling
// above exactly.
inline void UpdateOutputHandle(HandleArgument<GLuint>& arg, GLObjectKind kind) {
  if (arg.Value == 0) {
    return;
  }
  arg.Key = CaptureManager::Get().CreateHandleKey();
  ObjectNameMapService::Get().Register(kind, GroupIdentityFor(kind), arg.Value, arg.Key);
}

// Existing-name lookup for an array of GL object names (glDeleteTextures' `textures` - see
// object_name_lifecycle.py's DELETE role on an array field). Runs before the real driver call,
// same timing as the scalar UpdateHandle above - the names must still be resolvable to keys
// before ObjectNameMapService::Delete removes them post-call (see recorderWrappersAuto.cpp.mako).
// Value and Key are always the same length (see arguments.h's HandleArrayArgument<T>).
inline void UpdateHandleArray(HandleArrayArgument<GLuint>& arg, GLObjectKind kind) {
  const uint64_t groupIdentity = GroupIdentityFor(kind);
  for (size_t i = 0; i < arg.Value.size(); ++i) {
    arg.Key[i] = ObjectNameMapService::Get().GetKey(kind, groupIdentity, arg.Value[i]);
  }
}

// New-name create for an array of GL object names (glGenTextures'/glGenBuffers' array
// parameter - see object_name_lifecycle.py's CREATE role on an array field). Runs after
// Capture() has copied the driver's newly-written names into Value (see
// recorderWrappersAuto.cpp.mako) - one fresh key per element, mirroring the scalar
// UpdateOutputHandle above applied elementwise.
inline void UpdateOutputHandleArray(HandleArrayArgument<GLuint>& arg, GLObjectKind kind) {
  const uint64_t groupIdentity = GroupIdentityFor(kind);
  for (size_t i = 0; i < arg.Value.size(); ++i) {
    arg.Key[i] = CaptureManager::Get().CreateHandleKey();
    ObjectNameMapService::Get().Register(kind, groupIdentity, arg.Value[i], arg.Key[i]);
  }
}

} // namespace gits::opengl2

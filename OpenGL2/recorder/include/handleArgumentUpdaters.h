// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// Hand-written, not generated: HGLRC is the only HandleArgument<T> instantiation
// ContextHandleMapService needs, so there is nothing for codegen to loop over there. Mirrors
// the shape of Vulkan's handleArgumentUpdaters.h (UpdateHandle for a lookup, UpdateOutputHandle
// for a mint-and-register), minus the template-over-every-handle-type machinery that only pays
// off once a second mapped-handle type shows up - at which point this can grow a template +
// explicit specializations, or be regenerated the way Vulkan's is.
//
// The GLuint overloads below (UpdateHandle/UpdateOutputHandle for a single GL object name,
// UpdateHandleArray/UpdateOutputHandleArray for an array of them) are plain, non-templated
// overloads rather than a `template <typename T> UpdateHandle(HandleArgument<T>&)` - see
// docs/development/opengl2-object-name-mapping-plan.md: every GL object-name namespace
// (texture, buffer, program, shader, ...) is the same C++ type, GLuint, so a template
// specialized purely on T could never distinguish HandleArgument<HGLRC> (already handled below,
// no GLObjectKind involved) from HandleArgument<GLuint> (always needs one) - and more directly,
// a create/lookup call always needs to know *which* namespace it's creating/looking up in,
// which is a per-call-site fact (see object_name_lifecycle.py's OBJECT_NAME_PARAM_OVERRIDES), not
// something inferable from T alone the way it is for HGLRC's single-namespace case. Plain
// overloads taking an explicit GLObjectKind parameter make this explicit at every call site
// (recorderWrappersAuto.cpp.mako) instead of hiding it behind template deduction.

#include "arguments.h"
#include "captureManager.h"
#include "contextHandleMapService.h"
#include "glTypes.h"
#include "objectNameMapService.h"
#include "openglDriversAuto.h"
#include "shareGroupService.h"

namespace gits::opengl2 {

// The share group of whichever HGLRC is current on this thread right now, per the real
// driver - see common/services/shareGroupService.h's own header comment for why GL object
// names must be bucketed by this instead of by GLObjectKind alone. Queried directly from the
// real driver rather than tracked separately here, mirroring recorder/mapTrackingWrappers.cpp's
// existing precedent of calling real GL/WGL functions directly from wrapper code to introspect
// state that isn't itself a captured command parameter (see that file's QueryBoundBufferQuiet).
// Safe to call at any point a GL wrapper runs: a context, once made current on a thread, stays
// current until that thread's own next wglMakeCurrent call, so this always reflects the
// context the in-flight wrapped call is actually being made against.
inline uint64_t CurrentShareGroup() {
  auto realFunction = g_DispatchTable.wglGetCurrentContext.load(std::memory_order_acquire);
  return ShareGroupService::Get().GroupFor(realFunction ? realFunction() : nullptr);
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
// generator's _HANDLE_PARAM_TO_ERASE_ON_SUCCESS, which erases separately after this runs),
// wglShareLists, wglGetCurrentContext's return value, etc. A null handle (e.g.
// wglMakeCurrent's hglrc when detaching) is left at key 0 without a lookup or warning - it
// is a normal value, not a missing registration.
inline void UpdateHandle(HandleArgument<HGLRC>& arg) {
  if (!arg.Value) {
    return;
  }
  arg.Key = ContextHandleMapService::Get().GetKey(arg.Value);
}

// New-handle mint: only for wglCreateContext/wglCreateContextAttribsARB's return value (see
// generator's _HANDLE_CREATING_COMMANDS) - every other HGLRC-typed field is an existing
// handle and uses UpdateHandle above. A null handle (a failed create) is left unregistered.
// The key is minted here (via CaptureManager, a recorder-only concept) rather than inside
// ContextHandleMapService::Register itself, now that the service also has to build cleanly
// for the player - see common/services/contextHandleMapService.h's header comment.
inline void UpdateOutputHandle(HandleArgument<HGLRC>& arg) {
  if (!arg.Value) {
    return;
  }
  arg.Key = CaptureManager::Get().CreateHandleKey();
  ContextHandleMapService::Get().Register(arg.Value, arg.Key);
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

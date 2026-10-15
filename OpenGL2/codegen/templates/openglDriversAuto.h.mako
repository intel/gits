<%page args="commands, namespace='gits::opengl2'"/>\
// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${AUTO_GENERATED_HEADER}

// Flat dispatch table holding one real opengl32.dll function pointer per
// intercepted command (see codegen/generator_interceptor.py). Single global
// instance `g_DispatchTable` - no per-instance/per-device tiers, unlike
// Vulkan's dispatch tables, since GL has no such concept.
//
// Each member starts out pointing at its own generated resolve-thunk (see
// openglDriversAuto.cpp). The first time a member is called, its thunk
// resolves the real function and overwrites the member with it directly, so
// every subsequent call goes straight to the real function. A function that
// fails to resolve keeps calling its own thunk, which retries resolution on
// each call.
//
// Resolution happens lazily, per-function, on first actual use - not
// eagerly for the whole table at once - because most functions beyond GL1.1
// and core WGL aren't statically exported by the real opengl32.dll at all
// (only resolvable via wglGetProcAddress, and only once a context is
// current), so an eager one-shot pass at load time would leave most of the
// table permanently unresolved.
//
// `namespace` defaults to the recorder's own gits::opengl2 - the player instantiates this
// same template a second time under gits::opengl2::player (see
// codegen/generator_player.py), producing an independent dispatch table/thunk set with no
// hooking concerns of its own (see OpenGL2/player/include/realDriver.h).

#pragma once

#include "arguments.h"
#include "glTypes.h"

#include <atomic>

namespace ${namespace} {

% for cmd in commands:
<%
    ret = cmd.return_value.type.strip()
    param_decls = ', '.join(f'{p.type} {p.name}' for p in cmd.params)
%>\
${ret} APIENTRY ResolveAndCall_${cmd.name}(${param_decls});
% endfor

struct OpenGLDispatchTable {
% for cmd in commands:
<%
    ret = cmd.return_value.type.strip()
    param_types = ', '.join(p.type for p in cmd.params)
%>\
  using ${cmd.name}Ptr = ${ret} (APIENTRY*)(${param_types});
  std::atomic<${cmd.name}Ptr> ${cmd.name}{&ResolveAndCall_${cmd.name}};
% endfor
};

extern OpenGLDispatchTable g_DispatchTable;

// Stores an already-known-good real function pointer directly into the
// dispatch-table member for `name`, if we have one - a no-op otherwise.
// Used by the hand-written wglGetProcAddress override (interceptorCustom.cpp)
// to cache the pointer it just got from the real driver, at the moment it's
// known to be valid (e.g. while the app's own temporary/bootstrap context is
// still current) - without this, the first actual call through the
// trampoline we handed out would instead trigger that member's own
// resolve-thunk from scratch, which can fail if conditions have changed
// since (e.g. the bootstrap context was already destroyed).
bool TryCacheRealFunction(const char* name, void* value);

// Proactively resolves and caches every command's real function pointer that isn't cached yet
// (best-effort - failures are silently skipped, leaving that entry's lazy resolve-thunk in
// place for a later retry). Meant to be called right after a real wglMakeCurrent call makes
// `context` current, since that is the only reliable window in which wglGetProcAddress can
// resolve extension functions (see openglDriversAuto.cpp's ResolveDriverFunction) - resolving
// lazily on first *use* instead is too late once that context has since been released (e.g. the
// common "create dummy context -> fetch ARB functions -> destroy it -> create the real
// context" bootstrap pattern also documented on the recorder's WglGetProcAddress override in
// interceptorCustom.cpp, which solves the identical problem the other way - by hooking the
// app's own wglGetProcAddress calls, which the player has no equivalent of).
//
// Skips its whole resolution pass entirely for a context this function has already warmed up
// before (see openglDriversAuto.cpp's own already-warmed-contexts cache) - an application that
// reattaches the same context every frame (a common render-loop shape) would otherwise repeat a
// wglGetProcAddress/GetProcAddress probe for every still-unresolved (almost always never-
// supported-by-this-driver) enabled extension once per frame, forever, since a failed probe
// leaves that dispatch entry at its lazy-resolve thunk for the next retry - see this function's
// own definition for the full rationale.
//
// Takes the context's stream-stable GITSKey (WglMakeCurrentRunner::Run()'s own
// command.m_newContext.Key), not its live, replay-time HGLRC: a GITSKey is minted once per
// logical context and never reused, whereas Windows CAN and does reuse a destroyed context's
// numeric HGLRC value for a later, entirely unrelated wglCreateContext[AttribsARB] call - keying
// the already-warmed cache on the raw handle would then incorrectly treat that new, genuinely
// different context as already warmed and skip it, defeating the very bootstrap case this
// function exists for (see this function's own definition for the full rationale).
void WarmUpResolvableFunctions(GITSKey contextKey);

} // namespace ${namespace}

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

#pragma once

#include "glTypes.h"

#include <atomic>

namespace gits::opengl2 {

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

} // namespace gits::opengl2

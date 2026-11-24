// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <utility>

namespace gits::opengl {

// TODO: When player and recorder share driver dispatch tables, these helpers could be used also in
// the player. Keep cached results scoped to the owning context of course.

// Whether the current context has glCopyBufferSubData (GL 3.1 / ARB_copy_buffer). Cached per
// context, as are the other feature predicates below.
bool ContextSupportsCopyBuffer();

// Whether the current context has core direct state access (GL 4.5 / ARB_direct_state_access).
// Cached per context, like ContextSupportsCopyBuffer().
bool ContextSupportsDirectStateAccess();

// Whether the current context has glGetInternalformativ's format queries (GL 4.3 /
// ARB_internalformat_query2). Cached per context, like ContextSupportsCopyBuffer().
bool ContextSupportsInternalformatQuery2();

// (major, minor) parsed from the current context's GL_VERSION; (0, 0) if it cannot be parsed,
// which callers treat as "supports nothing".
std::pair<int, int> CurrentContextGLVersion();

// Whether the current context lists `extensionName`. Pass its version from
// CurrentContextGLVersion() to select the legal enumeration form.
bool CurrentContextHasExtension(std::pair<int, int> version, const char* extensionName);

// Uncached check using the current context's already queried version.
bool SupportsCoreOrExtension(std::pair<int, int> version,
                             int coreMajor,
                             int coreMinor,
                             const char* extension,
                             const char* alternate = nullptr);

bool ContextSupportsPixelBufferObject();
bool ContextSupportsQueryBufferObject();
bool ContextSupportsBufferObjects();

} // namespace gits::opengl

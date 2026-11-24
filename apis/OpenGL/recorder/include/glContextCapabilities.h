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
// registered context.
bool ContextSupportsCopyBuffer();

// Whether the current context has buffer storage (GL 4.4 / ARB or EXT_buffer_storage).
bool ContextSupportsBufferStorage(std::pair<int, int> version);

// Whether the current context has core direct state access (GL 4.5 / ARB_direct_state_access).
// Cached per context.
bool ContextSupportsDirectStateAccess();

// Whether the current context has glGetInternalformativ's format queries (GL 4.3 /
// ARB_internalformat_query2). Cached per context.
bool ContextSupportsInternalformatQuery2();

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

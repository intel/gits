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

bool ContextSupportsPixelBufferObject();
bool ContextSupportsQueryBufferObject();
bool ContextSupportsBufferObjects();

} // namespace gits::opengl

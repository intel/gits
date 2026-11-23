// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// The context current on the calling thread, which the real driver reports: an HGLRC on Windows
// and a GLXContext elsewhere.
#include "handleMapService.h"
#include "openglDriversAuto.h"

#include <atomic>

namespace gits::opengl {

#if defined(_WIN32)
inline HGLRC CurrentContext() {
  auto realFunction = g_DispatchTable.wglGetCurrentContext.load(std::memory_order_acquire);
  return realFunction ? realFunction() : nullptr;
}
#else
inline GLXContext CurrentContext() {
  auto realFunction = g_DispatchTable.glXGetCurrentContext.load(std::memory_order_acquire);
  return realFunction ? realFunction() : nullptr;
}
#endif

// The key of the current context. 0 if no context is current, or its creation was not captured.
inline GITSKey CurrentContextKey() {
#if defined(_WIN32)
  return ContextHandleMapService::Get().GetKey(CurrentContext());
#else
  return GlxContextHandleMapService::Get().GetKey(CurrentContext());
#endif
}

} // namespace gits::opengl

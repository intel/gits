// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "glContextQueries.h"
#include "handleMapService.h"

namespace gits::opengl {

// The key of the current context. 0 if no context is current, or its creation was not captured.
inline GITSKey CurrentContextKey() {
#if defined(_WIN32)
  return ContextHandleMapService::Get().GetKey(CurrentContext());
#else
  return GlxContextHandleMapService::Get().GetKey(CurrentContext());
#endif
}

} // namespace gits::opengl

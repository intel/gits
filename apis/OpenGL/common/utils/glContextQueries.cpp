// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "glContextQueries.h"

#include "glDispatchAuto.h"
#include "glEnumsAuto.h"
#include "glVersionString.h"

#include <atomic>

namespace gits::opengl {

std::pair<int, int> CurrentContextGLVersion() {
  auto realGetString = g_DispatchTable.glGetString.load(std::memory_order_acquire);
  return ParseGlVersionMajorMinor(reinterpret_cast<const char*>(realGetString(GL_VERSION)));
}

} // namespace gits::opengl

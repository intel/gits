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

// Queries the current context's GL_VERSION. Returns (0, 0) if it cannot be parsed.
std::pair<int, int> CurrentContextGLVersion();

} // namespace gits::opengl

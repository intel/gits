// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "recursionGuard.h"

namespace gits::opengl2 {

thread_local unsigned RecursionGuard::m_RecursionDepth = 0;

RecursionGuard::RecursionGuard() {
  ++m_RecursionDepth;
}

RecursionGuard::~RecursionGuard() {
  --m_RecursionDepth;
}

bool RecursionGuard::IsNested() const {
  return m_RecursionDepth > 1;
}

} // namespace gits::opengl2

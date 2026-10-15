// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

namespace gits::opengl2 {

class RecursionGuard {
public:
  RecursionGuard();
  ~RecursionGuard();

  RecursionGuard(const RecursionGuard&) = delete;
  RecursionGuard& operator=(const RecursionGuard&) = delete;

  bool IsNested() const;

private:
  static thread_local unsigned m_RecursionDepth;
};

} // namespace gits::opengl2

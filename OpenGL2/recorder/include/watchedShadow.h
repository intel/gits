// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <cstddef>

namespace gits::opengl2 {

class WatchedShadow {
public:
  explicit WatchedShadow(size_t size);
  ~WatchedShadow();

  WatchedShadow(const WatchedShadow&) = delete;
  WatchedShadow& operator=(const WatchedShadow&) = delete;

  void* Data() const {
    return m_Data;
  }
  void ResetWriteWatch() const;

private:
  void* m_Data{};
  size_t m_Size;
};

} // namespace gits::opengl2

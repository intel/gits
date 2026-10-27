// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <cstddef>
#include <optional>

namespace gits::opengl2 {

class WatchedShadow {
public:
  struct ByteRange {
    size_t Offset;
    size_t Length;
  };

  explicit WatchedShadow(size_t size);
  ~WatchedShadow();

  WatchedShadow(const WatchedShadow&) = delete;
  WatchedShadow& operator=(const WatchedShadow&) = delete;

  void* Data() const {
    return m_Data;
  }
  void ResetWriteWatch() const;
  // Returns one enclosing range relative to data, clipped to [data, data + size).
  // No touched pages returns std::nullopt.
  std::optional<ByteRange> DirtyRange(const void* data, size_t size) const;

private:
  void* m_Data{};
  size_t m_Size;
};

} // namespace gits::opengl2

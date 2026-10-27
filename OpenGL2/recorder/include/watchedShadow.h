// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace gits::opengl2 {

class WatchedShadow {
public:
  struct ByteRange {
    size_t Offset;
    size_t Length;
  };

  // Whether the shadow tracks the pages written to it, for DirtyRange().
  // TODO: Track writes on Linux once dirty-page capture is prototyped there, see
  // docs/development/opengl2-buffer-map-tracking-plan.md.
#if defined(_WIN32)
  static constexpr bool kTracksWrites = true;
#else
  static constexpr bool kTracksWrites = false;
#endif

  explicit WatchedShadow(size_t size);
  ~WatchedShadow();

  WatchedShadow(const WatchedShadow&) = delete;
  WatchedShadow& operator=(const WatchedShadow&) = delete;

  void* Data() const {
    return m_Data;
  }
  void ResetWriteWatch() const;
  // Returns one enclosing range relative to data, clipped to [data, data + size).
  // Without a baseline, includes touched page portions; with one, only differing bytes on
  // touched pages. Returns std::nullopt when no bytes qualify.
  std::optional<ByteRange> DirtyRange(const void* data,
                                      size_t size,
                                      std::span<const uint8_t> baseline = {}) const;

private:
  void* m_Data{};
  size_t m_Size;
};

} // namespace gits::opengl2

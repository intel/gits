// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace gits::opengl {

class WatchedShadow {
public:
  struct ByteRange {
    size_t Offset;
    size_t Length;
  };

  // Whether the shadow tracks the pages written to it, for DirtyRange().
  // TODO: Track writes on Linux once dirty-page capture is prototyped there, see
  // docs/development/opengl-buffer-map-tracking-plan.md.
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
  // Like DirtyRange() without a baseline, but returns each run of consecutive touched pages, and
  // resets the write watch in the same step, so a concurrent write is reported by the next call.
  std::vector<ByteRange> TakeDirtyRanges(const void* data, size_t size) const;

private:
  // The addresses of the touched pages in ascending order, and the page size.
  std::vector<uintptr_t> QueryDirtyPages(bool reset, size_t& pageSize) const;

  void* m_Data{};
  size_t m_Size;
  // GetWriteWatch's output list, sized for every watched page once, as queries can precede each
  // GL command. Callers do not query one shadow concurrently.
  mutable std::vector<void*> m_Pages;
};

// The runs of consecutive pages of pageSize bytes at the ascending addresses in pages, clipped to
// [begin, begin + size) and relative to begin.
inline std::vector<WatchedShadow::ByteRange> PageRuns(std::span<const uintptr_t> pages,
                                                      size_t pageSize,
                                                      uintptr_t begin,
                                                      size_t size) {
  std::vector<WatchedShadow::ByteRange> runs;
  const uintptr_t end = begin + size;
  for (const uintptr_t page : pages) {
    const uintptr_t first = std::max(begin, page);
    const uintptr_t last = std::min(end, page + pageSize);
    if (first >= last) {
      continue;
    }
    if (!runs.empty() && runs.back().Offset + runs.back().Length == first - begin) {
      runs.back().Length += last - first;
    } else {
      runs.push_back({first - begin, last - first});
    }
  }
  return runs;
}

} // namespace gits::opengl

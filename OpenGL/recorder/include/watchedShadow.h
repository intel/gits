// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace gits::opengl {

// Application-visible memory standing in for a driver buffer mapping, which tracks the pages
// written to it.
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

  enum class Tracking {
    // The OS write watch: a query costs the whole shadow, however little was written.
    WriteWatch,
    // Read-only pages whose first write faults: a query costs only the written pages, but each
    // first write after a query costs a fault. Pages can also be refreshed from the driver
    // mapping on their next access. The OS itself cannot write read-only pages or read stale
    // ones, so a system call given such memory as a buffer fails instead of faulting. The
    // recorder writes refreshed bytes through a second, always writable view of the same memory,
    // so that the application's view never becomes writable before its page is tracked.
    PageProtection
  };

  WatchedShadow(size_t size, Tracking tracking);
  ~WatchedShadow();

  WatchedShadow(const WatchedShadow&) = delete;
  WatchedShadow& operator=(const WatchedShadow&) = delete;

  void* Data() const {
    return m_Data;
  }
  Tracking GetTracking() const {
    return m_Tracking;
  }
  // Forgets the written pages.
  void ResetWriteWatch();
  // Returns one enclosing range relative to data, clipped to [data, data + size).
  // Without a baseline, includes touched page portions; with one, only differing bytes on
  // touched pages. Returns std::nullopt when no bytes qualify.
  std::optional<ByteRange> DirtyRange(const void* data,
                                      size_t size,
                                      std::span<const uint8_t> baseline = {});
  // Like DirtyRange() without a baseline, but returns each run of consecutive touched pages, and
  // forgets them in the same step, so a concurrent write is reported by the next call.
  std::vector<ByteRange> TakeDirtyRanges(const void* data, size_t size);

  // When a stale page gets the source bytes.
  enum class Refresh {
    // On its first read or write.
    OnAccess,
    // On its first read only. A page written first keeps its earlier bytes where it is not
    // written, which equal the baseline, so later reads there miss newer source bytes.
    OnRead
  };

  // Page protection only. A stale page holding bytes of [data, data + size) gets them from
  // `source`, laid out like `data`, and into `baseline` when it is not empty. Both outlive the
  // shadow or the next call; `baseline` covers the whole range.
  void SetRefreshSource(
      void* data, size_t size, const void* source, std::span<uint8_t> baseline, Refresh refresh);
  // Page protection only. Makes every page not written since the last TakeDirtyRanges() stale,
  // so that it is refreshed as SetRefreshSource() selects. A written page instead gets the
  // source bytes at once where it still holds the baseline bytes, and keeps the bytes written to
  // it; without a baseline it keeps all its bytes.
  void MarkUnwrittenStale();

  // Whether pages may have been written since they were last taken or reset; always true for
  // write watch, which only a query answers. Pages written by another thread meanwhile may be
  // missed, as by any query.
  bool MayHaveWrittenPages() const {
    return m_Tracking == Tracking::WriteWatch ||
           m_WrittenCount.load(std::memory_order_acquire) != 0;
  }
  // The pages written and not yet taken or reset, over all page-protected shadows.
  static uint64_t PendingWrittenPages();

private:
  // The addresses of the touched pages in ascending order, and the page size.
  std::vector<uintptr_t> QueryDirtyPages(bool reset, size_t& pageSize);

  // The application's view of the shadow.
  void* m_Data{};
  size_t m_Size;
  Tracking m_Tracking;
  // GetWriteWatch's output list, sized for every watched page once. Callers do not query one
  // shadow concurrently.
  std::vector<void*> m_Pages;

  // Page protection only: the section backing the shadow, and the recorder's view of it, which
  // stays writable whatever the protection of the application's view.
  void* m_Section{};
  uint8_t* m_RecorderData{};

  // Page protection state, guarded by the lock of the shadow registry, which faults take.
  enum class PageState : uint8_t {
    ReadOnly,
    Written,
    Stale
  };
  friend bool HandleShadowAccess(uintptr_t address, bool write);
  void Release();
  // The recorder's view of `data` in the application's view.
  uint8_t* RecorderAddress(const uint8_t* data) const;
  void RefreshPage(size_t page);
  void MergeWrittenPages(size_t first, size_t count);
  void ProtectPages(size_t first, size_t count, PageState state);
  void ForgetWrittenPages();
  std::vector<PageState> m_PageStates;
  // The written pages in fault order; reserved for every page, as the fault handler must not
  // allocate. m_WrittenCount mirrors its size for MayHaveWrittenPages().
  std::vector<size_t> m_WrittenPages;
  std::atomic<size_t> m_WrittenCount{0};
  // The written pages as of the last query, also reserved for every page: a query swaps or
  // copies them out of m_WrittenPages so as not to allocate under the registry lock. Callers do
  // not query one shadow concurrently.
  std::vector<size_t> m_QueriedPages;
  uint8_t* m_RefreshData{};
  size_t m_RefreshSize{0};
  const uint8_t* m_RefreshSource{};
  std::span<uint8_t> m_RefreshBaseline;
  Refresh m_Refresh{Refresh::OnAccess};
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

// The maximal spans of bytes differing from baseline, of the same size, relative to their start.
inline std::vector<WatchedShadow::ByteRange> ChangedSpans(std::span<const uint8_t> bytes,
                                                          std::span<const uint8_t> baseline) {
  std::vector<WatchedShadow::ByteRange> spans;
  size_t index = 0;
  while (index < bytes.size()) {
    if (bytes[index] == baseline[index]) {
      ++index;
      continue;
    }
    const size_t first = index;
    while (index < bytes.size() && bytes[index] != baseline[index]) {
      ++index;
    }
    spans.push_back({first, index - first});
  }
  return spans;
}

} // namespace gits::opengl

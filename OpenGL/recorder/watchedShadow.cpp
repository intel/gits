// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "watchedShadow.h"

#include "log.h"

#include <algorithm>
#include <cstdint>
#include <new>
#include <vector>
#if defined(_WIN32)
#include <windows.h>
#else
#include <cerrno>
#include <sys/mman.h>
#endif

namespace gits::opengl {

#if defined(_WIN32)

namespace {

size_t GetPageSize() {
  static const size_t pageSize = [] {
    SYSTEM_INFO systemInfo{};
    GetSystemInfo(&systemInfo);
    GITS_ASSERT(systemInfo.dwPageSize != 0, "Cannot determine write-watch page size");
    return static_cast<size_t>(systemInfo.dwPageSize);
  }();
  return pageSize;
}

} // namespace

WatchedShadow::WatchedShadow(size_t size) : m_Size(size) {
  m_Data = VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT | MEM_WRITE_WATCH, PAGE_READWRITE);
  if (m_Data == nullptr) {
    GITS_ASSERT(false, "Cannot allocate write watch for buffer map (Windows error " +
                           std::to_string(GetLastError()) + ")");
  }
  const size_t pageSize = GetPageSize();
  try {
    m_Pages.resize(size / pageSize + (size % pageSize != 0));
  } catch (const std::bad_alloc&) {
    VirtualFree(m_Data, 0, MEM_RELEASE);
    m_Data = nullptr;
    GITS_ASSERT(false, "Cannot allocate write-watch page list");
  }
}

WatchedShadow::~WatchedShadow() {
  if (m_Data != nullptr) {
    VirtualFree(m_Data, 0, MEM_RELEASE);
  }
}

void WatchedShadow::ResetWriteWatch() const {
  if (::ResetWriteWatch(m_Data, m_Size) != 0) {
    GITS_ASSERT(false, "Cannot reset write watch for buffer map (Windows error " +
                           std::to_string(GetLastError()) + ")");
  }
}

std::vector<uintptr_t> WatchedShadow::QueryDirtyPages(bool reset, size_t& pageSize) const {
  pageSize = GetPageSize();
  const size_t numWatchedPages = m_Pages.size();
  // GetWriteWatch takes watched page count as input and overwrites it with dirty page count.
  ULONG_PTR numDirtyPages = static_cast<ULONG_PTR>(numWatchedPages);
  DWORD granularity = 0;
  const UINT error = GetWriteWatch(reset ? WRITE_WATCH_FLAG_RESET : 0, m_Data, m_Size,
                                   m_Pages.data(), &numDirtyPages, &granularity);
  if (error != 0) {
    const DWORD windowsError = GetLastError();
    GITS_ASSERT(false, "Cannot query write watch for buffer map (returned " +
                           std::to_string(error) + ", Windows error " +
                           std::to_string(windowsError) + ")");
  }
  GITS_ASSERT(granularity == pageSize && numDirtyPages <= numWatchedPages,
              "Invalid write-watch page count or granularity");
  std::vector<uintptr_t> dirtyPages(numDirtyPages);
  std::transform(m_Pages.begin(), m_Pages.begin() + numDirtyPages, dirtyPages.begin(),
                 [](void* page) { return reinterpret_cast<uintptr_t>(page); });
  std::sort(dirtyPages.begin(), dirtyPages.end());
  return dirtyPages;
}

#else

WatchedShadow::WatchedShadow(size_t size) : m_Size(size) {
  void* data = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (data == MAP_FAILED) {
    GITS_ASSERT(false,
                "Cannot allocate shadow for buffer map (errno " + std::to_string(errno) + ")");
  }
  m_Data = data;
}

WatchedShadow::~WatchedShadow() {
  if (m_Data != nullptr) {
    munmap(m_Data, m_Size);
  }
}

void WatchedShadow::ResetWriteWatch() const {}

std::vector<uintptr_t> WatchedShadow::QueryDirtyPages(bool /*reset*/, size_t& pageSize) const {
  GITS_ASSERT(false, "Buffer map shadows do not track writes on this platform");
  pageSize = 0;
  return {};
}

#endif

std::optional<WatchedShadow::ByteRange> WatchedShadow::DirtyRange(
    const void* data, size_t size, std::span<const uint8_t> baseline) const {
  GITS_ASSERT(baseline.empty() || baseline.size() == size, "Invalid write-watch baseline size");
  size_t pageSize = 0;
  const std::vector<uintptr_t> pages = QueryDirtyPages(false, pageSize);
  const auto begin = reinterpret_cast<uintptr_t>(data);
  const auto end = begin + size;
  const auto* bytes = static_cast<const uint8_t*>(data);
  size_t first = size;
  size_t last = 0;
  for (const uintptr_t pageBegin : pages) {
    const auto pageEnd = pageBegin + pageSize;
    if (pageBegin < end && pageEnd > begin) {
      const size_t pageFirst = std::max(begin, pageBegin) - begin;
      const size_t pageLast = std::min(end, pageEnd) - begin;
      if (baseline.empty()) {
        first = std::min(first, pageFirst);
        last = std::max(last, pageLast);
      } else {
        for (size_t offset = pageFirst; offset < pageLast; ++offset) {
          if (bytes[offset] != baseline[offset]) {
            first = std::min(first, offset);
            last = std::max(last, offset + 1);
          }
        }
      }
    }
  }
  if (first == size) {
    return std::nullopt;
  }
  return ByteRange{first, last - first};
}

std::vector<WatchedShadow::ByteRange> WatchedShadow::TakeDirtyRanges(const void* data,
                                                                     size_t size) const {
  size_t pageSize = 0;
  const std::vector<uintptr_t> pages = QueryDirtyPages(true, pageSize);
  return PageRuns(pages, pageSize, reinterpret_cast<uintptr_t>(data), size);
}

} // namespace gits::opengl

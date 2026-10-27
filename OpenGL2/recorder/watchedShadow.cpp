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
#include <windows.h>

namespace gits::opengl2 {

namespace {

size_t GetPageSize() {
  SYSTEM_INFO systemInfo{};
  GetSystemInfo(&systemInfo);
  GITS_ASSERT(systemInfo.dwPageSize != 0, "Cannot determine write-watch page size");
  return systemInfo.dwPageSize;
}

} // namespace

WatchedShadow::WatchedShadow(size_t size) : m_Size(size) {
  m_Data = VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT | MEM_WRITE_WATCH, PAGE_READWRITE);
  if (m_Data == nullptr) {
    GITS_ASSERT(false, "Cannot allocate write watch for buffer map (Windows error " +
                           std::to_string(GetLastError()) + ")");
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

std::optional<WatchedShadow::ByteRange> WatchedShadow::DirtyRange(const void* data,
                                                                  size_t size) const {
  const size_t pageSize = GetPageSize();
  const size_t numWatchedPages = m_Size / pageSize + (m_Size % pageSize != 0);
  // GetWriteWatch takes watched page count as input and overwrites it with dirty page count.
  ULONG_PTR numDirtyPages = static_cast<ULONG_PTR>(numWatchedPages);
  std::vector<void*> pages;
  try {
    pages.resize(numWatchedPages);
  } catch (const std::bad_alloc&) {
    GITS_ASSERT(false, "Cannot allocate write-watch page list");
  }
  DWORD granularity = 0;
  const UINT error = GetWriteWatch(0, m_Data, m_Size, pages.data(), &numDirtyPages, &granularity);
  if (error != 0) {
    const DWORD windowsError = GetLastError();
    GITS_ASSERT(false, "Cannot query write watch for buffer map (returned " +
                           std::to_string(error) + ", Windows error " +
                           std::to_string(windowsError) + ")");
  }
  GITS_ASSERT(granularity == pageSize && numDirtyPages <= numWatchedPages,
              "Invalid write-watch page count or granularity");

  const auto begin = reinterpret_cast<uintptr_t>(data);
  const auto end = begin + size;
  size_t first = size;
  size_t last = 0;
  for (ULONG_PTR index = 0; index < numDirtyPages; ++index) {
    const auto pageBegin = reinterpret_cast<uintptr_t>(pages[index]);
    const auto pageEnd = pageBegin + granularity;
    if (pageBegin < end && pageEnd > begin) {
      first = std::min(first, static_cast<size_t>(std::max(begin, pageBegin) - begin));
      last = std::max(last, static_cast<size_t>(std::min(end, pageEnd) - begin));
    }
  }
  if (first == size) {
    return std::nullopt;
  }
  return ByteRange{first, last - first};
}

} // namespace gits::opengl2

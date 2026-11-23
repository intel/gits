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
#include <atomic>
#include <cstdint>
#include <cstring>
#include <map>
#include <mutex>
#include <new>
#include <numeric>
#include <utility>
#include <vector>
#if defined(_WIN32)
#include <windows.h>
#else
#include <cerrno>
#include <sys/mman.h>
#endif

namespace gits::opengl {

namespace {

std::atomic<uint64_t> g_PendingWrittenPages{0};

} // namespace

uint64_t WatchedShadow::PendingWrittenPages() {
  return g_PendingWrittenPages.load(std::memory_order_acquire);
}

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

using ShadowMap = std::map<uintptr_t, WatchedShadow*>;

// The page-protected shadows by the address of their application view. Never destroyed, as
// application threads may fault on a shadow until the process ends. Map nodes are allocated and
// freed outside its lock, as the fault handler takes it.
struct ShadowRegistry {
  std::mutex Mutex;
  ShadowMap Shadows;
};

ShadowRegistry& GetShadowRegistry() {
  static auto* registry = new ShadowRegistry();
  return *registry;
}

LONG CALLBACK ShadowAccessHandler(EXCEPTION_POINTERS* exceptionInfo);

void InstallShadowAccessHandler() {
  static const PVOID handler = [] {
    // First, so that the application's own handlers never see a fault on a shadow.
    PVOID installed = AddVectoredExceptionHandler(1, ShadowAccessHandler);
    GITS_ASSERT(installed != nullptr, "Cannot install the buffer map shadow access handler");
    return installed;
  }();
  (void)handler;
}

DWORD PageProtection(uint8_t state) {
  constexpr DWORD protections[] = {PAGE_READONLY, PAGE_READWRITE, PAGE_NOACCESS};
  return protections[state];
}

} // namespace

// Called by the fault handler: it must not allocate, as the faulting thread may hold the heap lock.
bool HandleShadowAccess(uintptr_t address, bool write) {
  ShadowRegistry& registry = GetShadowRegistry();
  std::lock_guard<std::mutex> lock(registry.Mutex);
  auto it = registry.Shadows.upper_bound(address);
  if (it == registry.Shadows.begin()) {
    return false;
  }
  --it;
  WatchedShadow& shadow = *it->second;
  const size_t page = (address - it->first) / GetPageSize();
  if (page >= shadow.m_PageStates.size()) {
    return false;
  }
  using PageState = WatchedShadow::PageState;
  const PageState state = shadow.m_PageStates[page];
  if (state == PageState::Stale) {
    if (!write || shadow.m_Refresh == WatchedShadow::Refresh::OnAccess) {
      shadow.RefreshPage(page);
    }
    if (!write) {
      shadow.ProtectPages(page, 1, PageState::ReadOnly);
      return true;
    }
  } else if (state == PageState::Written || !write) {
    // Another thread handled a fault on the page first.
    return true;
  }
  shadow.m_WrittenPages.push_back(page);
  shadow.m_WrittenCount.fetch_add(1, std::memory_order_release);
  g_PendingWrittenPages.fetch_add(1, std::memory_order_release);
  shadow.ProtectPages(page, 1, PageState::Written);
  return true;
}

namespace {

LONG CALLBACK ShadowAccessHandler(EXCEPTION_POINTERS* exceptionInfo) {
  const EXCEPTION_RECORD& record = *exceptionInfo->ExceptionRecord;
  constexpr ULONG_PTR readAccess = 0;
  constexpr ULONG_PTR writeAccess = 1;
  if (record.ExceptionCode != EXCEPTION_ACCESS_VIOLATION || record.NumberParameters < 2 ||
      (record.ExceptionInformation[0] != readAccess &&
       record.ExceptionInformation[0] != writeAccess)) {
    return EXCEPTION_CONTINUE_SEARCH;
  }
  return HandleShadowAccess(record.ExceptionInformation[1],
                            record.ExceptionInformation[0] == writeAccess)
             ? EXCEPTION_CONTINUE_EXECUTION
             : EXCEPTION_CONTINUE_SEARCH;
}

} // namespace

WatchedShadow::WatchedShadow(size_t size, Tracking tracking) : m_Size(size), m_Tracking(tracking) {
  const size_t pageSize = GetPageSize();
  const size_t pageCount = size / pageSize + (size % pageSize != 0);
  if (tracking == Tracking::WriteWatch) {
    m_Data =
        VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT | MEM_WRITE_WATCH, PAGE_READWRITE);
    if (m_Data == nullptr) {
      GITS_ASSERT(false, "Cannot allocate shadow for buffer map (Windows error " +
                             std::to_string(GetLastError()) + ")");
    }
  } else {
    // Both views of a pagefile-backed section share its memory, but not their protection.
    const size_t sectionSize = pageCount * pageSize;
    m_Section = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
                                   static_cast<DWORD>(static_cast<uint64_t>(sectionSize) >> 32),
                                   static_cast<DWORD>(sectionSize), nullptr);
    if (m_Section != nullptr) {
      m_Data = MapViewOfFile(m_Section, FILE_MAP_WRITE, 0, 0, sectionSize);
    }
    if (m_Data != nullptr) {
      m_RecorderData =
          static_cast<uint8_t*>(MapViewOfFile(m_Section, FILE_MAP_WRITE, 0, 0, sectionSize));
    }
    if (m_RecorderData == nullptr) {
      const DWORD error = GetLastError();
      Release();
      GITS_ASSERT(false, "Cannot allocate page-protected shadow for buffer map (Windows error " +
                             std::to_string(error) + ")");
    }
  }
  ShadowMap::node_type registration;
  try {
    if (tracking == Tracking::WriteWatch) {
      m_Pages.resize(pageCount);
    } else {
      // Every page starts writable, so written.
      m_PageStates.assign(pageCount, PageState::Written);
      m_WrittenPages.resize(pageCount);
      std::iota(m_WrittenPages.begin(), m_WrittenPages.end(), size_t{0});
      m_QueriedPages.reserve(pageCount);
      ShadowMap entry;
      entry.emplace(reinterpret_cast<uintptr_t>(m_Data), this);
      registration = entry.extract(entry.begin());
    }
  } catch (const std::bad_alloc&) {
    Release();
    GITS_ASSERT(false, "Cannot allocate shadow page tracking");
  }
  if (tracking == Tracking::PageProtection) {
    m_WrittenCount.store(pageCount, std::memory_order_release);
    g_PendingWrittenPages.fetch_add(pageCount, std::memory_order_release);
    InstallShadowAccessHandler();
    ShadowRegistry& registry = GetShadowRegistry();
    std::lock_guard<std::mutex> lock(registry.Mutex);
    registry.Shadows.insert(std::move(registration));
  }
}

WatchedShadow::~WatchedShadow() {
  if (m_Data == nullptr) {
    return;
  }
  if (m_Tracking == Tracking::PageProtection) {
    ShadowMap::node_type registration;
    ShadowRegistry& registry = GetShadowRegistry();
    std::lock_guard<std::mutex> lock(registry.Mutex);
    registration = registry.Shadows.extract(reinterpret_cast<uintptr_t>(m_Data));
    ForgetWrittenPages();
  }
  Release();
}

void WatchedShadow::Release() {
  if (m_Tracking == Tracking::WriteWatch) {
    if (m_Data != nullptr) {
      VirtualFree(m_Data, 0, MEM_RELEASE);
    }
  } else {
    if (m_RecorderData != nullptr) {
      UnmapViewOfFile(m_RecorderData);
    }
    if (m_Data != nullptr) {
      UnmapViewOfFile(m_Data);
    }
    if (m_Section != nullptr) {
      CloseHandle(m_Section);
    }
  }
  m_Data = nullptr;
  m_RecorderData = nullptr;
  m_Section = nullptr;
}

uint8_t* WatchedShadow::RecorderAddress(const uint8_t* data) const {
  return m_RecorderData + (data - static_cast<const uint8_t*>(m_Data));
}

void WatchedShadow::ForgetWrittenPages() {
  g_PendingWrittenPages.fetch_sub(m_WrittenPages.size(), std::memory_order_release);
  m_WrittenPages.clear();
  m_WrittenCount.store(0, std::memory_order_release);
}

void WatchedShadow::ProtectPages(size_t first, size_t count, PageState state) {
  const size_t pageSize = GetPageSize();
  DWORD previous = 0;
  if (VirtualProtect(static_cast<uint8_t*>(m_Data) + first * pageSize, count * pageSize,
                     PageProtection(static_cast<uint8_t>(state)), &previous) == 0) {
    GITS_ASSERT(false, "Cannot protect buffer map shadow pages (Windows error " +
                           std::to_string(GetLastError()) + ")");
  }
  std::fill_n(m_PageStates.begin() + first, count, state);
}

void WatchedShadow::RefreshPage(size_t page) {
  const size_t pageSize = GetPageSize();
  uint8_t* const pageBegin = static_cast<uint8_t*>(m_Data) + page * pageSize;
  uint8_t* const first = std::max(pageBegin, m_RefreshData);
  uint8_t* const last = std::min(pageBegin + pageSize, m_RefreshData + m_RefreshSize);
  if (first >= last) {
    return;
  }
  const auto offset = static_cast<size_t>(first - m_RefreshData);
  const auto length = static_cast<size_t>(last - first);
  uint8_t* const bytes = RecorderAddress(first);
  std::memcpy(bytes, m_RefreshSource + offset, length);
  // From the shadow, as later GPU work may change the driver mapping between two reads.
  if (!m_RefreshBaseline.empty()) {
    std::memcpy(m_RefreshBaseline.data() + offset, bytes, length);
  }
}

void WatchedShadow::MergeWrittenPages(size_t first, size_t count) {
  const size_t pageSize = GetPageSize();
  uint8_t* const runBegin = static_cast<uint8_t*>(m_Data) + first * pageSize;
  uint8_t* const begin = std::max(runBegin, m_RefreshData);
  uint8_t* const end = std::min(runBegin + count * pageSize, m_RefreshData + m_RefreshSize);
  if (begin >= end) {
    return;
  }
  // No write may land between the comparison of a byte and its refresh.
  ProtectPages(first, count, PageState::Stale);
  const auto offset = static_cast<size_t>(begin - m_RefreshData);
  const auto length = static_cast<size_t>(end - begin);
  uint8_t* const bytes = RecorderAddress(begin);
  uint8_t* const baseline = m_RefreshBaseline.data() + offset;
  const uint8_t* const source = m_RefreshSource + offset;
  for (size_t index = 0; index < length; ++index) {
    if (bytes[index] == baseline[index]) {
      bytes[index] = source[index];
      baseline[index] = bytes[index];
    }
  }
  ProtectPages(first, count, PageState::Written);
}

void WatchedShadow::ResetWriteWatch() {
  if (m_Tracking == Tracking::WriteWatch) {
    if (::ResetWriteWatch(m_Data, m_Size) != 0) {
      GITS_ASSERT(false, "Cannot reset write watch for buffer map (Windows error " +
                             std::to_string(GetLastError()) + ")");
    }
    return;
  }
  std::lock_guard<std::mutex> lock(GetShadowRegistry().Mutex);
  ProtectPages(0, m_PageStates.size(), PageState::ReadOnly);
  ForgetWrittenPages();
}

void WatchedShadow::SetRefreshSource(
    void* data, size_t size, const void* source, std::span<uint8_t> baseline, Refresh refresh) {
  GITS_ASSERT(m_Tracking == Tracking::PageProtection,
              "Only page-protected shadows are refreshed from the driver mapping");
  GITS_ASSERT(baseline.empty() || baseline.size() == size, "Invalid shadow refresh baseline size");
  std::lock_guard<std::mutex> lock(GetShadowRegistry().Mutex);
  m_RefreshData = static_cast<uint8_t*>(data);
  m_RefreshSize = size;
  m_RefreshSource = static_cast<const uint8_t*>(source);
  m_RefreshBaseline = baseline;
  m_Refresh = refresh;
}

void WatchedShadow::MarkUnwrittenStale() {
  GITS_ASSERT(m_Tracking == Tracking::PageProtection && m_RefreshSource != nullptr,
              "Shadow pages cannot become stale without a refresh source");
  std::lock_guard<std::mutex> lock(GetShadowRegistry().Mutex);
  const size_t pageCount = m_PageStates.size();
  size_t page = 0;
  while (page < pageCount) {
    const PageState state = m_PageStates[page];
    const size_t first = page;
    while (page < pageCount && m_PageStates[page] == state) {
      ++page;
    }
    if (state == PageState::ReadOnly) {
      ProtectPages(first, page - first, PageState::Stale);
    } else if (state == PageState::Written && !m_RefreshBaseline.empty()) {
      MergeWrittenPages(first, page - first);
    }
  }
}

std::vector<uintptr_t> WatchedShadow::QueryDirtyPages(bool reset, size_t& pageSize) {
  pageSize = GetPageSize();
  if (m_Tracking == Tracking::PageProtection) {
    {
      std::lock_guard<std::mutex> lock(GetShadowRegistry().Mutex);
      if (reset) {
        m_QueriedPages.clear();
        m_QueriedPages.swap(m_WrittenPages);
        g_PendingWrittenPages.fetch_sub(m_QueriedPages.size(), std::memory_order_release);
        m_WrittenCount.store(0, std::memory_order_release);
        std::sort(m_QueriedPages.begin(), m_QueriedPages.end());
        size_t index = 0;
        while (index < m_QueriedPages.size()) {
          const size_t first = index;
          while (index + 1 < m_QueriedPages.size() &&
                 m_QueriedPages[index + 1] == m_QueriedPages[index] + 1) {
            ++index;
          }
          ++index;
          ProtectPages(m_QueriedPages[first], index - first, PageState::ReadOnly);
        }
      } else {
        m_QueriedPages.assign(m_WrittenPages.begin(), m_WrittenPages.end());
      }
    }
    if (!reset) {
      std::sort(m_QueriedPages.begin(), m_QueriedPages.end());
    }
    std::vector<uintptr_t> dirtyPages(m_QueriedPages.size());
    const auto base = reinterpret_cast<uintptr_t>(m_Data);
    std::transform(m_QueriedPages.begin(), m_QueriedPages.end(), dirtyPages.begin(),
                   [base, pageSize](size_t page) { return base + page * pageSize; });
    return dirtyPages;
  }
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

WatchedShadow::WatchedShadow(size_t size, Tracking tracking) : m_Size(size), m_Tracking(tracking) {
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

void WatchedShadow::ResetWriteWatch() {}

void WatchedShadow::SetRefreshSource(void* /*data*/,
                                     size_t /*size*/,
                                     const void* /*source*/,
                                     std::span<uint8_t> /*baseline*/,
                                     Refresh /*refresh*/) {
  GITS_ASSERT(false, "Buffer map shadows are not refreshed on this platform");
}

void WatchedShadow::MarkUnwrittenStale() {
  GITS_ASSERT(false, "Buffer map shadows are not refreshed on this platform");
}

std::vector<uintptr_t> WatchedShadow::QueryDirtyPages(bool /*reset*/, size_t& pageSize) {
  GITS_ASSERT(false, "Buffer map shadows do not track writes on this platform");
  pageSize = 0;
  return {};
}

#endif

std::optional<WatchedShadow::ByteRange> WatchedShadow::DirtyRange(
    const void* data, size_t size, std::span<const uint8_t> baseline) {
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
                                                                     size_t size) {
  size_t pageSize = 0;
  const std::vector<uintptr_t> pages = QueryDirtyPages(true, pageSize);
  return PageRuns(pages, pageSize, reinterpret_cast<uintptr_t>(data), size);
}

} // namespace gits::opengl

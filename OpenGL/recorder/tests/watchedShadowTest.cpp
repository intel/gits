// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

// Host-only tests for watched shadows of persistent maps: publication must report every
// page written since the last one, and a refresh from the driver mapping must never replace bytes
// the application wrote, including from other threads while the shadow is published or refreshed.

#include "watchedShadow.h"

#include <atomic>
#include <cstdio>
#include <cstring>
#include <span>
#include <thread>
#include <vector>

namespace {

int g_Failures = 0;

void Check(bool condition, const char* what) {
  if (!condition) {
    std::printf("FAIL: %s\n", what);
    ++g_Failures;
  }
}

using gits::opengl::WatchedShadow;

constexpr size_t kPage = 4096;
constexpr size_t kPages = 8;
constexpr size_t kSize = kPages * kPage;

volatile uint8_t* Bytes(WatchedShadow& shadow) {
  return static_cast<volatile uint8_t*>(shadow.Data());
}

bool Equal(const std::vector<WatchedShadow::ByteRange>& runs,
           const std::vector<WatchedShadow::ByteRange>& expected) {
  if (runs.size() != expected.size()) {
    return false;
  }
  for (size_t i = 0; i < runs.size(); ++i) {
    if (runs[i].Offset != expected[i].Offset || runs[i].Length != expected[i].Length) {
      return false;
    }
  }
  return true;
}

void TestWrittenPagesAreTaken() {
  WatchedShadow shadow(kSize, WatchedShadow::Tracking::PageProtection);
  shadow.ResetWriteWatch();
  Check(shadow.TakeDirtyRanges(shadow.Data(), kSize).empty(), "a reset shadow has no writes");
  Bytes(shadow)[kPage + 1] = 1;
  Bytes(shadow)[2 * kPage + 5] = 2;
  Bytes(shadow)[6 * kPage] = 3;
  Check(
      Equal(shadow.TakeDirtyRanges(shadow.Data(), kSize), {{kPage, 2 * kPage}, {6 * kPage, kPage}}),
      "written pages are taken as runs");
  Check(shadow.TakeDirtyRanges(shadow.Data(), kSize).empty(), "taken pages are not taken again");
  Bytes(shadow)[kPage + 2] = 4;
  Check(Equal(shadow.TakeDirtyRanges(shadow.Data(), kSize), {{kPage, kPage}}),
        "a taken page written again is taken again");
  Check(Bytes(shadow)[kPage + 1] == 1 && Bytes(shadow)[kPage + 2] == 4, "writes reach the shadow");
  Bytes(shadow)[3 * kPage] = 5;
  Check(shadow.DirtyRange(shadow.Data(), kSize).has_value() &&
            !shadow.TakeDirtyRanges(shadow.Data(), kSize).empty(),
        "an enclosing dirty range does not forget the written pages");
}

void TestWrittenPagesAreCounted() {
  const uint64_t pending = WatchedShadow::PendingWrittenPages();
  {
    WatchedShadow shadow(kSize, WatchedShadow::Tracking::PageProtection);
    Check(shadow.MayHaveWrittenPages(), "a new shadow counts as written");
    shadow.ResetWriteWatch();
    Check(!shadow.MayHaveWrittenPages() && WatchedShadow::PendingWrittenPages() == pending,
          "a reset shadow has no written pages");
    Bytes(shadow)[0] = 1;
    Bytes(shadow)[1] = 2;
    Bytes(shadow)[kPage] = 3;
    Check(shadow.MayHaveWrittenPages() && WatchedShadow::PendingWrittenPages() == pending + 2,
          "each written page counts once");
    shadow.TakeDirtyRanges(shadow.Data(), kSize);
    Check(!shadow.MayHaveWrittenPages() && WatchedShadow::PendingWrittenPages() == pending,
          "taken pages no longer count");
    Bytes(shadow)[2 * kPage] = 4;
  }
  Check(WatchedShadow::PendingWrittenPages() == pending,
        "a destroyed shadow's written pages no longer count");
}

void TestWriteWatchPagesAreTaken() {
  const uint64_t pending = WatchedShadow::PendingWrittenPages();
  WatchedShadow shadow(kSize, WatchedShadow::Tracking::WriteWatch);
  shadow.ResetWriteWatch();
  Check(shadow.MayHaveWrittenPages() && WatchedShadow::PendingWrittenPages() == pending,
        "a write-watch shadow may always have written pages, and adds none to the count");
  Check(shadow.TakeDirtyRanges(shadow.Data(), kSize).empty(),
        "a reset write-watch shadow has no writes");
  Bytes(shadow)[kPage + 1] = 1;
  Bytes(shadow)[2 * kPage + 5] = 2;
  Bytes(shadow)[6 * kPage] = 3;
  Check(
      Equal(shadow.TakeDirtyRanges(shadow.Data(), kSize), {{kPage, 2 * kPage}, {6 * kPage, kPage}}),
      "pages written under write watch are taken as runs");
  Check(shadow.TakeDirtyRanges(shadow.Data(), kSize).empty(),
        "pages taken under write watch are not taken again");
}

void TestStalePagesRefreshOnAccess() {
  WatchedShadow shadow(kSize, WatchedShadow::Tracking::PageProtection);
  std::vector<uint8_t> source(kSize, 0xAB);
  std::vector<uint8_t> baseline(kSize, 0);
  shadow.ResetWriteWatch();
  Bytes(shadow)[3 * kPage] = 7;
  shadow.SetRefreshSource(shadow.Data(), kSize, source.data(), baseline);
  shadow.MarkUnwrittenStale();
  Check(Bytes(shadow)[10] == 0xAB, "a stale page reads the driver contents");
  Check(baseline[10] == 0xAB && baseline[kPage] == 0,
        "only the refreshed page updates the baseline");
  Check(shadow.TakeDirtyRanges(shadow.Data(), kSize).size() == 1,
        "reading a stale page does not make it written");
  Bytes(shadow)[kPage + 3] = 9;
  Check(Bytes(shadow)[kPage + 3] == 9 && Bytes(shadow)[kPage + 4] == 0xAB,
        "writing a stale page refreshes it first, then keeps the write");
  Check(Equal(shadow.TakeDirtyRanges(shadow.Data(), kSize), {{kPage, kPage}}),
        "writing a stale page makes it written");
  std::memset(source.data(), 0xEF, kSize);
  Check(Bytes(shadow)[11] == 0xAB && Bytes(shadow)[kPage + 5] == 0xAB && baseline[11] == 0xAB,
        "each stale page refreshes once");
}

void TestWrittenPagesDoNotBecomeStale() {
  WatchedShadow shadow(kSize, WatchedShadow::Tracking::PageProtection);
  std::vector<uint8_t> source(kSize, 0xCD);
  shadow.ResetWriteWatch();
  shadow.SetRefreshSource(shadow.Data(), kSize, source.data(), {});
  Bytes(shadow)[2 * kPage] = 1;
  shadow.MarkUnwrittenStale();
  Check(Bytes(shadow)[2 * kPage] == 1 && Bytes(shadow)[2 * kPage + 1] == 0,
        "a page written before it is taken keeps its bytes");
  Check(Bytes(shadow)[0] == 0xCD, "an unwritten page refreshes");
}

void TestWrittenPagesMergeRefresh() {
  WatchedShadow shadow(kSize, WatchedShadow::Tracking::PageProtection);
  std::vector<uint8_t> source(kSize, 0x5A);
  std::vector<uint8_t> baseline(kSize, 0);
  shadow.ResetWriteWatch();
  shadow.SetRefreshSource(shadow.Data(), kSize, source.data(), baseline);
  Bytes(shadow)[2 * kPage + 1] = 1;
  shadow.MarkUnwrittenStale();
  Check(baseline[2 * kPage] == 0x5A && baseline[2 * kPage + 1] == 0,
        "only the unwritten bytes of a written page update the baseline");
  Check(Bytes(shadow)[2 * kPage + 1] == 1 && Bytes(shadow)[2 * kPage] == 0x5A &&
            Bytes(shadow)[3 * kPage - 1] == 0x5A,
        "a written page keeps its written bytes and gets the others");
  Check(Equal(shadow.TakeDirtyRanges(shadow.Data(), kSize), {{2 * kPage, kPage}}),
        "a refreshed written page stays written");
}

void TestConcurrentWritesSurviveRefresh(bool withBaseline) {
  constexpr size_t writers = 4;
  constexpr uint32_t writes = 20000;
  WatchedShadow shadow(kSize, WatchedShadow::Tracking::PageProtection);
  std::vector<uint8_t> source(kSize, 0);
  std::vector<uint8_t> baseline(kSize, 0);
  shadow.ResetWriteWatch();
  shadow.SetRefreshSource(shadow.Data(), kSize, source.data(),
                          withBaseline ? std::span<uint8_t>(baseline) : std::span<uint8_t>());
  std::atomic<size_t> running{writers};
  std::vector<std::thread> threads;
  for (size_t writer = 0; writer < writers; ++writer) {
    threads.emplace_back([&shadow, &running, writer] {
      auto* value = reinterpret_cast<volatile uint32_t*>(Bytes(shadow) + (writer * 2 + 1) * kPage);
      for (uint32_t index = 1; index <= writes; ++index) {
        *value = index;
      }
      running.fetch_sub(1);
    });
  }
  // Publishes the taken pages to the driver mapping, as MapTrackingService does before a refresh.
  std::vector<bool> taken(kPages, false);
  const auto publish = [&] {
    for (const auto& run : shadow.TakeDirtyRanges(shadow.Data(), kSize)) {
      std::vector<uint8_t> snapshot(run.Length);
      std::memcpy(snapshot.data(), static_cast<const uint8_t*>(shadow.Data()) + run.Offset,
                  run.Length);
      std::memcpy(source.data() + run.Offset, snapshot.data(), run.Length);
      if (withBaseline) {
        std::memcpy(baseline.data() + run.Offset, snapshot.data(), run.Length);
      }
      for (size_t page = run.Offset / kPage; page < (run.Offset + run.Length) / kPage; ++page) {
        taken[page] = true;
      }
    }
  };
  while (running.load() != 0) {
    publish();
    shadow.MarkUnwrittenStale();
  }
  for (auto& thread : threads) {
    thread.join();
  }
  publish();
  for (size_t writer = 0; writer < writers; ++writer) {
    const size_t offset = (writer * 2 + 1) * kPage;
    uint32_t published = 0;
    std::memcpy(&published, source.data() + offset, sizeof(published));
    Check(published == writes, "the last write of each thread is published");
    Check(*reinterpret_cast<const volatile uint32_t*>(Bytes(shadow) + offset) == writes,
          "the last write of each thread survives concurrent refreshes");
    Check(taken[writer * 2 + 1], "every page written concurrently is taken");
  }
}

} // namespace

int main() {
  TestWrittenPagesAreTaken();
  TestWrittenPagesAreCounted();
  TestWriteWatchPagesAreTaken();
  TestStalePagesRefreshOnAccess();
  TestWrittenPagesDoNotBecomeStale();
  TestWrittenPagesMergeRefresh();
  TestConcurrentWritesSurviveRefresh(false);
  TestConcurrentWritesSurviveRefresh(true);
  if (g_Failures != 0) {
    std::printf("watchedShadowTest: %d check(s) failed\n", g_Failures);
    return 1;
  }
  std::printf("watchedShadowTest: all checks passed\n");
  return 0;
}

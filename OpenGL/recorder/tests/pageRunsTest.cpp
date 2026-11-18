// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

// Host-only tests for the runs of written pages that a coherent map publishes. A run spanning
// clean pages would copy and record them too, which for sparse writes to a large map inflates the
// capture overhead and the stream by up to the size of the map.

#include "watchedShadow.h"

#include <cstdio>
#include <vector>

namespace {

int g_Failures = 0;

void Check(bool condition, const char* what) {
  if (!condition) {
    std::printf("FAIL: %s\n", what);
    ++g_Failures;
  }
}

using gits::opengl::PageRuns;
using gits::opengl::WatchedShadow;

constexpr size_t kPage = 4096;
constexpr uintptr_t kBase = 0x100000;

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

void TestNoPages() {
  Check(PageRuns({}, kPage, kBase, 4 * kPage).empty(), "no written page gives no run");
}

void TestSeparatePagesGiveSeparateRuns() {
  // A page near each end of a large map.
  const size_t size = 1024 * kPage;
  const std::vector<uintptr_t> pages{kBase, kBase + 1023 * kPage};
  Check(Equal(PageRuns(pages, kPage, kBase, size), {{0, kPage}, {1023 * kPage, kPage}}),
        "pages at both ends give two runs without the clean pages between them");
}

void TestConsecutivePagesGiveOneRun() {
  const std::vector<uintptr_t> pages{kBase + kPage, kBase + 2 * kPage, kBase + 3 * kPage,
                                     kBase + 5 * kPage};
  Check(Equal(PageRuns(pages, kPage, kBase, 8 * kPage), {{kPage, 3 * kPage}, {5 * kPage, kPage}}),
        "consecutive pages give one run, and the next written page another");
}

void TestRunsAreClippedToTheMap() {
  // A map that starts and ends within pages.
  const uintptr_t begin = kBase + 100;
  const size_t size = 2 * kPage;
  const std::vector<uintptr_t> pages{kBase, kBase + kPage, kBase + 2 * kPage};
  Check(Equal(PageRuns(pages, kPage, begin, size), {{0, size}}),
        "a run starts at the map and ends with it");
}

void TestPagesOutsideTheMapAreSkipped() {
  const std::vector<uintptr_t> pages{kBase - kPage, kBase + kPage, kBase + 4 * kPage};
  Check(Equal(PageRuns(pages, kPage, kBase, 2 * kPage), {{kPage, kPage}}),
        "pages before and after the map are skipped");
}

} // namespace

int main() {
  TestNoPages();
  TestSeparatePagesGiveSeparateRuns();
  TestConsecutivePagesGiveOneRun();
  TestRunsAreClippedToTheMap();
  TestPagesOutsideTheMapAreSkipped();
  if (g_Failures != 0) {
    std::printf("pageRunsTest: %d check(s) failed\n", g_Failures);
    return 1;
  }
  std::printf("pageRunsTest: all checks passed\n");
  return 0;
}

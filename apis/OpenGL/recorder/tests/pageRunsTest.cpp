// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

// Host-only tests for the runs of written pages that a coherent map publishes. A run spanning
// clean pages would copy and record them too, which for sparse writes to a large map inflates the
// capture overhead and the stream by up to the size of the map. Also for the changed spans of a
// run that a GPU-written map publishes, so that its unchanged bytes do not overwrite GPU results,
// and for the bytes a map publishes around the ones the recorder read back after a GPU copy.

#include "watchedShadow.h"

#include <algorithm>
#include <cstdio>
#include <span>
#include <vector>

namespace {

int g_Failures = 0;

void Check(bool condition, const char* what) {
  if (!condition) {
    std::printf("FAIL: %s\n", what);
    ++g_Failures;
  }
}

using gits::opengl::ChangedSpans;
using gits::opengl::KnownBytes;
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

void TestUnchangedBytesGiveNoSpan() {
  const std::vector<uint8_t> bytes(16, 7);
  Check(ChangedSpans(bytes, bytes).empty(), "unchanged bytes give no span");
}

void TestChangedBytesGiveTheirSpans() {
  // 32-bit integers from 0 to 1 change one byte each.
  const std::vector<uint8_t> baseline(16, 0);
  std::vector<uint8_t> bytes(16, 0);
  for (size_t offset = 0; offset < bytes.size(); offset += 4) {
    bytes[offset] = 1;
  }
  Check(Equal(ChangedSpans(bytes, baseline), {{0, 1}, {4, 1}, {8, 1}, {12, 1}}),
        "each changed byte between unchanged ones is a span");
}

void TestAdjacentChangedBytesGiveOneSpan() {
  const std::vector<uint8_t> baseline(8, 0);
  const std::vector<uint8_t> bytes{0, 1, 2, 3, 0, 0, 5, 6};
  Check(Equal(ChangedSpans(bytes, baseline), {{1, 3}, {6, 2}}),
        "adjacent changed bytes give one span, up to the end");
}

void TestUnknownBytesAreAllPublished() {
  const KnownBytes known;
  const std::vector<uint8_t> bytes(16, 0);
  Check(Equal(known.UnknownOrChangedSpans(0, bytes), {{0, 16}}),
        "without known bytes, every byte is published");
}

void TestKnownBytesAreSkippedUnlessChanged() {
  // Bytes 4 to 12 were read back, and the application then wrote byte 8.
  KnownBytes known;
  known.Set(4, std::vector<uint8_t>(8, 9));
  std::vector<uint8_t> bytes(16, 0);
  std::fill(bytes.begin() + 4, bytes.begin() + 12, 9);
  bytes[8] = 1;
  Check(Equal(known.UnknownOrChangedSpans(0, bytes), {{0, 4}, {8, 1}, {12, 4}}),
        "known bytes the application did not change are skipped, and the others published");
  Check(Equal(known.UnknownOrChangedSpans(6, std::span(bytes).subspan(6, 4)), {{2, 1}}),
        "spans of a run starting within known bytes are relative to the run");
}

void TestKnownBytesAreReplacedAndForgotten() {
  KnownBytes known;
  known.Set(0, std::vector<uint8_t>(16, 1));
  known.Set(4, std::vector<uint8_t>(4, 2));
  std::vector<uint8_t> bytes(16, 1);
  std::fill(bytes.begin() + 4, bytes.begin() + 8, 2);
  Check(known.UnknownOrChangedSpans(0, bytes).empty(), "a later read-back replaces known bytes");
  known.Forget(2, 4);
  known.Forget(10, 2);
  Check(Equal(known.UnknownOrChangedSpans(0, bytes), {{2, 4}, {10, 2}}),
        "forgotten bytes are published again, within and across earlier read-backs");
  known.Forget(0, 16);
  Check(known.Empty(), "forgetting every known byte leaves none");
}

} // namespace

int main() {
  TestNoPages();
  TestSeparatePagesGiveSeparateRuns();
  TestConsecutivePagesGiveOneRun();
  TestRunsAreClippedToTheMap();
  TestPagesOutsideTheMapAreSkipped();
  TestUnchangedBytesGiveNoSpan();
  TestChangedBytesGiveTheirSpans();
  TestAdjacentChangedBytesGiveOneSpan();
  TestUnknownBytesAreAllPublished();
  TestKnownBytesAreSkippedUnlessChanged();
  TestKnownBytesAreReplacedAndForgotten();
  if (g_Failures != 0) {
    std::printf("pageRunsTest: %d check(s) failed\n", g_Failures);
    return 1;
  }
  std::printf("pageRunsTest: all checks passed\n");
  return 0;
}

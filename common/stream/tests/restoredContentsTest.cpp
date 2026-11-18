// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

// Host-only tests for the bytes of an exported object that an import uses and the exporting API
// does not restore. Missing one would leave the import with storage whose contents nothing
// restores.

#include "restoredContents.h"

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

using gits::stream::ContentRange;
using gits::stream::IsBound;
using gits::stream::Overlaps;
using gits::stream::RestoredContents;
using gits::stream::TextureRange;
using gits::stream::UnrestoredRanges;

constexpr uint64_t kSize = 0x10000;

bool Equal(const std::vector<ContentRange>& ranges, const std::vector<ContentRange>& expected) {
  if (ranges.size() != expected.size()) {
    return false;
  }
  for (size_t i = 0; i < ranges.size(); ++i) {
    if (ranges[i].Offset != expected[i].Offset || ranges[i].Size != expected[i].Size) {
      return false;
    }
  }
  return true;
}

void TestFullCover() {
  const RestoredContents contents{kSize, {{0, kSize}}, {{0, kSize}}, {}};
  Check(UnrestoredRanges(contents, {0, kSize}).empty(),
        "a resource over the whole object restores all of it");
  Check(UnrestoredRanges(contents, {0x4000, 0x100}).empty(), "and any range in it");
}

void TestPartialCover() {
  // A resource over the first half.
  const RestoredContents contents{kSize, {{0, kSize / 2}}, {{0, kSize / 2}}, {}};
  Check(Equal(UnrestoredRanges(contents, {kSize / 2, kSize / 2}), {{kSize / 2, kSize / 2}}),
        "an import of the half no resource covers is not restored");
  Check(Equal(UnrestoredRanges(contents, {0x1000, kSize / 2}), {{kSize / 2, 0x1000}}),
        "an import past the end of the resource is not restored from that end");
  Check(UnrestoredRanges(contents, {0, kSize / 2}).empty(),
        "an import of the covered half is restored");
}

void TestGapsBetweenResources() {
  const RestoredContents contents{kSize, {{0xC000, 0x2000}, {0, 0x4000}, {0x8000, 0x2000}}, {}, {}};
  Check(Equal(UnrestoredRanges(contents, {0, kSize}),
              {{0x4000, 0x4000}, {0xA000, 0x2000}, {0xE000, 0x2000}}),
        "each gap between resources and after the last is not restored, in whatever order the "
        "resources are reported");
  Check(Equal(UnrestoredRanges(contents, {0x2000, 0x3000}), {{0x4000, 0x1000}}),
        "a range ending in a gap is not restored to its end");
  Check(UnrestoredRanges(contents, {0x8000, 0x1000}).empty(), "a range in a resource is");
}

void TestAdjacentAndOverlappingResources() {
  const RestoredContents adjacent{kSize, {{0x4000, 0xC000}, {0, 0x4000}}, {}, {}};
  Check(UnrestoredRanges(adjacent, {0, kSize}).empty(),
        "adjacent resources restore the range they span");
  const RestoredContents overlapping{
      kSize, {{0, 0x6000}, {0x2000, 0x2000}, {0x5000, 0xB000}}, {}, {}};
  Check(UnrestoredRanges(overlapping, {0, kSize}).empty(),
        "a resource within another does not end the restored range");
}

void TestNothingRestored() {
  const RestoredContents contents{kSize, {}, {}, {}};
  Check(Equal(UnrestoredRanges(contents, {0x100, 0x10}), {{0x100, 0x10}}),
        "with no restored resource, all of an import is not restored");
  Check(UnrestoredRanges(contents, {0x100, 0}).empty(), "an empty range needs nothing restored");
}

void TestBound() {
  // An image bound at 0x4000 whose contents are not restored.
  const RestoredContents contents{
      kSize, {{0, 0x4000}}, {{0, 0x4000}, {0x4000, 0x2000}}, {{0x4000, 0x2000}}};
  Check(IsBound(contents, {0x5000, 0x4000}), "a range over part of a resource is bound");
  Check(!IsBound(contents, {0x6000, 0x1000}), "a range just past the last resource is not");
  Check(!IsBound(contents, {0x8000, 0}), "an empty range is not");
  Check(!IsBound(contents, {0x1000, 0}), "an empty range inside a resource is not");

  const RestoredContents empty{kSize, {}, {{0x2000, 0}}, {}};
  Check(!IsBound(empty, {0x1000, 0x2000}), "a range over an empty resource is not");
}

void TestRequired() {
  // An import relies on the contents of 0x4000 to 0x6000.
  RestoredContents contents{kSize, {{0, kSize}}, {{0, kSize}}, {}};
  contents.Required.push_back({0x4000, 0x2000});
  Check(Overlaps(contents.Required, {0x5000, 0x4000}),
        "a resource over part of a required range is required");
  Check(!Overlaps(contents.Required, {0x6000, 0x1000}),
        "a resource just past the required range is not");
  Check(!Overlaps(contents.Required, {0x2000, 0x2000}),
        "a resource just before the required range is not");
  Check(!Overlaps(contents.Required, {0x5000, 0}), "an empty resource is not");
  Check(!Overlaps({}, {0, kSize}), "nothing is required without an import");
}

void TestTextureRange() {
  const RestoredContents contents{kSize, {}, {{0x2000, 0x3000}}, {{0x2000, 0x3000}}};
  const ContentRange image = TextureRange(contents, 0x2000);
  Check(image.Offset == 0x2000 && image.Size == 0x3000,
        "a texture takes the range of the image bound at its offset");
  const ContentRange rest = TextureRange(contents, 0x1000);
  Check(rest.Offset == 0x1000 && rest.Size == kSize - 0x1000,
        "a texture where no image is bound takes the rest of the object");
}

} // namespace

int main() {
  TestFullCover();
  TestPartialCover();
  TestGapsBetweenResources();
  TestAdjacentAndOverlappingResources();
  TestNothingRestored();
  TestBound();
  TestRequired();
  TestTextureRange();
  if (g_Failures != 0) {
    std::printf("restoredContentsTest: %d check(s) failed\n", g_Failures);
    return 1;
  }
  std::printf("restoredContentsTest: all checks passed\n");
  return 0;
}

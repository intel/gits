// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <algorithm>
#include <cstdint>
#include <map>
#include <vector>

namespace gits {
namespace stream {

// Bytes of an exported object, from its start.
struct ContentRange {
  uint64_t Offset{};
  uint64_t Size{};
};

// The contents of an exported object that the state restore of the exporting API writes again.
struct RestoredContents {
  uint64_t Size{};
  // The ranges of the resources whose contents are read back and restored, in any order. They may
  // overlap.
  std::vector<ContentRange> Restored;
  // The ranges of all resources bound to the object, whether or not their contents are restored.
  // The GPU work of the exporting API accesses no other bytes of it.
  std::vector<ContentRange> Bound;
  // The size each image bound to the object takes, by the offset it is bound at, whether or not
  // its contents are restored.
  std::map<uint64_t, uint64_t> Images;
  // The ranges of the objects of the importing API, whose restored contents it does not write.
  std::vector<ContentRange> Required;
};

// Whether any of `ranges` covers a byte of `range`.
inline bool Overlaps(const std::vector<ContentRange>& ranges, ContentRange range) {
  if (range.Size == 0) {
    return false;
  }
  return std::any_of(ranges.begin(), ranges.end(), [&range](const ContentRange& entry) {
    return entry.Size != 0 && entry.Offset < range.Offset + range.Size &&
           range.Offset < entry.Offset + entry.Size;
  });
}

// The range of the object that a texture of the importing API with storage at `offset` takes. GL
// has no query for it: the storage has the layout and size of the image the exporting API created
// with the same parameters, which is the one bound at `offset`. Without one, it is the rest of the
// object.
inline ContentRange TextureRange(const RestoredContents& contents, uint64_t offset) {
  if (const auto image = contents.Images.find(offset); image != contents.Images.end()) {
    return {offset, image->second};
  }
  return {offset, offset < contents.Size ? contents.Size - offset : 0};
}

// The parts of `range` whose contents are not restored, in order.
inline std::vector<ContentRange> UnrestoredRanges(const RestoredContents& contents,
                                                  ContentRange range) {
  std::vector<ContentRange> restored = contents.Restored;
  std::sort(restored.begin(), restored.end(),
            [](const ContentRange& a, const ContentRange& b) { return a.Offset < b.Offset; });
  std::vector<ContentRange> unrestored;
  const uint64_t end = range.Offset + range.Size;
  uint64_t next = range.Offset;
  for (const ContentRange& entry : restored) {
    if (next >= end) {
      break;
    }
    if (entry.Offset > next) {
      unrestored.push_back({next, std::min(entry.Offset, end) - next});
    }
    next = std::max(next, entry.Offset + entry.Size);
  }
  if (next < end) {
    unrestored.push_back({next, end - next});
  }
  return unrestored;
}

// Whether a resource of the exporting API is bound over any byte of `range`.
inline bool IsBound(const RestoredContents& contents, ContentRange range) {
  return Overlaps(contents.Bound, range);
}

} // namespace stream
} // namespace gits

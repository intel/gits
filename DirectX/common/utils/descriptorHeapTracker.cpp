// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "descriptorHeapTracker.h"

#include <cassert>

namespace gits {
namespace DirectX {

void DescriptorHeapTracker::createDescriptor(DescriptorInfo* descriptorInfo) {
  descriptorsByHeapIndex_[descriptorInfo->heapKey][descriptorInfo->descriptorIndex].reset(
      descriptorInfo);
}

void DescriptorHeapTracker::destroyObject(unsigned key) {
  auto itHeap = descriptorsByHeapIndex_.find(key);
  if (itHeap != descriptorsByHeapIndex_.end()) {
    descriptorsByHeapIndex_.erase(itHeap);
    return;
  }
}

void DescriptorHeapTracker::copyDescriptors(ID3D12DeviceCopyDescriptorsSimpleCommand& c) {
  if (!c.NumDescriptors_.value) {
    return;
  }
  auto srcHeapIt = descriptorsByHeapIndex_.find(c.SrcDescriptorRangeStart_.interfaceKey);
  if (srcHeapIt == descriptorsByHeapIndex_.end()) {
    return;
  }
  auto& destHeap = descriptorsByHeapIndex_[c.DestDescriptorRangeStart_.interfaceKey];
  for (unsigned i = 0; i < c.NumDescriptors_.value; ++i) {
    auto srcIt = srcHeapIt->second.find(c.SrcDescriptorRangeStart_.index + i);
    if (srcIt != srcHeapIt->second.end()) {
      unsigned destHeapIndex = c.DestDescriptorRangeStart_.index + i;
      destHeap[destHeapIndex].reset(copyDescriptor(
          srcIt->second.get(), c.DestDescriptorRangeStart_.interfaceKey, destHeapIndex));
    }
  }
}

void DescriptorHeapTracker::copyDescriptors(ID3D12DeviceCopyDescriptorsCommand& c) {
  if (!c.NumDestDescriptorRanges_.value || !c.NumSrcDescriptorRanges_.value) {
    return;
  }

  unsigned destRangeIndex = 0;
  unsigned destIndex = 0;
  unsigned destRangeSize =
      c.pDestDescriptorRangeSizes_.value ? c.pDestDescriptorRangeSizes_.value[destRangeIndex] : 1;

  unsigned destHeapKey = c.pDestDescriptorRangeStarts_.interfaceKeys[destRangeIndex];

  for (unsigned srcRangeIndex = 0; srcRangeIndex < c.NumSrcDescriptorRanges_.value;
       ++srcRangeIndex) {
    unsigned srcRangeSize =
        c.pSrcDescriptorRangeSizes_.value ? c.pSrcDescriptorRangeSizes_.value[srcRangeIndex] : 1;
    auto srcHeapIt =
        descriptorsByHeapIndex_.find(c.pSrcDescriptorRangeStarts_.interfaceKeys[srcRangeIndex]);
    assert(srcHeapIt != descriptorsByHeapIndex_.end());
    for (unsigned srcIndex = 0; srcIndex < srcRangeSize; ++srcIndex, ++destIndex) {
      auto srcIt =
          srcHeapIt->second.find(c.pSrcDescriptorRangeStarts_.indexes[srcRangeIndex] + srcIndex);
      if (destIndex == destRangeSize) {
        destIndex = 0;
        ++destRangeIndex;
        destRangeSize = c.pDestDescriptorRangeSizes_.value
                            ? c.pDestDescriptorRangeSizes_.value[destRangeIndex]
                            : 1;
        destHeapKey = c.pDestDescriptorRangeStarts_.interfaceKeys[destRangeIndex];
      }
      if (srcIt != srcHeapIt->second.end()) {
        unsigned destHeapIndex = c.pDestDescriptorRangeStarts_.indexes[destRangeIndex] + destIndex;
        descriptorsByHeapIndex_[destHeapKey][destHeapIndex].reset(
            copyDescriptor(srcIt->second.get(), destHeapKey, destHeapIndex));
      }
    }
  }
}

DescriptorHeapTracker::DescriptorInfo* DescriptorHeapTracker::copyDescriptor(
    DescriptorInfo* descriptorInfo, unsigned destHeapKey, unsigned destHeapIndex) {
  DescriptorInfo* dest = new DescriptorInfo();
  dest->heapKey = destHeapKey;
  dest->descriptorIndex = destHeapIndex;
  dest->resourceKey = descriptorInfo->resourceKey;
  dest->descriptorType = descriptorInfo->descriptorType;
  return dest;
}

} // namespace DirectX
} // namespace gits

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

void DescriptorHeapTracker::CreateDescriptor(Descriptor* descriptor) {
  m_DescriptorByHeapByIndex[descriptor->HeapKey][descriptor->DescriptorIndex].reset(descriptor);
}

void DescriptorHeapTracker::DestroyObject(unsigned key) {
  auto itHeap = m_DescriptorByHeapByIndex.find(key);
  if (itHeap != m_DescriptorByHeapByIndex.end()) {
    m_DescriptorByHeapByIndex.erase(itHeap);
    return;
  }
}

void DescriptorHeapTracker::CopyDescriptors(ID3D12DeviceCopyDescriptorsSimpleCommand& c) {
  if (!c.m_NumDescriptors.Value) {
    return;
  }
  auto srcHeapIt = m_DescriptorByHeapByIndex.find(c.m_SrcDescriptorRangeStart.InterfaceKey);
  if (srcHeapIt == m_DescriptorByHeapByIndex.end()) {
    return;
  }
  auto& destHeap = m_DescriptorByHeapByIndex[c.m_DestDescriptorRangeStart.InterfaceKey];
  for (unsigned i = 0; i < c.m_NumDescriptors.Value; ++i) {
    auto srcIt = srcHeapIt->second.find(c.m_SrcDescriptorRangeStart.Index + i);
    if (srcIt != srcHeapIt->second.end()) {
      unsigned destHeapIndex = c.m_DestDescriptorRangeStart.Index + i;
      destHeap[destHeapIndex].reset(CopyDescriptor(
          srcIt->second.get(), c.m_DestDescriptorRangeStart.InterfaceKey, destHeapIndex));
    }
  }
}

void DescriptorHeapTracker::CopyDescriptors(ID3D12DeviceCopyDescriptorsCommand& c) {
  if (!c.m_NumDestDescriptorRanges.Value || !c.m_NumSrcDescriptorRanges.Value) {
    return;
  }

  unsigned destRangeIndex = 0;
  unsigned destIndex = 0;
  unsigned destRangeSize =
      c.m_pDestDescriptorRangeSizes.Value ? c.m_pDestDescriptorRangeSizes.Value[destRangeIndex] : 1;

  unsigned destHeapKey = c.m_pDestDescriptorRangeStarts.InterfaceKeys[destRangeIndex];

  for (unsigned srcRangeIndex = 0; srcRangeIndex < c.m_NumSrcDescriptorRanges.Value;
       ++srcRangeIndex) {
    unsigned srcRangeSize =
        c.m_pSrcDescriptorRangeSizes.Value ? c.m_pSrcDescriptorRangeSizes.Value[srcRangeIndex] : 1;
    auto srcHeapIt =
        m_DescriptorByHeapByIndex.find(c.m_pSrcDescriptorRangeStarts.InterfaceKeys[srcRangeIndex]);
    assert(srcHeapIt != m_DescriptorByHeapByIndex.end());
    for (unsigned srcIndex = 0; srcIndex < srcRangeSize; ++srcIndex, ++destIndex) {
      auto srcIt =
          srcHeapIt->second.find(c.m_pSrcDescriptorRangeStarts.Indexes[srcRangeIndex] + srcIndex);
      if (destIndex == destRangeSize) {
        destIndex = 0;
        ++destRangeIndex;
        destRangeSize = c.m_pDestDescriptorRangeSizes.Value
                            ? c.m_pDestDescriptorRangeSizes.Value[destRangeIndex]
                            : 1;
        destHeapKey = c.m_pDestDescriptorRangeStarts.InterfaceKeys[destRangeIndex];
      }
      if (srcIt != srcHeapIt->second.end()) {
        unsigned destHeapIndex = c.m_pDestDescriptorRangeStarts.Indexes[destRangeIndex] + destIndex;
        m_DescriptorByHeapByIndex[destHeapKey][destHeapIndex].reset(
            CopyDescriptor(srcIt->second.get(), destHeapKey, destHeapIndex));
      }
    }
  }
}

DescriptorHeapTracker::Descriptor* DescriptorHeapTracker::CopyDescriptor(
    Descriptor* descriptor, unsigned destHeapKey, unsigned destDescriptorIndex) {
  Descriptor* dest = new Descriptor(*descriptor);
  dest->HeapKey = destHeapKey;
  dest->DescriptorIndex = destDescriptorIndex;
  return dest;
}

} // namespace DirectX
} // namespace gits

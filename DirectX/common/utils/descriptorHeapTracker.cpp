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

void DescriptorHeapTracker::CreateDescriptor(DescriptorInfo* descriptorInfo) {
  m_DescriptorsByHeapIndex[descriptorInfo->HeapKey][descriptorInfo->DescriptorIndex].reset(
      descriptorInfo);
}

void DescriptorHeapTracker::DestroyObject(unsigned key) {
  auto itHeap = m_DescriptorsByHeapIndex.find(key);
  if (itHeap != m_DescriptorsByHeapIndex.end()) {
    m_DescriptorsByHeapIndex.erase(itHeap);
    return;
  }
}

void DescriptorHeapTracker::CopyDescriptors(ID3D12DeviceCopyDescriptorsSimpleCommand& command) {
  if (!command.m_NumDescriptors.Value) {
    return;
  }
  auto srcHeapIt = m_DescriptorsByHeapIndex.find(command.m_SrcDescriptorRangeStart.InterfaceKey);
  if (srcHeapIt == m_DescriptorsByHeapIndex.end()) {
    return;
  }
  auto& destHeap = m_DescriptorsByHeapIndex[command.m_DestDescriptorRangeStart.InterfaceKey];
  for (unsigned i = 0; i < command.m_NumDescriptors.Value; ++i) {
    auto srcIt = srcHeapIt->second.find(command.m_SrcDescriptorRangeStart.Index + i);
    if (srcIt != srcHeapIt->second.end()) {
      unsigned destHeapIndex = command.m_DestDescriptorRangeStart.Index + i;
      destHeap[destHeapIndex].reset(CopyDescriptor(
          srcIt->second.get(), command.m_DestDescriptorRangeStart.InterfaceKey, destHeapIndex));
    }
  }
}

void DescriptorHeapTracker::CopyDescriptors(ID3D12DeviceCopyDescriptorsCommand& command) {
  if (!command.m_NumDestDescriptorRanges.Value || !command.m_NumSrcDescriptorRanges.Value) {
    return;
  }

  unsigned destRangeIndex = 0;
  unsigned destIndex = 0;
  unsigned destRangeSize = command.m_pDestDescriptorRangeSizes.Value
                               ? command.m_pDestDescriptorRangeSizes.Value[destRangeIndex]
                               : 1;

  unsigned destHeapKey = command.m_pDestDescriptorRangeStarts.InterfaceKeys[destRangeIndex];

  for (unsigned srcRangeIndex = 0; srcRangeIndex < command.m_NumSrcDescriptorRanges.Value;
       ++srcRangeIndex) {
    unsigned srcRangeSize = command.m_pSrcDescriptorRangeSizes.Value
                                ? command.m_pSrcDescriptorRangeSizes.Value[srcRangeIndex]
                                : 1;
    auto srcHeapIt = m_DescriptorsByHeapIndex.find(
        command.m_pSrcDescriptorRangeStarts.InterfaceKeys[srcRangeIndex]);
    assert(srcHeapIt != m_DescriptorsByHeapIndex.end());
    for (unsigned srcIndex = 0; srcIndex < srcRangeSize; ++srcIndex, ++destIndex) {
      auto srcIt = srcHeapIt->second.find(
          command.m_pSrcDescriptorRangeStarts.Indexes[srcRangeIndex] + srcIndex);
      if (destIndex == destRangeSize) {
        destIndex = 0;
        ++destRangeIndex;
        destRangeSize = command.m_pDestDescriptorRangeSizes.Value
                            ? command.m_pDestDescriptorRangeSizes.Value[destRangeIndex]
                            : 1;
        destHeapKey = command.m_pDestDescriptorRangeStarts.InterfaceKeys[destRangeIndex];
      }
      if (srcIt != srcHeapIt->second.end()) {
        unsigned destHeapIndex =
            command.m_pDestDescriptorRangeStarts.Indexes[destRangeIndex] + destIndex;
        m_DescriptorsByHeapIndex[destHeapKey][destHeapIndex].reset(
            CopyDescriptor(srcIt->second.get(), destHeapKey, destHeapIndex));
      }
    }
  }
}

DescriptorHeapTracker::DescriptorInfo* DescriptorHeapTracker::CopyDescriptor(
    DescriptorInfo* descriptorInfo, unsigned destHeapKey, unsigned destDescriptorIndex) {
  DescriptorInfo* dest = new DescriptorInfo();
  dest->HeapKey = destHeapKey;
  dest->DescriptorIndex = destDescriptorIndex;
  dest->ResourceKey = descriptorInfo->ResourceKey;
  dest->Kind = descriptorInfo->Kind;
  return dest;
}

} // namespace DirectX
} // namespace gits

// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "heapAllocationService.h"
#include "gits.h"

#include <windows.h>

namespace gits {
namespace DirectX {

void HeapAllocationService::createHeapAllocation(unsigned heapKey,
                                                 void* captureAddress,
                                                 void* data,
                                                 size_t size) {
  void* replayAddress = VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  GITS_ASSERT(replayAddress);
  heapAllocationsByCaptureAddress_[captureAddress] = replayAddress;
  heapAllocationsByReplayAddress_[replayAddress] = captureAddress;
  heapAllocationsCaptureAddressByHeapKey_[heapKey] = captureAddress;
}

void* HeapAllocationService::getHeapAllocation(void* captureAddress) {
  auto it = heapAllocationsByCaptureAddress_.find(captureAddress);
  GITS_ASSERT(it != heapAllocationsByCaptureAddress_.end());
  return it->second;
}

void HeapAllocationService::destroyHeapAllocation(unsigned heapKey) {
  auto itCaptureAddress = heapAllocationsCaptureAddressByHeapKey_.find(heapKey);
  if (itCaptureAddress == heapAllocationsCaptureAddressByHeapKey_.end()) {
    return;
  }

  auto itReplayAddress = heapAllocationsByCaptureAddress_.find(itCaptureAddress->second);
  GITS_ASSERT(itReplayAddress != heapAllocationsByCaptureAddress_.end());

  VirtualFree(itReplayAddress->second, 0, MEM_RELEASE);
  heapAllocationsByCaptureAddress_.erase(itCaptureAddress->second);
  heapAllocationsByReplayAddress_.erase(itReplayAddress->second);
}

} // namespace DirectX
} // namespace gits

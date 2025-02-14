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

void HeapAllocationService::createHeapAllocation(void* captureAddress, void* data, size_t size) {

  void* replayAddress = VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  GITS_ASSERT(replayAddress);
  heapAllocationsByCaptureAddress_[captureAddress] = replayAddress;
  heapAllocationsByReplayAddress_[replayAddress] = captureAddress;
}

void* HeapAllocationService::getHeapAllocation(void* captureAddress) {

  auto it = heapAllocationsByCaptureAddress_.find(captureAddress);
  GITS_ASSERT(it != heapAllocationsByCaptureAddress_.end());
  return it->second;
}

void HeapAllocationService::destroyHeapAllocation(void* replayAddress) {

  VirtualFree(replayAddress, 0, MEM_RELEASE);
  auto it = heapAllocationsByReplayAddress_.find(replayAddress);
  GITS_ASSERT(it != heapAllocationsByReplayAddress_.end());
  heapAllocationsByCaptureAddress_.erase(it->second);
  heapAllocationsByReplayAddress_.erase(it);
}

} // namespace DirectX
} // namespace gits

// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "heapAllocationService.h"
#include "log.h"

#include <windows.h>

namespace gits {
namespace DirectX {

void HeapAllocationService::CreateHeapAllocation(unsigned heapKey,
                                                 void* captureAddress,
                                                 void* data,
                                                 size_t size) {
  void* replayAddress = VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  GITS_ASSERT(replayAddress);
  m_HeapAllocationsByCaptureAddress[captureAddress] = replayAddress;
  m_HeapAllocationsByReplayAddress[replayAddress] = captureAddress;
  m_HeapAllocationsCaptureAddressByHeapKey[heapKey] = captureAddress;
}

void* HeapAllocationService::GetHeapAllocation(void* captureAddress) {
  auto it = m_HeapAllocationsByCaptureAddress.find(captureAddress);
  GITS_ASSERT(it != m_HeapAllocationsByCaptureAddress.end());
  return it->second;
}

void HeapAllocationService::DestroyHeapAllocation(unsigned heapKey) {
  auto itCaptureAddress = m_HeapAllocationsCaptureAddressByHeapKey.find(heapKey);
  if (itCaptureAddress == m_HeapAllocationsCaptureAddressByHeapKey.end()) {
    return;
  }

  auto itReplayAddress = m_HeapAllocationsByCaptureAddress.find(itCaptureAddress->second);
  GITS_ASSERT(itReplayAddress != m_HeapAllocationsByCaptureAddress.end());

  VirtualFree(itReplayAddress->second, 0, MEM_RELEASE);
  m_HeapAllocationsByReplayAddress.erase(itReplayAddress->second);
  m_HeapAllocationsByCaptureAddress.erase(itCaptureAddress->second);
}

} // namespace DirectX
} // namespace gits

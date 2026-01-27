// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "directx/heapAllocationService.h"

#include <cassert>
#include <windows.h>

namespace directx {

HeapAllocationService& HeapAllocationService::Get() {
  static HeapAllocationService s_Instance;
  return s_Instance;
}

void HeapAllocationService::CreateHeapAllocation(unsigned heapKey, size_t size) {
  if (m_HeapAllocations.find(heapKey) != m_HeapAllocations.end()) {
    return;
  }
  void* allocation = VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  m_HeapAllocations[heapKey] = allocation;
}

void* HeapAllocationService::GetHeapAllocation(unsigned heapKey) {
  auto it = m_HeapAllocations.find(heapKey);
  if (it != m_HeapAllocations.end()) {
    return it->second;
  }
  return nullptr;
}

void HeapAllocationService::DestroyHeapAllocation(unsigned heapKey) {
  auto it = m_HeapAllocations.find(heapKey);
  if (it != m_HeapAllocations.end()) {
    VirtualFree(it->second, 0, MEM_RELEASE);
    m_HeapAllocations.erase(it);
  }
}

} // namespace directx

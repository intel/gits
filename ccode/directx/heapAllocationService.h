// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "directx/directx.h"
#include <unordered_map>

namespace directx {

class HeapAllocationService {
public:
  static HeapAllocationService& Get();

  void CreateHeapAllocation(unsigned heapKey, size_t size);
  void* GetHeapAllocation(unsigned heapKey);
  void DestroyHeapAllocation(unsigned heapKey);

private:
  HeapAllocationService() = default;
  ~HeapAllocationService() = default;

  // Prevent copying and assignment
  HeapAllocationService(const HeapAllocationService&) = delete;
  HeapAllocationService& operator=(const HeapAllocationService&) = delete;

  std::unordered_map<unsigned, void*> m_HeapAllocations;
};

} // namespace directx

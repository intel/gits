// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <map>
#include <unordered_map>

namespace gits {
namespace DirectX {

class HeapAllocationService {
public:
  void CreateHeapAllocation(unsigned heapKey, void* captureAddress, void* data, size_t size);
  void* GetHeapAllocation(void* captureAddress);
  void DestroyHeapAllocation(unsigned heapKey);

private:
  std::map<void*, void*> m_HeapAllocationsByCaptureAddress;
  std::map<void*, void*> m_HeapAllocationsByReplayAddress;
  std::unordered_map<unsigned, void*> m_HeapAllocationsCaptureAddressByHeapKey;
};

} // namespace DirectX
} // namespace gits

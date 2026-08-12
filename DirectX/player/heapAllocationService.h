// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"

#include <map>
#include <unordered_map>

namespace gits {
namespace DirectX {

class HeapAllocationService {
public:
  void CreateHeapAllocation(GITSKey heapKey, void* captureAddress, void* data, size_t size);
  void* GetHeapAllocation(void* captureAddress);
  void DestroyHeapAllocation(GITSKey heapKey);

private:
  std::map<void*, void*> m_HeapAllocationsByCaptureAddress;
  std::map<void*, void*> m_HeapAllocationsByReplayAddress;
  std::unordered_map<GITSKey, void*> m_HeapAllocationsCaptureAddressByHeapKey;
};

} // namespace DirectX
} // namespace gits

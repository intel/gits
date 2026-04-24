// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "objectState.h"
#include "log.h"

#include <unordered_map>

namespace gits {
namespace DirectX {

class HeapAllocationStateService {
public:
  void SetHeapState(D3D12HeapFromAddressState* state) {
    m_HeapStates[state->Key] = state;
  }

  D3D12HeapFromAddressState* GetHeapState(unsigned heapKey) {
    auto it = m_HeapStates.find(heapKey);
    GITS_ASSERT(it != m_HeapStates.end());

    D3D12HeapFromAddressState* state = it->second;
    m_HeapStates.erase(it);
    return state;
  }

  void DestroyHeap(unsigned heapKey) {
    m_HeapStates.erase(heapKey);
  }

private:
  std::unordered_map<unsigned, D3D12HeapFromAddressState*> m_HeapStates;
};

} // namespace DirectX
} // namespace gits

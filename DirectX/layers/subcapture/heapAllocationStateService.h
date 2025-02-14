// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "objectState.h"
#include "gits.h"

#include <unordered_map>

namespace gits {
namespace DirectX {

class HeapAllocationStateService {
public:
  void setHeapState(D3D12HeapFromAddressState* state) {
    heapStates_[state->key] = state;
  }

  D3D12HeapFromAddressState* getHeapState(unsigned heapKey) {
    auto it = heapStates_.find(heapKey);
    GITS_ASSERT(it != heapStates_.end());

    D3D12HeapFromAddressState* state = it->second;
    heapStates_.erase(it);
    return state;
  }

  void destroyHeap(unsigned heapKey) {
    heapStates_.erase(heapKey);
  }

private:
  std::unordered_map<unsigned, D3D12HeapFromAddressState*> heapStates_;
};

} // namespace DirectX
} // namespace gits

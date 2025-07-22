// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "directx.h"

#include <unordered_map>
#include <unordered_set>

namespace gits {
namespace DirectX {

class PlayerGpuAddressService {
public:
  void createResource(unsigned resourceKey, ID3D12Resource* resource);
  void createPlacedResource(unsigned resourceKey,
                            ID3D12Resource* resource,
                            unsigned heapKey,
                            ID3D12Heap* heap,
                            UINT64 heapOffset);
  void createHeap(unsigned heapKey, ID3D12Heap* heap);
  D3D12_GPU_VIRTUAL_ADDRESS getGpuAddress(unsigned resourceKey, unsigned offset);
  void destroyInterface(unsigned interfaceKey);

private:
  std::unordered_map<unsigned, D3D12_GPU_VIRTUAL_ADDRESS> startAddressesByKey_;
  std::unordered_map<unsigned, std::unordered_set<unsigned>> placedResourcesByHeap_;
  std::unordered_map<unsigned, unsigned> heapByPlacedResource_;

private:
  D3D12_GPU_VIRTUAL_ADDRESS getHeapGPUVirtualAddress(ID3D12Heap* heap);
};

} // namespace DirectX
} // namespace gits

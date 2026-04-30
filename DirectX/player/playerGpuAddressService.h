// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <d3d12.h>
#include <unordered_map>
#include <unordered_set>

namespace gits {
namespace DirectX {

class PlayerGpuAddressService {
public:
  void CreateResource(unsigned ResourceKey, ID3D12Resource* resource);
  void CreatePlacedResource(unsigned ResourceKey,
                            ID3D12Resource* resource,
                            unsigned heapKey,
                            ID3D12Heap* heap,
                            UINT64 heapOffset);
  void CreateHeap(unsigned heapKey, ID3D12Heap* heap);
  D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress(unsigned ResourceKey, unsigned offset);
  void DestroyInterface(unsigned InterfaceKey);

private:
  std::unordered_map<unsigned, D3D12_GPU_VIRTUAL_ADDRESS> m_StartAddressesByKey;
  std::unordered_set<unsigned> m_PlacedResources;
  std::unordered_map<unsigned, D3D12_GPU_VIRTUAL_ADDRESS> m_ReleasedPlacedResources;

private:
  D3D12_GPU_VIRTUAL_ADDRESS GetHeapGpuVirtualAddress(ID3D12Heap* heap);
};

} // namespace DirectX
} // namespace gits

// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"

#include <d3d12.h>
#include <unordered_map>
#include <unordered_set>

namespace gits {
namespace DirectX {

class PlayerGpuAddressService {
public:
  void CreateResource(GITSKey resourceKey, ID3D12Resource* resource);
  void CreatePlacedResource(GITSKey resourceKey,
                            ID3D12Resource* resource,
                            GITSKey heapKey,
                            ID3D12Heap* heap,
                            UINT64 heapOffset);
  void CreateHeap(GITSKey heapKey, ID3D12Heap* heap);
  D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress(GITSKey resourceKey, unsigned offset);
  void DestroyInterface(GITSKey interfaceKey);

private:
  std::unordered_map<GITSKey, D3D12_GPU_VIRTUAL_ADDRESS> m_StartAddressesByKey;
  std::unordered_set<unsigned> m_PlacedResources;
  std::unordered_map<GITSKey, D3D12_GPU_VIRTUAL_ADDRESS> m_ReleasedPlacedResources;

private:
  D3D12_GPU_VIRTUAL_ADDRESS GetHeapGpuVirtualAddress(ID3D12Heap* heap);
};

} // namespace DirectX
} // namespace gits

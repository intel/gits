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
  void CreateResource(ObjectKey resourceKey, ID3D12Resource* resource);
  void CreatePlacedResource(ObjectKey resourceKey,
                            ID3D12Resource* resource,
                            ObjectKey heapKey,
                            ID3D12Heap* heap,
                            UINT64 heapOffset);
  void CreateHeap(ObjectKey heapKey, ID3D12Heap* heap);
  D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress(ObjectKey resourceKey, unsigned offset);
  void DestroyInterface(ObjectKey interfaceKey);

private:
  std::unordered_map<ObjectKey, D3D12_GPU_VIRTUAL_ADDRESS> m_StartAddressesByKey;
  std::unordered_set<ObjectKey> m_PlacedResources;
  std::unordered_map<ObjectKey, D3D12_GPU_VIRTUAL_ADDRESS> m_ReleasedPlacedResources;

private:
  D3D12_GPU_VIRTUAL_ADDRESS GetHeapGpuVirtualAddress(ID3D12Heap* heap);
};

} // namespace DirectX
} // namespace gits

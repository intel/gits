// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "directx.h"

#include <unordered_map>
#include <unordered_set>

namespace directx {

class GpuAddressService {
public:
  static GpuAddressService& Get();

  void CreateResource(unsigned resourceKey, ID3D12Resource* resource);
  void CreatePlacedResource(unsigned resourceKey,
                            ID3D12Resource* resource,
                            unsigned heapKey,
                            ID3D12Heap* heap,
                            UINT64 heapOffset);
  void CreateHeap(unsigned heapKey, ID3D12Heap* heap);
  D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress(unsigned resourceKey, unsigned offset);
  void DestroyInterface(unsigned interfaceKey);

private:
  GpuAddressService() = default;
  ~GpuAddressService() = default;

  // Prevent copying and assignment
  GpuAddressService(const GpuAddressService&) = delete;
  GpuAddressService& operator=(const GpuAddressService&) = delete;

  std::unordered_map<unsigned, D3D12_GPU_VIRTUAL_ADDRESS> m_StartAddressesByKey;
  std::unordered_set<unsigned> m_PlacedResources;
  std::unordered_map<unsigned, D3D12_GPU_VIRTUAL_ADDRESS> m_ReleasedPlacedResources;

private:
  D3D12_GPU_VIRTUAL_ADDRESS GetHeapGPUVirtualAddress(ID3D12Heap* heap);
};

} // namespace directx

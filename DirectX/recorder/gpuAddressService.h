// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"

#include "directx.h"
#include "tbb/spin_rw_mutex.h"

#include <unordered_set>
#include <map>
#include <unordered_map>
#include <d3d12.h>

namespace gits {
namespace DirectX {

class GpuAddressService {
public:
  struct GpuAddressInfo {
    GITSKey ResourceKey;
    unsigned Offset;
  };

  void CreateResource(GITSKey resourceKey, ID3D12Resource* resource);
  void CreatePlacedResource(GITSKey resourceKey,
                            ID3D12Resource* resource,
                            GITSKey heapKey,
                            ID3D12Heap* heap,
                            UINT64 heapOffset,
                            bool raytracingAS);
  void CreateHeap(GITSKey heapKey, ID3D12Heap* heap);
  GpuAddressInfo GetGpuAddressInfo(UINT64 gpuAddress, bool raytracingAS = false) const;
  void DestroyInterface(GITSKey interfaceKey);

private:
  struct HeapInfo {
    GITSKey Key{};
    D3D12_GPU_VIRTUAL_ADDRESS Start{};
    D3D12_GPU_VIRTUAL_ADDRESS End{};
  };
  struct ResourceInfo {
    GITSKey Key{};
    D3D12_GPU_VIRTUAL_ADDRESS Start{};
    D3D12_GPU_VIRTUAL_ADDRESS End{};
    virtual ~ResourceInfo() = default;
  };
  struct PlacedResourceInfo : public ResourceInfo {
    HeapInfo* HeapInfo{};
    GITSKey HeapKey{};
    unsigned Layer{};
    bool RaytracingAS{};
    std::unordered_set<PlacedResourceInfo*> Intersecting;
  };
  struct HeapInfoLayered : public HeapInfo {
    std::vector<std::map<D3D12_GPU_VIRTUAL_ADDRESS, PlacedResourceInfo*>> Resources;
  };

  std::map<D3D12_GPU_VIRTUAL_ADDRESS, ResourceInfo*> m_ResourcesByStartAddress;
  std::map<D3D12_GPU_VIRTUAL_ADDRESS, HeapInfoLayered*> m_HeapsByStartAddress;

  std::unordered_map<GITSKey, std::unique_ptr<ResourceInfo>> m_ResourcesByKey;
  std::unordered_map<GITSKey, std::unique_ptr<HeapInfoLayered>> m_HeapsByKey;
  std::unordered_map<GITSKey, std::unique_ptr<PlacedResourceInfo>> m_PlacedResourcesByKey;
  std::unordered_map<GITSKey, std::unordered_set<GITSKey>> m_PlacedResourcesByHeap;

  mutable tbb::spin_rw_mutex m_RwMutex;

private:
  const ResourceInfo* GetResourceFromHeap(HeapInfoLayered* heapInfo,
                                          D3D12_GPU_VIRTUAL_ADDRESS gpuAddress,
                                          bool raytracingAS) const;
  D3D12_GPU_VIRTUAL_ADDRESS GetHeapGPUVirtualAddress(ID3D12Heap* heap);
};

} // namespace DirectX
} // namespace gits

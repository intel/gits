// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "directx.h"

#include <shared_mutex>
#include <unordered_set>
#include <map>
#include <unordered_map>

namespace gits {
namespace DirectX {

class GpuAddressService {
public:
  struct GpuAddressInfo {
    unsigned resourceKey;
    unsigned offset;
  };

  void createResource(unsigned resourceKey, ID3D12Resource* resource);
  void createPlacedResource(unsigned resourceKey,
                            ID3D12Resource* resource,
                            unsigned heapKey,
                            ID3D12Heap* heap,
                            UINT64 heapOffset,
                            bool raytracingAS);
  void createHeap(unsigned heapKey, ID3D12Heap* heap);
  GpuAddressInfo getGpuAddressInfo(UINT64 gpuAddress, bool raytracingAS = false) const;
  void destroyInterface(unsigned interfaceKey);

private:
  struct HeapInfo {
    unsigned key;
    D3D12_GPU_VIRTUAL_ADDRESS start;
    D3D12_GPU_VIRTUAL_ADDRESS end;
  };
  struct ResourceInfo {
    unsigned key;
    D3D12_GPU_VIRTUAL_ADDRESS start;
    D3D12_GPU_VIRTUAL_ADDRESS end;
    virtual ~ResourceInfo() = default;
  };
  struct PlacedResourceInfo : public ResourceInfo {
    HeapInfo* heapInfo;
    unsigned heapKey;
    unsigned layer;
    bool raytracingAS;
    std::unordered_set<PlacedResourceInfo*> intersecting;
  };
  struct HeapInfoLayered : public HeapInfo {
    std::vector<std::map<D3D12_GPU_VIRTUAL_ADDRESS, PlacedResourceInfo*>> resources;
  };

  std::map<D3D12_GPU_VIRTUAL_ADDRESS, ResourceInfo*> resourcesByStartAddress_;
  std::map<D3D12_GPU_VIRTUAL_ADDRESS, HeapInfoLayered*> heapsByStartAddress_;

  std::unordered_map<unsigned, std::unique_ptr<ResourceInfo>> resourcesByKey_;
  std::unordered_map<unsigned, std::unique_ptr<HeapInfoLayered>> heapsByKey_;
  std::unordered_map<unsigned, std::unique_ptr<PlacedResourceInfo>> placedResourcesByKey_;
  std::unordered_map<unsigned, std::unordered_set<unsigned>> placedResourcesByHeap_;

  mutable std::shared_mutex rwMutex_;

private:
  const ResourceInfo* getResourceFromHeap(HeapInfoLayered* heapInfo,
                                          D3D12_GPU_VIRTUAL_ADDRESS gpuAddress,
                                          bool raytracingAS) const;
  D3D12_GPU_VIRTUAL_ADDRESS getHeapGPUVirtualAddress(ID3D12Heap* heap);
};

} // namespace DirectX
} // namespace gits

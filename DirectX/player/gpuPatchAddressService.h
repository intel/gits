// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <map>
#include <unordered_map>
#include <memory>
#include <d3d12.h>
#include <unordered_set>

namespace gits {
namespace DirectX {

class GpuPatchAddressService {
public:
  struct GpuAddressMapping {
    D3D12_GPU_VIRTUAL_ADDRESS captureStart;
    D3D12_GPU_VIRTUAL_ADDRESS playerStart;
    UINT64 size;
  };
  struct ResourceInfo : public GpuAddressMapping {
    virtual ~ResourceInfo() {}
    ID3D12Resource* resource;
    unsigned key;
  };

public:
  void createPlacedResource(unsigned heapKey, unsigned resourceKey, D3D12_RESOURCE_FLAGS flags) {
    gpuPatchAddress_.createPlacedResource(heapKey, resourceKey, flags);
    if (gpuPatchPlayerAddress_) {
      gpuPatchPlayerAddress_->createPlacedResource(heapKey, resourceKey, flags);
    }
  }
  void addGpuCaptureAddress(ID3D12Resource* resource,
                            unsigned resourceKey,
                            unsigned size,
                            D3D12_GPU_VIRTUAL_ADDRESS captureAddress) {
    gpuPatchAddress_.addGpuCaptureAddress(resource, resourceKey, size, captureAddress);
  }
  void addGpuPlayerAddress(ID3D12Resource* resource,
                           unsigned resourceKey,
                           unsigned size,
                           D3D12_GPU_VIRTUAL_ADDRESS playerAddress) {
    gpuPatchAddress_.addGpuPlayerAddress(resourceKey, playerAddress);
    if (gpuPatchPlayerAddress_) {
      gpuPatchPlayerAddress_->addGpuCaptureAddress(resource, resourceKey, size, playerAddress);
      gpuPatchPlayerAddress_->addGpuPlayerAddress(resourceKey, playerAddress);
    }
  }
  void destroyInterface(unsigned interfaceKey) {
    gpuPatchAddress_.destroyInterface(interfaceKey);
    if (gpuPatchPlayerAddress_) {
      gpuPatchPlayerAddress_->destroyInterface(interfaceKey);
    }
  }
  ResourceInfo* getResourceInfoByCaptureAddress(D3D12_GPU_VIRTUAL_ADDRESS address) {
    return gpuPatchAddress_.getResourceInfo(address);
  }
  ResourceInfo* getResourceInfoByPlayerAddress(D3D12_GPU_VIRTUAL_ADDRESS address) {
    if (gpuPatchPlayerAddress_) {
      return gpuPatchPlayerAddress_->getResourceInfo(address);
    } else {
      return nullptr;
    }
  }
  void getMappings(std::vector<GpuAddressMapping>& mappings) {
    gpuPatchAddress_.getMappings(mappings);
  }
  void enableDumpLookup() {
    gpuPatchPlayerAddress_.reset(new GpuPatchAddress());
  }

private:
  class GpuPatchAddress {
  public:
    void createPlacedResource(unsigned heapKey, unsigned resourceKey, D3D12_RESOURCE_FLAGS flags);
    void addGpuCaptureAddress(ID3D12Resource* resource,
                              unsigned resourceKey,
                              unsigned size,
                              D3D12_GPU_VIRTUAL_ADDRESS captureAddress);
    void addGpuPlayerAddress(unsigned resourceKey, D3D12_GPU_VIRTUAL_ADDRESS playerAddress);
    void destroyInterface(unsigned interfaceKey);
    void getMappings(std::vector<GpuAddressMapping>& mappings);
    ResourceInfo* getResourceInfo(D3D12_GPU_VIRTUAL_ADDRESS address);

  private:
    struct PlacedResourceInfo : public ResourceInfo {
      unsigned layer;
      std::unordered_set<PlacedResourceInfo*> intersecting;
    };

    std::map<D3D12_GPU_VIRTUAL_ADDRESS, ResourceInfo*> resourcesByAddress_;
    std::vector<std::map<D3D12_GPU_VIRTUAL_ADDRESS, PlacedResourceInfo*>> placedResourcesByAddress_;

    struct HeapInfo {
      D3D12_GPU_VIRTUAL_ADDRESS captureStart;
      D3D12_GPU_VIRTUAL_ADDRESS captureEnd;
      D3D12_GPU_VIRTUAL_ADDRESS playerStart;
      std::unordered_set<unsigned> resources;
    };

    std::unordered_map<unsigned, std::unique_ptr<HeapInfo>> heapsByKey_;
    std::unordered_map<unsigned, HeapInfo*> heapsByResourceKey_;

    std::unordered_map<unsigned, std::unique_ptr<ResourceInfo>> resourcesByKey_;
    std::unordered_map<unsigned, std::unique_ptr<PlacedResourceInfo>> placedResourcesByKey_;
    std::unordered_set<unsigned> deniedShaderResources_;
  };
  GpuPatchAddress gpuPatchAddress_;
  std::unique_ptr<GpuPatchAddress> gpuPatchPlayerAddress_;
};

} // namespace DirectX
} // namespace gits

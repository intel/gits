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
#include <mutex>

namespace gits {
namespace DirectX {

class CapturePlayerGpuAddressService {
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
    std::lock_guard<std::mutex> lock(mutex_);
    gpuAddressService_.createPlacedResource(heapKey, resourceKey, flags);
    if (gpuPlayerAddress_) {
      gpuPlayerAddress_->createPlacedResource(heapKey, resourceKey, flags);
    }
  }
  void addGpuCaptureAddress(ID3D12Resource* resource,
                            unsigned resourceKey,
                            unsigned size,
                            D3D12_GPU_VIRTUAL_ADDRESS captureAddress) {
    std::lock_guard<std::mutex> lock(mutex_);
    gpuAddressService_.addGpuCaptureAddress(resource, resourceKey, size, captureAddress);
  }
  void addGpuPlayerAddress(ID3D12Resource* resource,
                           unsigned resourceKey,
                           unsigned size,
                           D3D12_GPU_VIRTUAL_ADDRESS playerAddress) {
    std::lock_guard<std::mutex> lock(mutex_);
    gpuAddressService_.addGpuPlayerAddress(resourceKey, playerAddress);
    if (gpuPlayerAddress_) {
      gpuPlayerAddress_->addGpuCaptureAddress(resource, resourceKey, size, playerAddress);
      gpuPlayerAddress_->addGpuPlayerAddress(resourceKey, playerAddress);
    }
  }
  void destroyInterface(unsigned interfaceKey) {
    std::lock_guard<std::mutex> lock(mutex_);
    gpuAddressService_.destroyInterface(interfaceKey);
    if (gpuPlayerAddress_) {
      gpuPlayerAddress_->destroyInterface(interfaceKey);
    }
  }
  ResourceInfo* getResourceInfoByCaptureAddress(D3D12_GPU_VIRTUAL_ADDRESS address) {
    std::lock_guard<std::mutex> lock(mutex_);
    return gpuAddressService_.getResourceInfo(address);
  }
  ResourceInfo* getResourceInfoByPlayerAddress(D3D12_GPU_VIRTUAL_ADDRESS address) {
    if (gpuPlayerAddress_) {
      std::lock_guard<std::mutex> lock(mutex_);
      return gpuPlayerAddress_->getResourceInfo(address);
    } else {
      return nullptr;
    }
  }
  void getMappings(std::vector<GpuAddressMapping>& mappings) {
    gpuAddressService_.getMappings(mappings);
  }
  void enablePlayerAddressLookup() {
    gpuPlayerAddress_.reset(new GpuAddressService());
  }

private:
  class GpuAddressService {
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
  GpuAddressService gpuAddressService_;
  std::unique_ptr<GpuAddressService> gpuPlayerAddress_;
  std::mutex mutex_;
};

} // namespace DirectX
} // namespace gits

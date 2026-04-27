// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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
#include <vector>

namespace gits {
namespace DirectX {

class CapturePlayerGpuAddressService {
public:
  struct GpuAddressMapping {
    D3D12_GPU_VIRTUAL_ADDRESS CaptureStart;
    D3D12_GPU_VIRTUAL_ADDRESS PlayerStart;
    UINT64 Size;
  };
  struct ResourceInfo : public GpuAddressMapping {
    virtual ~ResourceInfo() {}
    virtual bool Overlapping() {
      return false;
    }
    ID3D12Resource* Resource;
    unsigned Key;
  };

  void CreatePlacedResource(unsigned heapKey, unsigned resourceKey, D3D12_RESOURCE_FLAGS flags) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_GpuAddressService.CreatePlacedResource(heapKey, resourceKey, flags);
    if (m_GpuPlayerAddress) {
      m_GpuPlayerAddress->CreatePlacedResource(heapKey, resourceKey, flags);
    }
  }
  void AddGpuCaptureAddress(ID3D12Resource* resource,
                            unsigned resourceKey,
                            unsigned size,
                            D3D12_GPU_VIRTUAL_ADDRESS captureAddress) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_GpuAddressService.AddGpuCaptureAddress(resource, resourceKey, size, captureAddress);
  }
  void AddGpuPlayerAddress(ID3D12Resource* resource,
                           unsigned resourceKey,
                           unsigned size,
                           D3D12_GPU_VIRTUAL_ADDRESS playerAddress) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_GpuAddressService.AddGpuPlayerAddress(resourceKey, playerAddress);
    if (m_GpuPlayerAddress) {
      m_GpuPlayerAddress->AddGpuCaptureAddress(resource, resourceKey, size, playerAddress);
      m_GpuPlayerAddress->AddGpuPlayerAddress(resourceKey, playerAddress);
    }
  }
  void DestroyInterface(unsigned interfaceKey) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_GpuAddressService.DestroyInterface(interfaceKey);
    if (m_GpuPlayerAddress) {
      m_GpuPlayerAddress->DestroyInterface(interfaceKey);
    }
  }
  ResourceInfo* GetResourceInfoByCaptureAddress(D3D12_GPU_VIRTUAL_ADDRESS address) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_GpuAddressService.GetResourceInfo(address);
  }
  ResourceInfo* GetResourceInfoByPlayerAddress(D3D12_GPU_VIRTUAL_ADDRESS address) {
    if (m_GpuPlayerAddress) {
      std::lock_guard<std::mutex> lock(m_Mutex);
      return m_GpuPlayerAddress->GetResourceInfo(address);
    }
    return nullptr;
  }
  void GetMappings(std::vector<GpuAddressMapping>& mappings) {
    m_GpuAddressService.GetMappings(mappings);
  }
  void EnablePlayerAddressLookup() {
    if (!m_GpuPlayerAddress) {
      m_GpuPlayerAddress.reset(new GpuAddressService());
    }
  }

private:
  class GpuAddressService {
  public:
    void CreatePlacedResource(unsigned heapKey, unsigned resourceKey, D3D12_RESOURCE_FLAGS flags);
    void AddGpuCaptureAddress(ID3D12Resource* resource,
                              unsigned resourceKey,
                              unsigned size,
                              D3D12_GPU_VIRTUAL_ADDRESS captureAddress);
    void AddGpuPlayerAddress(unsigned resourceKey, D3D12_GPU_VIRTUAL_ADDRESS playerAddress);
    void DestroyInterface(unsigned interfaceKey);
    void GetMappings(std::vector<GpuAddressMapping>& mappings);
    ResourceInfo* GetResourceInfo(D3D12_GPU_VIRTUAL_ADDRESS address);

  private:
    struct PlacedResourceInfo : public ResourceInfo {
      bool Overlapping() override {
        return !Intersecting.empty();
      }
      unsigned Layer;
      std::unordered_set<PlacedResourceInfo*> Intersecting;
    };

    std::map<D3D12_GPU_VIRTUAL_ADDRESS, ResourceInfo*> m_ResourcesByAddress;
    std::vector<std::map<D3D12_GPU_VIRTUAL_ADDRESS, PlacedResourceInfo*>>
        m_PlacedResourcesByAddress;

    struct HeapInfo {
      D3D12_GPU_VIRTUAL_ADDRESS CaptureStart;
      D3D12_GPU_VIRTUAL_ADDRESS CaptureEnd;
      D3D12_GPU_VIRTUAL_ADDRESS PlayerStart;
      std::unordered_set<unsigned> Resources;
    };

    std::unordered_map<unsigned, std::unique_ptr<HeapInfo>> m_HeapsByKey;
    std::unordered_map<unsigned, HeapInfo*> m_HeapsByResourceKey;

    std::unordered_map<unsigned, std::unique_ptr<ResourceInfo>> m_ResourcesByKey;
    std::unordered_map<unsigned, std::unique_ptr<PlacedResourceInfo>> m_PlacedResourcesByKey;
    std::unordered_set<unsigned> m_DeniedShaderResources;
  };
  GpuAddressService m_GpuAddressService;
  std::unique_ptr<GpuAddressService> m_GpuPlayerAddress;
  std::mutex m_Mutex;
};

} // namespace DirectX
} // namespace gits

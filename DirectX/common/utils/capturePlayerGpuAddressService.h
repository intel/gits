// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"

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
    D3D12_GPU_VIRTUAL_ADDRESS CaptureStart{};
    D3D12_GPU_VIRTUAL_ADDRESS PlayerStart{};
    UINT64 Size{};
  };
  struct ResourceInfo : public GpuAddressMapping {
    virtual ~ResourceInfo() {}
    virtual bool Overlapping() {
      return false;
    }
    ID3D12Resource* Resource{};
    ObjectKey Key{};
  };

  void CreatePlacedResource(ObjectKey heapKey,
                            ObjectKey resourceKey,
                            D3D12_RESOURCE_FLAGS flags,
                            D3D12_RESOURCE_STATES initialState);
  void CreatePlacedResource(ObjectKey heapKey,
                            ObjectKey resourceKey,
                            D3D12_RESOURCE_FLAGS flags,
                            D3D12_BARRIER_LAYOUT initialLayout);
  void AddGpuCaptureAddress(ID3D12Resource* resource,
                            ObjectKey resourceKey,
                            unsigned size,
                            D3D12_GPU_VIRTUAL_ADDRESS captureAddress);
  void AddGpuPlayerAddress(ID3D12Resource* resource,
                           ObjectKey resourceKey,
                           unsigned size,
                           D3D12_GPU_VIRTUAL_ADDRESS playerAddress);
  void DestroyInterface(ObjectKey interfaceKey);
  ResourceInfo* GetResourceInfoByCaptureAddress(D3D12_GPU_VIRTUAL_ADDRESS address,
                                                bool raytracingAS = false);
  ResourceInfo* GetResourceInfoByPlayerAddress(D3D12_GPU_VIRTUAL_ADDRESS address,
                                               bool raytracingAS = false);
  void GetMappings(std::vector<GpuAddressMapping>& mappings);
  void EnablePlayerAddressLookup();

private:
  class GpuAddressService {
  public:
    void CreatePlacedResource(ObjectKey heapKey,
                              ObjectKey resourceKey,
                              D3D12_RESOURCE_FLAGS flags,
                              bool raytracingAS);
    void AddGpuCaptureAddress(ID3D12Resource* resource,
                              ObjectKey resourceKey,
                              unsigned size,
                              D3D12_GPU_VIRTUAL_ADDRESS captureAddress);
    void AddGpuPlayerAddress(ObjectKey resourceKey, D3D12_GPU_VIRTUAL_ADDRESS playerAddress);
    void DestroyInterface(ObjectKey interfaceKey);
    void GetMappings(std::vector<GpuAddressMapping>& mappings);
    ResourceInfo* GetResourceInfo(D3D12_GPU_VIRTUAL_ADDRESS address, bool raytracingAS);

  private:
    struct PlacedResourceInfo : public ResourceInfo {
      bool Overlapping() override {
        return !Intersecting.empty();
      }
      unsigned Layer{};
      std::unordered_set<PlacedResourceInfo*> Intersecting;
      bool DeniedShaderResource{};
      bool RaytracingAS{};
      ObjectKey HeapKey{};
    };

    std::map<D3D12_GPU_VIRTUAL_ADDRESS, ResourceInfo*> m_ResourcesByAddress;
    std::vector<std::map<D3D12_GPU_VIRTUAL_ADDRESS, PlacedResourceInfo*>>
        m_PlacedResourcesByAddress;

    struct HeapInfo {
      D3D12_GPU_VIRTUAL_ADDRESS CaptureStart;
      D3D12_GPU_VIRTUAL_ADDRESS CaptureEnd;
      D3D12_GPU_VIRTUAL_ADDRESS PlayerStart;
      std::unordered_set<ObjectKey> Resources;
    };

    std::unordered_map<ObjectKey, std::unique_ptr<HeapInfo>> m_HeapsByKey;

    std::unordered_map<ObjectKey, std::unique_ptr<ResourceInfo>> m_ResourcesByKey;
    std::unordered_map<ObjectKey, std::unique_ptr<PlacedResourceInfo>> m_PlacedResourcesByKey;
  };
  GpuAddressService m_GpuAddressService;
  std::unique_ptr<GpuAddressService> m_GpuPlayerAddress;
  std::mutex m_Mutex;
};

} // namespace DirectX
} // namespace gits

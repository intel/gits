// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "gpuAddressService.h"
#include "log.h"

#include <wrl/client.h>

namespace gits {
namespace DirectX {

void GpuAddressService::CreateResource(unsigned resourceKey, ID3D12Resource* resource) {

  D3D12_RESOURCE_DESC desc = resource->GetDesc();
  if (desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }

  tbb::spin_rw_mutex::scoped_lock lock(m_RwMutex);

  ResourceInfo* resourceInfo = new ResourceInfo{};
  resourceInfo->Key = resourceKey;
  resourceInfo->Start = resource->GetGPUVirtualAddress();
  resourceInfo->End = resourceInfo->Start + desc.Width;
  if (!resourceInfo->Start) {
    LOG_ERROR << "GpuAddressService: can't GetGPUVirtualAddress for O" << resourceKey;
  }

  {
    std::vector<ResourceInfo*> intersecting;
    for (auto& it : m_ResourcesByStartAddress) {
      if (it.second->End > resourceInfo->Start && it.second->Start < resourceInfo->End) {
        intersecting.push_back(it.second);
      }
    }
    // resource can be already removed but destroyInterface method not called yet because of multithreading
    for (ResourceInfo* info : intersecting) {
      auto itResource = m_ResourcesByKey.find(info->Key);
      if (itResource != m_ResourcesByKey.end()) {
        m_ResourcesByStartAddress.erase(itResource->second->Start);
        m_ResourcesByKey.erase(itResource);
      }
    }
  }

  m_ResourcesByStartAddress[resourceInfo->Start] = resourceInfo;
  m_ResourcesByKey[resourceKey].reset(resourceInfo);
}

void GpuAddressService::CreatePlacedResource(unsigned resourceKey,
                                             ID3D12Resource* resource,
                                             unsigned heapKey,
                                             ID3D12Heap* heap,
                                             UINT64 heapOffset,
                                             bool raytracingAS) {
  D3D12_HEAP_DESC heapDesc = heap->GetDesc();
  if (heapDesc.Flags & D3D12_HEAP_FLAG_DENY_BUFFERS) {
    return;
  }

  D3D12_RESOURCE_DESC desc = resource->GetDesc();
  if (desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }

  tbb::spin_rw_mutex::scoped_lock lock(m_RwMutex);

  auto itHeapByKey = m_HeapsByKey.find(heapKey);
  GITS_ASSERT(itHeapByKey != m_HeapsByKey.end());

  HeapInfoLayered* heapInfo = itHeapByKey->second.get();

  PlacedResourceInfo* resourceInfo = new PlacedResourceInfo{};
  resourceInfo->Key = resourceKey;
  resourceInfo->Start = resource->GetGPUVirtualAddress();
  resourceInfo->End = resourceInfo->Start + desc.Width;
  resourceInfo->HeapInfo = heapInfo;
  resourceInfo->HeapKey = heapKey;
  resourceInfo->RaytracingAS = raytracingAS;

  GITS_ASSERT(resourceInfo->Start);

  bool stored = false;
  for (unsigned layerIndex = 0; layerIndex < heapInfo->Resources.size(); ++layerIndex) {
    bool intersecting = false;
    for (auto& it : heapInfo->Resources[layerIndex]) {
      if (it.second->End > resourceInfo->Start && it.second->Start < resourceInfo->End) {
        resourceInfo->Intersecting.insert(it.second);
        intersecting = true;
      }
    }
    if (!intersecting && !stored) {
      resourceInfo->Layer = layerIndex;
      heapInfo->Resources[layerIndex][resourceInfo->Start] = resourceInfo;
      stored = true;
    }
  }

  if (!stored) {
    resourceInfo->Layer = static_cast<unsigned>(heapInfo->Resources.size());
    heapInfo->Resources.push_back(std::map<D3D12_GPU_VIRTUAL_ADDRESS, PlacedResourceInfo*>{});
    heapInfo->Resources[heapInfo->Resources.size() - 1][resourceInfo->Start] = resourceInfo;
  }

  for (PlacedResourceInfo* info : resourceInfo->Intersecting) {
    info->Intersecting.insert(resourceInfo);
  }

  m_PlacedResourcesByKey[resourceInfo->Key].reset(resourceInfo);
  m_PlacedResourcesByHeap[resourceInfo->HeapInfo->Key].insert(resourceInfo->Key);
}

void GpuAddressService::CreateHeap(unsigned heapKey, ID3D12Heap* heap) {

  D3D12_HEAP_DESC desc = heap->GetDesc();
  if (desc.Flags & D3D12_HEAP_FLAG_DENY_BUFFERS) {
    return;
  }

  HeapInfoLayered* heapInfo = new HeapInfoLayered{};
  heapInfo->Key = heapKey;
  heapInfo->Start = GetHeapGPUVirtualAddress(heap);
  heapInfo->End = heapInfo->Start + desc.SizeInBytes;

  if (!heapInfo->Start) {
    delete heapInfo;
    return;
  }

  tbb::spin_rw_mutex::scoped_lock lock(m_RwMutex);

  // check for resources intersections
  {
    std::vector<ResourceInfo*> intersecting;
    for (auto& it : m_ResourcesByStartAddress) {
      if (it.second->End > heapInfo->Start && it.second->Start < heapInfo->End) {
        intersecting.push_back(it.second);
      }
    }
    GITS_ASSERT(intersecting.empty());
  }

  // check for heaps intersections
  {
    std::vector<HeapInfoLayered*> intersecting;
    for (auto& it : m_HeapsByStartAddress) {
      if (it.second->End > heapInfo->Start && it.second->Start < heapInfo->End) {
        intersecting.push_back(it.second);
      }
    }
    GITS_ASSERT(intersecting.empty());
  }

  m_HeapsByStartAddress[heapInfo->Start] = heapInfo;
  m_HeapsByKey[heapKey].reset(heapInfo);
}

D3D12_GPU_VIRTUAL_ADDRESS GpuAddressService::GetHeapGPUVirtualAddress(ID3D12Heap* heap) {

  D3D12_HEAP_DESC heapDesc = heap->GetDesc();

  D3D12_RESOURCE_DESC dummyDesc{};
  dummyDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  dummyDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
  dummyDesc.Width = 1;
  dummyDesc.Height = 1;
  dummyDesc.DepthOrArraySize = 1;
  dummyDesc.MipLevels = 1;
  dummyDesc.Format = DXGI_FORMAT_UNKNOWN;
  dummyDesc.SampleDesc = {1, 0};
  dummyDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  dummyDesc.Flags = heapDesc.Flags & D3D12_HEAP_FLAG_SHARED_CROSS_ADAPTER
                        ? D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER
                        : D3D12_RESOURCE_FLAG_NONE;

  Microsoft::WRL::ComPtr<ID3D12Device> device;
  HRESULT res = heap->GetDevice(IID_PPV_ARGS(&device));
  GITS_ASSERT(res == S_OK);

  D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;
  if (heapDesc.Properties.Type == D3D12_HEAP_TYPE_UPLOAD ||
      heapDesc.Properties.Type == D3D12_HEAP_TYPE_GPU_UPLOAD) {
    initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
  } else if (heapDesc.Properties.Type == D3D12_HEAP_TYPE_READBACK) {
    initialState = D3D12_RESOURCE_STATE_COPY_DEST;
  }

  Microsoft::WRL::ComPtr<ID3D12Resource> dummyResource;
  res = device->CreatePlacedResource(heap, 0, &dummyDesc, initialState, nullptr,
                                     IID_PPV_ARGS(&dummyResource));
  GITS_ASSERT(res == S_OK);

  D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = dummyResource->GetGPUVirtualAddress();
  GITS_ASSERT(gpuAddress);

  return gpuAddress;
}

GpuAddressService::GpuAddressInfo GpuAddressService::GetGpuAddressInfo(
    D3D12_GPU_VIRTUAL_ADDRESS gpuAddress, bool raytracingAS) const {

  if (!gpuAddress) {
    return GpuAddressInfo{};
  }

  tbb::spin_rw_mutex::scoped_lock lock(m_RwMutex, false);

  const ResourceInfo* resourceInfo = nullptr;

  auto itResource = m_ResourcesByStartAddress.upper_bound(gpuAddress);
  if (itResource != m_ResourcesByStartAddress.begin() && !m_ResourcesByStartAddress.empty()) {
    --itResource;
    ResourceInfo* info = itResource->second;
    if (gpuAddress >= info->Start && gpuAddress < info->End) {
      resourceInfo = info;
    }
  }

  if (!resourceInfo) {
    HeapInfoLayered* heapInfo = nullptr;

    auto itHeap = m_HeapsByStartAddress.upper_bound(gpuAddress);
    if (itHeap != m_HeapsByStartAddress.begin() && !m_HeapsByStartAddress.empty()) {
      --itHeap;
      HeapInfoLayered* info = itHeap->second;
      if (gpuAddress >= info->Start && gpuAddress < info->End) {
        heapInfo = info;
      }
    }
    if (heapInfo) {
      resourceInfo = GetResourceFromHeap(heapInfo, gpuAddress, raytracingAS);
    }
  }

  HeapInfoLayered* heapInfo = nullptr;
  if (!resourceInfo) {
    auto itHeap = m_HeapsByStartAddress.upper_bound(gpuAddress);
    if (itHeap != m_HeapsByStartAddress.begin() && !m_HeapsByStartAddress.empty()) {
      --itHeap;
      HeapInfoLayered* info = itHeap->second;
      if (gpuAddress >= info->Start && gpuAddress < info->End) {
        heapInfo = info;
      }
    }
  }

  if (!resourceInfo && !heapInfo) {
    LOG_ERROR << "GpuAddressService: can't find resource for gpu address " << gpuAddress;
  }

  GpuAddressInfo info{};
  if (resourceInfo) {
    info.ResourceKey = resourceInfo->Key;
    info.Offset = gpuAddress - resourceInfo->Start;
  } else if (heapInfo) {
    info.ResourceKey = heapInfo->Key;
    info.Offset = gpuAddress - heapInfo->Start;
  }

  return info;
}

const GpuAddressService::ResourceInfo* GpuAddressService::GetResourceFromHeap(
    HeapInfoLayered* heapInfo, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress, bool raytracingAS) const {

  const PlacedResourceInfo* resourceInfo = nullptr;
  for (unsigned layerIndex = 0; layerIndex < heapInfo->Resources.size(); ++layerIndex) {
    auto itResource = heapInfo->Resources[layerIndex].upper_bound(gpuAddress);
    if (itResource != heapInfo->Resources[layerIndex].begin() &&
        !heapInfo->Resources[layerIndex].empty()) {
      --itResource;
      const PlacedResourceInfo* info = itResource->second;
      if (gpuAddress >= info->Start && gpuAddress < info->End) {
        resourceInfo = info;
        break;
      }
    }
  }

  if (resourceInfo && !resourceInfo->Intersecting.empty()) {
    const PlacedResourceInfo* selectedResource = resourceInfo;
    for (const PlacedResourceInfo* resource : resourceInfo->Intersecting) {
      if (gpuAddress >= resource->Start && gpuAddress < resource->End &&
          resource->RaytracingAS == raytracingAS) {
        if (resource->End > selectedResource->End ||
            selectedResource->RaytracingAS != raytracingAS) {
          selectedResource = resource;
        }
      }
    }
    resourceInfo = selectedResource;
  }

  return resourceInfo;
}

void GpuAddressService::DestroyInterface(unsigned interfaceKey) {

  tbb::spin_rw_mutex::scoped_lock lock(m_RwMutex);

  auto itResource = m_ResourcesByKey.find(interfaceKey);
  if (itResource != m_ResourcesByKey.end()) {
    m_ResourcesByStartAddress.erase(itResource->second->Start);
    m_ResourcesByKey.erase(itResource);
    return;
  }

  auto itPlacedResource = m_PlacedResourcesByKey.find(interfaceKey);
  if (itPlacedResource != m_PlacedResourcesByKey.end()) {
    PlacedResourceInfo* info = itPlacedResource->second.get();

    for (PlacedResourceInfo* intersecting : info->Intersecting) {
      intersecting->Intersecting.erase(info);
    }

    auto itHeap = m_HeapsByKey.find(info->HeapKey);
    if (itHeap != m_HeapsByKey.end()) {
      HeapInfoLayered* heapInfo = itHeap->second.get();
      heapInfo->Resources[info->Layer].erase(info->Start);
    }

    m_PlacedResourcesByKey.erase(itPlacedResource);
    return;
  }

  auto itHeap = m_HeapsByKey.find(interfaceKey);
  if (itHeap != m_HeapsByKey.end()) {
    m_HeapsByStartAddress.erase(itHeap->second->Start);
    m_HeapsByKey.erase(itHeap);

    auto itPlacedResourceHeap = m_PlacedResourcesByHeap.find(interfaceKey);
    if (itPlacedResourceHeap != m_PlacedResourcesByHeap.end()) {
      for (unsigned placedResourceKey : itPlacedResourceHeap->second) {
        m_PlacedResourcesByKey.erase(placedResourceKey);
      }

      m_PlacedResourcesByHeap.erase(interfaceKey);
    }
  }
}

} // namespace DirectX
} // namespace gits

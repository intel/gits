// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "gpuAddressService.h"
#include "gits.h"
#include "log2.h"

#include <wrl/client.h>

namespace gits {
namespace DirectX {

void GpuAddressService::createResource(unsigned resourceKey, ID3D12Resource* resource) {

  D3D12_RESOURCE_DESC desc = resource->GetDesc();
  if (desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }

  std::lock_guard<std::mutex> lock(mutex_);

  ResourceInfo* resourceInfo = new ResourceInfo{};
  resourceInfo->key = resourceKey;
  resourceInfo->start = resource->GetGPUVirtualAddress();
  resourceInfo->end = resourceInfo->start + desc.Width;
  if (!resourceInfo->start) {
    LOG_ERROR << "GpuAddressService: can't GetGPUVirtualAddress for O" << resourceKey;
  }

  {
    std::vector<ResourceInfo*> intersecting;
    for (auto& it : resourcesByStartAddress_) {
      if (it.second->end > resourceInfo->start && it.second->start < resourceInfo->end) {
        intersecting.push_back(it.second);
      }
    }
    // resource can be already removed but destroyInterface method not called yet because of multithreading
    for (ResourceInfo* info : intersecting) {
      auto itResource = resourcesByKey_.find(info->key);
      if (itResource != resourcesByKey_.end()) {
        resourcesByStartAddress_.erase(itResource->second->start);
        resourcesByKey_.erase(itResource);
      }
    }
  }

  resourcesByStartAddress_[resourceInfo->start] = resourceInfo;
  resourcesByKey_[resourceKey].reset(resourceInfo);
}

void GpuAddressService::createPlacedResource(unsigned resourceKey,
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

  std::lock_guard<std::mutex> lock(mutex_);

  auto itHeapByKey = heapsByKey_.find(heapKey);
  GITS_ASSERT(itHeapByKey != heapsByKey_.end());

  HeapInfoLayered* heapInfo = itHeapByKey->second.get();

  PlacedResourceInfo* resourceInfo = new PlacedResourceInfo{};
  resourceInfo->key = resourceKey;
  resourceInfo->start = resource->GetGPUVirtualAddress();
  resourceInfo->end = resourceInfo->start + desc.Width;
  resourceInfo->heapInfo = heapInfo;
  resourceInfo->heapKey = heapKey;
  resourceInfo->raytracingAS = raytracingAS;

  GITS_ASSERT(resourceInfo->start);

  bool stored = false;
  for (unsigned layerIndex = 0; layerIndex < heapInfo->resources.size(); ++layerIndex) {
    bool intersecting = false;
    for (auto& it : heapInfo->resources[layerIndex]) {
      if (it.second->end > resourceInfo->start && it.second->start < resourceInfo->end) {
        resourceInfo->intersecting.insert(it.second);
        intersecting = true;
      }
    }
    if (!intersecting && !stored) {
      resourceInfo->layer = layerIndex;
      heapInfo->resources[layerIndex][resourceInfo->start] = resourceInfo;
      stored = true;
    }
  }

  if (!stored) {
    resourceInfo->layer = static_cast<unsigned>(heapInfo->resources.size());
    heapInfo->resources.push_back(std::map<D3D12_GPU_VIRTUAL_ADDRESS, PlacedResourceInfo*>{});
    heapInfo->resources[heapInfo->resources.size() - 1][resourceInfo->start] = resourceInfo;
  }

  for (PlacedResourceInfo* info : resourceInfo->intersecting) {
    info->intersecting.insert(resourceInfo);
  }

  placedResourcesByKey_[resourceInfo->key].reset(resourceInfo);
  placedResourcesByHeap_[resourceInfo->heapInfo->key].insert(resourceInfo->key);
}

void GpuAddressService::createHeap(unsigned heapKey, ID3D12Heap* heap) {

  D3D12_HEAP_DESC desc = heap->GetDesc();
  if (desc.Flags & D3D12_HEAP_FLAG_DENY_BUFFERS) {
    return;
  }

  HeapInfoLayered* heapInfo = new HeapInfoLayered{};
  heapInfo->key = heapKey;
  heapInfo->start = getHeapGPUVirtualAddress(heap);
  heapInfo->end = heapInfo->start + desc.SizeInBytes;

  if (!heapInfo->start) {
    delete heapInfo;
    return;
  }

  std::lock_guard<std::mutex> lock(mutex_);

  // check for resources intersections
  {
    std::vector<ResourceInfo*> intersecting;
    for (auto& it : resourcesByStartAddress_) {
      if (it.second->end > heapInfo->start && it.second->start < heapInfo->end) {
        intersecting.push_back(it.second);
      }
    }
    GITS_ASSERT(intersecting.empty());
  }

  // check for heaps intersections
  {
    std::vector<HeapInfoLayered*> intersecting;
    for (auto& it : heapsByStartAddress_) {
      if (it.second->end > heapInfo->start && it.second->start < heapInfo->end) {
        intersecting.push_back(it.second);
      }
    }
    GITS_ASSERT(intersecting.empty());
  }

  heapsByStartAddress_[heapInfo->start] = heapInfo;
  heapsByKey_[heapKey].reset(heapInfo);
}

D3D12_GPU_VIRTUAL_ADDRESS GpuAddressService::getHeapGPUVirtualAddress(ID3D12Heap* heap) {

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

GpuAddressService::GpuAddressInfo GpuAddressService::getGpuAddressInfo(
    D3D12_GPU_VIRTUAL_ADDRESS gpuAddress, bool raytracingAS) {

  if (!gpuAddress) {
    return GpuAddressInfo{};
  }

  std::lock_guard<std::mutex> lock(mutex_);

  ResourceInfo* resourceInfo = nullptr;

  auto itResource = resourcesByStartAddress_.upper_bound(gpuAddress);
  if (itResource != resourcesByStartAddress_.begin() && !resourcesByStartAddress_.empty()) {
    --itResource;
    ResourceInfo* info = itResource->second;
    if (gpuAddress >= info->start && gpuAddress < info->end) {
      resourceInfo = info;
    }
  }

  if (!resourceInfo) {
    HeapInfoLayered* heapInfo = nullptr;

    auto itHeap = heapsByStartAddress_.upper_bound(gpuAddress);
    if (itHeap != heapsByStartAddress_.begin() && !heapsByStartAddress_.empty()) {
      --itHeap;
      HeapInfoLayered* info = itHeap->second;
      if (gpuAddress >= info->start && gpuAddress < info->end) {
        heapInfo = info;
      }
    }
    if (heapInfo) {
      resourceInfo = getResourceFromHeap(heapInfo, gpuAddress, raytracingAS);
    }
  }

  HeapInfoLayered* heapInfo = nullptr;
  if (!resourceInfo) {
    auto itHeap = heapsByStartAddress_.upper_bound(gpuAddress);
    if (itHeap != heapsByStartAddress_.begin() && !heapsByStartAddress_.empty()) {
      --itHeap;
      HeapInfoLayered* info = itHeap->second;
      if (gpuAddress >= info->start && gpuAddress < info->end) {
        heapInfo = info;
      }
    }
  }

  if (!resourceInfo && !heapInfo) {
    LOG_ERROR << "GpuAddressService: can't find resource for gpu address " << gpuAddress;
  }

  GpuAddressInfo info{};
  if (resourceInfo) {
    info.resourceKey = resourceInfo->key;
    info.offset = gpuAddress - resourceInfo->start;
  } else if (heapInfo) {
    info.resourceKey = heapInfo->key;
    info.offset = gpuAddress - heapInfo->start;
  }

  return info;
}

GpuAddressService::ResourceInfo* GpuAddressService::getResourceFromHeap(
    HeapInfoLayered* heapInfo, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress, bool raytracingAS) {

  PlacedResourceInfo* resourceInfo = nullptr;
  for (unsigned layerIndex = 0; layerIndex < heapInfo->resources.size(); ++layerIndex) {
    auto itResource = heapInfo->resources[layerIndex].upper_bound(gpuAddress);
    if (itResource != heapInfo->resources[layerIndex].begin() &&
        !heapInfo->resources[layerIndex].empty()) {
      --itResource;
      PlacedResourceInfo* info = itResource->second;
      if (gpuAddress >= info->start && gpuAddress < info->end) {
        resourceInfo = info;
        break;
      }
    }
  }

  if (resourceInfo && !resourceInfo->intersecting.empty()) {
    PlacedResourceInfo* selectedResource = resourceInfo;
    for (PlacedResourceInfo* resource : resourceInfo->intersecting) {
      if (gpuAddress >= resource->start && gpuAddress < resource->end &&
          resource->raytracingAS == raytracingAS) {
        if (resource->end > selectedResource->end ||
            selectedResource->raytracingAS != raytracingAS) {
          selectedResource = resource;
        }
      }
    }
    resourceInfo = selectedResource;
  }

  return resourceInfo;
}

void GpuAddressService::destroyInterface(unsigned interfaceKey) {

  std::lock_guard<std::mutex> lock(mutex_);

  auto itResource = resourcesByKey_.find(interfaceKey);
  if (itResource != resourcesByKey_.end()) {
    resourcesByStartAddress_.erase(itResource->second->start);
    resourcesByKey_.erase(itResource);
    return;
  }

  auto itPlacedResource = placedResourcesByKey_.find(interfaceKey);
  if (itPlacedResource != placedResourcesByKey_.end()) {
    PlacedResourceInfo* info = itPlacedResource->second.get();

    for (PlacedResourceInfo* intersecting : info->intersecting) {
      intersecting->intersecting.erase(info);
    }

    auto itHeap = heapsByKey_.find(info->heapKey);
    if (itHeap != heapsByKey_.end()) {
      HeapInfoLayered* heapInfo = itHeap->second.get();
      heapInfo->resources[info->layer].erase(info->start);
    }

    placedResourcesByKey_.erase(itPlacedResource);
    return;
  }

  auto itHeap = heapsByKey_.find(interfaceKey);
  if (itHeap != heapsByKey_.end()) {
    heapsByStartAddress_.erase(itHeap->second->start);
    heapsByKey_.erase(itHeap);

    auto itPlacedResourceHeap = placedResourcesByHeap_.find(interfaceKey);
    if (itPlacedResourceHeap != placedResourcesByHeap_.end()) {
      for (unsigned placedResourceKey : itPlacedResourceHeap->second) {
        placedResourcesByKey_.erase(placedResourceKey);
      }

      placedResourcesByHeap_.erase(interfaceKey);
    }
  }
}

} // namespace DirectX
} // namespace gits

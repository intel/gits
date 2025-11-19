// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "capturePlayerGpuAddressService.h"
#include "gits.h"

#include <algorithm>

namespace gits {
namespace DirectX {

void CapturePlayerGpuAddressService::GpuAddressService::createPlacedResource(
    unsigned heapKey, unsigned resourceKey, D3D12_RESOURCE_FLAGS flags) {
  HeapInfo* heapInfo{};
  auto it = heapsByKey_.find(heapKey);
  if (it != heapsByKey_.end()) {
    heapInfo = it->second.get();
  } else {
    heapInfo = new HeapInfo{};
    heapsByKey_[heapKey].reset(heapInfo);
  }
  heapInfo->resources.insert(resourceKey);
  heapsByResourceKey_[resourceKey] = heapInfo;

  if (flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) {
    deniedShaderResources_.insert(resourceKey);
  }
}

void CapturePlayerGpuAddressService::GpuAddressService::addGpuCaptureAddress(
    ID3D12Resource* resource,
    unsigned resourceKey,
    unsigned size,
    D3D12_GPU_VIRTUAL_ADDRESS captureAddress) {
  if (!captureAddress) {
    return;
  }
  auto itHeap = heapsByResourceKey_.find(resourceKey);
  if (itHeap != heapsByResourceKey_.end()) {
    auto it = placedResourcesByKey_.find(resourceKey);
    if (it != placedResourcesByKey_.end()) {
      return;
    }

    PlacedResourceInfo* info = new PlacedResourceInfo{};
    info->captureStart = captureAddress;
    info->key = resourceKey;
    info->size = size;
    info->resource = resource;

    bool stored = false;
    for (unsigned layerIndex = 0; layerIndex < placedResourcesByAddress_.size(); ++layerIndex) {
      bool intersecting = false;
      for (auto& it : placedResourcesByAddress_[layerIndex]) {
        if (it.second->captureStart + it.second->size > info->captureStart &&
            it.second->captureStart < info->captureStart + info->size) {
          info->intersecting.insert(it.second);
          intersecting = true;
        }
      }
      if (!intersecting && !stored) {
        info->layer = layerIndex;
        placedResourcesByAddress_[layerIndex][info->captureStart] = info;
        stored = true;
      }
    }

    if (!stored) {
      info->layer = static_cast<unsigned>(placedResourcesByAddress_.size());
      placedResourcesByAddress_.push_back(
          std::map<D3D12_GPU_VIRTUAL_ADDRESS, PlacedResourceInfo*>{});
      placedResourcesByAddress_[placedResourcesByAddress_.size() - 1][info->captureStart] = info;
    }

    for (PlacedResourceInfo* inf : info->intersecting) {
      inf->intersecting.insert(info);
    }

    placedResourcesByKey_[resourceKey].reset(info);

  } else {
    auto it = resourcesByKey_.find(resourceKey);
    if (it != resourcesByKey_.end()) {
      return;
    }

    ResourceInfo* info = new ResourceInfo{};
    info->captureStart = captureAddress;
    info->key = resourceKey;
    info->size = size;
    info->resource = resource;
    resourcesByAddress_[captureAddress] = info;
    resourcesByKey_[resourceKey].reset(info);
  }
}

void CapturePlayerGpuAddressService::GpuAddressService::addGpuPlayerAddress(
    unsigned resourceKey, D3D12_GPU_VIRTUAL_ADDRESS playerAddress) {
  auto itHeap = heapsByResourceKey_.find(resourceKey);
  if (itHeap != heapsByResourceKey_.end()) {
    PlacedResourceInfo* info = placedResourcesByKey_[resourceKey].get();
    info->playerStart = playerAddress;

    HeapInfo* heapInfo = itHeap->second;
    if (!heapInfo->captureStart || heapInfo->captureStart > info->captureStart) {
      heapInfo->captureStart = info->captureStart;
      heapInfo->playerStart = info->playerStart;
    }
    if (!heapInfo->captureEnd || heapInfo->captureEnd < info->captureStart + info->size) {
      heapInfo->captureEnd = info->captureStart + info->size;
    }
  } else {
    auto it = resourcesByKey_.find(resourceKey);
    if (it != resourcesByKey_.end()) {
      ResourceInfo* info = resourcesByKey_[resourceKey].get();
      it->second->playerStart = playerAddress;
    }
  }
}

void CapturePlayerGpuAddressService::GpuAddressService::destroyInterface(unsigned interfaceKey) {
  {
    auto it = resourcesByKey_.find(interfaceKey);
    if (it != resourcesByKey_.end()) {
      auto itAddr = resourcesByAddress_.find(it->second->captureStart);
      GITS_ASSERT(itAddr != resourcesByAddress_.end());
      if (itAddr->second->key == interfaceKey) {
        resourcesByAddress_.erase(itAddr);
      }
      resourcesByKey_.erase(it);
      return;
    }
  }
  {
    auto it = placedResourcesByKey_.find(interfaceKey);
    if (it != placedResourcesByKey_.end()) {
      PlacedResourceInfo* info = it->second.get();
      for (PlacedResourceInfo* intersecting : info->intersecting) {
        intersecting->intersecting.erase(info);
      }
      placedResourcesByAddress_[info->layer].erase(info->captureStart);
      placedResourcesByKey_.erase(it);
      auto heapIt = heapsByResourceKey_.find(interfaceKey);
      GITS_ASSERT(heapIt != heapsByResourceKey_.end());
      heapIt->second->resources.erase(interfaceKey);
      heapsByResourceKey_.erase(heapIt);
      return;
    }
  }
  {
    auto it = heapsByKey_.find(interfaceKey);
    if (it != heapsByKey_.end()) {
      HeapInfo* heapInfo = it->second.get();
      std::vector<unsigned> resources;
      for (unsigned resourceKey : heapInfo->resources) {
        resources.push_back(resourceKey);
      }
      for (unsigned resourceKey : resources) {
        destroyInterface(resourceKey);
      }
      heapsByKey_.erase(it);
    }
  }
  deniedShaderResources_.erase(interfaceKey);
}

CapturePlayerGpuAddressService::ResourceInfo* CapturePlayerGpuAddressService::GpuAddressService::
    getResourceInfo(D3D12_GPU_VIRTUAL_ADDRESS address) {

  ResourceInfo* resourceInfo{};
  if (!address) {
    return resourceInfo;
  }

  auto itResource = resourcesByAddress_.upper_bound(address);
  if (itResource != resourcesByAddress_.begin() && !resourcesByAddress_.empty()) {
    --itResource;
    ResourceInfo* info = itResource->second;
    D3D12_GPU_VIRTUAL_ADDRESS start = info->captureStart;
    if (address >= start && address < start + info->size) {
      resourceInfo = info;
    }
  }
  if (resourceInfo) {
    return resourceInfo;
  }

  PlacedResourceInfo* placedResourceInfo{};
  for (unsigned layerIndex = 0; layerIndex < placedResourcesByAddress_.size(); ++layerIndex) {
    auto itResource = placedResourcesByAddress_[layerIndex].upper_bound(address);
    if (itResource != placedResourcesByAddress_[layerIndex].begin() &&
        !placedResourcesByAddress_[layerIndex].empty()) {
      --itResource;
      PlacedResourceInfo* info = itResource->second;
      D3D12_GPU_VIRTUAL_ADDRESS start = info->captureStart;
      if (address >= start && address < start + info->size) {
        placedResourceInfo = info;
        break;
      }
    }
  }
  if (placedResourceInfo && !placedResourceInfo->intersecting.empty()) {
    PlacedResourceInfo* selectedInfo = placedResourceInfo;
    for (PlacedResourceInfo* info : placedResourceInfo->intersecting) {
      D3D12_GPU_VIRTUAL_ADDRESS selectedStart = selectedInfo->captureStart;
      D3D12_GPU_VIRTUAL_ADDRESS selectedEnd = selectedStart + selectedInfo->size;
      D3D12_GPU_VIRTUAL_ADDRESS start = info->captureStart;
      D3D12_GPU_VIRTUAL_ADDRESS end = start + info->size;
      if (address >= start && address < end) {
        if (end > selectedEnd) {
          auto it = deniedShaderResources_.find(info->key);
          if (it == deniedShaderResources_.end()) {
            selectedInfo = info;
          }
        }
      }
    }
    placedResourceInfo = selectedInfo;
  }
  return placedResourceInfo;
}

void CapturePlayerGpuAddressService::GpuAddressService::getMappings(
    std::vector<CapturePlayerGpuAddressService::GpuAddressMapping>& mappings) {
  mappings.resize(resourcesByAddress_.size() + heapsByKey_.size());
  unsigned index = 0;
  for (auto& it : resourcesByAddress_) {
    mappings[index].captureStart = it.second->captureStart;
    mappings[index].playerStart = it.second->playerStart;
    mappings[index].size = it.second->size;
    ++index;
  }
  for (auto& it : heapsByKey_) {
    mappings[index].captureStart = it.second->captureStart;
    mappings[index].playerStart = it.second->playerStart;
    mappings[index].size = it.second->captureEnd - it.second->captureStart;
    ++index;
  }

  std::sort(mappings.begin(), mappings.end(),
            [](CapturePlayerGpuAddressService::GpuAddressMapping& m1,
               CapturePlayerGpuAddressService::GpuAddressMapping& m2) {
              return m1.captureStart < m2.captureStart;
            });
}

} // namespace DirectX
} // namespace gits

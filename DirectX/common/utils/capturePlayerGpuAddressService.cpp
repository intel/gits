// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "capturePlayerGpuAddressService.h"
#include "log.h"

#include <algorithm>

namespace gits {
namespace DirectX {

void CapturePlayerGpuAddressService::GpuAddressService::CreatePlacedResource(
    unsigned heapKey, unsigned resourceKey, D3D12_RESOURCE_FLAGS flags) {
  HeapInfo* heapInfo{};
  auto it = m_HeapsByKey.find(heapKey);
  if (it != m_HeapsByKey.end()) {
    heapInfo = it->second.get();
  } else {
    heapInfo = new HeapInfo{};
    m_HeapsByKey[heapKey].reset(heapInfo);
  }
  heapInfo->Resources.insert(resourceKey);
  m_HeapsByResourceKey[resourceKey] = heapInfo;

  if (flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) {
    m_DeniedShaderResources.insert(resourceKey);
  }
}

void CapturePlayerGpuAddressService::GpuAddressService::AddGpuCaptureAddress(
    ID3D12Resource* resource,
    unsigned resourceKey,
    unsigned size,
    D3D12_GPU_VIRTUAL_ADDRESS captureAddress) {
  if (!captureAddress) {
    return;
  }
  auto itHeap = m_HeapsByResourceKey.find(resourceKey);
  if (itHeap != m_HeapsByResourceKey.end()) {
    auto it = m_PlacedResourcesByKey.find(resourceKey);
    if (it != m_PlacedResourcesByKey.end()) {
      return;
    }

    PlacedResourceInfo* info = new PlacedResourceInfo{};
    info->CaptureStart = captureAddress;
    info->Key = resourceKey;
    info->Size = size;
    info->Resource = resource;

    bool stored = false;
    for (unsigned layerIndex = 0; layerIndex < m_PlacedResourcesByAddress.size(); ++layerIndex) {
      bool intersecting = false;
      for (auto& placedIt : m_PlacedResourcesByAddress[layerIndex]) {
        if (placedIt.second->CaptureStart + placedIt.second->Size > info->CaptureStart &&
            placedIt.second->CaptureStart < info->CaptureStart + info->Size) {
          info->Intersecting.insert(placedIt.second);
          intersecting = true;
        }
      }
      if (!intersecting && !stored) {
        info->Layer = layerIndex;
        m_PlacedResourcesByAddress[layerIndex][info->CaptureStart] = info;
        stored = true;
      }
    }

    if (!stored) {
      info->Layer = static_cast<unsigned>(m_PlacedResourcesByAddress.size());
      m_PlacedResourcesByAddress.push_back(
          std::map<D3D12_GPU_VIRTUAL_ADDRESS, PlacedResourceInfo*>{});
      m_PlacedResourcesByAddress[m_PlacedResourcesByAddress.size() - 1][info->CaptureStart] = info;
    }

    for (PlacedResourceInfo* inf : info->Intersecting) {
      inf->Intersecting.insert(info);
    }

    m_PlacedResourcesByKey[resourceKey].reset(info);

  } else {
    auto it = m_ResourcesByKey.find(resourceKey);
    if (it != m_ResourcesByKey.end()) {
      return;
    }

    ResourceInfo* info = new ResourceInfo{};
    info->CaptureStart = captureAddress;
    info->Key = resourceKey;
    info->Size = size;
    info->Resource = resource;
    m_ResourcesByAddress[captureAddress] = info;
    m_ResourcesByKey[resourceKey].reset(info);
  }
}

void CapturePlayerGpuAddressService::GpuAddressService::AddGpuPlayerAddress(
    unsigned resourceKey, D3D12_GPU_VIRTUAL_ADDRESS playerAddress) {
  auto itHeap = m_HeapsByResourceKey.find(resourceKey);
  if (itHeap != m_HeapsByResourceKey.end()) {
    PlacedResourceInfo* info = m_PlacedResourcesByKey[resourceKey].get();
    info->PlayerStart = playerAddress;

    HeapInfo* heapInfo = itHeap->second;
    if (!heapInfo->CaptureStart || heapInfo->CaptureStart > info->CaptureStart) {
      heapInfo->CaptureStart = info->CaptureStart;
      heapInfo->PlayerStart = info->PlayerStart;
    }
    if (!heapInfo->CaptureEnd || heapInfo->CaptureEnd < info->CaptureStart + info->Size) {
      heapInfo->CaptureEnd = info->CaptureStart + info->Size;
    }
  } else {
    auto it = m_ResourcesByKey.find(resourceKey);
    if (it != m_ResourcesByKey.end()) {
      ResourceInfo* info = m_ResourcesByKey[resourceKey].get();
      it->second->PlayerStart = playerAddress;
    }
  }
}

void CapturePlayerGpuAddressService::GpuAddressService::DestroyInterface(unsigned interfaceKey) {
  {
    auto it = m_ResourcesByKey.find(interfaceKey);
    if (it != m_ResourcesByKey.end()) {
      auto itAddr = m_ResourcesByAddress.find(it->second->CaptureStart);
      GITS_ASSERT(itAddr != m_ResourcesByAddress.end());
      if (itAddr->second->Key == interfaceKey) {
        m_ResourcesByAddress.erase(itAddr);
      }
      m_ResourcesByKey.erase(it);
      return;
    }
  }
  {
    auto it = m_PlacedResourcesByKey.find(interfaceKey);
    if (it != m_PlacedResourcesByKey.end()) {
      PlacedResourceInfo* info = it->second.get();
      for (PlacedResourceInfo* intersecting : info->Intersecting) {
        intersecting->Intersecting.erase(info);
      }
      m_PlacedResourcesByAddress[info->Layer].erase(info->CaptureStart);
      m_PlacedResourcesByKey.erase(it);
      auto heapIt = m_HeapsByResourceKey.find(interfaceKey);
      GITS_ASSERT(heapIt != m_HeapsByResourceKey.end());
      heapIt->second->Resources.erase(interfaceKey);
      m_HeapsByResourceKey.erase(heapIt);
      return;
    }
  }
  {
    auto it = m_HeapsByKey.find(interfaceKey);
    if (it != m_HeapsByKey.end()) {
      HeapInfo* heapInfo = it->second.get();
      std::vector<unsigned> resources;
      for (unsigned resourceKey : heapInfo->Resources) {
        resources.push_back(resourceKey);
      }
      for (unsigned resourceKey : resources) {
        DestroyInterface(resourceKey);
      }
      m_HeapsByKey.erase(it);
    }
  }
  m_DeniedShaderResources.erase(interfaceKey);
}

CapturePlayerGpuAddressService::ResourceInfo* CapturePlayerGpuAddressService::GpuAddressService::
    GetResourceInfo(D3D12_GPU_VIRTUAL_ADDRESS address) {

  ResourceInfo* resourceInfo{};
  if (!address) {
    return resourceInfo;
  }

  auto itResource = m_ResourcesByAddress.upper_bound(address);
  if (itResource != m_ResourcesByAddress.begin() && !m_ResourcesByAddress.empty()) {
    --itResource;
    ResourceInfo* info = itResource->second;
    D3D12_GPU_VIRTUAL_ADDRESS start = info->CaptureStart;
    if (address >= start && address < start + info->Size) {
      resourceInfo = info;
    }
  }
  if (resourceInfo) {
    return resourceInfo;
  }

  PlacedResourceInfo* placedResourceInfo{};
  for (unsigned layerIndex = 0; layerIndex < m_PlacedResourcesByAddress.size(); ++layerIndex) {
    auto itPlaced = m_PlacedResourcesByAddress[layerIndex].upper_bound(address);
    if (itPlaced != m_PlacedResourcesByAddress[layerIndex].begin() &&
        !m_PlacedResourcesByAddress[layerIndex].empty()) {
      --itPlaced;
      PlacedResourceInfo* info = itPlaced->second;
      D3D12_GPU_VIRTUAL_ADDRESS start = info->CaptureStart;
      if (address >= start && address < start + info->Size) {
        placedResourceInfo = info;
        break;
      }
    }
  }
  if (placedResourceInfo && !placedResourceInfo->Intersecting.empty()) {
    PlacedResourceInfo* selectedInfo = placedResourceInfo;
    for (PlacedResourceInfo* info : placedResourceInfo->Intersecting) {
      D3D12_GPU_VIRTUAL_ADDRESS selectedStart = selectedInfo->CaptureStart;
      D3D12_GPU_VIRTUAL_ADDRESS selectedEnd = selectedStart + selectedInfo->Size;
      D3D12_GPU_VIRTUAL_ADDRESS start = info->CaptureStart;
      D3D12_GPU_VIRTUAL_ADDRESS end = start + info->Size;
      if (address >= start && address < end) {
        if (end > selectedEnd) {
          auto deniedIt = m_DeniedShaderResources.find(info->Key);
          if (deniedIt == m_DeniedShaderResources.end()) {
            selectedInfo = info;
          }
        }
      }
    }
    placedResourceInfo = selectedInfo;
  }
  return placedResourceInfo;
}

void CapturePlayerGpuAddressService::GpuAddressService::GetMappings(
    std::vector<CapturePlayerGpuAddressService::GpuAddressMapping>& mappings) {
  mappings.resize(m_ResourcesByAddress.size() + m_HeapsByKey.size());
  unsigned index = 0;
  for (auto& it : m_ResourcesByAddress) {
    mappings[index].CaptureStart = it.second->CaptureStart;
    mappings[index].PlayerStart = it.second->PlayerStart;
    mappings[index].Size = it.second->Size;
    ++index;
  }
  for (auto& it : m_HeapsByKey) {
    mappings[index].CaptureStart = it.second->CaptureStart;
    mappings[index].PlayerStart = it.second->PlayerStart;
    mappings[index].Size = it.second->CaptureEnd - it.second->CaptureStart;
    ++index;
  }

  std::sort(mappings.begin(), mappings.end(),
            [](CapturePlayerGpuAddressService::GpuAddressMapping& m1,
               CapturePlayerGpuAddressService::GpuAddressMapping& m2) {
              return m1.CaptureStart < m2.CaptureStart;
            });
}

} // namespace DirectX
} // namespace gits

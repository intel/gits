// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "capturePlayerGpuAddressService.h"
#include "arguments.h"
#include "log.h"
#include "configurationLib.h"

#include <algorithm>

namespace gits {
namespace DirectX {

CapturePlayerGpuAddressService::CapturePlayerGpuAddressService() {
  m_ResourcePlacement = Configurator::IsPlayer() &&
                        Configurator::Get().directx.player.portability.resourcePlacement == "use";
}

void CapturePlayerGpuAddressService::CreatePlacedResource(ObjectKey heapKey,
                                                          ObjectKey resourceKey,
                                                          D3D12_RESOURCE_FLAGS flags,
                                                          D3D12_RESOURCE_STATES initialState) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  bool raytracingAS = false;
  if (initialState & D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
    raytracingAS = true;
  }
  if (flags & D3D12_RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE) {
    raytracingAS = true;
  }
  m_GpuAddressService.CreatePlacedResource(heapKey, resourceKey, flags, raytracingAS);
  if (m_GpuPlayerAddress) {
    m_GpuPlayerAddress->CreatePlacedResource(heapKey, resourceKey, flags, raytracingAS);
  }
}

void CapturePlayerGpuAddressService::CreatePlacedResource(ObjectKey heapKey,
                                                          ObjectKey resourceKey,
                                                          D3D12_RESOURCE_FLAGS flags,
                                                          D3D12_BARRIER_LAYOUT initialLayout) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  bool raytracingAS = false;
  if (flags & D3D12_RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE) {
    raytracingAS = true;
  }
  m_GpuAddressService.CreatePlacedResource(heapKey, resourceKey, flags, raytracingAS);
  if (m_GpuPlayerAddress) {
    m_GpuPlayerAddress->CreatePlacedResource(heapKey, resourceKey, flags, raytracingAS);
  }
}

void CapturePlayerGpuAddressService::AddGpuCaptureAddress(
    ID3D12Resource* resource,
    ObjectKey resourceKey,
    unsigned size,
    D3D12_GPU_VIRTUAL_ADDRESS captureAddress) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_GpuAddressService.AddGpuCaptureAddress(resource, resourceKey, size, captureAddress);
}

void CapturePlayerGpuAddressService::AddGpuPlayerAddress(ID3D12Resource* resource,
                                                         ObjectKey resourceKey,
                                                         unsigned size,
                                                         D3D12_GPU_VIRTUAL_ADDRESS playerAddress) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_GpuAddressService.AddGpuPlayerAddress(resourceKey, playerAddress);
  if (m_GpuPlayerAddress) {
    m_GpuPlayerAddress->AddGpuCaptureAddress(resource, resourceKey, size, playerAddress);
    m_GpuPlayerAddress->AddGpuPlayerAddress(resourceKey, playerAddress);
  }
}

void CapturePlayerGpuAddressService::DestroyInterface(ObjectKey interfaceKey) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_GpuAddressService.DestroyInterface(interfaceKey);
  if (m_GpuPlayerAddress) {
    m_GpuPlayerAddress->DestroyInterface(interfaceKey);
  }
}

CapturePlayerGpuAddressService::ResourceInfo* CapturePlayerGpuAddressService::
    GetResourceInfoByCaptureAddress(D3D12_GPU_VIRTUAL_ADDRESS address, bool raytracingAS) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  return m_GpuAddressService.GetResourceInfo(address, raytracingAS);
}

CapturePlayerGpuAddressService::ResourceInfo* CapturePlayerGpuAddressService::
    GetResourceInfoByPlayerAddress(D3D12_GPU_VIRTUAL_ADDRESS address, bool raytracingAS) {
  if (m_GpuPlayerAddress) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_GpuPlayerAddress->GetResourceInfo(address, raytracingAS);
  }
  return nullptr;
}

void CapturePlayerGpuAddressService::GetMappings(std::vector<GpuAddressMapping>& mappings) {
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_ResourcePlacement) {
      m_GpuAddressService.GetMappingsResourcePlacement(mappings);
    } else {
      m_GpuAddressService.GetMappings(mappings);
    }
  }
  std::sort(mappings.begin(), mappings.end(),
            [](CapturePlayerGpuAddressService::GpuAddressMapping& m1,
               CapturePlayerGpuAddressService::GpuAddressMapping& m2) {
              return m1.CaptureStart < m2.CaptureStart;
            });
}

void CapturePlayerGpuAddressService::EnablePlayerAddressLookup() {
  if (!m_GpuPlayerAddress) {
    m_GpuPlayerAddress.reset(new GpuAddressService());
  }
}

void CapturePlayerGpuAddressService::GpuAddressService::CreatePlacedResource(
    ObjectKey heapKey, ObjectKey resourceKey, D3D12_RESOURCE_FLAGS flags, bool raytracingAS) {
  HeapInfo* heapInfo{};
  auto it = m_HeapsByKey.find(heapKey);
  if (it != m_HeapsByKey.end()) {
    heapInfo = it->second.get();
  } else {
    heapInfo = new HeapInfo{};
    m_HeapsByKey[heapKey].reset(heapInfo);
  }
  heapInfo->Resources.insert(resourceKey);

  PlacedResourceInfo* info = new PlacedResourceInfo();
  info->Key = resourceKey;
  info->HeapKey = heapKey;
  if (flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) {
    info->DeniedShaderResource = true;
  }
  info->RaytracingAS = raytracingAS;

  m_PlacedResourcesByKey[resourceKey].reset(info);
}

void CapturePlayerGpuAddressService::GpuAddressService::AddGpuCaptureAddress(
    ID3D12Resource* resource,
    ObjectKey resourceKey,
    unsigned size,
    D3D12_GPU_VIRTUAL_ADDRESS captureAddress) {
  if (!captureAddress) {
    return;
  }
  auto it = m_PlacedResourcesByKey.find(resourceKey);
  if (it != m_PlacedResourcesByKey.end()) {
    if (it->second->CaptureStart) {
      return;
    }

    PlacedResourceInfo* info = it->second.get();
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
    ObjectKey resourceKey, D3D12_GPU_VIRTUAL_ADDRESS playerAddress) {
  auto it = m_PlacedResourcesByKey.find(resourceKey);
  if (it != m_PlacedResourcesByKey.end()) {
    PlacedResourceInfo* info = it->second.get();
    info->PlayerStart = playerAddress;

    auto itHeap = m_HeapsByKey.find(info->HeapKey);
    GITS_ASSERT(itHeap != m_HeapsByKey.end());
    HeapInfo* heapInfo = itHeap->second.get();
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

void CapturePlayerGpuAddressService::GpuAddressService::DestroyInterface(ObjectKey interfaceKey) {
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
      if (info->CaptureStart) {
        for (PlacedResourceInfo* intersecting : info->Intersecting) {
          intersecting->Intersecting.erase(info);
        }
        m_PlacedResourcesByAddress[info->Layer].erase(info->CaptureStart);
      }
      auto itHeap = m_HeapsByKey.find(it->second->HeapKey);
      GITS_ASSERT(itHeap != m_HeapsByKey.end());
      itHeap->second->Resources.erase(interfaceKey);
      m_PlacedResourcesByKey.erase(it);
      return;
    }
  }
  {
    auto it = m_HeapsByKey.find(interfaceKey);
    if (it != m_HeapsByKey.end()) {
      HeapInfo* heapInfo = it->second.get();
      std::vector<ObjectKey> resources;
      for (ObjectKey resourceKey : heapInfo->Resources) {
        resources.push_back(resourceKey);
      }
      for (ObjectKey resourceKey : resources) {
        DestroyInterface(resourceKey);
      }
      m_HeapsByKey.erase(it);
    }
  }
}

CapturePlayerGpuAddressService::ResourceInfo* CapturePlayerGpuAddressService::GpuAddressService::
    GetResourceInfo(D3D12_GPU_VIRTUAL_ADDRESS address, bool raytracingAS) {

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
        if (raytracingAS) {
          if (!selectedInfo->RaytracingAS || end > selectedEnd) {
            if (info->RaytracingAS) {
              selectedInfo = info;
            }
          }
        } else {
          if (end > selectedEnd) {
            if (!info->DeniedShaderResource) {
              selectedInfo = info;
            }
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
}

void CapturePlayerGpuAddressService::GpuAddressService::GetMappingsResourcePlacement(
    std::vector<CapturePlayerGpuAddressService::GpuAddressMapping>& mappings) {
  mappings.clear();
  mappings.reserve(m_ResourcesByAddress.size() + m_PlacedResourcesByKey.size());
  for (auto& it : m_ResourcesByAddress) {
    mappings.push_back({it.second->CaptureStart, it.second->PlayerStart, it.second->Size});
  }
  // Resource placement may shift placed resources within the heap so per-resource mappings are needed for changed heaps
  std::vector<PlacedResourceInfo*> heapResources;
  for (auto& it : m_HeapsByKey) {
    HeapInfo* heapInfo = it.second.get();
    heapResources.clear();
    bool heapChanged = false;
    D3D12_GPU_VIRTUAL_ADDRESS heapDelta = heapInfo->PlayerStart - heapInfo->CaptureStart;
    for (ObjectKey resourceKey : heapInfo->Resources) {
      PlacedResourceInfo* info = m_PlacedResourcesByKey[resourceKey].get();
      if (info->CaptureStart && info->PlayerStart) {
        heapResources.push_back(info);
        if (info->PlayerStart - info->CaptureStart != heapDelta) {
          heapChanged = true;
        }
      }
    }
    if (!heapChanged) {
      mappings.push_back({heapInfo->CaptureStart, heapInfo->PlayerStart,
                          heapInfo->CaptureEnd - heapInfo->CaptureStart});
      continue;
    }
    static bool logged = false;
    if (!logged) {
      LOG_WARNING << "Resource placement changed - using per resource GPU address mappings";
      logged = true;
    }
    // Mappings are binary searched on GPU so aliased resources are clipped to non-overlapping segments
    std::sort(heapResources.begin(), heapResources.end(),
              [](PlacedResourceInfo* a, PlacedResourceInfo* b) {
                return a->CaptureStart != b->CaptureStart ? a->CaptureStart < b->CaptureStart
                                                          : a->Size > b->Size;
              });
    D3D12_GPU_VIRTUAL_ADDRESS coveredEnd = 0;
    for (PlacedResourceInfo* info : heapResources) {
      D3D12_GPU_VIRTUAL_ADDRESS start = std::max(info->CaptureStart, coveredEnd);
      D3D12_GPU_VIRTUAL_ADDRESS end = info->CaptureStart + info->Size;
      if (start < end) {
        mappings.push_back({start, info->PlayerStart + (start - info->CaptureStart), end - start});
        coveredEnd = end;
      }
    }
  }
}

} // namespace DirectX
} // namespace gits

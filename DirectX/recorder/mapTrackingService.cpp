// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "mapTrackingService.h"
#include "commandSerializersCustom.h"
#include "captureManager.h"
#include "log.h"
#include "configurator.h"
#include "resourceSizeUtils.h"

#include <processthreadsapi.h>
#include <wrl/client.h>

namespace gits {
namespace DirectX {

MapTrackingService::MapTrackingService(stream::OrderingRecorder& recorder) : m_Recorder(recorder) {
  SYSTEM_INFO sSysInfo;
  GetSystemInfo(&sSysInfo);
  m_PageSize = sSysInfo.dwPageSize;
  m_ShadowMemory = Configurator::Get().directx.recorder.shadowMemory;
}

void MapTrackingService::EnableWriteWatch(D3D12_HEAP_PROPERTIES& properties,
                                          D3D12_HEAP_FLAGS& flags) {
  if (!IsUploadHeap(properties.Type, properties.CPUPageProperty)) {
    return;
  }
  if (!m_ShadowMemory) {
    flags |= D3D12_HEAP_FLAG_ALLOW_WRITE_WATCH;
    properties.Type = D3D12_HEAP_TYPE_CUSTOM;
    properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
    properties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
  }
}

void MapTrackingService::MapResource(unsigned resourceKey,
                                     ID3D12Resource* resource,
                                     unsigned subresourceIndex,
                                     void** mappedData) {
  if (!mappedData) {
    return;
  }

  m_Mutex.lock();

  MappedInfo* info{};
  auto itResource = m_MappedData.find(resourceKey);
  if (itResource != m_MappedData.end()) {
    auto it = itResource->second.find(subresourceIndex);
    if (it != itResource->second.end()) {
      info = it->second.get();
    }
  }

  if (!info) {

    m_Mutex.unlock();

    D3D12_HEAP_PROPERTIES properties{};
    D3D12_HEAP_FLAGS flags{};
    HRESULT hr = resource->GetHeapProperties(&properties, &flags);
    GITS_ASSERT(hr == S_OK);
    if (!IsUploadHeap(properties.Type, properties.CPUPageProperty)) {
      return;
    }

    info = new MappedInfo{};
    info->ResourceKey = resourceKey;
    info->Size = GetSubresourceSize(resource, subresourceIndex);
    info->WatchedPages.resize((info->Size + (m_PageSize - 1)) / m_PageSize);

    if (m_ShadowMemory) {
      info->ShadowAddress = static_cast<char*>(VirtualAlloc(
          nullptr, info->Size, MEM_COMMIT | MEM_RESERVE | MEM_WRITE_WATCH, PAGE_READWRITE));
      memcpy(info->ShadowAddress, *mappedData, info->Size);
      ResetWriteWatch(info->ShadowAddress, info->Size);
    }

    m_Mutex.lock();
    m_MappedData[resourceKey][subresourceIndex].reset(info);
  }

  info->MappedAddress = static_cast<char*>(*mappedData);
  ++info->MapCount;

  if (m_ShadowMemory) {
    *mappedData = info->ShadowAddress;
  }

  m_Mutex.unlock();
}

void MapTrackingService::UnmapResource(unsigned resourceKey, unsigned subresourceIndex) {

  std::lock_guard<std::mutex> lock(m_Mutex);

  auto itResource = m_MappedData.find(resourceKey);
  if (itResource != m_MappedData.end()) {
    auto it = itResource->second.find(subresourceIndex);
    if (it != itResource->second.end()) {

      MappedInfo* info = it->second.get();
      if (--info->MapCount == 0) {
        CaptureModifiedData(info);
      }
    }
  }
}

void MapTrackingService::ExecuteCommandLists() {

  std::lock_guard<std::mutex> lock(m_Mutex);

  for (auto& itResource : m_MappedData) {
    for (auto& it : itResource.second) {
      CaptureModifiedData(it.second.get());
    }
  }
}

void MapTrackingService::DestroyResource(unsigned resourceKey) {

  std::lock_guard<std::mutex> lock(m_Mutex);

  auto it = m_MappedData.find(resourceKey);
  if (it == m_MappedData.end()) {
    return;
  }
  m_MappedData.erase(resourceKey);
}

bool MapTrackingService::IsUploadHeap(D3D12_HEAP_TYPE heapType,
                                      D3D12_CPU_PAGE_PROPERTY cpuPageProperty) {
  return heapType == D3D12_HEAP_TYPE_UPLOAD || heapType == D3D12_HEAP_TYPE_GPU_UPLOAD ||
         heapType == D3D12_HEAP_TYPE_CUSTOM &&
             cpuPageProperty != D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE &&
             cpuPageProperty != D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
}

void MapTrackingService::CaptureModifiedData(MappedInfo* info) {

  UINT64 pageCount = info->WatchedPages.size();
  unsigned long pageSize;
  char* watchedAddress = m_ShadowMemory ? info->ShadowAddress : info->MappedAddress;
  UINT ret = GetWriteWatch(WRITE_WATCH_FLAG_RESET, watchedAddress, info->Size,
                           info->WatchedPages.data(), &pageCount, &pageSize);
  if (ret != 0) {
    static bool logged = false;
    if (!logged) {
      LOG_ERROR << "GetWriteWatch failed " << ret << ". Try ShadowMemory option if not used.";
      logged = true;
    }
  }

  for (UINT64 i = 0; i < pageCount;) {

    char* data = static_cast<char*>(info->WatchedPages[i]);
    unsigned offset = static_cast<unsigned>(data - watchedAddress);
    unsigned dataSize = pageSize;

    for (++i; i < pageCount; ++i) {
      if (static_cast<char*>(info->WatchedPages[i]) ==
          static_cast<char*>(info->WatchedPages[i - 1]) + pageSize) {
        dataSize += pageSize;
      } else {
        break;
      }
    }

    if (i == pageCount) {
      const char* mappedMemoryEnd = watchedAddress + info->Size;
      const char* dataEnd = data + dataSize;

      if (dataEnd > mappedMemoryEnd) {
        dataSize -= static_cast<unsigned>(dataEnd - mappedMemoryEnd);
      }
    }

    if (m_ShadowMemory) {
      memcpy(info->MappedAddress + offset, watchedAddress + offset, dataSize);
    }
    CaptureData(info->ResourceKey, watchedAddress, offset, data, dataSize);
  }
}

void MapTrackingService::CaptureData(
    unsigned resourceKey, void* mappedAddress, unsigned offset, void* data, unsigned dataSize) {

  MappedDataMetaCommand command(GetCurrentThreadId());
  command.Key = CaptureManager::get().createCommandKey();
  command.m_resource.Key = resourceKey;
  command.m_mappedAddress.Value = mappedAddress;
  command.m_offset.Value = offset;
  command.m_data.Value = data;
  command.m_data.Size = dataSize;

  m_Recorder.Record(command.Key, new MappedDataMetaSerializer(command));
}

size_t MapTrackingService::GetSubresourceSize(ID3D12Resource* resource, unsigned subresource) {

  D3D12_RESOURCE_DESC desc = resource->GetDesc();

  size_t size = 0;
  if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
    size = desc.Width;
  } else {
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    HRESULT res = resource->GetDevice(IID_PPV_ARGS(&device));
    GITS_ASSERT(res == S_OK);

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
    GetCopyableFootprintsSafe(device.Get(), &desc, subresource, 1, 0, &footprint, nullptr, nullptr,
                              nullptr);
    size = footprint.Footprint.RowPitch * footprint.Footprint.Height * footprint.Footprint.Depth;
  }
  return size;
}

} // namespace DirectX
} // namespace gits

// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "mapTrackingService.h"
#include "commandWritersCustom.h"
#include "captureManager.h"

#include <processthreadsapi.h>
#include <wrl/client.h>

namespace gits {
namespace DirectX {

MapTrackingService::MapTrackingService(GitsRecorder& recorder) : recorder_(recorder) {
  SYSTEM_INFO sSysInfo;
  GetSystemInfo(&sSysInfo);
  pageSize_ = sSysInfo.dwPageSize;
  shadowMemory_ = Config::Get().directx.capture.shadowMemory;
}

void MapTrackingService::enableWriteWatch(D3D12_HEAP_PROPERTIES& properties,
                                          D3D12_HEAP_FLAGS& flags) {
  if (!isUploadHeap(properties.Type, properties.CPUPageProperty)) {
    return;
  }
  if (!shadowMemory_) {
    flags |= D3D12_HEAP_FLAG_ALLOW_WRITE_WATCH;
    properties.Type = D3D12_HEAP_TYPE_CUSTOM;
    properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
    properties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
  }
}

void MapTrackingService::mapResource(unsigned resourceKey,
                                     ID3D12Resource* resource,
                                     unsigned subresourceIndex,
                                     void** mappedData) {
  if (!mappedData) {
    return;
  }

  mutex_.lock();

  MappedInfo* info{};
  auto itResource = mappedData_.find(resourceKey);
  if (itResource != mappedData_.end()) {
    auto it = itResource->second.find(subresourceIndex);
    if (it != itResource->second.end()) {
      info = it->second.get();
    }
  }

  if (!info) {

    mutex_.unlock();

    D3D12_HEAP_PROPERTIES properties{};
    D3D12_HEAP_FLAGS flags{};
    HRESULT hr = resource->GetHeapProperties(&properties, &flags);
    GITS_ASSERT(hr == S_OK);
    if (!isUploadHeap(properties.Type, properties.CPUPageProperty)) {
      return;
    }

    info = new MappedInfo{};
    info->resourceKey = resourceKey;
    info->size = getSubresourceSize(resource, subresourceIndex);
    info->watchedPages.resize((info->size + (pageSize_ - 1)) / pageSize_);

    if (shadowMemory_) {
      info->shadowAddress = static_cast<char*>(VirtualAlloc(
          nullptr, info->size, MEM_COMMIT | MEM_RESERVE | MEM_WRITE_WATCH, PAGE_READWRITE));
      memcpy(info->shadowAddress, *mappedData, info->size);
      ResetWriteWatch(info->shadowAddress, info->size);
    }

    mutex_.lock();
    mappedData_[resourceKey][subresourceIndex].reset(info);
  }

  info->mappedAddress = static_cast<char*>(*mappedData);
  ++info->mapCount;

  if (shadowMemory_) {
    *mappedData = info->shadowAddress;
  }

  mutex_.unlock();
}

void MapTrackingService::unmapResource(unsigned resourceKey, unsigned subresourceIndex) {

  std::lock_guard<std::mutex> lock(mutex_);

  auto itResource = mappedData_.find(resourceKey);
  if (itResource != mappedData_.end()) {
    auto it = itResource->second.find(subresourceIndex);
    if (it != itResource->second.end()) {

      MappedInfo* info = it->second.get();
      if (--info->mapCount == 0) {
        captureModifiedData(info);
      }
    }
  }
}

void MapTrackingService::executeCommandLists() {

  std::lock_guard<std::mutex> lock(mutex_);

  for (auto& itResource : mappedData_) {
    for (auto& it : itResource.second) {
      captureModifiedData(it.second.get());
    }
  }
}

void MapTrackingService::destroyResource(unsigned resourceKey) {

  std::lock_guard<std::mutex> lock(mutex_);

  auto it = mappedData_.find(resourceKey);
  if (it == mappedData_.end()) {
    return;
  }
  mappedData_.erase(resourceKey);
}

bool MapTrackingService::isUploadHeap(D3D12_HEAP_TYPE heapType,
                                      D3D12_CPU_PAGE_PROPERTY cpuPageProperty) {
  return heapType == D3D12_HEAP_TYPE_UPLOAD ||
         heapType == D3D12_HEAP_TYPE_CUSTOM &&
             cpuPageProperty != D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE &&
             cpuPageProperty != D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
}

void MapTrackingService::captureModifiedData(MappedInfo* info) {

  UINT64 pageCount = info->watchedPages.size();
  unsigned long pageSize;
  char* watchedAddress = shadowMemory_ ? info->shadowAddress : info->mappedAddress;
  UINT ret = GetWriteWatch(WRITE_WATCH_FLAG_RESET, watchedAddress, info->size,
                           info->watchedPages.data(), &pageCount, &pageSize);
  if (ret != 0) {
    static bool logged = false;
    if (!logged) {
      Log(ERR) << "GetWriteWatch failed " << ret << ". Try ShadowMemory option if not used.";
      logged = true;
    }
  }

  for (UINT64 i = 0; i < pageCount;) {

    char* data = static_cast<char*>(info->watchedPages[i]);
    unsigned offset = static_cast<unsigned>(data - watchedAddress);
    unsigned dataSize = pageSize;

    for (++i; i < pageCount; ++i) {
      if (static_cast<char*>(info->watchedPages[i]) ==
          static_cast<char*>(info->watchedPages[i - 1]) + pageSize) {
        dataSize += pageSize;
      } else {
        break;
      }
    }

    if (i == pageCount) {
      const char* mappedMemoryEnd = watchedAddress + info->size;
      const char* dataEnd = data + dataSize;

      if (dataEnd > mappedMemoryEnd) {
        dataSize -= static_cast<unsigned>(dataEnd - mappedMemoryEnd);
      }
    }

    if (shadowMemory_) {
      memcpy(info->mappedAddress + offset, watchedAddress + offset, dataSize);
    }
    captureData(info->resourceKey, watchedAddress, offset, data, dataSize);
  }
}

void MapTrackingService::captureData(
    unsigned resourceKey, void* mappedAddress, unsigned offset, void* data, unsigned dataSize) {

  MappedDataMetaCommand command(GetCurrentThreadId());
  command.key = CaptureManager::get().createCommandKey();
  command.resource_.key = resourceKey;
  command.mappedAddress_.value = mappedAddress;
  command.offset_.value = offset;
  command.data_.value = data;
  command.data_.size = dataSize;

  recorder_.record(command.key, new MappedDataMetaWriter(command));
}

size_t MapTrackingService::getSubresourceSize(ID3D12Resource* resource, unsigned subresource) {

  D3D12_RESOURCE_DESC desc = resource->GetDesc();

  size_t size = 0;
  if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
    size = desc.Width;
  } else {
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    HRESULT res = resource->GetDevice(IID_PPV_ARGS(&device));
    GITS_ASSERT(res == S_OK);

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
    device->GetCopyableFootprints(&desc, subresource, 1, 0, &footprint, nullptr, nullptr, nullptr);
    size = footprint.Footprint.RowPitch * footprint.Footprint.Height * footprint.Footprint.Depth;
  }
  return size;
}

} // namespace DirectX
} // namespace gits

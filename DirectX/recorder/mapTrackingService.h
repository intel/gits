// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "orderingRecorder.h"

#include <mutex>
#include <unordered_map>
#include <vector>
#include <memory>
#include <d3d12.h>

namespace gits {
namespace DirectX {

class MapTrackingService {
public:
  MapTrackingService(stream::OrderingRecorder& recorder);
  MapTrackingService(const MapTrackingService&) = delete;
  MapTrackingService& operator=(const MapTrackingService&) = delete;

  void enableWriteWatch(D3D12_HEAP_PROPERTIES& properties, D3D12_HEAP_FLAGS& flags);
  void mapResource(unsigned resourceKey,
                   ID3D12Resource* resource,
                   unsigned subresourceIndex,
                   void** mappedData);
  void unmapResource(unsigned resourceKey, unsigned subresourceIndex);
  void executeCommandLists();
  void destroyResource(unsigned resourceKey);

private:
  bool shadowMemory_{false};
  struct MappedInfo {
    MappedInfo() = default;
    ~MappedInfo() {
      if (shadowAddress) {
        VirtualFree(shadowAddress, 0, MEM_RELEASE);
      }
    }
    MappedInfo(const MappedInfo&) = delete;
    MappedInfo& operator=(const MappedInfo&) = delete;

    unsigned resourceKey;
    char* mappedAddress;
    char* shadowAddress;
    size_t size;
    int mapCount;
    std::vector<void*> watchedPages;
  };
  std::unordered_map<unsigned, std::map<unsigned, std::unique_ptr<MappedInfo>>> mappedData_;

  size_t pageSize_{0};
  std::mutex mutex_;

  stream::OrderingRecorder& recorder_;

private:
  bool isUploadHeap(D3D12_HEAP_TYPE heapType, D3D12_CPU_PAGE_PROPERTY cpuPageProperty);
  void captureModifiedData(MappedInfo* info);
  void captureData(
      unsigned resourceKey, void* mappedAddress, unsigned offset, void* data, unsigned dataSize);
  size_t getSubresourceSize(ID3D12Resource* resource, unsigned subresource);
};

} // namespace DirectX
} // namespace gits

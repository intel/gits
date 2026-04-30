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
  void mapResource(unsigned ResourceKey,
                   ID3D12Resource* resource,
                   unsigned subresourceIndex,
                   void** mappedData);
  void unmapResource(unsigned ResourceKey, unsigned subresourceIndex);
  void executeCommandLists();
  void destroyResource(unsigned ResourceKey);

private:
  bool m_ShadowMemory{false};
  struct MappedInfo {
    MappedInfo() = default;
    ~MappedInfo() {
      if (shadowAddress) {
        VirtualFree(shadowAddress, 0, MEM_RELEASE);
      }
    }
    MappedInfo(const MappedInfo&) = delete;
    MappedInfo& operator=(const MappedInfo&) = delete;

    unsigned ResourceKey;
    char* mappedAddress;
    char* shadowAddress;
    size_t size;
    int mapCount;
    std::vector<void*> watchedPages;
  };
  std::unordered_map<unsigned, std::map<unsigned, std::unique_ptr<MappedInfo>>> m_MappedData;

  size_t m_PageSize{0};
  std::mutex m_Mutex;

  stream::OrderingRecorder& m_Recorder;

private:
  bool isUploadHeap(D3D12_HEAP_TYPE heapType, D3D12_CPU_PAGE_PROPERTY cpuPageProperty);
  void captureModifiedData(MappedInfo* info);
  void captureData(
      unsigned ResourceKey, void* mappedAddress, unsigned offset, void* data, unsigned dataSize);
  size_t getSubresourceSize(ID3D12Resource* resource, unsigned subresource);
};

} // namespace DirectX
} // namespace gits

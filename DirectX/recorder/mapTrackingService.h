// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "gitsRecorder.h"
#include "command.h"

#include <mutex>
#include <unordered_map>
#include <vector>
#include <memory>

namespace gits {
namespace DirectX {

class MapTrackingService : public gits::noncopyable {
public:
  MapTrackingService(GitsRecorder& recorder);

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
  struct MappedInfo : public gits::noncopyable {
    MappedInfo() = default;
    ~MappedInfo() {
      if (shadowAddress) {
        VirtualFree(shadowAddress, 0, MEM_RELEASE);
      }
    }
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

  GitsRecorder& recorder_;

private:
  bool isUploadHeap(D3D12_HEAP_TYPE heapType, D3D12_CPU_PAGE_PROPERTY cpuPageProperty);
  void captureModifiedData(MappedInfo* info);
  void captureData(
      unsigned resourceKey, void* mappedAddress, unsigned offset, void* data, unsigned dataSize);
  size_t getSubresourceSize(ID3D12Resource* resource, unsigned subresource);
};

} // namespace DirectX
} // namespace gits

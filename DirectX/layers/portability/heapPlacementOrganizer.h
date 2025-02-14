// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "config.h"
#include "log.h"

#include <vector>
#include <set>
#include <unordered_map>
#include <fstream>

namespace gits {
namespace DirectX {

class HeapPlacementOrganizer : public gits::noncopyable {

  struct ResData;
  struct HeapResourceMarker;
  struct HeapData;

  std::unordered_map<unsigned /*heapKey*/, HeapData> heapData_;
  std::unordered_map<unsigned /*heapKey*/, std::vector<ResData>> resData_;

  bool isPlayer_;

  std::mutex mutex_;

public:
  HeapPlacementOrganizer();
  ~HeapPlacementOrganizer();

public:
  void onPostCreateDevice(D3D12CreateDeviceCommand& c);

  template <typename CreateHeapCommmand>
  void onPreCreateHeap(CreateHeapCommmand& c) {
    if (!isPlayer_) {
      return;
    }

    // Resize heap if needed
    auto heapIt = heapData_.find(c.ppvHeap_.key);
    if (heapIt == heapData_.end()) {
      return;
    }

    if (heapIt->second.size > c.pDesc_.value->SizeInBytes) {
      c.pDesc_.value->SizeInBytes = heapIt->second.size;
    }
  }

  template <typename CreateHeapCommmand>
  void onPostCreateHeap(CreateHeapCommmand& c) {
    std::unique_lock<std::mutex> lock(mutex_, std::defer_lock);
    if (!isPlayer_) {
      lock.lock();
    }
    heapData_[c.ppvHeap_.key] = {c.pDesc_.value->SizeInBytes, c.pDesc_.value->Alignment};
  }

  void onPreCreateHeapAllocationMetaCommand(CreateHeapAllocationMetaCommand& c);
  void onPostCreateHeapAllocationMetaCommand(CreateHeapAllocationMetaCommand& c);

  template <class CreatePlacedResourceCommand>
  void onPreCreatePlacedRes(CreatePlacedResourceCommand& c) {
    if (!isPlayer_) {
      return;
    }

    auto heapIt = resData_.find(c.pHeap_.key);
    if (heapIt != resData_.end()) {
      const auto& resData = heapIt->second;
      auto resIt = std::find_if(resData.begin(), resData.end(),
                                [&c](const ResData& res) { return res.key == c.ppvResource_.key; });
      if (resIt != resData.end()) {
        c.HeapOffset_.value = resIt->curOffset;
      }
    }
  }

  template <class CreatePlacedResourceCommand>
  void onPostCreatePlacedRes(CreatePlacedResourceCommand& c) {

    auto* resDesc = reinterpret_cast<const D3D12_RESOURCE_DESC*>(c.pDesc_.value);
    const auto allocInfo = c.object_.value->GetResourceAllocationInfo(0, 1, resDesc);

    ResData resData{c.ppvResource_.key,    c.HeapOffset_.value,   c.HeapOffset_.value,
                    allocInfo.SizeInBytes, allocInfo.SizeInBytes, allocInfo.Alignment};

    auto heapDataIter = heapData_.find(c.pHeap_.key);
    if (heapDataIter == heapData_.end()) {
      Log(ERR) << "onPostCreatePlacedRes: heapKey " << c.pHeap_.key << " NOT found.";
      return;
    }

    if ((resData.curOffset % resData.alignment) != 0) {
      Log(ERR) << "onPostCreatePlacedRes: key " << c.ppvResource_.key << " of heap " << c.pHeap_.key
               << " offset " << resData.curOffset << " does not fit heap alignment "
               << resData.alignment;
      return;
    }

    if (resData.curOffset > heapDataIter->second.size) {
      Log(ERR) << "onPostCreatePlacedRes: resource " << c.ppvResource_.key << " of heap "
               << c.pHeap_.key << " start " << resData.curOffset << " exceeds heap boundary "
               << heapDataIter->second.size;
      return;
    }

    if (resData.curOffset + resData.curSize > heapDataIter->second.size) {
      Log(ERR) << "onPostCreatePlacedRes: resource " << c.ppvResource_.key << " of heap "
               << c.pHeap_.key << " end " << resData.curOffset + resData.curSize
               << " exceeds heap boundary " << heapDataIter->second.size;
      return;
    }

    if (!storeResourceData_) {
      return;
    }

    memcpy(&resData.description, resDesc, sizeof(D3D12_RESOURCE_DESC));
    storeResourceDataEntry(c.pHeap_.key, resData);
  }

  void onPreUpdateTileMappings(ID3D12CommandQueueUpdateTileMappingsCommand& c);

private:
  std::filesystem::path resDataFilePath_;
  const std::string singleFileName_{"resourcePlacementData.dat"};

  bool storeResourceData_{false};
  bool reconstructResourceData_{true};

  std::ofstream outFile_;
  std::set<unsigned> storedHeapKeys_;
  void openStoreForResourceData();
  void storeResourceDataEntry(unsigned heapKey, const ResData& resData);
  void storeResourceClosureDataEntry(unsigned heapKey);

  void getResourceData();

private:
  std::vector<unsigned> sizeChangeDetectedAtHeap_;

  bool updateResourceSizeData(ID3D12Device* device, std::vector<ResData>& data);
  void updateResourceSizeData(ID3D12Device* device);

  size_t updateResourcePlacementDataAtHeap(unsigned heapKey);
  void updateResourcePlacementData();

  uint64_t heapAlignedOffset(unsigned alignment, uint64_t offset) {
    return ((offset - 1) / alignment + 1) * alignment;
  }

private:
  void redistributeResources(std::vector<HeapResourceMarker>& markers);

private:
  bool HeapPlacementOrganizer::markersOverlap(const HeapResourceMarker& marker1,
                                              const HeapResourceMarker& marker2);
  void sortHeapMarkers(std::vector<HeapResourceMarker>& heapMarkers);
  int findFirstMarkerInIncrementZone(const std::vector<HeapResourceMarker>& markers, size_t idx);

  const ResData* findGreatestOffsetSizeLessThanPos(const std::vector<ResData>& data, uint64_t pos);

private:
  struct ResData {

    unsigned key;
    uint64_t curOffset{0};
    uint64_t orgOffset{0};
    size_t curSize{0};
    size_t orgSize{0};
    uint64_t alignment;

    D3D12_RESOURCE_DESC description;
  };
  void writeResData(std::ostream& stream, const ResData& resData);
  void readResData(std::istream& stream, ResData& resData);

  struct HeapResourceMarker {
    uint64_t start{0};
    uint64_t size{0};
    uint64_t shift{0};
    uint64_t increment{0};
    uint64_t alignment{0};
    bool fixed{false};

    ResData* resource{nullptr};
  };

  struct HeapData {
    size_t size;
    uint64_t alignment;
  };
};

} // namespace DirectX
} // namespace gits

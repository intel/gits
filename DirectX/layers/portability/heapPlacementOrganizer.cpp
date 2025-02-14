// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "HeapPlacementOrganizer.h"
#include "AsyncTaskScheduler.h"
#include "exception.h"
#include "gits.h"
#include "messageBus.h"

#include <algorithm>
#include <iterator>
#include <iostream>
#include <vector>
#include <filesystem>
#include <cstdint>
#include <stdexcept>
#include <limits>

namespace gits {
namespace DirectX {

HeapPlacementOrganizer::HeapPlacementOrganizer() : isPlayer_(Config::IsPlayer()) {

  auto& cfg = Config::Get();
  resDataFilePath_ = isPlayer_ ? cfg.common.player.streamDir : cfg.common.recorder.dumpPath;

  if ((Config::IsRecorder() && cfg.directx.features.portability.storePlacedResourceDataOnCapture) ||
      cfg.directx.features.portability.storePlacedResourceDataOnPlayback ||
      std::filesystem::exists(std::filesystem::path(resDataFilePath_ / "captureResData.txt"))) {
    storeResourceData_ = true;
  }
  getResourceData();
  openStoreForResourceData();

  if (!isPlayer_) {
    gits::CGits::Instance().GetMessageBus().subscribe(
        {PUBLISHER_RECORDER, TOPIC_END}, [this](Topic t, const MessagePtr& m) {
          auto msg = std::dynamic_pointer_cast<EndOfRecordingMessage>(m);
          if (msg && storeResourceData_) {
            for (auto heapKey : storedHeapKeys_) {
              storeResourceClosureDataEntry(heapKey);
            }
            storedHeapKeys_.clear();
          }
        });
  }
}

HeapPlacementOrganizer::~HeapPlacementOrganizer() {

  if (!storeResourceData_) {
    return;
  }
  try {
    for (auto heapKey : storedHeapKeys_) {
      storeResourceClosureDataEntry(heapKey);
    }
  } catch (...) {
    topmost_exception_handler("HeapPlacementOrganizer::~HeapPlacementOrganizer");
  }
}

void HeapPlacementOrganizer::onPostCreateDevice(D3D12CreateDeviceCommand& c) {
  if (!isPlayer_) {
    return;
  }

  if (*c.ppDevice_.value) {
    auto* device = reinterpret_cast<ID3D12Device*>(*c.ppDevice_.value);
    updateResourceSizeData(device);
    updateResourcePlacementData();
  }
}

void HeapPlacementOrganizer::onPreCreateHeapAllocationMetaCommand(
    CreateHeapAllocationMetaCommand& c) {
  if (!isPlayer_) {
    return;
  }

  // Resize heap if needed
  auto heapIt = heapData_.find(c.heap_.key);
  if (heapIt == heapData_.end()) {
    return;
  }

  if (heapIt->second.size > c.data_.size) {
    c.data_.size = heapIt->second.size;
  }
}

void HeapPlacementOrganizer::onPostCreateHeapAllocationMetaCommand(
    CreateHeapAllocationMetaCommand& c) {
  std::unique_lock<std::mutex> lock(mutex_, std::defer_lock);
  if (!isPlayer_) {
    lock.lock();
  }
  if (!c.heap_.value) {
    return;
  }
  const auto heapDesc = c.heap_.value->GetDesc();
  const auto alignment =
      (heapDesc.Alignment == 0) ? D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT : heapDesc.Alignment;
  heapData_[c.heap_.key] = {heapDesc.SizeInBytes, alignment};
}

void HeapPlacementOrganizer::onPreUpdateTileMappings(
    ID3D12CommandQueueUpdateTileMappingsCommand& c) {
  if (!isPlayer_) {
    return;
  }

  auto heapResIt = resData_.find(c.pHeap_.key);
  if (heapResIt == resData_.end()) {
    return; // original heap preserved, nothig to do...
  }

  auto heapIt = heapData_.find(c.pHeap_.key);
  if (heapIt == heapData_.end()) {
    Log(ERR) << "onPreUpdateTileMappings: heapData for heap " << c.pHeap_.key
             << " NOT found -> unknown heap ?? ..." << std::endl;
    return;
  }

  auto* tileIndexes = c.pHeapRangeStartOffsets_.value;
  if (!tileIndexes) {
    return;
  }

  const auto& resData = heapResIt->second;
  const auto heapAlignment = heapIt->second.alignment;

  for (auto i = 0u; i < c.pHeapRangeStartOffsets_.size; ++i) {

    auto& tileSlotIndex = tileIndexes[i];

    uint64_t tileByteOffset = heapAlignment * tileSlotIndex;
    auto* found = findGreatestOffsetSizeLessThanPos(resData, tileByteOffset);
    if (!found) {
      continue;
    }

    tileByteOffset += found->curOffset - found->orgOffset;
    tileSlotIndex = static_cast<unsigned>(tileByteOffset / heapAlignment);
  }
}

void HeapPlacementOrganizer::openStoreForResourceData() {
  if (!storeResourceData_) {
    return;
  }

  const std::filesystem::path resFile(resDataFilePath_ / singleFileName_);
  outFile_.open(resFile, std::ios::binary);
  if (!outFile_) {
    throw std::runtime_error("Failed to open file for writing");
  }
  static constexpr unsigned version = 1;
  outFile_.write(reinterpret_cast<const char*>(&version), sizeof(version));
  outFile_.flush();
}

void HeapPlacementOrganizer::storeResourceDataEntry(unsigned heapKey, const ResData& resData) {

  if (!storeResourceData_) {
    return;
  }

  std::unique_lock<std::mutex> lock(mutex_, std::defer_lock);
  if (!isPlayer_) {
    lock.lock();
  }

  storedHeapKeys_.emplace(heapKey);
  outFile_.write(reinterpret_cast<const char*>(&heapKey), sizeof(heapKey));
  writeResData(outFile_, resData);
  outFile_.flush();
}

void HeapPlacementOrganizer::storeResourceClosureDataEntry(unsigned heapKey) {

  if (!storeResourceData_) {
    return;
  }

  uint64_t offset{0};
  uint64_t alignment{0};
  {
    std::unique_lock<std::mutex> lock(mutex_, std::defer_lock);
    if (!isPlayer_) {
      lock.lock();
    }

    auto heapIt = heapData_.find(heapKey);
    if (heapIt == heapData_.end()) {
      Log(ERR) << "storeResourceData: heap " << heapKey << " data not found." << std::endl;
      return;
    }
    offset = heapIt->second.size;
    alignment = heapIt->second.alignment;
  }

  // Closure "end of heap"
  ResData closureResData{UINT32_MAX, offset, offset, 0, 0, alignment};
  memset(&closureResData.description, 0x0, sizeof(D3D12_RESOURCE_DESC));
  storeResourceDataEntry(heapKey, closureResData);
}

void HeapPlacementOrganizer::getResourceData() {
  if (!reconstructResourceData_) {
    return;
  }

  const std::filesystem::path resFile(resDataFilePath_ / singleFileName_);
  if (!std::filesystem::is_regular_file(resFile)) {
    return;
  }

  std::ifstream inFile(resFile, std::ios::binary);
  if (!inFile) {
    throw std::runtime_error("Failed to open file for reading");
  }

  auto isReadingOk = [&inFile](const char* label) -> bool {
    if (inFile.eof()) {
      return false; // Check for end of file after reading the key
    }
    if (inFile.fail()) {
      Log(ERR) << "An error occurred while reading " << label << std::endl;
      return false;
    }
    return true;
  };

  unsigned version = 0;
  inFile.read(reinterpret_cast<char*>(&version), sizeof(version));
  if (!isReadingOk("version") || version != 1) {
    return;
  }

  while (true) {
    unsigned heapKey{};
    inFile.read(reinterpret_cast<char*>(&heapKey), sizeof(heapKey));
    if (!isReadingOk("key")) {
      break;
    }

    ResData resData{0};
    readResData(inFile, resData);

    if (!isReadingOk("resData")) {
      break;
    }
    resData_[heapKey].emplace_back(std::move(resData));
  }
}

bool HeapPlacementOrganizer::updateResourceSizeData(ID3D12Device* device,
                                                    std::vector<ResData>& data) {
  bool changeDetected{false};
  for (auto& resData : data) {

    if (resData.key == UINT32_MAX) { // end_of_heap, not real resource
      continue;
    }

    const auto resAllocInfo = device->GetResourceAllocationInfo(0, 1, &resData.description);
    if (resAllocInfo.SizeInBytes == UINT64_MAX) {
      Log(ERR) << "updateResourceSizeData: error while GetResourceAllocationInfo for "
               << resData.key;
    }
    resData.curSize = std::max(resAllocInfo.SizeInBytes, resData.orgSize);

    if (!changeDetected && resData.curSize > resData.orgSize) {
      changeDetected = true;
    }
  }
  return changeDetected;
}

void HeapPlacementOrganizer::updateResourceSizeData(ID3D12Device* device) {
  AsyncTaskScheduler scheduler;
  std::thread schedulerThread([&scheduler]() { scheduler.run(); });

  std::vector<std::future<bool>> futures;
  for (auto& heapResData : resData_) {
    auto updateCall = [this, device, &heapResData]() -> bool {
      return updateResourceSizeData(device, heapResData.second);
    };
    futures.emplace_back(scheduler.enqueue<bool>(updateCall));
  }

  scheduler.stopScheduler();
  schedulerThread.join();

  unsigned i{0};
  for (auto& heapResData : resData_) {
    if (futures[i++].get()) {
      sizeChangeDetectedAtHeap_.emplace_back(heapResData.first);
    }
  }
}

void HeapPlacementOrganizer::redistributeResources(std::vector<HeapResourceMarker>& markers) {

  auto updateMarker = [](HeapResourceMarker& m) -> void {
    m.size += m.increment;
    m.increment = 0;
    m.fixed = true;
  };

  sortHeapMarkers(markers);

  for (auto i = 0u; i < markers.size(); ++i) {
    auto& marker = markers[i];

    if (marker.increment == 0) {
      continue;
    }

    const auto idx = findFirstMarkerInIncrementZone(markers, i);
    if (idx == -1) {
      updateMarker(marker);
      continue;
    }

    auto& pivotMarker = markers[idx];

    auto pivotTotalShift = pivotMarker.shift + marker.increment;
    const auto alignmentDelta =
        heapAlignedOffset(pivotMarker.alignment, pivotTotalShift) - pivotTotalShift;
    const auto alignedShift = marker.increment + alignmentDelta;
    for (auto j = idx; j < markers.size(); ++j) {
      markers[j].shift += alignedShift;
    }

    updateMarker(marker);
  }

  for (auto& marker : markers) {
    if (marker.shift == 0) {
      marker.fixed = true;
      continue;
    }
    marker.start += marker.shift;
    marker.shift = 0;
    marker.fixed = true;
    marker.resource->curOffset = marker.start;
  }
}

size_t HeapPlacementOrganizer::updateResourcePlacementDataAtHeap(unsigned heapKey) {
  std::vector<HeapResourceMarker> markers;
  for (auto& resData : resData_[heapKey]) {
    markers.emplace_back(HeapResourceMarker{resData.orgOffset, resData.orgSize, 0,
                                            resData.curSize - resData.orgSize, resData.alignment,
                                            false, &resData});
  }

  redistributeResources(markers);

  return markers.back().start + markers.back().size;
}

void HeapPlacementOrganizer::updateResourcePlacementData() {

  if (sizeChangeDetectedAtHeap_.empty()) {
#ifdef _DEBUG
    Log(INFO) << "updateResourcePlacementData: NO size change detected. Proceeding...";
#endif
    return;
  }

  AsyncTaskScheduler scheduler;
  std::thread schedulerThread([&scheduler]() { scheduler.run(); });

  std::vector<std::future<size_t>> futures;
  for (const auto heapKey : sizeChangeDetectedAtHeap_) {
    auto updateCall = [this, heapKey]() -> size_t {
      return updateResourcePlacementDataAtHeap(heapKey);
    };

    futures.emplace_back(scheduler.enqueue<size_t>(updateCall));
  }

  scheduler.stopScheduler();
  schedulerThread.join();

  for (auto i = 0u; i < futures.size(); ++i) {
    heapData_[sizeChangeDetectedAtHeap_[i]].size = futures[i].get();
  }
}

bool HeapPlacementOrganizer::markersOverlap(const HeapResourceMarker& marker1,
                                            const HeapResourceMarker& marker2) {
  return (marker1.start < marker2.start + marker2.size) &&
         (marker2.start < marker1.start + marker1.size);
}

void HeapPlacementOrganizer::sortHeapMarkers(std::vector<HeapResourceMarker>& heapMarkers) {
  std::sort(heapMarkers.begin(), heapMarkers.end(),
            [](const HeapResourceMarker& a, const HeapResourceMarker& b) {
              if (a.start == b.start) {
                return a.size < b.size;
              }
              return a.start < b.start;
            });
}

// true if m.Start >= orgend && m.Start < incrementedEnd && original markers did not overlap
int HeapPlacementOrganizer::findFirstMarkerInIncrementZone(
    const std::vector<HeapResourceMarker>& markers, size_t idx) {
  if (idx >= markers.size()) {
    throw std::out_of_range("Index is out of range");
  }

  auto& marker = markers[idx];
  unsigned targetStart = markers[idx].start + markers[idx].shift + markers[idx].size;
  unsigned targetEnd = targetStart + markers[idx].increment;

  auto it =
      std::find_if(markers.begin() + idx + 1, markers.end(),
                   [this, &marker, targetStart, targetEnd](const HeapResourceMarker& m) -> bool {
                     if (markersOverlap(marker, m)) {
                       return false;
                     }
                     const auto start = m.start + m.shift;
                     return (start >= targetStart && start < targetEnd);
                   });

  // Return -1 if no matching element is found
  return (it != markers.end()) ? std::distance(markers.begin(), it) : -1;
}

const HeapPlacementOrganizer::ResData* HeapPlacementOrganizer::findGreatestOffsetSizeLessThanPos(
    const std::vector<ResData>& data, uint64_t pos) {
  const ResData* result = nullptr;
  unsigned maxOffsetSize = 0;

  for (auto& item : data) {
    auto currentOffsetSize = item.orgOffset + item.orgSize;
    if (currentOffsetSize <= pos && currentOffsetSize > maxOffsetSize) {
      maxOffsetSize = currentOffsetSize;
      result = &item;
    }
  }

  return result;
}

void HeapPlacementOrganizer::writeResData(std::ostream& stream, const ResData& resData) {
  outFile_.write(reinterpret_cast<const char*>(&resData.key), sizeof(resData.key));
  outFile_.write(reinterpret_cast<const char*>(&resData.orgOffset), sizeof(resData.orgOffset));
  outFile_.write(reinterpret_cast<const char*>(&resData.orgSize), sizeof(resData.orgSize));
  outFile_.write(reinterpret_cast<const char*>(&resData.alignment), sizeof(resData.alignment));
  outFile_.write(reinterpret_cast<const char*>(&resData.description), sizeof(resData.description));
}

void HeapPlacementOrganizer::readResData(std::istream& stream, ResData& resData) {
  stream.read(reinterpret_cast<char*>(&resData.key), sizeof(resData.key));
  stream.read(reinterpret_cast<char*>(&resData.orgOffset), sizeof(resData.orgOffset));
  stream.read(reinterpret_cast<char*>(&resData.orgSize), sizeof(resData.orgSize));
  stream.read(reinterpret_cast<char*>(&resData.alignment), sizeof(resData.alignment));
  stream.read(reinterpret_cast<char*>(&resData.description), sizeof(resData.description));

  resData.curOffset = resData.orgOffset;
  resData.curSize = resData.orgSize;
}

} // namespace DirectX
} // namespace gits

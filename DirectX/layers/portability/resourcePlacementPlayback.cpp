// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "resourcePlacementPlayback.h"
#include "gits.h"
#include "log.h"
#include "configurationLib.h"

#include <filesystem>
#include <fstream>

namespace gits {
namespace DirectX {

void ResourcePlacementPlayback::createHeap(ID3D12Device* device, unsigned heapKey, UINT64& size) {
  if (!initialized_) {
    calculateResourcePlacement(device);
    initialized_ = true;
    if (!heapSizeShifts_.empty()) {
      LOG_INFO << "Resource placement changed for " << heapSizeShifts_.size() << " heaps";
    }
  }
  auto it = heapSizeShifts_.find(heapKey);
  if (it != heapSizeShifts_.end()) {
    size += it->second;
  }
}

void ResourcePlacementPlayback::createPlacedResource(unsigned resourceKey, UINT64& offset) {
  auto it = changedResourceOffsets_.find(resourceKey);
  if (it != changedResourceOffsets_.end()) {
    offset = it->second;
  }
}

void ResourcePlacementPlayback::updateTileMappings(ID3D12CommandQueueUpdateTileMappingsCommand& c) {
  auto it = infos_.find(c.pHeap_.key);
  if (it == infos_.end()) {
    return;
  }
  unsigned tileSize = 64 * 1024;
  std::vector<ResourcePlacementShiftInfo>& infos = it->second;
  for (unsigned i = 0; i < c.pHeapRangeStartOffsets_.size; ++i) {
    unsigned& tileOffset = c.pHeapRangeStartOffsets_.value[i];
    unsigned tileShift{};
    for (ResourcePlacementShiftInfo& info : infos) {
      if (info.offset > tileOffset * tileSize) {
        break;
      }
      tileShift = info.shift / tileSize;
    }
    tileOffset += tileShift;
  }
}

void ResourcePlacementPlayback::calculateResourcePlacement(ID3D12Device* device) {
  std::filesystem::path filePath = Configurator::IsPlayer()
                                       ? Configurator::Get().common.player.streamDir
                                       : Configurator::Get().common.recorder.dumpPath;
  filePath /= "resourcePlacementData.dat";

  std::ifstream file(filePath, std::ios::binary);
  while (true) {
    ResourcePlacementShiftInfo info{};
    if (!file.read(reinterpret_cast<char*>(&info), sizeof(ResourcePlacementInfo))) {
      break;
    }
    infos_[info.heapKey].push_back(info);
  }

  for (auto& it : infos_) {
    calculateResourcePlacement(device, it.first, it.second);
  }
}

void ResourcePlacementPlayback::calculateResourcePlacement(
    ID3D12Device* device, unsigned heapKey, std::vector<ResourcePlacementShiftInfo>& infos) {

  unsigned sizeChanged = 0;
  for (ResourcePlacementShiftInfo& info : infos) {
    D3D12_RESOURCE_ALLOCATION_INFO allocInfo = device->GetResourceAllocationInfo(0, 1, &info.desc);
    if (allocInfo.SizeInBytes == UINT64_MAX) {
      LOG_ERROR << "Portability - GetResourceAllocationInfo failed for resource O" << info.key;
      continue;
    }
    if (allocInfo.SizeInBytes > info.size) {
      info.increment = allocInfo.SizeInBytes - info.size;
      info.alignment = allocInfo.Alignment;
      ++sizeChanged;
    }
  }

  if (!sizeChanged) {
    return;
  }

  std::sort(infos.begin(), infos.end(),
            [](const ResourcePlacementInfo& a, const ResourcePlacementInfo& b) {
              if (a.offset == b.offset) {
                return a.size < b.size;
              }
              return a.offset < b.offset;
            });

  for (unsigned infoIndex = 0; infoIndex < infos.size(); ++infoIndex) {
    ResourcePlacementShiftInfo& currentInfo = infos[infoIndex];

    if (!currentInfo.increment) {
      continue;
    }

    UINT64 shiftStart = currentInfo.offset + currentInfo.shift + currentInfo.size;
    UINT64 shiftEnd = shiftStart + currentInfo.increment;
    auto itShift = std::find_if(
        infos.begin() + infoIndex + 1, infos.end(), [&](ResourcePlacementShiftInfo& info) {
          return info.offset + info.shift >= shiftStart && info.offset + info.shift < shiftEnd;
        });
    if (itShift == infos.end()) {
      continue;
    }
    ResourcePlacementShiftInfo& shiftInfo = *itShift;

    UINT64 shift = shiftInfo.shift + currentInfo.increment;
    UINT64 alignmentAdjustment = getAlignedOffset(shiftInfo.alignment, shift) - shift;
    UINT64 alignedShift = currentInfo.increment + alignmentAdjustment;

    for (auto& it = itShift; itShift != infos.end(); ++itShift) {
      if (it->alignment > shiftInfo.alignment) {
        UINT64 alignmentAdjustment = getAlignedOffset(it->alignment, alignedShift) - alignedShift;
        alignedShift += alignmentAdjustment;
      }
      it->shift += alignedShift;
    }
  }

  UINT64 heapShift{};
  for (ResourcePlacementShiftInfo& info : infos) {
    if (info.shift) {
      changedResourceOffsets_[info.key] = info.offset + info.shift;
    }
    heapShift = std::max(heapShift, info.shift + info.increment);
  }

  heapSizeShifts_[heapKey] = heapShift;
}

UINT64 ResourcePlacementPlayback::getAlignedOffset(UINT64 alignment, UINT64 offset) {
  if (!alignment) {
    alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
  }
  return ((offset - 1) / alignment + 1) * alignment;
}

} // namespace DirectX
} // namespace gits

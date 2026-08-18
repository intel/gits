// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "resourcePlacementPlayback.h"
#include "arguments.h"
#include "log.h"
#include "configurationLib.h"

#include <filesystem>
#include <fstream>

namespace gits {
namespace DirectX {

void ResourcePlacementPlayback::CreateHeap(ID3D12Device* device, GITSKey heapKey, UINT64& size) {
  if (!m_Initialized) {
    CalculateResourcePlacement(device);
    m_Initialized = true;
    if (!m_HeapSizeShifts.empty()) {
      LOG_INFO << "Resource placement changed for " << m_HeapSizeShifts.size() << " heaps";
    }
  }
  auto it = m_HeapSizeShifts.find(heapKey);
  if (it != m_HeapSizeShifts.end()) {
    size += it->second;
  }
}

void ResourcePlacementPlayback::CreatePlacedResource(GITSKey resourceKey, UINT64& offset) {
  auto it = m_ChangedResourceOffsets.find(resourceKey);
  if (it != m_ChangedResourceOffsets.end()) {
    offset = it->second;
  }
}

void ResourcePlacementPlayback::UpdateTileMappings(ID3D12CommandQueueUpdateTileMappingsCommand& c) {
  auto it = m_Infos.find(c.m_pHeap.Key);
  if (it == m_Infos.end()) {
    return;
  }
  unsigned tileSize = 64 * 1024;
  std::vector<ResourcePlacementShiftInfo>& infos = it->second;
  for (unsigned i = 0; i < c.m_pHeapRangeStartOffsets.Size; ++i) {
    unsigned& tileOffset = c.m_pHeapRangeStartOffsets.Value[i];
    unsigned tileShift{};
    for (ResourcePlacementShiftInfo& info : infos) {
      if (info.Offset > tileOffset * tileSize) {
        break;
      }
      tileShift = info.Shift / tileSize;
    }
    tileOffset += tileShift;
  }
}

void ResourcePlacementPlayback::CalculateResourcePlacement(ID3D12Device* device) {
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
    m_Infos[info.HeapKey].push_back(info);
  }

  for (auto& it : m_Infos) {
    CalculateResourcePlacement(device, it.first, it.second);
  }
}

void ResourcePlacementPlayback::CalculateResourcePlacement(
    ID3D12Device* device, GITSKey heapKey, std::vector<ResourcePlacementShiftInfo>& infos) {

  unsigned sizeChanged = 0;
  for (ResourcePlacementShiftInfo& info : infos) {
    D3D12_RESOURCE_ALLOCATION_INFO allocInfo = device->GetResourceAllocationInfo(0, 1, &info.Desc);
    if (allocInfo.SizeInBytes == UINT64_MAX) {
      LOG_ERROR << "Portability - GetResourceAllocationInfo failed for resource O" << info.Key;
      continue;
    }
    if (allocInfo.SizeInBytes > info.Size) {
      info.Increment = allocInfo.SizeInBytes - info.Size;
      info.Alignment = allocInfo.Alignment;
      ++sizeChanged;
    }
  }

  if (!sizeChanged) {
    return;
  }

  std::sort(infos.begin(), infos.end(),
            [](const ResourcePlacementInfo& a, const ResourcePlacementInfo& b) {
              if (a.Offset == b.Offset) {
                return a.Size < b.Size;
              }
              return a.Offset < b.Offset;
            });

  for (unsigned infoIndex = 0; infoIndex < infos.size(); ++infoIndex) {
    ResourcePlacementShiftInfo& currentInfo = infos[infoIndex];

    if (!currentInfo.Increment) {
      continue;
    }

    UINT64 shiftStart = currentInfo.Offset + currentInfo.Shift + currentInfo.Size;
    UINT64 shiftEnd = shiftStart + currentInfo.Increment;
    auto itShift = std::find_if(
        infos.begin() + infoIndex + 1, infos.end(), [&](ResourcePlacementShiftInfo& info) {
          return info.Offset + info.Shift >= shiftStart && info.Offset + info.Shift < shiftEnd;
        });
    if (itShift == infos.end()) {
      continue;
    }
    ResourcePlacementShiftInfo& shiftInfo = *itShift;

    UINT64 shift = shiftInfo.Shift + currentInfo.Increment;
    UINT64 alignmentAdjustment = GetAlignedOffset(shiftInfo.Alignment, shift) - shift;
    UINT64 alignedShift = currentInfo.Increment + alignmentAdjustment;

    for (auto& it = itShift; itShift != infos.end(); ++itShift) {
      if (it->Alignment > shiftInfo.Alignment) {
        UINT64 alignmentAdjustment = GetAlignedOffset(it->Alignment, alignedShift) - alignedShift;
        alignedShift += alignmentAdjustment;
      }
      it->Shift += alignedShift;
    }
  }

  UINT64 heapShift{};
  for (ResourcePlacementShiftInfo& info : infos) {
    if (info.Shift) {
      m_ChangedResourceOffsets[info.Key] = info.Offset + info.Shift;
    }
    heapShift = std::max(heapShift, info.Shift + info.Increment);
  }

  m_HeapSizeShifts[heapKey] = heapShift;
}

UINT64 ResourcePlacementPlayback::GetAlignedOffset(UINT64 alignment, UINT64 offset) {
  if (!alignment) {
    alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
  }
  return ((offset - 1) / alignment + 1) * alignment;
}

} // namespace DirectX
} // namespace gits

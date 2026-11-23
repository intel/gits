// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"

#include "resourcePlacementCapture.h"
#include "commandsAuto.h"

#include <d3d12.h>
#include <unordered_map>

namespace gits {
namespace DirectX {

class ResourcePlacementPlayback {
public:
  void CreateHeap(ID3D12Device* device, ObjectKey heapKey, UINT64& size);
  void CreatePlacedResource(ObjectKey resourceKey, UINT64& offset);
  void UpdateTileMappings(ID3D12CommandQueueUpdateTileMappingsCommand& c);
  void CalculateResourcePlacement(ID3D12Device* device);

private:
  struct ResourcePlacementShiftInfo : ResourcePlacementInfo {
    UINT64 Increment{};
    UINT64 Shift{};
  };

  std::mutex m_Mutex;
  std::unordered_map<ObjectKey, UINT64> m_ChangedResourceOffsets;
  std::unordered_map<ObjectKey, UINT64> m_HeapSizeShifts;
  bool m_Initialized{};
  std::unordered_map<ObjectKey, std::vector<ResourcePlacementShiftInfo>> m_Infos;

private:
  void CalculateResourcePlacement(ID3D12Device* device,
                                  ObjectKey heapKey,
                                  std::vector<ResourcePlacementShiftInfo>& infos);
  UINT64 GetAlignedOffset(UINT64 alignment, UINT64 offset);
};

} // namespace DirectX
} // namespace gits

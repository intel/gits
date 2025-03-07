// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "resourcePlacementCapture.h"

#include <d3d12.h>
#include <unordered_map>

namespace gits {
namespace DirectX {

class ResourcePlacementPlayback {
public:
  void createHeap(ID3D12Device* device, unsigned heapKey, UINT64& size);
  void createPlacedResource(unsigned resourceKey, UINT64& offset);
  void calculateResourcePlacement(ID3D12Device* device);

private:
  struct ResourcePlacementShiftInfo : ResourcePlacementInfo {
    UINT64 increment{};
    UINT64 shift{};
  };

  std::mutex mutex_;
  std::unordered_map<unsigned, UINT64> changedResourceOffsets_;
  std::unordered_map<unsigned, UINT64> changedHeapSizes_;
  bool initialized_{};

private:
  void calculateResourcePlacement(ID3D12Device* device,
                                  unsigned heapKey,
                                  std::vector<ResourcePlacementShiftInfo>& infos);
  UINT64 getAlignedOffset(UINT64 alignment, UINT64 offset);
};

} // namespace DirectX
} // namespace gits

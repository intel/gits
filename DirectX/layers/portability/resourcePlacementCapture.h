// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <d3d12.h>
#include <mutex>
#include <vector>

namespace gits {
namespace DirectX {

struct ResourcePlacementInfo {
  unsigned heapKey{};
  unsigned key{};
  UINT64 offset{};
  UINT64 size{};
  UINT64 alignment{};
  D3D12_RESOURCE_DESC desc{};
};

class ResourcePlacementCapture {
public:
  void createPlacedResource(unsigned heapKey,
                            unsigned resourceKey,
                            UINT64 offset,
                            ID3D12Device* device,
                            D3D12_RESOURCE_DESC& desc);
  void storeResourcePlacement();

private:
  std::mutex mutex_;
  std::vector<ResourcePlacementInfo> resourcePlacementInfos_;
};

} // namespace DirectX
} // namespace gits

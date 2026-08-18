// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"

#include <d3d12.h>
#include <mutex>
#include <vector>

namespace gits {
namespace DirectX {

struct ResourcePlacementInfo {
  GITSKey HeapKey{};
  GITSKey Key{};
  UINT64 Offset{};
  UINT64 Size{};
  UINT64 Alignment{};
  D3D12_RESOURCE_DESC Desc{};
};

class ResourcePlacementCapture {
public:
  void CreatePlacedResource(GITSKey heapKey,
                            GITSKey resourceKey,
                            UINT64 offset,
                            ID3D12Device* device,
                            D3D12_RESOURCE_DESC& desc);
  void StoreResourcePlacement();

private:
  std::mutex m_Mutex;
  std::vector<ResourcePlacementInfo> m_ResourcePlacementInfos;
};

} // namespace DirectX
} // namespace gits

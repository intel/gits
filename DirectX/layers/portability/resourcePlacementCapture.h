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
  GITSKey heapKey{};
  GITSKey key{};
  UINT64 offset{};
  UINT64 size{};
  UINT64 alignment{};
  D3D12_RESOURCE_DESC desc{};
};

class ResourcePlacementCapture {
public:
  void createPlacedResource(GITSKey heapKey,
                            GITSKey resourceKey,
                            UINT64 offset,
                            ID3D12Device* device,
                            D3D12_RESOURCE_DESC& desc);
  void storeResourcePlacement();

private:
  std::mutex m_Mutex;
  std::vector<ResourcePlacementInfo> m_ResourcePlacementInfos;
};

} // namespace DirectX
} // namespace gits

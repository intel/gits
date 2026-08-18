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

#include <d3d12.h>
#include <unordered_map>

namespace gits {
namespace DirectX {

class ResourcePlacementAssertions {
public:
  ResourcePlacementAssertions();

  void CreatePlacedResource(GITSKey resourceKey,
                            const D3D12_RESOURCE_DESC& desc,
                            ID3D12Device* device);
  void CreatePlacedResource(GITSKey resourceKey,
                            const D3D12_RESOURCE_DESC1& desc,
                            ID3D12Device* device);

private:
  struct AllocationInfo {
    D3D12_RESOURCE_ALLOCATION_INFO Pre{};
    D3D12_RESOURCE_ALLOCATION_INFO Post{};
  };

  const ResourcePlacementInfo* FindPlacementData(GITSKey resourceKey);
  D3D12_RESOURCE_ALLOCATION_INFO QueryAllocationFromDevice(ID3D12Device* device,
                                                           const D3D12_RESOURCE_DESC& desc,
                                                           GITSKey resourceKey);
  void CheckCompatibility(const AllocationInfo& allocationInfo,
                          const D3D12_RESOURCE_DESC& desc,
                          GITSKey resourceKey);

  void LoadResourcePlacementData();

  std::unordered_map<GITSKey, ResourcePlacementInfo> m_PlacementDataFromFile;
  bool m_PlacementDataLoaded{};
};

} // namespace DirectX
} // namespace gits

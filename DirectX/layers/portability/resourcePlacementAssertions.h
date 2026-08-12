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

  void createPlacedResource(GITSKey resourceKey,
                            const D3D12_RESOURCE_DESC& desc,
                            ID3D12Device* device);
  void createPlacedResource(GITSKey resourceKey,
                            const D3D12_RESOURCE_DESC1& desc,
                            ID3D12Device* device);

private:
  struct AllocationInfo {
    D3D12_RESOURCE_ALLOCATION_INFO pre{};
    D3D12_RESOURCE_ALLOCATION_INFO post{};
  };

  const ResourcePlacementInfo* findPlacementData(GITSKey resourceKey);
  D3D12_RESOURCE_ALLOCATION_INFO queryAllocationFromDevice(ID3D12Device* device,
                                                           const D3D12_RESOURCE_DESC& desc,
                                                           GITSKey resourceKey);
  void checkCompatibility(const AllocationInfo& allocationInfo,
                          const D3D12_RESOURCE_DESC& desc,
                          GITSKey resourceKey);

  void loadResourcePlacementData();

  std::unordered_map<GITSKey, ResourcePlacementInfo> m_PlacementDataFromFile;
  bool m_PlacementDataLoaded{};
};

} // namespace DirectX
} // namespace gits

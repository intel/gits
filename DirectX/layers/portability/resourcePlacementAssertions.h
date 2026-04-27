// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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

class ResourcePlacementAssertions {
public:
  ResourcePlacementAssertions();

  void createPlacedResource(unsigned resourceKey,
                            const D3D12_RESOURCE_DESC& desc,
                            ID3D12Device* device);
  void createPlacedResource(unsigned resourceKey,
                            const D3D12_RESOURCE_DESC1& desc,
                            ID3D12Device* device);

private:
  struct AllocationInfo {
    D3D12_RESOURCE_ALLOCATION_INFO pre{};
    D3D12_RESOURCE_ALLOCATION_INFO post{};
  };

  const ResourcePlacementInfo* findPlacementData(unsigned resourceKey);
  D3D12_RESOURCE_ALLOCATION_INFO queryAllocationFromDevice(ID3D12Device* device,
                                                           const D3D12_RESOURCE_DESC& desc,
                                                           unsigned resourceKey);
  void checkCompatibility(const AllocationInfo& allocationInfo, unsigned resourceKey);

  void loadResourcePlacementData();

  std::unordered_map<unsigned, ResourcePlacementInfo> m_PlacementDataFromFile;
  bool m_PlacementDataLoaded{};
};

} // namespace DirectX
} // namespace gits

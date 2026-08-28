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

  void CreatePlacedResource(ObjectKey resourceKey,
                            const D3D12_RESOURCE_DESC& desc,
                            ID3D12Device* device);
  void CreatePlacedResource(ObjectKey resourceKey,
                            const D3D12_RESOURCE_DESC1& desc,
                            ID3D12Device* device);

private:
  struct AllocationInfo {
    D3D12_RESOURCE_ALLOCATION_INFO Pre{};
    D3D12_RESOURCE_ALLOCATION_INFO Post{};
  };

  const ResourcePlacementInfo* FindPlacementData(ObjectKey resourceKey);
  D3D12_RESOURCE_ALLOCATION_INFO QueryAllocationFromDevice(ID3D12Device* device,
                                                           const D3D12_RESOURCE_DESC& desc,
                                                           ObjectKey resourceKey);
  void CheckCompatibility(const AllocationInfo& allocationInfo,
                          const D3D12_RESOURCE_DESC& desc,
                          ObjectKey resourceKey);

  void LoadResourcePlacementData();

  std::unordered_map<ObjectKey, ResourcePlacementInfo> m_PlacementDataFromFile;
  bool m_PlacementDataLoaded{};
};

} // namespace DirectX
} // namespace gits

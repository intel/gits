// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "resourcePlacementAssertions.h"
#include "configurationLib.h"
#include "log.h"

#include <filesystem>
#include <fstream>

namespace gits {
namespace DirectX {

ResourcePlacementAssertions::ResourcePlacementAssertions() {
  loadResourcePlacementData();
}

void ResourcePlacementAssertions::createPlacedResource(unsigned resourceKey,
                                                       const D3D12_RESOURCE_DESC& desc,
                                                       ID3D12Device* device) {
  if (!m_PlacementDataLoaded) {
    return;
  }

  const ResourcePlacementInfo* placementInfo = findPlacementData(resourceKey);

  if (!placementInfo) {
    static bool logged = false;
    if (!logged) {
      LOG_ERROR << "Portability - no resource placement data for resource: O" << resourceKey;
      logged = true;
    }
    return;
  }

  AllocationInfo info{};
  info.pre = {placementInfo->size, placementInfo->alignment};
  info.post = queryAllocationFromDevice(device, desc, resourceKey);
  checkCompatibility(info, resourceKey);
}

void ResourcePlacementAssertions::createPlacedResource(unsigned resourceKey,
                                                       const D3D12_RESOURCE_DESC1& desc,
                                                       ID3D12Device* device) {
  if (!m_PlacementDataLoaded) {
    return;
  }

  const ResourcePlacementInfo* placementInfo = findPlacementData(resourceKey);

  if (!placementInfo) {
    static bool logged = false;
    if (!logged) {
      LOG_ERROR << "Portability - no resource placement data for resource: O" << resourceKey;
      logged = true;
    }
    return;
  }

  if (desc.Format == DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE ||
      desc.Format == DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE) {
    if (desc.SamplerFeedbackMipRegion.Width || desc.SamplerFeedbackMipRegion.Height ||
        desc.SamplerFeedbackMipRegion.Depth) {
      static bool logged = false;
      if (!logged) {
        LOG_WARNING << "Portability - incompatibility in SamplerFeedbackMipRegion not checked";
        logged = true;
      }
    }
  }

  D3D12_RESOURCE_DESC baseDesc{desc.Dimension,        desc.Alignment, desc.Width,  desc.Height,
                               desc.DepthOrArraySize, desc.MipLevels, desc.Format, desc.SampleDesc,
                               desc.Layout,           desc.Flags};

  AllocationInfo info{};
  info.pre = {placementInfo->size, placementInfo->alignment};
  info.post = queryAllocationFromDevice(device, baseDesc, resourceKey);
  checkCompatibility(info, resourceKey);
}

const ResourcePlacementInfo* ResourcePlacementAssertions::findPlacementData(unsigned resourceKey) {
  const auto it = m_PlacementDataFromFile.find(resourceKey);
  if (it != m_PlacementDataFromFile.end()) {
    return &it->second;
  }
  return nullptr;
}

D3D12_RESOURCE_ALLOCATION_INFO ResourcePlacementAssertions::queryAllocationFromDevice(
    ID3D12Device* device, const D3D12_RESOURCE_DESC& desc, unsigned resourceKey) {
  D3D12_RESOURCE_ALLOCATION_INFO allocInfo = device->GetResourceAllocationInfo(0, 1, &desc);
  if (allocInfo.SizeInBytes == UINT64_MAX) {
    LOG_ERROR << "Portability - GetResourceAllocationInfo failed for resource: O" << resourceKey;
  }
  return allocInfo;
}

void ResourcePlacementAssertions::checkCompatibility(const AllocationInfo& allocationInfo,
                                                     unsigned resourceKey) {
  if (allocationInfo.pre.Alignment != allocationInfo.post.Alignment) {
    LOG_ERROR << "Portability - incompatible alignment for resource: O" << resourceKey;
  }
  GITS_ASSERT(allocationInfo.pre.Alignment == allocationInfo.post.Alignment);

  if (allocationInfo.pre.SizeInBytes < allocationInfo.post.SizeInBytes) {
    LOG_ERROR << "Portability - incompatible size for resource: O" << resourceKey;
  }
  GITS_ASSERT(allocationInfo.pre.SizeInBytes >= allocationInfo.post.SizeInBytes);
}

void ResourcePlacementAssertions::loadResourcePlacementData() {
  std::filesystem::path filePath =
      Configurator::Get().common.player.streamDir / "resourcePlacementData.dat";

  if (!std::filesystem::exists(filePath)) {
    LOG_INFO << "Portability - resourcePlacementData.dat not found, "
                "capture-time allocation data unavailable for assertions";
    return;
  }

  std::ifstream file(filePath, std::ios::binary);
  if (!file.is_open()) {
    LOG_ERROR << "Portability - failed to open resourcePlacementData.dat";
    return;
  }

  ResourcePlacementInfo info{};
  while (file.read(reinterpret_cast<char*>(&info), sizeof(ResourcePlacementInfo))) {
    m_PlacementDataFromFile[info.key] = info;
  }

  m_PlacementDataLoaded = true;

  LOG_INFO << "Portability - loaded " << m_PlacementDataFromFile.size()
           << " resource placement entries from resourcePlacementData.dat";
}

} // namespace DirectX
} // namespace gits

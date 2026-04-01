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
  const ResourcePlacementInfo* placementInfo = findPlacementData(resourceKey);
  if (placementInfo && device) {
    AllocationInfo info{};
    info.pre = {placementInfo->size, placementInfo->alignment};
    AllocationInfo deviceInfo = queryAllocationFromDevice(device, desc, resourceKey);
    info.post = deviceInfo.post;
    checkCompatibility(info, resourceKey);
    return;
  }
  static bool logged = false;
  if (!logged) {
    LOG_ERROR << "Portability - no resource placement data for resource: O" << resourceKey;
    logged = true;
  }
}

void ResourcePlacementAssertions::createPlacedResource(unsigned resourceKey,
                                                       const D3D12_RESOURCE_DESC1& desc,
                                                       ID3D12Device* device) {
  const ResourcePlacementInfo* placementInfo = findPlacementData(resourceKey);
  if (placementInfo && device) {
    D3D12_RESOURCE_DESC baseDesc{
        desc.Dimension, desc.Alignment, desc.Width,      desc.Height, desc.DepthOrArraySize,
        desc.MipLevels, desc.Format,    desc.SampleDesc, desc.Layout, desc.Flags};
    AllocationInfo info{};
    info.pre = {placementInfo->size, placementInfo->alignment};
    AllocationInfo deviceInfo = queryAllocationFromDevice(device, baseDesc, resourceKey);
    info.post = deviceInfo.post;
    checkCompatibility(info, resourceKey);
    return;
  }
  static bool logged = false;
  if (!logged) {
    LOG_ERROR << "Portability - no resource placement data for resource: O" << resourceKey;
    logged = true;
  }
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
  std::lock_guard<std::mutex> lock(mutex_);
  if (placementDataLoaded_) {
    return;
  }
  placementDataLoaded_ = true;

  std::filesystem::path filePath = Configurator::Get().common.player.streamDir;
  filePath /= "resourcePlacementData.dat";

  if (!std::filesystem::exists(filePath)) {
    LOG_INFO << "Portability - resourcePlacementData.dat not found, "
                "capture-time allocation data unavailable for assertions";
    return;
  }

  std::ifstream file(filePath, std::ios::binary);
  if (!file.is_open()) {
    LOG_WARNING << "Portability - failed to open resourcePlacementData.dat";
    return;
  }

  ResourcePlacementInfo info{};
  while (file.read(reinterpret_cast<char*>(&info), sizeof(ResourcePlacementInfo))) {
    placementDataFromFile_[info.key] = info;
  }

  LOG_INFO << "Portability - loaded " << placementDataFromFile_.size()
           << " resource placement entries from resourcePlacementData.dat";
}

const ResourcePlacementInfo* ResourcePlacementAssertions::findPlacementData(unsigned resourceKey) {
  const auto it = placementDataFromFile_.find(resourceKey);
  if (it != placementDataFromFile_.end()) {
    return &it->second;
  }
  return nullptr;
}

ResourcePlacementAssertions::AllocationInfo ResourcePlacementAssertions::queryAllocationFromDevice(
    ID3D12Device* device, const D3D12_RESOURCE_DESC& desc, unsigned resourceKey) {
  AllocationInfo info{};
  D3D12_RESOURCE_ALLOCATION_INFO allocInfo = device->GetResourceAllocationInfo(0, 1, &desc);
  if (allocInfo.SizeInBytes == UINT64_MAX) {
    LOG_ERROR << "Portability - GetResourceAllocationInfo failed for resource: O" << resourceKey;
    return info;
  }
  info.post = allocInfo;
  info.pre = allocInfo;
  return info;
}

} // namespace DirectX
} // namespace gits

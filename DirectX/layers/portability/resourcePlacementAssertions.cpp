// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "resourcePlacementAssertions.h"
#include "gits.h"

namespace gits {
namespace DirectX {

void ResourcePlacementAssertions::createPlacedResource(unsigned resourceKey,
                                                       D3D12_RESOURCE_DESC& desc) {
  const auto allocationIt = resourceDescToAllocation_.find(desc);
  if (allocationIt != resourceDescToAllocation_.end()) {
    checkCompatibility(allocationIt->second, resourceKey);
  }
}

void ResourcePlacementAssertions::createPlacedResource(unsigned resourceKey,
                                                       D3D12_RESOURCE_DESC1& desc) {
  const auto allocationIt = resourceDesc1ToAllocation_.find(desc);
  if (allocationIt != resourceDesc1ToAllocation_.end()) {
    checkCompatibility(allocationIt->second, resourceKey);
  }
}

void ResourcePlacementAssertions::getResourceAllocationPre(const D3D12_RESOURCE_DESC& desc,
                                                           uint64_t sizeInBytes,
                                                           uint64_t alignment) {
  resourceDescToAllocation_[desc].pre = {sizeInBytes, alignment};
}

void ResourcePlacementAssertions::getResourceAllocationPost(const D3D12_RESOURCE_DESC& desc,
                                                            uint64_t sizeInBytes,
                                                            uint64_t alignment) {
  resourceDescToAllocation_[desc].post = {sizeInBytes, alignment};
}

void ResourcePlacementAssertions::getResourceAllocationPre(const D3D12_RESOURCE_DESC1& desc,
                                                           uint64_t sizeInBytes,
                                                           uint64_t alignment) {
  resourceDesc1ToAllocation_[desc].pre = {sizeInBytes, alignment};
}

void ResourcePlacementAssertions::getResourceAllocationPost(const D3D12_RESOURCE_DESC1& desc,
                                                            uint64_t sizeInBytes,
                                                            uint64_t alignment) {
  resourceDesc1ToAllocation_[desc].post = {sizeInBytes, alignment};
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

} // namespace DirectX
} // namespace gits

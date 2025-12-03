// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <d3d12.h>
#include <vector>
#include <unordered_set>
#include <unordered_map>

namespace gits {
namespace DirectX {

class ResourcePlacementAssertions {
public:
  void createPlacedResource(unsigned resourceKey, D3D12_RESOURCE_DESC& desc);
  void createPlacedResource(unsigned resourceKey, D3D12_RESOURCE_DESC1& desc);
  void getResourceAllocationPre(const D3D12_RESOURCE_DESC& desc,
                                uint64_t sizeInBytes,
                                uint64_t alignment);
  void getResourceAllocationPost(const D3D12_RESOURCE_DESC& desc,
                                 uint64_t sizeInBytes,
                                 uint64_t alignment);
  void getResourceAllocationPre(const D3D12_RESOURCE_DESC1& desc,
                                uint64_t sizeInBytes,
                                uint64_t alignment);
  void getResourceAllocationPost(const D3D12_RESOURCE_DESC1& desc,
                                 uint64_t sizeInBytes,
                                 uint64_t alignment);

private:
  struct D3D12_RESOURCE_DESC_Equal {
    bool operator()(const D3D12_RESOURCE_DESC& a, const D3D12_RESOURCE_DESC& b) const noexcept {
      return a.Dimension == b.Dimension && a.Alignment == b.Alignment && a.Width == b.Width &&
             a.Height == b.Height && a.DepthOrArraySize == b.DepthOrArraySize &&
             a.MipLevels == b.MipLevels && a.Format == b.Format &&
             a.SampleDesc.Count == b.SampleDesc.Count &&
             a.SampleDesc.Quality == b.SampleDesc.Quality && a.Layout == b.Layout &&
             a.Flags == b.Flags;
    }
  };

  struct D3D12_RESOURCE_DESC_Hash {
    size_t operator()(const D3D12_RESOURCE_DESC& d) const noexcept {
      auto h = std::hash<uint64_t>{}(static_cast<uint64_t>(d.Dimension));

      auto hash_combine = [&](auto&& v) {
        constexpr uint64_t goldenRatio = 0x9e3779b97f4a7c15ULL;
        h ^= std::hash<std::decay_t<decltype(v)>>{}(v) + goldenRatio + (h << 6) + (h >> 2);
      };

      hash_combine(d.Alignment);
      hash_combine(d.Width);
      hash_combine(d.Height);
      hash_combine(d.DepthOrArraySize);
      hash_combine(d.MipLevels);
      hash_combine(static_cast<uint32_t>(d.Format));
      hash_combine(d.SampleDesc.Count);
      hash_combine(d.SampleDesc.Quality);
      hash_combine(static_cast<uint32_t>(d.Layout));
      hash_combine(static_cast<uint32_t>(d.Flags));

      return h;
    }
  };

  struct D3D12_RESOURCE_DESC1_Equal {
    bool operator()(const D3D12_RESOURCE_DESC1& a, const D3D12_RESOURCE_DESC1& b) const noexcept {
      return a.Dimension == b.Dimension && a.Alignment == b.Alignment && a.Width == b.Width &&
             a.Height == b.Height && a.DepthOrArraySize == b.DepthOrArraySize &&
             a.MipLevels == b.MipLevels && a.Format == b.Format &&
             a.SampleDesc.Count == b.SampleDesc.Count &&
             a.SampleDesc.Quality == b.SampleDesc.Quality && a.Layout == b.Layout &&
             a.Flags == b.Flags &&
             a.SamplerFeedbackMipRegion.Width == b.SamplerFeedbackMipRegion.Width &&
             a.SamplerFeedbackMipRegion.Height == b.SamplerFeedbackMipRegion.Height &&
             a.SamplerFeedbackMipRegion.Depth == b.SamplerFeedbackMipRegion.Depth;
    }
  };

  struct D3D12_RESOURCE_DESC1_Hash {
    size_t operator()(const D3D12_RESOURCE_DESC1& d) const noexcept {
      auto h = std::hash<uint64_t>{}(static_cast<uint64_t>(d.Dimension));

      auto hash_combine = [&](auto&& v) {
        constexpr uint64_t goldenRatio = 0x9e3779b97f4a7c15ULL;
        h ^= std::hash<std::decay_t<decltype(v)>>{}(v) + goldenRatio + (h << 6) + (h >> 2);
      };

      hash_combine(d.Alignment);
      hash_combine(d.Width);
      hash_combine(d.Height);
      hash_combine(d.DepthOrArraySize);
      hash_combine(d.MipLevels);
      hash_combine(static_cast<uint32_t>(d.Format));
      hash_combine(d.SampleDesc.Count);
      hash_combine(d.SampleDesc.Quality);
      hash_combine(static_cast<uint32_t>(d.Layout));
      hash_combine(static_cast<uint32_t>(d.Flags));
      hash_combine(d.SamplerFeedbackMipRegion.Width);
      hash_combine(d.SamplerFeedbackMipRegion.Height);
      hash_combine(d.SamplerFeedbackMipRegion.Depth);

      return h;
    }
  };

  struct AllocationInfo {
    D3D12_RESOURCE_ALLOCATION_INFO pre{};
    D3D12_RESOURCE_ALLOCATION_INFO post{};
  };

  void checkCompatibility(const AllocationInfo& allocationInfo, unsigned resourceKey);

  std::unordered_set<unsigned> placedResources_;
  std::unordered_map<D3D12_RESOURCE_DESC,
                     AllocationInfo,
                     D3D12_RESOURCE_DESC_Hash,
                     D3D12_RESOURCE_DESC_Equal>
      resourceDescToAllocation_;
  std::unordered_map<D3D12_RESOURCE_DESC1,
                     AllocationInfo,
                     D3D12_RESOURCE_DESC1_Hash,
                     D3D12_RESOURCE_DESC1_Equal>
      resourceDesc1ToAllocation_;
};

} // namespace DirectX
} // namespace gits

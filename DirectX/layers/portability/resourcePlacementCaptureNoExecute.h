// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <d3d12.h>
#include <vector>
#include <unordered_set>
#include <map>
#include <unordered_map>

namespace gits {
namespace DirectX {

class ResourcePlacementCaptureNoExecute {
public:
  void createPlacedResource(unsigned heapKey,
                            unsigned resourceKey,
                            UINT64 offset,
                            ID3D12Device* device,
                            D3D12_RESOURCE_DESC& desc);
  void getResourceAllocation(const D3D12_RESOURCE_DESC& desc,
                             uint64_t sizeInBytes,
                             uint64_t alignment);
  void storeResourcePlacement();

private:
  struct ResourcePlacementInfo {
    unsigned heapKey{};
    unsigned key{};
    UINT64 offset{};
    UINT64 size{};
    UINT64 alignment{};
    D3D12_RESOURCE_DESC desc{};
  };

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

  std::map<unsigned, ResourcePlacementInfo> resourcePlacementInfos_;
  std::unordered_set<unsigned> placedResources_;
  std::unordered_map<D3D12_RESOURCE_DESC,
                     D3D12_RESOURCE_ALLOCATION_INFO,
                     D3D12_RESOURCE_DESC_Hash,
                     D3D12_RESOURCE_DESC_Equal>
      resourceDescToAllocation_;
};

} // namespace DirectX
} // namespace gits

// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "resourceSizeUtils.h"
#include "log.h"

#include <wrl/client.h>
#include <unordered_map>

namespace gits {
namespace DirectX {

void GetCopyableFootprintsSafe(ID3D12Device* device,
                               const D3D12_RESOURCE_DESC* pResourceDesc,
                               UINT firstSubresource,
                               UINT numSubresources,
                               UINT64 BaseOffset,
                               D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
                               UINT* pNumRows,
                               UINT64* pRowSizeInBytes,
                               UINT64* pTotalBytes) {
  device->GetCopyableFootprints(pResourceDesc, firstSubresource, numSubresources, BaseOffset,
                                pLayouts, pNumRows, pRowSizeInBytes, pTotalBytes);
  if (pTotalBytes && *pTotalBytes == UINT64_MAX) {
    if (!(pResourceDesc->Flags & D3D12_RESOURCE_FLAG_USE_TIGHT_ALIGNMENT)) {
      static bool logged = false;
      if (!logged) {
        LOG_ERROR << "GetCopyableFootprints failed for not tight aligned resource or resource flag "
                     "is missing";
        logged = true;
      }
    }
    D3D12_RESOURCE_DESC descCopy = *pResourceDesc;
    descCopy.Alignment = 0;
    device->GetCopyableFootprints(&descCopy, firstSubresource, numSubresources, BaseOffset,
                                  pLayouts, pNumRows, pRowSizeInBytes, pTotalBytes);
    if (*pTotalBytes == UINT64_MAX &&
        (descCopy.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)) {
      static bool logged = false;
      if (!logged) {
        LOG_WARNING << "Retrying GetCopyableFootprints after removing "
                       "D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS. This can happen "
                       "when high-interface resource creation (e.g. CreateCommittedResource3 with "
                       "castable formats) creates a resource whose primary format is incompatible "
                       "with unordered access while the UAV flag is set because a castable format "
                       "is compatible.";
        logged = true;
      }
      descCopy.Flags &= ~D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
      device->GetCopyableFootprints(&descCopy, firstSubresource, numSubresources, BaseOffset,
                                    pLayouts, pNumRows, pRowSizeInBytes, pTotalBytes);
    }
    GITS_ASSERT(*pTotalBytes != UINT64_MAX);
  }
}

unsigned GetSubresourcesCount(ID3D12Resource* resource) {
  static std::unordered_map<DXGI_FORMAT, unsigned> planesByFormat;

  D3D12_RESOURCE_DESC desc = resource->GetDesc();
  unsigned planes = 1;
  if (desc.Format != DXGI_FORMAT_UNKNOWN) {
    auto it = planesByFormat.find(desc.Format);
    if (it == planesByFormat.end()) {
      Microsoft::WRL::ComPtr<ID3D12Device> device;
      HRESULT hr = resource->GetDevice(IID_PPV_ARGS(&device));
      GITS_ASSERT(hr == S_OK);
      D3D12_FEATURE_DATA_FORMAT_INFO formatInfo = {desc.Format, 0};
      if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_INFO, &formatInfo,
                                                sizeof(formatInfo)))) {
        planes = formatInfo.PlaneCount;
        planesByFormat[desc.Format] = planes;
      }
    } else {
      planes = it->second;
    }
  }
  unsigned subresources = desc.MipLevels * planes;
  if (desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D ||
      desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D) {
    subresources *= desc.DepthOrArraySize;
  }
  return subresources;
}

} // namespace DirectX
} // namespace gits

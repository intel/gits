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

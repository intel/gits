// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "resourceSizeUtils.h"

#include "log.h"

namespace gits {
namespace DirectX {

void GetCopyableFootprintsSafe(ID3D12Device* device,
                               const D3D12_RESOURCE_DESC* pResourceDesc,
                               UINT FirstSubresource,
                               UINT NumSubresources,
                               UINT64 BaseOffset,
                               D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
                               UINT* pNumRows,
                               UINT64* pRowSizeInBytes,
                               UINT64* pTotalBytes) {
  device->GetCopyableFootprints(pResourceDesc, FirstSubresource, NumSubresources, BaseOffset,
                                pLayouts, pNumRows, pRowSizeInBytes, pTotalBytes);
  if (*pTotalBytes == UINT64_MAX) {
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
    device->GetCopyableFootprints(&descCopy, FirstSubresource, NumSubresources, BaseOffset,
                                  pLayouts, pNumRows, pRowSizeInBytes, pTotalBytes);
  }
  GITS_ASSERT(*pTotalBytes != UINT64_MAX);
}

} // namespace DirectX
} // namespace gits

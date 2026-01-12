// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "directx.h"

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
                               UINT64* pTotalBytes);

} // namespace DirectX
} // namespace gits

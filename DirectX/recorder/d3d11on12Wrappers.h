// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

namespace gits {
namespace DirectX {

HRESULT D3D11On12CreateDeviceWrapper(IUnknown* pDevice,
                                     UINT Flags,
                                     const D3D_FEATURE_LEVEL* pFeatureLevels,
                                     UINT FeatureLevels,
                                     IUnknown* const* ppCommandQueues,
                                     UINT NumQueues,
                                     UINT NodeMask,
                                     ID3D11Device** ppDevice,
                                     ID3D11DeviceContext** ppImmediateContext,
                                     D3D_FEATURE_LEVEL* pChosenFeatureLevel);

} // namespace DirectX
} // namespace gits

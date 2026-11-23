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

// Preview interfaces as declared by Agility SDK 1.717 preview. Applications built against that SDK
// use these IIDs and vtable layouts, which differ from the preview interfaces of the same name in
// the Agility SDK GITS is built with.

struct DECLSPEC_UUID("536d9bb6-9eee-4c75-86e8-e29e29e08ed3")
    DECLSPEC_NOVTABLE ID3D12GraphicsCommandListPreviewSdk717 : public ID3D12GraphicsCommandList10 {
public:
  virtual void STDMETHODCALLTYPE SetWorkGraphMaximumGPUInputRecords(UINT MaxRecords,
                                                                    UINT MaxNodeInputs) = 0;

  virtual void STDMETHODCALLTYPE ConvertLinearAlgebraMatrix(
      const D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_INFO* pDesc, UINT DescCount) = 0;
};

struct DECLSPEC_UUID("55ea41d3-6bf5-4332-bbf9-905e6b4e2930")
    DECLSPEC_NOVTABLE ID3D12DevicePreviewSdk717 : public IUnknown {
public:
  virtual void STDMETHODCALLTYPE GetLinearAlgebraMatrixConversionDestinationInfo(
      D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_DEST_INFO* pDesc) = 0;
};

} // namespace DirectX
} // namespace gits

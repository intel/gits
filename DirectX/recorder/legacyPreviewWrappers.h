// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "wrappersAuto.h"
#include "legacyPreviewInterfaces.h"

#include <functional>
#include <unordered_map>

namespace gits {
namespace DirectX {

// Virtual methods must stay in the declaration order of ID3D12GraphicsCommandListPreviewSdk717,
// the application calls them through its vtable.
class ID3D12GraphicsCommandListPreviewSdk717Wrapper : public ID3D12GraphicsCommandList10Wrapper {
public:
  ID3D12GraphicsCommandListPreviewSdk717Wrapper(REFIID riid, IUnknown* object)
      : ID3D12GraphicsCommandList10Wrapper(riid, object) {
    InsertIID(__uuidof(ID3D12GraphicsCommandListPreviewSdk717));
  }

  virtual void SetWorkGraphMaximumGPUInputRecords(UINT MaxRecords, UINT MaxNodeInputs);

  virtual void ConvertLinearAlgebraMatrix(const D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_INFO* pDesc,
                                          UINT DescCount);
};

class ID3D12DevicePreviewSdk717Wrapper : public IUnknownWrapper {
public:
  ID3D12DevicePreviewSdk717Wrapper(REFIID riid, IUnknown* object) : IUnknownWrapper(riid, object) {
    InsertIID(__uuidof(ID3D12DevicePreviewSdk717));
  }

  virtual void GetLinearAlgebraMatrixConversionDestinationInfo(
      D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_DEST_INFO* pDesc);
};

extern const std::unordered_map<IID, std::function<void(REFIID, void**)>, IIDHash>
    g_legacyPreviewWrapperCreatorsDispatchTable;

extern const std::unordered_map<IID, std::function<void(REFIID, void**)>, IIDHash>
    g_legacyPreviewWrapperCreatorsNoStoreDispatchTable;

} // namespace DirectX
} // namespace gits

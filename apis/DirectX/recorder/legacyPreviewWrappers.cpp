// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "legacyPreviewWrappers.h"
#include "captureManager.h"
#include "interfaceArgumentUpdaters.h"
#include "commandsAuto.h"
#include "commandsCustom.h"
#include "layerAuto.h"
#include "log.h"

#include <processthreadsapi.h>

namespace gits {
namespace DirectX {

void ID3D12GraphicsCommandListPreviewSdk717Wrapper::SetWorkGraphMaximumGPUInputRecords(
    UINT MaxRecords, UINT MaxNodeInputs) {

  auto& manager = CaptureManager::Get();
  if (auto atTopOfStack = AtTopOfStackLocal()) {

    ID3D12GraphicsCommandListPreviewSetWorkGraphMaximumGPUInputRecordsCommand command(
        GetCurrentThreadId(), reinterpret_cast<ID3D12GraphicsCommandListPreview*>(this), MaxRecords,
        MaxNodeInputs);

    UpdateInterface(command.m_Object, this);

    for (Layer* layer : manager.GetPreLayers()) {
      layer->Pre(command);
    }

    command.Key = manager.CreateCommandKey();
    if (!command.Skip) {
      reinterpret_cast<ID3D12GraphicsCommandListPreviewSdk717*>(command.m_Object.Value)
          ->SetWorkGraphMaximumGPUInputRecords(command.m_MaxRecords.Value,
                                               command.m_MaxNodeInputs.Value);
    }

    for (Layer* layer : manager.GetPostLayers()) {
      layer->Post(command);
    }
  } else {
    GetWrappedObject<ID3D12GraphicsCommandListPreviewSdk717>()->SetWorkGraphMaximumGPUInputRecords(
        MaxRecords, MaxNodeInputs);
  }
}

void ID3D12GraphicsCommandListPreviewSdk717Wrapper::ConvertLinearAlgebraMatrix(
    const D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_INFO* pDesc, UINT DescCount) {

  auto& manager = CaptureManager::Get();
  if (auto atTopOfStack = AtTopOfStackLocal()) {

    ID3D12GraphicsCommandListPreviewConvertLinearAlgebraMatrixCommand command(
        GetCurrentThreadId(), reinterpret_cast<ID3D12GraphicsCommandListPreview*>(this), pDesc,
        DescCount);

    UpdateInterface(command.m_Object, this);

    for (Layer* layer : manager.GetPreLayers()) {
      layer->Pre(command);
    }

    command.Key = manager.CreateCommandKey();
    if (!command.Skip) {
      reinterpret_cast<ID3D12GraphicsCommandListPreviewSdk717*>(command.m_Object.Value)
          ->ConvertLinearAlgebraMatrix(command.m_pDesc.Value, command.m_DescCount.Value);
    }

    for (Layer* layer : manager.GetPostLayers()) {
      layer->Post(command);
    }
  } else {
    GetWrappedObject<ID3D12GraphicsCommandListPreviewSdk717>()->ConvertLinearAlgebraMatrix(
        pDesc, DescCount);
  }
}

void ID3D12DevicePreviewSdk717Wrapper::GetLinearAlgebraMatrixConversionDestinationInfo(
    D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_DEST_INFO* pDesc) {

  auto& manager = CaptureManager::Get();
  if (auto atTopOfStack = AtTopOfStackLocal()) {

    ID3D12DevicePreviewGetLinearAlgebraMatrixConversionDestinationInfoCommand command(
        GetCurrentThreadId(), reinterpret_cast<ID3D12DevicePreview*>(this), pDesc);

    UpdateInterface(command.m_Object, this);

    for (Layer* layer : manager.GetPreLayers()) {
      layer->Pre(command);
    }

    command.Key = manager.CreateCommandKey();
    if (!command.Skip) {
      reinterpret_cast<ID3D12DevicePreviewSdk717*>(command.m_Object.Value)
          ->GetLinearAlgebraMatrixConversionDestinationInfo(command.m_pDesc.Value);
    }

    for (Layer* layer : manager.GetPostLayers()) {
      layer->Post(command);
    }
  } else {
    GetWrappedObject<ID3D12DevicePreviewSdk717>()->GetLinearAlgebraMatrixConversionDestinationInfo(
        pDesc);
  }
}

template <typename WrapperT>
void wrapLegacyPreview(REFIID riid, void** object) {
  IUnknownWrapper* wrapper =
      CaptureManager::Get().FindWrapper(*reinterpret_cast<IUnknown**>(object));
  if (!wrapper) {
    wrapper = new WrapperT(riid, *reinterpret_cast<IUnknown**>(object));
    CaptureManager::Get().AddWrapper(wrapper);
  }
  GITS_ASSERT(dynamic_cast<WrapperT*>(wrapper),
              "Object already has a wrapper that does not implement the requested Agility SDK "
              "1.717 preview interface");
  *object = wrapper;
}

template <typename WrapperT>
void wrapLegacyPreviewNoStore(REFIID riid, void** object) {
  *object = new WrapperT(riid, *reinterpret_cast<IUnknown**>(object));
}

const std::unordered_map<IID, std::function<void(REFIID, void**)>, IIDHash>
    g_legacyPreviewWrapperCreatorsDispatchTable{
        {__uuidof(ID3D12GraphicsCommandListPreviewSdk717),
         wrapLegacyPreview<ID3D12GraphicsCommandListPreviewSdk717Wrapper>},
        {__uuidof(ID3D12DevicePreviewSdk717), wrapLegacyPreview<ID3D12DevicePreviewSdk717Wrapper>},
    };

const std::unordered_map<IID, std::function<void(REFIID, void**)>, IIDHash>
    g_legacyPreviewWrapperCreatorsNoStoreDispatchTable{
        {__uuidof(ID3D12GraphicsCommandListPreviewSdk717),
         wrapLegacyPreviewNoStore<ID3D12GraphicsCommandListPreviewSdk717Wrapper>},
        {__uuidof(ID3D12DevicePreviewSdk717),
         wrapLegacyPreviewNoStore<ID3D12DevicePreviewSdk717Wrapper>},
    };

} // namespace DirectX
} // namespace gits

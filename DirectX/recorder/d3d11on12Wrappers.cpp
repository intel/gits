// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "captureManager.h"
#include "d3d11on12Wrappers.h"
#include "interfaceArgumentUpdaters.h"

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
                                     D3D_FEATURE_LEVEL* pChosenFeatureLevel) {
  HRESULT result{};
  auto& manager = CaptureManager::get();
  if (auto atTopOfStack = AtTopOfStackGlobal()) {
    IUnknown* pDeviceUnwrap{};
    if (pDevice) {
      IUnknownWrapper* pDeviceWrapper = reinterpret_cast<IUnknownWrapper*>(pDevice);
      pDeviceUnwrap = pDeviceWrapper->getWrappedObject<IUnknown>();
    }
    IUnknown* pCommandQueuesUnwrapped{};
    if (ppCommandQueues && *ppCommandQueues) {
      IUnknownWrapper* commandQueuesWrapper = reinterpret_cast<IUnknownWrapper*>(*ppCommandQueues);
      pCommandQueuesUnwrapped = commandQueuesWrapper->getWrappedObject<IUnknown>();
    }
    result = manager.getd3d11on12DispatchTable().D3D11On12CreateDevice(
        pDeviceUnwrap, Flags, pFeatureLevels, FeatureLevels, &pCommandQueuesUnwrapped, NumQueues,
        NodeMask, ppDevice, ppImmediateContext, pChosenFeatureLevel);
  } else {
    result = manager.getd3d11on12DispatchTable().D3D11On12CreateDevice(
        pDevice, Flags, pFeatureLevels, FeatureLevels, ppCommandQueues, NumQueues, NodeMask,
        ppDevice, ppImmediateContext, pChosenFeatureLevel);
  }

  return result;
}

} // namespace DirectX
} // namespace gits

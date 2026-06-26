// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "directx/directx.h"

namespace directx {

class DebugLayerService {
public:
  static DebugLayerService& Get();

  void Enable();
  void OnDxgiFactoryCreated(REFIID riid, void** ppFactory, UINT createFactory2Flags = 0);
  void OnDeviceCreated(ID3D12Device* device);
  void FlushRaytracingValidation(IUnknown* object);

private:
  struct DebugLayerFeatures {
    bool DxgiFactoryDebug{};
    bool D3D12DebugLayer{};
    bool GpuBasedValidation{};
    bool NvApiRaytracingValidation{};
    bool Dred{};
  };

  DebugLayerService() = default;

  UINT CombineDxgiFactoryFlags(UINT capturedFlags) const;
  void InitNvApiValidation(ID3D12Device* device);

  bool m_Enabled{};
  DebugLayerFeatures m_Features{};
  bool m_NvApiCallbackRegistered{};
};

} // namespace directx

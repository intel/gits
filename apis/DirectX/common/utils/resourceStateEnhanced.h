// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "resourceStateTracker.h"

#include <wrl/client.h>

namespace gits {
namespace DirectX {

class ResourceStateEnhanced {
public:
  ResourceStateEnhanced(ID3D12GraphicsCommandList* commandList,
                        ID3D12Resource* resource,
                        BarrierState currentState,
                        unsigned subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
  void SetState(D3D12_RESOURCE_STATES state);
  void RevertState();

  static D3D12_BARRIER_SYNC GetSync(D3D12_RESOURCE_STATES state);
  static D3D12_BARRIER_ACCESS GetAccess(D3D12_RESOURCE_STATES state);
  static D3D12_BARRIER_LAYOUT GetLayout(D3D12_RESOURCE_STATES state);

private:
  ID3D12GraphicsCommandList* m_CommandList;
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList7> m_CommandList7;
  ID3D12Resource* m_Resource;
  unsigned m_Subresource;
  BarrierState m_CurrentState;
  D3D12_RESOURCE_STATES m_State{};
  bool m_BarrierSet{};
};

} // namespace DirectX
} // namespace gits

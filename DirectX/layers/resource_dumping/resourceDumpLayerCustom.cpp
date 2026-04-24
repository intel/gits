// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "resourceDumpLayerAuto.h"
#include "log.h"

namespace gits {
namespace DirectX {

void ResourceDumpLayer::Post(IUnknownReleaseCommand& c) {
  if (c.m_Result.Value == 0) {
    m_ResourceDumpService.DestroyResource(c.m_Object.Key);
  }
}

void ResourceDumpLayer::Post(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  m_ResourceDumpService.ExecuteCommandLists(c.Key, c.m_Object.Key, c.m_Object.Value,
                                            c.m_ppCommandLists.Value, c.m_NumCommandLists.Value);
}

void ResourceDumpLayer::Post(ID3D12CommandQueueWaitCommand& c) {
  m_ResourceDumpService.CommandQueueWait(c.Key, c.m_Object.Key, c.m_pFence.Key, c.m_Value.Value);
}

void ResourceDumpLayer::Post(ID3D12CommandQueueSignalCommand& c) {
  m_ResourceDumpService.CommandQueueSignal(c.Key, c.m_Object.Key, c.m_pFence.Key, c.m_Value.Value);
}

void ResourceDumpLayer::Post(ID3D12FenceSignalCommand& c) {
  m_ResourceDumpService.FenceSignal(c.Key, c.m_Object.Key, c.m_Value.Value);
}

void ResourceDumpLayer::Post(ID3D12DeviceCreateFenceCommand& c) {
  m_ResourceDumpService.FenceSignal(c.Key, c.m_ppFence.Key, c.m_InitialValue.Value);
}

void ResourceDumpLayer::Post(ID3D12Device3EnqueueMakeResidentCommand& c) {
  m_ResourceDumpService.FenceSignal(c.Key, c.m_pFenceToSignal.Key, c.m_FenceValueToSignal.Value);
}

void ResourceDumpLayer::Post(IDXGISwapChainGetBufferCommand& c) {
  if (c.m_Result.Value == S_OK) {
    m_ResourceDumpService.CreateResource(c.m_ppSurface.Key,
                                         static_cast<ID3D12Resource*>(*c.m_ppSurface.Value),
                                         D3D12_RESOURCE_STATE_COMMON);
  }
}

void ResourceDumpLayer::Post(ID3D12GraphicsCommandListResourceBarrierCommand& c) {
  m_ResourceDumpService.ResourceBarrier(c);
  m_ResourceDumpService.CommandListCall(c.Key, c.m_Object.Value);
}

void ResourceDumpLayer::Post(ID3D12DeviceCreateCommittedResourceCommand& c) {
  if (c.m_Result.Value == S_OK) {
    m_ResourceDumpService.CreateResource(c.m_ppvResource.Key,
                                         static_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                         c.m_InitialResourceState.Value);
  }
}

void ResourceDumpLayer::Post(ID3D12Device4CreateCommittedResource1Command& c) {
  if (c.m_Result.Value == S_OK) {
    m_ResourceDumpService.CreateResource(c.m_ppvResource.Key,
                                         static_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                         c.m_InitialResourceState.Value);
  }
}

void ResourceDumpLayer::Post(ID3D12Device8CreateCommittedResource2Command& c) {
  if (c.m_Result.Value == S_OK) {
    m_ResourceDumpService.CreateResource(c.m_ppvResource.Key,
                                         static_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                         c.m_InitialResourceState.Value);
  }
}

void ResourceDumpLayer::Post(ID3D12Device10CreateCommittedResource3Command& c) {
  LOG_ERROR << "Resource dumping: ID3D12Device10::CreateCommittedResource3 not handled";
}

void ResourceDumpLayer::Post(ID3D12DeviceCreatePlacedResourceCommand& c) {
  if (c.m_Result.Value == S_OK) {
    m_ResourceDumpService.CreateResource(c.m_ppvResource.Key,
                                         static_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                         c.m_InitialState.Value);
  }
}

void ResourceDumpLayer::Post(ID3D12Device8CreatePlacedResource1Command& c) {
  if (c.m_Result.Value == S_OK) {
    m_ResourceDumpService.CreateResource(c.m_ppvResource.Key,
                                         static_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                         c.m_InitialState.Value);
  }
}

void ResourceDumpLayer::Post(ID3D12Device10CreatePlacedResource2Command& c) {
  LOG_ERROR << "Resource dumping: ID3D12Device10::CreatePlacedResource2 not handled";
}

void ResourceDumpLayer::Post(ID3D12DeviceCreateReservedResourceCommand& c) {
  if (c.m_Result.Value == S_OK) {
    m_ResourceDumpService.CreateResource(c.m_ppvResource.Key,
                                         static_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                         c.m_InitialState.Value);
  }
}

void ResourceDumpLayer::Post(ID3D12Device4CreateReservedResource1Command& c) {
  if (c.m_Result.Value == S_OK) {
    m_ResourceDumpService.CreateResource(c.m_ppvResource.Key,
                                         static_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                         c.m_InitialState.Value);
  }
}

void ResourceDumpLayer::Post(ID3D12Device10CreateReservedResource2Command& c) {
  LOG_ERROR << "Resource dumping: ID3D12Device10::CreateReservedResource2 not handled";
}

} // namespace DirectX
} // namespace gits

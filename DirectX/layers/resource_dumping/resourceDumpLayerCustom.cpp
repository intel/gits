// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "resourceDumpLayerAuto.h"
#include "log.h"

namespace gits {
namespace DirectX {

void ResourceDumpLayer::post(IUnknownReleaseCommand& c) {
  if (c.result_.value == 0) {
    resourceDumpService_.destroyResource(c.object_.key);
  }
}

void ResourceDumpLayer::post(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  resourceDumpService_.executeCommandLists(c.key, c.object_.key, c.object_.value,
                                           c.ppCommandLists_.value, c.NumCommandLists_.value);
}

void ResourceDumpLayer::post(ID3D12CommandQueueWaitCommand& c) {
  resourceDumpService_.commandQueueWait(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
}

void ResourceDumpLayer::post(ID3D12CommandQueueSignalCommand& c) {
  resourceDumpService_.commandQueueSignal(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
}

void ResourceDumpLayer::post(ID3D12FenceSignalCommand& c) {
  resourceDumpService_.fenceSignal(c.key, c.object_.key, c.Value_.value);
}

void ResourceDumpLayer::post(ID3D12DeviceCreateFenceCommand& c) {
  resourceDumpService_.fenceSignal(c.key, c.ppFence_.key, c.InitialValue_.value);
}

void ResourceDumpLayer::post(ID3D12Device3EnqueueMakeResidentCommand& c) {
  resourceDumpService_.fenceSignal(c.key, c.pFenceToSignal_.key, c.FenceValueToSignal_.value);
}

void ResourceDumpLayer::post(IDXGISwapChainGetBufferCommand& c) {
  if (c.result_.value == S_OK) {
    resourceDumpService_.createResource(c.ppSurface_.key,
                                        static_cast<ID3D12Resource*>(*c.ppSurface_.value),
                                        D3D12_RESOURCE_STATE_COMMON);
  }
}

void ResourceDumpLayer::post(ID3D12GraphicsCommandListResourceBarrierCommand& c) {
  resourceDumpService_.resourceBarrier(c);
  resourceDumpService_.commandListCall(c.key, c.object_.value);
}

void ResourceDumpLayer::post(ID3D12DeviceCreateCommittedResourceCommand& c) {
  if (c.result_.value == S_OK) {
    resourceDumpService_.createResource(c.ppvResource_.key,
                                        static_cast<ID3D12Resource*>(*c.ppvResource_.value),
                                        c.InitialResourceState_.value);
  }
}

void ResourceDumpLayer::post(ID3D12Device4CreateCommittedResource1Command& c) {
  if (c.result_.value == S_OK) {
    resourceDumpService_.createResource(c.ppvResource_.key,
                                        static_cast<ID3D12Resource*>(*c.ppvResource_.value),
                                        c.InitialResourceState_.value);
  }
}

void ResourceDumpLayer::post(ID3D12Device8CreateCommittedResource2Command& c) {
  if (c.result_.value == S_OK) {
    resourceDumpService_.createResource(c.ppvResource_.key,
                                        static_cast<ID3D12Resource*>(*c.ppvResource_.value),
                                        c.InitialResourceState_.value);
  }
}

void ResourceDumpLayer::post(ID3D12Device10CreateCommittedResource3Command& c) {
  Log(ERR) << "Resource dumping: ID3D12Device10::CreateCommittedResource3 not handled";
}

void ResourceDumpLayer::post(ID3D12DeviceCreatePlacedResourceCommand& c) {
  if (c.result_.value == S_OK) {
    resourceDumpService_.createResource(c.ppvResource_.key,
                                        static_cast<ID3D12Resource*>(*c.ppvResource_.value),
                                        c.InitialState_.value);
  }
}

void ResourceDumpLayer::post(ID3D12Device8CreatePlacedResource1Command& c) {
  if (c.result_.value == S_OK) {
    resourceDumpService_.createResource(c.ppvResource_.key,
                                        static_cast<ID3D12Resource*>(*c.ppvResource_.value),
                                        c.InitialState_.value);
  }
}

void ResourceDumpLayer::post(ID3D12Device10CreatePlacedResource2Command& c) {
  Log(ERR) << "Resource dumping: ID3D12Device10::CreatePlacedResource2 not handled";
}

void ResourceDumpLayer::post(ID3D12DeviceCreateReservedResourceCommand& c) {
  if (c.result_.value == S_OK) {
    resourceDumpService_.createResource(c.ppvResource_.key,
                                        static_cast<ID3D12Resource*>(*c.ppvResource_.value),
                                        c.InitialState_.value);
  }
}

void ResourceDumpLayer::post(ID3D12Device4CreateReservedResource1Command& c) {
  if (c.result_.value == S_OK) {
    resourceDumpService_.createResource(c.ppvResource_.key,
                                        static_cast<ID3D12Resource*>(*c.ppvResource_.value),
                                        c.InitialState_.value);
  }
}

void ResourceDumpLayer::post(ID3D12Device10CreateReservedResource2Command& c) {
  Log(ERR) << "Resource dumping: ID3D12Device10::CreateReservedResource2 not handled";
}

} // namespace DirectX
} // namespace gits

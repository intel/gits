// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "analyzerLayerAuto.h"
#include "config.h"
#include "gits.h"
#include "configurationLib.h"

#include <fstream>
#include <sstream>

namespace gits {
namespace DirectX {

void AnalyzerLayer::notifyObject(unsigned objectKey) {
  analyzerService_.notifyObject(objectKey);
}

void AnalyzerLayer::post(IDXGISwapChainPresentCommand& c) {
  if (c.Flags_.value & DXGI_PRESENT_TEST) {
    return;
  }
  analyzerService_.present(c.key, c.object_.key);
}

void AnalyzerLayer::post(IDXGISwapChain1Present1Command& c) {
  if (c.PresentFlags_.value & DXGI_PRESENT_TEST) {
    return;
  }
  analyzerService_.present(c.key, c.object_.key);
}

void AnalyzerLayer::post(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  analyzerService_.executeCommandLists(c.key, c.object_.key, c.ppCommandLists_.keys);
  raytracingService_.executeCommandLists(c.key, c.object_.key, c.object_.value,
                                         c.ppCommandLists_.value, c.NumCommandLists_.value);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListResetCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  analyzerService_.executionStart();
  subcaptureRange_.executionStart();
  analyzerService_.commandListReset(c.object_.key, c.pAllocator_.key, c.pInitialState_.key);
}

void AnalyzerLayer::post(ID3D12FenceGetCompletedValueCommand& c) {
  subcaptureRange_.executionEnd();
  analyzerService_.executionEnd();
}

void AnalyzerLayer::post(ID3D12CommandQueueWaitCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  analyzerService_.commandQueueWait(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
  raytracingService_.commandQueueWait(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
}

void AnalyzerLayer::post(ID3D12CommandQueueSignalCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  analyzerService_.commandQueueSignal(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
  raytracingService_.commandQueueSignal(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
}

void AnalyzerLayer::post(ID3D12FenceSignalCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  analyzerService_.fenceSignal(c.key, c.object_.key, c.Value_.value);
  raytracingService_.fenceSignal(c.key, c.object_.key, c.Value_.value);
}

void AnalyzerLayer::post(ID3D12DeviceCreateFenceCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  analyzerService_.fenceSignal(c.key, c.ppFence_.key, c.InitialValue_.value);
}

void AnalyzerLayer::post(ID3D12Device3EnqueueMakeResidentCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  analyzerService_.fenceSignal(c.key, c.pFenceToSignal_.key, c.FenceValueToSignal_.value);
  raytracingService_.fenceSignal(c.key, c.pFenceToSignal_.key, c.FenceValueToSignal_.value);
}

void AnalyzerLayer::post(ID3D12CommandQueueCopyTileMappingsCommand& c) {
  analyzerService_.copyTileMappings(c.key, c.object_.key);
}

void AnalyzerLayer::post(ID3D12CommandQueueUpdateTileMappingsCommand& c) {
  analyzerService_.updateTileMappings(c.key, c.object_.key);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetDescriptorHeapsCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  bindingService_.setDescriptorHeaps(c);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetComputeRootSignatureCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  bindingService_.setRootSignature(c);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetGraphicsRootSignatureCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  bindingService_.setRootSignature(c);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  bindingService_.setRootDescriptorTable(c);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  bindingService_.setRootDescriptorTable(c);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  bindingService_.setRootConstantBufferView(c);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  bindingService_.setRootConstantBufferView(c);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  bindingService_.setRootShaderResourceView(c);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  bindingService_.setRootShaderResourceView(c);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  bindingService_.setRootUnorderedAccessView(c);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  bindingService_.setRootUnorderedAccessView(c);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListIASetIndexBufferCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  bindingService_.setIndexBuffer(c);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListIASetVertexBuffersCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  bindingService_.setVertexBuffers(c);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSOSetTargetsCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  bindingService_.setSOTargets(c);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  bindingService_.setRenderTargets(c);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListClearDepthStencilViewCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  bindingService_.clearView(c);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListClearRenderTargetViewCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  bindingService_.clearView(c);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  bindingService_.clearView(c);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  bindingService_.clearView(c);
}

void AnalyzerLayer::post(ID3D12DeviceCopyDescriptorsSimpleCommand& c) {
  descriptorService_.copyDescriptors(c);
  bindingService_.copyDescriptors(c);
}

void AnalyzerLayer::post(ID3D12DeviceCopyDescriptorsCommand& c) {
  descriptorService_.copyDescriptors(c);
  bindingService_.copyDescriptors(c);
}

void AnalyzerLayer::post(ID3D12DeviceCreateRenderTargetViewCommand& c) {
  D3D12RenderTargetViewState* state = new D3D12RenderTargetViewState();
  state->deviceKey = c.object_.key;
  state->resourceKey = c.pResource_.key;
  if (state->isDesc = c.pDesc_.value ? true : false) {
    state->desc = *c.pDesc_.value;
  }
  state->destDescriptor = c.DestDescriptor_.value;
  state->destDescriptorKey = c.DestDescriptor_.interfaceKey;
  state->destDescriptorIndex = c.DestDescriptor_.index;
  descriptorService_.storeState(state);
}

void AnalyzerLayer::post(ID3D12DeviceCreateDepthStencilViewCommand& c) {
  D3D12DepthStencilViewState* state = new D3D12DepthStencilViewState();
  state->deviceKey = c.object_.key;
  state->resourceKey = c.pResource_.key;
  if (state->isDesc = c.pDesc_.value ? true : false) {
    state->desc = *c.pDesc_.value;
  }
  state->destDescriptor = c.DestDescriptor_.value;
  state->destDescriptorKey = c.DestDescriptor_.interfaceKey;
  state->destDescriptorIndex = c.DestDescriptor_.index;
  descriptorService_.storeState(state);
}

void AnalyzerLayer::post(ID3D12DeviceCreateShaderResourceViewCommand& c) {
  D3D12ShaderResourceViewState* state = new D3D12ShaderResourceViewState();
  state->deviceKey = c.object_.key;
  state->resourceKey = c.pResource_.key;
  if (c.pDesc_.value) {
    state->isDesc = true;
    state->desc = *c.pDesc_.value;
    if (c.pDesc_.value->ViewDimension == D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE) {
      state->resourceKey = c.pDesc_.raytracingLocationKey;
      state->raytracingLocationOffset = c.pDesc_.raytracingLocationOffset;
    }
  }
  state->destDescriptor = c.DestDescriptor_.value;
  state->destDescriptorKey = c.DestDescriptor_.interfaceKey;
  state->destDescriptorIndex = c.DestDescriptor_.index;
  descriptorService_.storeState(state);
}

void AnalyzerLayer::post(ID3D12DeviceCreateUnorderedAccessViewCommand& c) {
  D3D12UnorderedAccessViewState* state = new D3D12UnorderedAccessViewState();
  state->deviceKey = c.object_.key;
  state->resourceKey = c.pResource_.key;
  state->counterResourceKey = c.pCounterResource_.key;
  if (state->isDesc = c.pDesc_.value ? true : false) {
    state->desc = *c.pDesc_.value;
  }
  state->destDescriptor = c.DestDescriptor_.value;
  state->destDescriptorKey = c.DestDescriptor_.interfaceKey;
  state->destDescriptorIndex = c.DestDescriptor_.index;
  descriptorService_.storeState(state);
}

void AnalyzerLayer::post(ID3D12DeviceCreateConstantBufferViewCommand& c) {
  D3D12ConstantBufferViewState* state = new D3D12ConstantBufferViewState();
  state->deviceKey = c.object_.key;
  if (state->isDesc = c.pDesc_.value ? true : false) {
    state->desc = *c.pDesc_.value;
  }
  state->resourceKey = c.pDesc_.bufferLocationKey;
  state->bufferLocationKey = c.pDesc_.bufferLocationKey;
  state->bufferLocationOffset = c.pDesc_.bufferLocationOffset;
  state->destDescriptor = c.DestDescriptor_.value;
  state->destDescriptorKey = c.DestDescriptor_.interfaceKey;
  state->destDescriptorIndex = c.DestDescriptor_.index;
  descriptorService_.storeState(state);
}

void AnalyzerLayer::post(ID3D12DeviceCreateSamplerCommand& c) {
  D3D12SamplerState* state = new D3D12SamplerState();
  state->deviceKey = c.object_.key;
  state->desc = *c.pDesc_.value;
  state->destDescriptor = c.DestDescriptor_.value;
  state->destDescriptorKey = c.DestDescriptor_.interfaceKey;
  state->destDescriptorIndex = c.DestDescriptor_.index;
  descriptorService_.storeState(state);
}

void AnalyzerLayer::post(ID3D12DeviceCreateRootSignatureCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  rootSignatureService_.createRootSignature(c);
}

void AnalyzerLayer::post(ID3D12Device5CreateStateObjectCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  raytracingService_.createStateObject(c);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandList4SetPipelineState1Command& c) {
  analyzerService_.commandListCommand(c.object_.key);
  bindingService_.setPipelineState(c);
}

void AnalyzerLayer::pre(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  bindingService_.buildRaytracingAccelerationStructure(c);
}

void AnalyzerLayer::pre(ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) {
  bindingService_.copyRaytracingAccelerationStructure(c);
}

void AnalyzerLayer::pre(ID3D12ResourceGetGPUVirtualAddressCommand& c) {
  raytracingService_.captureGPUVirtualAddress(c);
}

void AnalyzerLayer::post(ID3D12ResourceGetGPUVirtualAddressCommand& c) {
  raytracingService_.playerGPUVirtualAddress(c);
}

void AnalyzerLayer::post(IUnknownReleaseCommand& c) {
  if (c.result_.value == 0) {
    raytracingService_.getGpuAddressService().destroyInterface(c.object_.key);
  }
}

void AnalyzerLayer::post(ID3D12DeviceCreateCommittedResourceCommand& c) {
  if (c.InitialResourceState_.value == D3D12_RESOURCE_STATE_GENERIC_READ) {
    raytracingService_.genericReadResource(c.ppvResource_.key);
  }
}

void AnalyzerLayer::post(ID3D12Device4CreateCommittedResource1Command& c) {
  if (c.InitialResourceState_.value == D3D12_RESOURCE_STATE_GENERIC_READ) {
    raytracingService_.genericReadResource(c.ppvResource_.key);
  }
}

void AnalyzerLayer::post(ID3D12Device8CreateCommittedResource2Command& c) {
  if (c.InitialResourceState_.value == D3D12_RESOURCE_STATE_GENERIC_READ) {
    raytracingService_.genericReadResource(c.ppvResource_.key);
  }
}

void AnalyzerLayer::post(ID3D12Device10CreateCommittedResource3Command& c) {
  if (c.InitialLayout_.value == D3D12_BARRIER_LAYOUT_GENERIC_READ) {
    raytracingService_.genericReadResource(c.ppvResource_.key);
  }
}

void AnalyzerLayer::post(ID3D12DeviceCreatePlacedResourceCommand& c) {
  if (c.result_.value == S_OK) {
    raytracingService_.getGpuAddressService().createPlacedResource(c.pHeap_.key, c.ppvResource_.key,
                                                                   c.pDesc_.value->Flags);
    if (c.InitialState_.value == D3D12_RESOURCE_STATE_GENERIC_READ) {
      raytracingService_.genericReadResource(c.ppvResource_.key);
    }
  }
}

void AnalyzerLayer::post(ID3D12Device8CreatePlacedResource1Command& c) {
  if (c.result_.value == S_OK) {
    raytracingService_.getGpuAddressService().createPlacedResource(c.pHeap_.key, c.ppvResource_.key,
                                                                   c.pDesc_.value->Flags);
    if (c.InitialState_.value == D3D12_RESOURCE_STATE_GENERIC_READ) {
      raytracingService_.genericReadResource(c.ppvResource_.key);
    }
  }
}

void AnalyzerLayer::post(ID3D12Device10CreatePlacedResource2Command& c) {
  if (c.result_.value == S_OK) {
    raytracingService_.getGpuAddressService().createPlacedResource(c.pHeap_.key, c.ppvResource_.key,
                                                                   c.pDesc_.value->Flags);
    if (c.InitialLayout_.value == D3D12_BARRIER_LAYOUT_GENERIC_READ) {
      raytracingService_.genericReadResource(c.ppvResource_.key);
    }
  }
}

} // namespace DirectX
} // namespace gits

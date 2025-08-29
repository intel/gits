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

AnalyzerLayer::AnalyzerLayer(SubcaptureRange& subcaptureRange)
    : Layer("Analyzer"),
      subcaptureRange_(subcaptureRange),
      analyzerService_(
          subcaptureRange, bindingService_, commandListRestoreService_, raytracingService_),
      bindingService_(analyzerService_,
                      descriptorService_,
                      rootSignatureService_,
                      raytracingService_,
                      executeIndirectService_,
                      subcaptureRange_.commandListSubcapture()),
      raytracingService_(
          descriptorService_, gpuAddressService_, descriptorHandleService_, bindingService_),
      executeIndirectService_(gpuAddressService_, raytracingService_, bindingService_),
      commandListRestoreService_(analyzerService_, subcaptureRange_.commandListSubcapture()) {
  optimize_ = Configurator::Get().directx.features.subcapture.optimize;
  optimizeRaytracing_ = Configurator::Get().directx.features.subcapture.optimizeRaytracing;
}

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
  if (optimize_ || optimizeRaytracing_) {
    raytracingService_.executeCommandLists(c.key, c.object_.key, c.object_.value,
                                           c.ppCommandLists_.value, c.NumCommandLists_.value);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListResetCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  analyzerService_.executionStart();
  subcaptureRange_.executionStart();
  analyzerService_.commandListReset(c.object_.key, c.pAllocator_.key, c.pInitialState_.key);
  if (optimize_ || optimizeRaytracing_) {
    bindingService_.commandListReset(c);
  }
  if (optimize_) {
    commandListRestoreService_.commandListReset(c);
  }
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
  if (optimize_ || optimizeRaytracing_) {
    raytracingService_.commandQueueWait(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
  }
}

void AnalyzerLayer::post(ID3D12CommandQueueSignalCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  analyzerService_.commandQueueSignal(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
  if (optimize_ || optimizeRaytracing_) {
    raytracingService_.commandQueueSignal(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
  }
}

void AnalyzerLayer::post(ID3D12FenceSignalCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  analyzerService_.fenceSignal(c.key, c.object_.key, c.Value_.value);
  if (optimize_ || optimizeRaytracing_) {
    raytracingService_.fenceSignal(c.key, c.object_.key, c.Value_.value);
  }
}

void AnalyzerLayer::post(ID3D12DeviceCreateFenceCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  analyzerService_.fenceSignal(c.key, c.ppFence_.key, c.InitialValue_.value);
  if (optimize_ || optimizeRaytracing_) {
    raytracingService_.fenceSignal(c.key, c.ppFence_.key, c.InitialValue_.value);
  }
}

void AnalyzerLayer::post(ID3D12Device3EnqueueMakeResidentCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  analyzerService_.fenceSignal(c.key, c.pFenceToSignal_.key, c.FenceValueToSignal_.value);
  if (optimize_ || optimizeRaytracing_) {
    raytracingService_.fenceSignal(c.key, c.pFenceToSignal_.key, c.FenceValueToSignal_.value);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetComputeRootSignatureCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    bindingService_.setRootSignature(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetGraphicsRootSignatureCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    bindingService_.setRootSignature(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    bindingService_.setRootDescriptorTable(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    bindingService_.setRootDescriptorTable(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    bindingService_.setRootConstantBufferView(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    bindingService_.setRootConstantBufferView(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    bindingService_.setRootShaderResourceView(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    bindingService_.setRootShaderResourceView(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    bindingService_.setRootUnorderedAccessView(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    bindingService_.setRootUnorderedAccessView(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListIASetIndexBufferCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    bindingService_.setIndexBuffer(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListIASetVertexBuffersCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    bindingService_.setVertexBuffers(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSOSetTargetsCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    bindingService_.setSOTargets(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    bindingService_.setRenderTargets(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListClearDepthStencilViewCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    bindingService_.clearView(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListClearRenderTargetViewCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    bindingService_.clearView(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    bindingService_.clearView(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    bindingService_.clearView(c);
  }
}

void AnalyzerLayer::post(ID3D12DeviceCopyDescriptorsSimpleCommand& c) {
  if (optimize_) {
    descriptorService_.copyDescriptors(c);
    bindingService_.copyDescriptors(c);
  }
}

void AnalyzerLayer::post(ID3D12DeviceCopyDescriptorsCommand& c) {
  if (optimize_) {
    descriptorService_.copyDescriptors(c);
    bindingService_.copyDescriptors(c);
  }
}

void AnalyzerLayer::post(ID3D12DeviceCreateRenderTargetViewCommand& c) {
  if (optimize_) {
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
}

void AnalyzerLayer::post(ID3D12DeviceCreateDepthStencilViewCommand& c) {
  if (optimize_) {
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
}

void AnalyzerLayer::post(ID3D12DeviceCreateShaderResourceViewCommand& c) {
  if (optimize_) {
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
}

void AnalyzerLayer::post(ID3D12DeviceCreateUnorderedAccessViewCommand& c) {
  if (optimize_) {
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
}

void AnalyzerLayer::post(ID3D12DeviceCreateConstantBufferViewCommand& c) {
  if (optimize_) {
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
}

void AnalyzerLayer::post(ID3D12DeviceCreateSamplerCommand& c) {
  if (optimize_) {
    D3D12SamplerState* state = new D3D12SamplerState();
    state->deviceKey = c.object_.key;
    state->desc = *c.pDesc_.value;
    state->destDescriptor = c.DestDescriptor_.value;
    state->destDescriptorKey = c.DestDescriptor_.interfaceKey;
    state->destDescriptorIndex = c.DestDescriptor_.index;
    descriptorService_.storeState(state);
  }
}

void AnalyzerLayer::post(ID3D12DeviceCreateDescriptorHeapCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  if (optimize_) {
    bindingService_.createDescriptorHeap(c);
  }
}

void AnalyzerLayer::post(ID3D12DeviceCreateRootSignatureCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  if (optimize_) {
    rootSignatureService_.createRootSignature(c);
  }
}

void AnalyzerLayer::post(ID3D12Device5CreateStateObjectCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  if (optimize_) {
    raytracingService_.createStateObject(c);
  }
}

void AnalyzerLayer::post(ID3D12Device7AddToStateObjectCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  if (optimize_) {
    raytracingService_.addToStateObject(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandList4SetPipelineState1Command& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    bindingService_.setPipelineState(c);
  }
}

void AnalyzerLayer::pre(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (optimize_ || optimizeRaytracing_) {
    bindingService_.buildRaytracingAccelerationStructure(c);
  }
}

void AnalyzerLayer::pre(ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) {
  if (optimize_ || optimizeRaytracing_) {
    bindingService_.copyRaytracingAccelerationStructure(c);
  }
}

void AnalyzerLayer::pre(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) {
  if (optimize_ || optimizeRaytracing_) {
    bindingService_.nvapiBuildAccelerationStructureEx(c);
  }
}

void AnalyzerLayer::pre(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) {
  if (optimize_ || optimizeRaytracing_) {
    bindingService_.nvapiBuildOpacityMicromapArray(c);
  }
}

void AnalyzerLayer::pre(ID3D12GraphicsCommandList4DispatchRaysCommand& c) {
  if (optimize_) {
    bindingService_.dispatchRays(c);
  }
}

void AnalyzerLayer::post(ID3D12DeviceCreateCommandSignatureCommand& c) {
  if (optimize_) {
    executeIndirectService_.createCommandSignature(c);
  }
}

void AnalyzerLayer::pre(ID3D12GraphicsCommandListExecuteIndirectCommand& c) {
  if (optimize_) {
    bindingService_.executeIndirect(c);
  }
}

void AnalyzerLayer::pre(ID3D12ResourceGetGPUVirtualAddressCommand& c) {
  if (optimize_ || optimizeRaytracing_) {
    raytracingService_.getGPUVirtualAddress(c);
    D3D12_RESOURCE_DESC desc = c.object_.value->GetDesc();
    gpuAddressService_.addGpuCaptureAddress(c.object_.value, c.object_.key, desc.Width,
                                            c.result_.value);
  }
}

void AnalyzerLayer::post(ID3D12ResourceGetGPUVirtualAddressCommand& c) {
  if (optimize_ || optimizeRaytracing_) {
    D3D12_RESOURCE_DESC desc = c.object_.value->GetDesc();
    gpuAddressService_.addGpuPlayerAddress(c.object_.value, c.object_.key, desc.Width,
                                           c.result_.value);
  }
}

void AnalyzerLayer::pre(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) {
  if (optimize_) {
    descriptorHandleService_.addCaptureHandle(c.object_.value, c.object_.key, c.result_.value);
  }
}

void AnalyzerLayer::post(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) {
  if (optimize_) {
    descriptorHandleService_.addPlayerHandle(c.object_.key, c.result_.value);
  }
}

void AnalyzerLayer::post(IUnknownReleaseCommand& c) {
  if (optimize_ || optimizeRaytracing_) {
    if (c.result_.value == 0) {
      gpuAddressService_.destroyInterface(c.object_.key);
    }
  }
}

void AnalyzerLayer::post(ID3D12DeviceCreateCommittedResourceCommand& c) {
  if (optimize_ || optimizeRaytracing_) {
    if (c.InitialResourceState_.value == D3D12_RESOURCE_STATE_GENERIC_READ) {
      raytracingService_.genericReadResource(c.ppvResource_.key);
    }
  }
}

void AnalyzerLayer::post(ID3D12Device4CreateCommittedResource1Command& c) {
  if (optimize_ || optimizeRaytracing_) {
    if (c.InitialResourceState_.value == D3D12_RESOURCE_STATE_GENERIC_READ) {
      raytracingService_.genericReadResource(c.ppvResource_.key);
    }
  }
}

void AnalyzerLayer::post(ID3D12Device8CreateCommittedResource2Command& c) {
  if (optimize_ || optimizeRaytracing_) {
    if (c.InitialResourceState_.value == D3D12_RESOURCE_STATE_GENERIC_READ) {
      raytracingService_.genericReadResource(c.ppvResource_.key);
    }
  }
}

void AnalyzerLayer::post(ID3D12Device10CreateCommittedResource3Command& c) {
  if (optimize_ || optimizeRaytracing_) {
    if (c.InitialLayout_.value == D3D12_BARRIER_LAYOUT_GENERIC_READ) {
      raytracingService_.genericReadResource(c.ppvResource_.key);
    }
  }
}

void AnalyzerLayer::post(ID3D12DeviceCreatePlacedResourceCommand& c) {
  if (optimize_ || optimizeRaytracing_) {
    if (c.result_.value == S_OK) {
      gpuAddressService_.createPlacedResource(c.pHeap_.key, c.ppvResource_.key,
                                              c.pDesc_.value->Flags);
      if (c.InitialState_.value == D3D12_RESOURCE_STATE_GENERIC_READ) {
        raytracingService_.genericReadResource(c.ppvResource_.key);
      }
    }
  }
}

void AnalyzerLayer::post(ID3D12Device8CreatePlacedResource1Command& c) {
  if (optimize_ || optimizeRaytracing_) {
    if (c.result_.value == S_OK) {
      gpuAddressService_.createPlacedResource(c.pHeap_.key, c.ppvResource_.key,
                                              c.pDesc_.value->Flags);
      if (c.InitialState_.value == D3D12_RESOURCE_STATE_GENERIC_READ) {
        raytracingService_.genericReadResource(c.ppvResource_.key);
      }
    }
  }
}

void AnalyzerLayer::post(ID3D12Device10CreatePlacedResource2Command& c) {
  if (optimize_ || optimizeRaytracing_) {
    if (c.result_.value == S_OK) {
      gpuAddressService_.createPlacedResource(c.pHeap_.key, c.ppvResource_.key,
                                              c.pDesc_.value->Flags);
      if (c.InitialLayout_.value == D3D12_BARRIER_LAYOUT_GENERIC_READ) {
        raytracingService_.genericReadResource(c.ppvResource_.key);
      }
    }
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandList2WriteBufferImmediateCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    bindingService_.writeBufferImmediate(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListCopyBufferRegionCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListRestoreService_.copyBufferRegion(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListCopyTextureRegionCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListRestoreService_.copyTextureRegion(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListCopyResourceCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListRestoreService_.copyResource(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListCopyTilesCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListRestoreService_.copyTiles(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListResolveSubresourceCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListRestoreService_.resolveSubresource(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListDiscardResourceCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListRestoreService_.discardResource(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetPipelineStateCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListRestoreService_.setPipelineState(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListResourceBarrierCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListRestoreService_.resourceBarrier(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandList1ResolveSubresourceRegionCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListRestoreService_.resolveSubresourceRegion(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandList3SetProtectedResourceSessionCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListRestoreService_.setProtectedResourceSession(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandList4InitializeMetaCommandCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListRestoreService_.initializeMetaCommand(c);
  }
}

void AnalyzerLayer::post(MappedDataMetaCommand& c) {
  if (optimize_) {
    analyzerService_.mappedDataMeta(c.resource_.key);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandList7BarrierCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListRestoreService_.barrier(c);
  }
}

void AnalyzerLayer::post(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) {
  analyzerService_.commandListCommand(c.pCommandList_.key);
}

void AnalyzerLayer::post(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) {
  analyzerService_.commandListCommand(c.pCommandList_.key);
}

void AnalyzerLayer::post(xessD3D12CreateContextCommand& c) {
  if (optimize_) {
    analyzerService_.createXessContext(c);
  }
}

void AnalyzerLayer::post(INTC_D3D12_CreateDeviceExtensionContextCommand& c) {
  if (optimize_) {
    analyzerService_.createDeviceExtensionContext(c);
  }
}

void AnalyzerLayer::post(INTC_D3D12_CreateDeviceExtensionContext1Command& c) {
  if (optimize_) {
    analyzerService_.createDeviceExtensionContext(c);
  }
}

} // namespace DirectX
} // namespace gits

// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "analyzerLayerAuto.h"
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
          subcaptureRange, commandListService_, raytracingService_, executeIndirectService_),
      commandListService_(analyzerService_,
                          descriptorService_,
                          rootSignatureService_,
                          raytracingService_,
                          executeIndirectService_,
                          subcaptureRange_.commandListSubcapture()),
      raytracingService_(descriptorService_,
                         gpuAddressService_,
                         descriptorHandleService_,
                         shaderIdentifierService_,
                         commandListService_,
                         rootSignatureService_,
                         resourceStateTracker_),
      executeIndirectService_(gpuAddressService_, raytracingService_, commandListService_) {
  optimize_ = Configurator::Get().directx.features.subcapture.optimize;
  optimizeRaytracing_ = Configurator::Get().directx.features.subcapture.optimizeRaytracing;
  if (optimize_) {
    shaderIdentifierService_.enablePlayerIdentifierLookup();
  }
}

void AnalyzerLayer::post(IDXGISwapChainPresentCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  if (c.Flags_.value & DXGI_PRESENT_TEST) {
    return;
  }
  analyzerService_.present(c.key, c.object_.key);
  commandListService_.present();
}

void AnalyzerLayer::post(IDXGISwapChain1Present1Command& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  if (c.PresentFlags_.value & DXGI_PRESENT_TEST) {
    return;
  }
  analyzerService_.present(c.key, c.object_.key);
  commandListService_.present();
}

void AnalyzerLayer::post(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObjects(c.ppCommandLists_.keys);
  analyzerService_.executeCommandLists(c.key, c.object_.key, c.ppCommandLists_.keys);
  if (optimize_ || optimizeRaytracing_) {
    resourceStateTracker_.executeCommandLists(
        reinterpret_cast<ID3D12GraphicsCommandList**>(c.ppCommandLists_.value),
        c.NumCommandLists_.value);
    raytracingService_.executeCommandLists(c.key, c.object_.key, c.object_.value,
                                           c.ppCommandLists_.value, c.NumCommandLists_.value);
    executeIndirectService_.executeCommandLists(c.key, c.object_.key, c.object_.value,
                                                c.ppCommandLists_.value, c.NumCommandLists_.value);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListResetCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.pAllocator_.key);
  analyzerService_.notifyObject(c.pInitialState_.key);
  if (c.result_.value != S_OK) {
    return;
  }
  analyzerService_.executionStart();
  subcaptureRange_.executionStart();
  analyzerService_.commandListReset(c.object_.key, c.pAllocator_.key, c.pInitialState_.key);
  if (optimize_ || optimizeRaytracing_) {
    commandListService_.commandListReset(c);
  }
  if (optimize_) {
    commandListService_.commandListReset(c);
  }
}

void AnalyzerLayer::post(ID3D12FenceGetCompletedValueCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  subcaptureRange_.executionEnd();
  analyzerService_.executionEnd();
}

void AnalyzerLayer::post(ID3D12CommandQueueWaitCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.pFence_.key);
  if (c.result_.value != S_OK) {
    return;
  }
  analyzerService_.commandQueueWait(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
  if (optimize_ || optimizeRaytracing_) {
    raytracingService_.commandQueueWait(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
    executeIndirectService_.commandQueueWait(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
  }
}

void AnalyzerLayer::post(ID3D12CommandQueueSignalCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.pFence_.key);
  if (c.result_.value != S_OK) {
    return;
  }
  analyzerService_.commandQueueSignal(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
  if (optimize_ || optimizeRaytracing_) {
    raytracingService_.commandQueueSignal(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
    executeIndirectService_.commandQueueSignal(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
  }
}

void AnalyzerLayer::post(ID3D12FenceSignalCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  if (c.result_.value != S_OK) {
    return;
  }
  analyzerService_.fenceSignal(c.key, c.object_.key, c.Value_.value);
  if (optimize_ || optimizeRaytracing_) {
    raytracingService_.fenceSignal(c.key, c.object_.key, c.Value_.value);
    executeIndirectService_.fenceSignal(c.key, c.object_.key, c.Value_.value);
  }
}

void AnalyzerLayer::post(ID3D12Device3EnqueueMakeResidentCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObjects(c.ppObjects_.keys);
  analyzerService_.notifyObject(c.pFenceToSignal_.key);
  if (c.result_.value != S_OK) {
    return;
  }
  analyzerService_.fenceSignal(c.key, c.pFenceToSignal_.key, c.FenceValueToSignal_.value);
  if (optimize_ || optimizeRaytracing_) {
    raytracingService_.fenceSignal(c.key, c.pFenceToSignal_.key, c.FenceValueToSignal_.value);
    executeIndirectService_.fenceSignal(c.key, c.pFenceToSignal_.key, c.FenceValueToSignal_.value);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetComputeRootSignatureCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetGraphicsRootSignatureCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListIASetIndexBufferCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListIASetVertexBuffersCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSOSetTargetsCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListClearDepthStencilViewCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListClearRenderTargetViewCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12DeviceCopyDescriptorsSimpleCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  if (optimize_) {
    descriptorService_.copyDescriptors(c);
    commandListService_.copyDescriptors(c);
  }
}

void AnalyzerLayer::post(ID3D12DeviceCopyDescriptorsCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  if (optimize_) {
    descriptorService_.copyDescriptors(c);
    commandListService_.copyDescriptors(c);
  }
}

void AnalyzerLayer::post(ID3D12DeviceCreateRenderTargetViewCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.pResource_.key);
  analyzerService_.notifyObject(c.DestDescriptor_.interfaceKey);
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
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.pResource_.key);
  analyzerService_.notifyObject(c.DestDescriptor_.interfaceKey);
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
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.pResource_.key);
  analyzerService_.notifyObject(c.DestDescriptor_.interfaceKey);
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
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.pResource_.key);
  analyzerService_.notifyObject(c.pCounterResource_.key);
  analyzerService_.notifyObject(c.DestDescriptor_.interfaceKey);
  if (optimize_) {
    D3D12UnorderedAccessViewState* state = new D3D12UnorderedAccessViewState();
    state->deviceKey = c.object_.key;
    state->resourceKey = c.pResource_.key;
    state->auxiliaryResourceKey = c.pCounterResource_.key;
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
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.pDesc_.bufferLocationKey);
  analyzerService_.notifyObject(c.DestDescriptor_.interfaceKey);
  if (optimize_) {
    D3D12ConstantBufferViewState* state = new D3D12ConstantBufferViewState();
    state->deviceKey = c.object_.key;
    if (state->isDesc = c.pDesc_.value ? true : false) {
      state->desc = *c.pDesc_.value;
    }
    state->resourceKey = c.pDesc_.bufferLocationKey;
    state->bufferLocationOffset = c.pDesc_.bufferLocationOffset;
    state->destDescriptor = c.DestDescriptor_.value;
    state->destDescriptorKey = c.DestDescriptor_.interfaceKey;
    state->destDescriptorIndex = c.DestDescriptor_.index;
    descriptorService_.storeState(state);
  }
}

void AnalyzerLayer::post(ID3D12DeviceCreateSamplerCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.DestDescriptor_.interfaceKey);
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

void AnalyzerLayer::pre(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  CapturePlayerShaderIdentifierService::ShaderIdentifier shaderIdentifier;
  memcpy(shaderIdentifier.data(), c.result_.value, shaderIdentifier.size());
  shaderIdentifierService_.addCaptureShaderIdentifier(c.key, shaderIdentifier,
                                                      c.pExportName_.value);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandList4SetPipelineState1Command& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetDescriptorHeapsCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::pre(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  if (optimize_ || optimizeRaytracing_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::pre(ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  if (optimize_ || optimizeRaytracing_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::pre(
    ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  if (optimize_ || optimizeRaytracing_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::pre(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  if (optimize_ || optimizeRaytracing_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::pre(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  if (optimize_ || optimizeRaytracing_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::pre(ID3D12GraphicsCommandList4DispatchRaysCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::pre(ID3D12GraphicsCommandListExecuteIndirectCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::pre(ID3D12GraphicsCommandListDispatchCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::pre(ID3D12GraphicsCommandListDrawIndexedInstancedCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::pre(ID3D12GraphicsCommandListDrawInstancedCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::pre(ID3D12ResourceGetGPUVirtualAddressCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  if (optimize_ || optimizeRaytracing_) {
    raytracingService_.getGPUVirtualAddress(c);
    D3D12_RESOURCE_DESC desc = c.object_.value->GetDesc();
    gpuAddressService_.addGpuCaptureAddress(c.object_.value, c.object_.key, desc.Width,
                                            c.result_.value);
  }
}

void AnalyzerLayer::post(ID3D12ResourceGetGPUVirtualAddressCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  if (optimize_ || optimizeRaytracing_) {
    analyzerService_.notifyObject(c.object_.key);
    D3D12_RESOURCE_DESC desc = c.object_.value->GetDesc();
    gpuAddressService_.addGpuPlayerAddress(c.object_.value, c.object_.key, desc.Width,
                                           c.result_.value);
  }
}

void AnalyzerLayer::pre(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  if (optimize_) {
    descriptorHandleService_.addCaptureHandle(c.object_.value, c.object_.key, c.result_.value);
  }
}

void AnalyzerLayer::post(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  if (optimize_) {
    analyzerService_.notifyObject(c.object_.key);
    descriptorHandleService_.addPlayerHandle(c.object_.key, c.result_.value);
  }
}

void AnalyzerLayer::post(IUnknownReleaseCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  if (optimize_ || optimizeRaytracing_) {
    if (c.result_.value == 0) {
      gpuAddressService_.destroyInterface(c.object_.key);
    }
  }
}

void AnalyzerLayer::post(IUnknownQueryInterfaceCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
}

void AnalyzerLayer::post(IUnknownAddRefCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandList2WriteBufferImmediateCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListCopyBufferRegionCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListCopyTextureRegionCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListCopyResourceCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListCopyTilesCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListResolveSubresourceCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListDiscardResourceCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetPipelineStateCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListResourceBarrierCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    resourceStateTracker_.resourceBarrier(c.object_.value, c.pBarriers_.value, c.NumBarriers_.value,
                                          c.pBarriers_.resourceKeys.data());
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandList1ResolveSubresourceRegionCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandList3SetProtectedResourceSessionCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandList4InitializeMetaCommandCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandList4ExecuteMetaCommandCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(MappedDataMetaCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.resource_.key);
  if (optimize_) {
    analyzerService_.mappedDataMeta(c.resource_.key);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandList7BarrierCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.commandListCommand(c.pCommandList_.key);
}

void AnalyzerLayer::post(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.commandListCommand(c.pCommandList_.key);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListClearStateCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.commandListCommand(c.object_.key);
  commandListService_.command(c);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListExecuteBundleCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.commandListCommand(c.object_.key);
  commandListService_.command(c);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListBeginQueryCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.commandListCommand(c.object_.key);
  commandListService_.command(c);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListEndQueryCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.commandListCommand(c.object_.key);
  commandListService_.command(c);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListResolveQueryDataCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.commandListCommand(c.object_.key);
  commandListService_.command(c);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetPredicationCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.commandListCommand(c.object_.key);
  commandListService_.command(c);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandList1AtomicCopyBufferUINTCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.commandListCommand(c.object_.key);
  commandListService_.command(c);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandList1AtomicCopyBufferUINT64Command& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.commandListCommand(c.object_.key);
  commandListService_.command(c);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandList4BeginRenderPassCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.commandListCommand(c.object_.key);
  commandListService_.command(c);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandList5RSSetShadingRateImageCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.commandListCommand(c.object_.key);
  commandListService_.command(c);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandList6DispatchMeshCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.commandListCommand(c.object_.key);
  commandListService_.command(c);
}

void AnalyzerLayer::post(IDMLBindingTableResetCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.desc_.data.cpuDescHandleKey);
  analyzerService_.notifyObject(c.desc_.data.gpuDescHandleKey);
}

void AnalyzerLayer::post(IDMLBindingTableBindInputsCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObjects(c.bindings_.resourceKeys);
}

void AnalyzerLayer::post(IDMLBindingTableBindOutputsCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObjects(c.bindings_.resourceKeys);
}

void AnalyzerLayer::post(xessD3D12InitCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.hContext_.key);
  analyzerService_.notifyObject(c.pInitParams_.key);
  analyzerService_.notifyObject(c.pInitParams_.tempBufferHeapKey);
  analyzerService_.notifyObject(c.pInitParams_.tempTextureHeapKey);
  analyzerService_.notifyObject(c.pInitParams_.pipelineLibraryKey);
}

void AnalyzerLayer::post(xessD3D12GetInitParamsCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.hContext_.key);
  analyzerService_.notifyObject(c.pInitParams_.key);
  analyzerService_.notifyObject(c.pInitParams_.tempBufferHeapKey);
  analyzerService_.notifyObject(c.pInitParams_.tempTextureHeapKey);
  analyzerService_.notifyObject(c.pInitParams_.pipelineLibraryKey);
}

void AnalyzerLayer::post(xessD3D12ExecuteCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.hContext_.key);
  analyzerService_.notifyObject(c.pCommandList_.key);
  analyzerService_.notifyObject(c.pExecParams_.colorTextureKey);
  analyzerService_.notifyObject(c.pExecParams_.velocityTextureKey);
  analyzerService_.notifyObject(c.pExecParams_.depthTextureKey);
  analyzerService_.notifyObject(c.pExecParams_.exposureScaleTextureKey);
  analyzerService_.notifyObject(c.pExecParams_.responsivePixelMaskTextureKey);
  analyzerService_.notifyObject(c.pExecParams_.outputTextureKey);
  analyzerService_.notifyObject(c.pExecParams_.descriptorHeapKey);
}

void AnalyzerLayer::post(IDStorageQueueEnqueueRequestCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.request_.fileKey);
  analyzerService_.notifyObject(c.request_.resourceKey);
}

void AnalyzerLayer::post(INTC_D3D12_GetSupportedVersionsCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.pDevice_.key);
}

void AnalyzerLayer::post(INTC_D3D12_CreateDeviceExtensionContextCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.pDevice_.key);
  analyzerService_.notifyObject(c.ppExtensionContext_.key);
  analyzerService_.createDeviceExtensionContext(c);
  analyzerService_.addParent(c.ppExtensionContext_.key, c.pDevice_.key);
}

void AnalyzerLayer::post(INTC_D3D12_CreateDeviceExtensionContext1Command& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.pDevice_.key);
  analyzerService_.notifyObject(c.ppExtensionContext_.key);
  analyzerService_.createDeviceExtensionContext(c);
  analyzerService_.addParent(c.ppExtensionContext_.key, c.pDevice_.key);
}

void AnalyzerLayer::post(INTC_DestroyDeviceExtensionContextCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.ppExtensionContext_.key);
}

void AnalyzerLayer::post(INTC_D3D12_CheckFeatureSupportCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.pExtensionContext_.key);
}

void AnalyzerLayer::post(INTC_D3D12_CreateCommandQueueCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.pExtensionContext_.key);
  analyzerService_.notifyObject(c.ppCommandQueue_.key);
  analyzerService_.addParent(c.ppCommandQueue_.key, c.pExtensionContext_.key);
}

void AnalyzerLayer::post(INTC_D3D12_CreateReservedResourceCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.pExtensionContext_.key);
  analyzerService_.notifyObject(c.ppvResource_.key);
  analyzerService_.addParent(c.ppvResource_.key, c.pExtensionContext_.key);
  resourceStateTracker_.addResource(c.ppvResource_.key, c.InitialState_.value);
}

void AnalyzerLayer::post(INTC_D3D12_SetFeatureSupportCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.pExtensionContext_.key);
}

void AnalyzerLayer::post(INTC_D3D12_GetResourceAllocationInfoCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.pExtensionContext_.key);
}

void AnalyzerLayer::post(INTC_D3D12_CreateComputePipelineStateCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.pExtensionContext_.key);
  analyzerService_.notifyObject(c.ppPipelineState_.key);
  analyzerService_.notifyObject(c.pDesc_.rootSignatureKey);
  analyzerService_.addParent(c.ppPipelineState_.key, c.pExtensionContext_.key);
  analyzerService_.addParent(c.ppPipelineState_.key, c.pDesc_.rootSignatureKey);
}

void AnalyzerLayer::post(INTC_D3D12_CreatePlacedResourceCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.pExtensionContext_.key);
  analyzerService_.notifyObject(c.pHeap_.key);
  analyzerService_.notifyObject(c.ppvResource_.key);
  analyzerService_.addParent(c.ppvResource_.key, c.pExtensionContext_.key);
  analyzerService_.addParent(c.ppvResource_.key, c.pHeap_.key);
  resourceStateTracker_.addResource(c.ppvResource_.key, c.InitialState_.value);
}

void AnalyzerLayer::post(INTC_D3D12_CreateCommittedResourceCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.pExtensionContext_.key);
  analyzerService_.notifyObject(c.ppvResource_.key);
  analyzerService_.addParent(c.ppvResource_.key, c.pExtensionContext_.key);
  resourceStateTracker_.addResource(c.ppvResource_.key, c.InitialResourceState_.value);
}

void AnalyzerLayer::post(INTC_D3D12_CreateHeapCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.pExtensionContext_.key);
  analyzerService_.notifyObject(c.ppvHeap_.key);
  analyzerService_.addParent(c.ppvHeap_.key, c.pExtensionContext_.key);
}

void AnalyzerLayer::post(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.pDevice_.key);
}

void AnalyzerLayer::post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.pDev_.key);
}

void AnalyzerLayer::post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.pDev_.key);
}

void AnalyzerLayer::post(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.pCommandList_.key);
}

void AnalyzerLayer::post(D3D12CreateDeviceCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.pAdapter_.key);
  analyzerService_.notifyObject(c.ppDevice_.key);
  analyzerService_.addParent(c.ppDevice_.key, c.pAdapter_.key);
}

void AnalyzerLayer::post(DMLCreateDeviceCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.d3d12Device_.key);
  analyzerService_.notifyObject(c.ppv_.key);
  analyzerService_.addParent(c.ppv_.key, c.d3d12Device_.key);
}

void AnalyzerLayer::post(DMLCreateDevice1Command& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.d3d12Device_.key);
  analyzerService_.notifyObject(c.ppv_.key);
  analyzerService_.addParent(c.ppv_.key, c.d3d12Device_.key);
}

void AnalyzerLayer::post(xessD3D12CreateContextCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.pDevice_.key);
  analyzerService_.notifyObject(c.phContext_.key);
  analyzerService_.addParent(c.phContext_.key, c.pDevice_.key);
  if (optimize_) {
    analyzerService_.createXessContext(c);
  }
}

void AnalyzerLayer::post(ID3D12DeviceCreateCommandQueueCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppCommandQueue_.key);
  analyzerService_.addParent(c.ppCommandQueue_.key, c.object_.key);
}

void AnalyzerLayer::post(ID3D12DeviceCreateCommandAllocatorCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppCommandAllocator_.key);
  analyzerService_.addParent(c.ppCommandAllocator_.key, c.object_.key);
}

void AnalyzerLayer::post(ID3D12DeviceCreateGraphicsPipelineStateCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppPipelineState_.key);
  analyzerService_.notifyObject(c.pDesc_.rootSignatureKey);
  analyzerService_.addParent(c.ppPipelineState_.key, c.object_.key);
  analyzerService_.addParent(c.ppPipelineState_.key, c.pDesc_.rootSignatureKey);
}

void AnalyzerLayer::post(ID3D12DeviceCreateComputePipelineStateCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppPipelineState_.key);
  analyzerService_.notifyObject(c.pDesc_.rootSignatureKey);
  analyzerService_.addParent(c.ppPipelineState_.key, c.object_.key);
  analyzerService_.addParent(c.ppPipelineState_.key, c.pDesc_.rootSignatureKey);
}

void AnalyzerLayer::post(ID3D12DeviceCreateCommandListCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.pCommandAllocator_.key);
  analyzerService_.notifyObject(c.pInitialState_.key);
  analyzerService_.notifyObject(c.ppCommandList_.key);
  analyzerService_.addParent(c.ppCommandList_.key, c.object_.key);
  analyzerService_.addParent(c.ppCommandList_.key, c.pInitialState_.key);
  analyzerService_.addParent(c.ppCommandList_.key, c.pCommandAllocator_.key);
}

void AnalyzerLayer::post(ID3D12DeviceCreateDescriptorHeapCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppvHeap_.key);
  analyzerService_.addParent(c.ppvHeap_.key, c.object_.key);
  if (c.result_.value != S_OK) {
    return;
  }
  if (optimize_) {
    commandListService_.createDescriptorHeap(c);
  }
}

void AnalyzerLayer::post(ID3D12DeviceCreateRootSignatureCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppvRootSignature_.key);
  analyzerService_.addParent(c.ppvRootSignature_.key, c.object_.key);
  if (c.result_.value != S_OK) {
    return;
  }
  if (optimize_) {
    rootSignatureService_.createRootSignature(c);
  }
}

void AnalyzerLayer::post(ID3D12DeviceCreateCommittedResourceCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppvResource_.key);
  analyzerService_.addParent(c.ppvResource_.key, c.object_.key);
  resourceStateTracker_.addResource(c.ppvResource_.key, c.InitialResourceState_.value);
}

void AnalyzerLayer::post(ID3D12DeviceCreateHeapCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppvHeap_.key);
  analyzerService_.addParent(c.ppvHeap_.key, c.object_.key);
}

void AnalyzerLayer::post(ID3D12DeviceCreatePlacedResourceCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.pHeap_.key);
  analyzerService_.notifyObject(c.ppvResource_.key);
  analyzerService_.addParent(c.ppvResource_.key, c.pHeap_.key);
  if (optimize_ || optimizeRaytracing_) {
    if (c.result_.value == S_OK) {
      resourceStateTracker_.addResource(c.ppvResource_.key, c.InitialState_.value);
      gpuAddressService_.createPlacedResource(c.pHeap_.key, c.ppvResource_.key,
                                              c.pDesc_.value->Flags);
    }
  }
}

void AnalyzerLayer::post(ID3D12DeviceCreateReservedResourceCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppvResource_.key);
  analyzerService_.addParent(c.ppvResource_.key, c.object_.key);
  resourceStateTracker_.addResource(c.ppvResource_.key, c.InitialState_.value);
}

void AnalyzerLayer::post(ID3D12DeviceCreateSharedHandleCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.pObject_.key);
  analyzerService_.addParent(c.pObject_.key, c.object_.key);
}

void AnalyzerLayer::post(ID3D12DeviceOpenSharedHandleCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppvObj_.key);
  analyzerService_.addParent(c.ppvObj_.key, c.object_.key);
}

void AnalyzerLayer::post(ID3D12DeviceCreateFenceCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppFence_.key);
  analyzerService_.addParent(c.ppFence_.key, c.object_.key);
  if (c.result_.value != S_OK) {
    return;
  }
  analyzerService_.fenceSignal(c.key, c.ppFence_.key, c.InitialValue_.value);
  if (optimize_ || optimizeRaytracing_) {
    raytracingService_.fenceSignal(c.key, c.ppFence_.key, c.InitialValue_.value);
  }
}

void AnalyzerLayer::post(ID3D12DeviceCreateQueryHeapCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppvHeap_.key);
  analyzerService_.addParent(c.ppvHeap_.key, c.object_.key);
}

void AnalyzerLayer::post(ID3D12DeviceCreateCommandSignatureCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppvCommandSignature_.key);
  analyzerService_.addParent(c.ppvCommandSignature_.key, c.object_.key);
  if (c.result_.value != S_OK) {
    return;
  }
  if (optimize_) {
    executeIndirectService_.createCommandSignature(c);
    commandListService_.createCommandSignature(c);
  }
}

void AnalyzerLayer::post(ID3D12Device1CreatePipelineLibraryCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppPipelineLibrary_.key);
}

void AnalyzerLayer::post(ID3D12Device2CreatePipelineStateCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppPipelineState_.key);
  analyzerService_.notifyObject(c.pDesc_.rootSignatureKey);
  analyzerService_.addParent(c.ppPipelineState_.key, c.object_.key);
  analyzerService_.addParent(c.ppPipelineState_.key, c.pDesc_.rootSignatureKey);
}

void AnalyzerLayer::post(ID3D12Device4CreateCommandList1Command& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppCommandList_.key);
  analyzerService_.addParent(c.ppCommandList_.key, c.object_.key);
}

void AnalyzerLayer::post(ID3D12Device4CreateProtectedResourceSessionCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppSession_.key);
  analyzerService_.addParent(c.ppSession_.key, c.object_.key);
}

void AnalyzerLayer::post(ID3D12Device4CreateCommittedResource1Command& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppvResource_.key);
  analyzerService_.addParent(c.ppvResource_.key, c.object_.key);
  resourceStateTracker_.addResource(c.ppvResource_.key, c.InitialResourceState_.value);
}

void AnalyzerLayer::post(ID3D12Device4CreateHeap1Command& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.pProtectedSession_.key);
  analyzerService_.notifyObject(c.ppvHeap_.key);
  analyzerService_.addParent(c.ppvHeap_.key, c.object_.key);
  analyzerService_.addParent(c.ppvHeap_.key, c.pProtectedSession_.key);
}

void AnalyzerLayer::post(ID3D12Device4CreateReservedResource1Command& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.pProtectedSession_.key);
  analyzerService_.notifyObject(c.ppvResource_.key);
  analyzerService_.addParent(c.ppvResource_.key, c.object_.key);
  analyzerService_.addParent(c.ppvResource_.key, c.pProtectedSession_.key);
  resourceStateTracker_.addResource(c.ppvResource_.key, c.InitialState_.value);
}

void AnalyzerLayer::post(ID3D12Device5CreateLifetimeTrackerCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.pOwner_.key);
  analyzerService_.notifyObject(c.ppvTracker_.key);
  analyzerService_.addParent(c.ppvTracker_.key, c.object_.key);
  analyzerService_.addParent(c.ppvTracker_.key, c.pOwner_.key);
}

void AnalyzerLayer::post(ID3D12Device5CreateMetaCommandCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppMetaCommand_.key);
  analyzerService_.addParent(c.ppMetaCommand_.key, c.object_.key);
}

void AnalyzerLayer::post(ID3D12Device5CreateStateObjectCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppStateObject_.key);
  analyzerService_.addParent(c.ppStateObject_.key, c.object_.key);
  for (auto& it : c.pDesc_.interfaceKeysBySubobject) {
    analyzerService_.notifyObject(it.second);
  }
  if (c.result_.value != S_OK) {
    return;
  }
  if (optimize_) {
    raytracingService_.createStateObject(c);
  }
}

void AnalyzerLayer::post(ID3D12Device5GetRaytracingAccelerationStructurePrebuildInfoCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  if (optimize_) {
    for (unsigned key : c.pDesc_.inputKeys) {
      analyzerService_.notifyObject(key);
    }
  }
}

void AnalyzerLayer::post(ID3D12Device7CreateProtectedResourceSession1Command& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppSession_.key);
  analyzerService_.addParent(c.ppSession_.key, c.object_.key);
}

void AnalyzerLayer::post(ID3D12Device8CreateCommittedResource2Command& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppvResource_.key);
  analyzerService_.addParent(c.ppvResource_.key, c.object_.key);
  resourceStateTracker_.addResource(c.ppvResource_.key, c.InitialResourceState_.value);
}

void AnalyzerLayer::post(ID3D12Device8CreatePlacedResource1Command& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.pHeap_.key);
  analyzerService_.notifyObject(c.ppvResource_.key);
  analyzerService_.addParent(c.ppvResource_.key, c.pHeap_.key);
  if (optimize_ || optimizeRaytracing_) {
    if (c.result_.value == S_OK) {
      resourceStateTracker_.addResource(c.ppvResource_.key, c.InitialState_.value);
      gpuAddressService_.createPlacedResource(c.pHeap_.key, c.ppvResource_.key,
                                              c.pDesc_.value->Flags);
    }
  }
}

void AnalyzerLayer::post(ID3D12Device9CreateShaderCacheSessionCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppvSession_.key);
  analyzerService_.addParent(c.ppvSession_.key, c.object_.key);
}

void AnalyzerLayer::post(ID3D12Device9CreateCommandQueue1Command& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppCommandQueue_.key);
  analyzerService_.addParent(c.ppCommandQueue_.key, c.object_.key);
}

void AnalyzerLayer::post(ID3D12Device10CreateCommittedResource3Command& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppvResource_.key);
  analyzerService_.addParent(c.ppvResource_.key, c.object_.key);
  resourceStateTracker_.addResource(c.ppvResource_.key, c.InitialLayout_.value);
}

void AnalyzerLayer::post(ID3D12Device10CreatePlacedResource2Command& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.pHeap_.key);
  analyzerService_.notifyObject(c.ppvResource_.key);
  analyzerService_.addParent(c.pHeap_.key, c.pHeap_.key);
  if (optimize_ || optimizeRaytracing_) {
    if (c.result_.value == S_OK) {
      resourceStateTracker_.addResource(c.ppvResource_.key, c.InitialLayout_.value);
      gpuAddressService_.createPlacedResource(c.pHeap_.key, c.ppvResource_.key,
                                              c.pDesc_.value->Flags);
    }
  }
}

void AnalyzerLayer::post(ID3D12Device10CreateReservedResource2Command& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.pProtectedSession_.key);
  analyzerService_.notifyObject(c.ppvResource_.key);
  analyzerService_.addParent(c.ppvResource_.key, c.object_.key);
  analyzerService_.addParent(c.ppvResource_.key, c.pProtectedSession_.key);
  resourceStateTracker_.addResource(c.ppvResource_.key, c.InitialLayout_.value);
}

void AnalyzerLayer::post(ID3D12Device14CreateRootSignatureFromSubobjectInLibraryCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppvRootSignature_.key);
  analyzerService_.addParent(c.ppvRootSignature_.key, c.object_.key);
}

void AnalyzerLayer::post(ID3D12SDKConfiguration1CreateDeviceFactoryCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppvFactory_.key);
  analyzerService_.addParent(c.ppvFactory_.key, c.object_.key);
}

void AnalyzerLayer::post(ID3D12DSRDeviceFactoryCreateDSRDeviceCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.pD3D12Device_.key);
  analyzerService_.notifyObject(c.ppvDSRDevice_.key);
  analyzerService_.addParent(c.ppvDSRDevice_.key, c.object_.key);
  analyzerService_.addParent(c.ppvDSRDevice_.key, c.pD3D12Device_.key);
}

void AnalyzerLayer::post(IDXGIFactoryCreateSoftwareAdapterCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppAdapter_.key);
  analyzerService_.addParent(c.ppAdapter_.key, c.object_.key);
}

void AnalyzerLayer::post(IDXGIDeviceCreateSurfaceCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppSurface_.key);
  analyzerService_.addParent(c.ppSurface_.key, c.object_.key);
}

void AnalyzerLayer::post(IDXGIResource1CreateSubresourceSurfaceCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppSurface_.key);
  analyzerService_.addParent(c.ppSurface_.key, c.object_.key);
}

void AnalyzerLayer::post(IDXGIFactory2CreateSwapChainForHwndCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.pDevice_.key);
  analyzerService_.notifyObject(c.pRestrictToOutput_.key);
  analyzerService_.notifyObject(c.ppSwapChain_.key);
  analyzerService_.addParent(c.ppSwapChain_.key, c.object_.key);
  analyzerService_.addParent(c.ppSwapChain_.key, c.pDevice_.key);
  analyzerService_.addParent(c.ppSwapChain_.key, c.pRestrictToOutput_.key);
}

void AnalyzerLayer::post(IDXGIFactory2CreateSwapChainForCoreWindowCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.pDevice_.key);
  analyzerService_.notifyObject(c.pWindow_.key);
  analyzerService_.notifyObject(c.pRestrictToOutput_.key);
  analyzerService_.notifyObject(c.ppSwapChain_.key);
  analyzerService_.addParent(c.ppSwapChain_.key, c.object_.key);
  analyzerService_.addParent(c.ppSwapChain_.key, c.pDevice_.key);
  analyzerService_.addParent(c.ppSwapChain_.key, c.pWindow_.key);
  analyzerService_.addParent(c.ppSwapChain_.key, c.pRestrictToOutput_.key);
}

void AnalyzerLayer::post(IDXGIFactory2CreateSwapChainForCompositionCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.pDevice_.key);
  analyzerService_.notifyObject(c.pRestrictToOutput_.key);
  analyzerService_.notifyObject(c.ppSwapChain_.key);
  analyzerService_.addParent(c.ppSwapChain_.key, c.object_.key);
  analyzerService_.addParent(c.ppSwapChain_.key, c.pDevice_.key);
  analyzerService_.addParent(c.ppSwapChain_.key, c.pRestrictToOutput_.key);
}

void AnalyzerLayer::post(IDXGIFactoryMediaCreateSwapChainForCompositionSurfaceHandleCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.pDevice_.key);
  analyzerService_.notifyObject(c.pRestrictToOutput_.key);
  analyzerService_.notifyObject(c.ppSwapChain_.key);
  analyzerService_.addParent(c.ppSwapChain_.key, c.object_.key);
  analyzerService_.addParent(c.ppSwapChain_.key, c.pDevice_.key);
  analyzerService_.addParent(c.ppSwapChain_.key, c.pRestrictToOutput_.key);
}

void AnalyzerLayer::post(
    IDXGIFactoryMediaCreateDecodeSwapChainForCompositionSurfaceHandleCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.pDevice_.key);
  analyzerService_.notifyObject(c.pYuvDecodeBuffers_.key);
  analyzerService_.notifyObject(c.pRestrictToOutput_.key);
  analyzerService_.notifyObject(c.ppSwapChain_.key);
  analyzerService_.addParent(c.ppSwapChain_.key, c.object_.key);
  analyzerService_.addParent(c.ppSwapChain_.key, c.pDevice_.key);
  analyzerService_.addParent(c.ppSwapChain_.key, c.pYuvDecodeBuffers_.key);
  analyzerService_.addParent(c.ppSwapChain_.key, c.pRestrictToOutput_.key);
}

void AnalyzerLayer::post(IDMLDeviceCreateOperatorCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppv_.key);
  analyzerService_.addParent(c.ppv_.key, c.object_.key);
}

void AnalyzerLayer::post(IDMLDeviceCreateOperatorInitializerCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObjects(c.operators_.keys);
  analyzerService_.notifyObject(c.ppv_.key);
  analyzerService_.addParent(c.ppv_.key, c.object_.key);
  for (unsigned key : c.operators_.keys) {
    analyzerService_.addParent(c.ppv_.key, key);
  }
}

void AnalyzerLayer::post(IDMLDeviceCreateCommandRecorderCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppv_.key);
  analyzerService_.addParent(c.ppv_.key, c.object_.key);
}

void AnalyzerLayer::post(IDMLDeviceCreateBindingTableCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppv_.key);
  analyzerService_.notifyObject(c.desc_.data.cpuDescHandleKey);
  analyzerService_.notifyObject(c.desc_.data.gpuDescHandleKey);
  analyzerService_.addParent(c.ppv_.key, c.object_.key);
  analyzerService_.addParent(c.ppv_.key, c.desc_.data.cpuDescHandleKey);
  analyzerService_.addParent(c.ppv_.key, c.desc_.data.gpuDescHandleKey);
}

void AnalyzerLayer::post(IDStorageFactoryCreateQueueCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppv_.key);
  analyzerService_.notifyObject(c.desc_.deviceKey);
  analyzerService_.addParent(c.ppv_.key, c.object_.key);
  analyzerService_.addParent(c.ppv_.key, c.desc_.deviceKey);
}

void AnalyzerLayer::post(IDStorageFactoryCreateStatusArrayCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppv_.key);
  analyzerService_.addParent(c.ppv_.key, c.object_.key);
}

void AnalyzerLayer::post(ID3D12PipelineStateGetCachedBlobCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppBlob_.key);
  analyzerService_.addParent(c.ppBlob_.key, c.object_.key);
}

void AnalyzerLayer::post(ID3D12PipelineLibraryLoadGraphicsPipelineCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppPipelineState_.key);
  analyzerService_.notifyObject(c.pDesc_.rootSignatureKey);
  analyzerService_.addParent(c.ppPipelineState_.key, c.object_.key);
  analyzerService_.addParent(c.ppPipelineState_.key, c.pDesc_.rootSignatureKey);
}

void AnalyzerLayer::post(ID3D12PipelineLibraryLoadComputePipelineCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppPipelineState_.key);
  analyzerService_.notifyObject(c.pDesc_.rootSignatureKey);
  analyzerService_.addParent(c.ppPipelineState_.key, c.object_.key);
  analyzerService_.addParent(c.ppPipelineState_.key, c.pDesc_.rootSignatureKey);
}

void AnalyzerLayer::post(ID3D12PipelineLibrary1LoadPipelineCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppPipelineState_.key);
  analyzerService_.notifyObject(c.pDesc_.rootSignatureKey);
  analyzerService_.addParent(c.ppPipelineState_.key, c.object_.key);
  analyzerService_.addParent(c.ppPipelineState_.key, c.pDesc_.rootSignatureKey);
}

void AnalyzerLayer::post(ID3D12Device3OpenExistingHeapFromAddressCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppvHeap_.key);
  analyzerService_.addParent(c.ppvHeap_.key, c.object_.key);
}

void AnalyzerLayer::post(ID3D12Device3OpenExistingHeapFromFileMappingCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppvHeap_.key);
  analyzerService_.addParent(c.ppvHeap_.key, c.object_.key);
}

void AnalyzerLayer::post(ID3D12ProtectedSessionGetStatusFenceCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppFence_.key);
  analyzerService_.addParent(c.ppFence_.key, c.object_.key);
}

void AnalyzerLayer::post(ID3D12SwapChainAssistantGetSwapChainObjectCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppv_.key);
  analyzerService_.addParent(c.ppv_.key, c.object_.key);
}

void AnalyzerLayer::post(ID3D12SwapChainAssistantGetCurrentResourceAndCommandQueueCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppvResource_.key);
  analyzerService_.notifyObject(c.ppvQueue_.key);
  analyzerService_.addParent(c.ppvResource_.key, c.object_.key);
  analyzerService_.addParent(c.ppvQueue_.key, c.object_.key);
}

void AnalyzerLayer::post(ID3D12Device7AddToStateObjectCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.pStateObjectToGrowFrom_.key);
  analyzerService_.notifyObject(c.ppNewStateObject_.key);
  analyzerService_.addParent(c.ppNewStateObject_.key, c.object_.key);
  analyzerService_.addParent(c.ppNewStateObject_.key, c.pStateObjectToGrowFrom_.key);
  for (auto& it : c.pAddition_.interfaceKeysBySubobject) {
    analyzerService_.notifyObject(it.second);
  }
  if (c.result_.value != S_OK) {
    return;
  }
  if (optimize_) {
    raytracingService_.addToStateObject(c);
  }
}

void AnalyzerLayer::post(ID3D12Device13OpenExistingHeapFromAddress1Command& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppvHeap_.key);
  analyzerService_.addParent(c.ppvHeap_.key, c.object_.key);
}

void AnalyzerLayer::post(ID3D12DeviceTools1GetApplicationSpecificDriverStateCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppBlob_.key);
  analyzerService_.addParent(c.ppBlob_.key, c.object_.key);
}

void AnalyzerLayer::post(ID3D12DeviceFactoryGetConfigurationInterfaceCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppv_.key);
  analyzerService_.addParent(c.ppv_.key, c.object_.key);
}

void AnalyzerLayer::post(ID3D12DeviceConfigurationSerializeVersionedRootSignatureCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppResult_.key);
  analyzerService_.notifyObject(c.ppError_.key);
  analyzerService_.addParent(c.ppResult_.key, c.object_.key);
  analyzerService_.addParent(c.ppError_.key, c.object_.key);
}

void AnalyzerLayer::post(
    ID3D12DeviceConfigurationCreateVersionedRootSignatureDeserializerCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppvDeserializer_.key);
  analyzerService_.addParent(c.ppvDeserializer_.key, c.object_.key);
}

void AnalyzerLayer::post(
    ID3D12DeviceConfiguration1CreateVersionedRootSignatureDeserializerFromSubobjectInLibraryCommand&
        c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppvDeserializer_.key);
  analyzerService_.addParent(c.ppvDeserializer_.key, c.object_.key);
}

void AnalyzerLayer::post(IDXGIAdapterEnumOutputsCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppOutput_.key);
  analyzerService_.addParent(c.ppOutput_.key, c.object_.key);
}

void AnalyzerLayer::post(IDXGISwapChainGetBufferCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppSurface_.key);
  analyzerService_.addParent(c.ppSurface_.key, c.object_.key);
}

void AnalyzerLayer::post(IDXGISwapChainGetFullscreenStateCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppTarget_.key);
  analyzerService_.addParent(c.ppTarget_.key, c.object_.key);
}

void AnalyzerLayer::post(IDXGISwapChainGetContainingOutputCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppOutput_.key);
  analyzerService_.addParent(c.ppOutput_.key, c.object_.key);
}

void AnalyzerLayer::post(IDXGIFactoryEnumAdaptersCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppAdapter_.key);
  analyzerService_.addParent(c.ppAdapter_.key, c.object_.key);
}

void AnalyzerLayer::post(IDXGIFactory1EnumAdapters1Command& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppAdapter_.key);
  analyzerService_.addParent(c.ppAdapter_.key, c.object_.key);
}

void AnalyzerLayer::post(IDXGIOutputDuplicationAcquireNextFrameCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppDesktopResource_.key);
  analyzerService_.addParent(c.ppDesktopResource_.key, c.object_.key);
}

void AnalyzerLayer::post(IDXGISurface2GetResourceCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppParentResource_.key);
  analyzerService_.addParent(c.ppParentResource_.key, c.object_.key);
}

void AnalyzerLayer::post(IDXGISwapChain1GetCoreWindowCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppUnk_.key);
  analyzerService_.addParent(c.ppUnk_.key, c.object_.key);
}

void AnalyzerLayer::post(IDXGISwapChain1GetRestrictToOutputCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppRestrictToOutput_.key);
  analyzerService_.addParent(c.ppRestrictToOutput_.key, c.object_.key);
}

void AnalyzerLayer::post(IDXGIOutput1DuplicateOutputCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.pDevice_.key);
  analyzerService_.notifyObject(c.ppOutputDuplication_.key);
  analyzerService_.addParent(c.ppOutputDuplication_.key, c.object_.key);
}

void AnalyzerLayer::post(IDXGIFactory4EnumAdapterByLuidCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppvAdapter_.key);
  analyzerService_.addParent(c.ppvAdapter_.key, c.object_.key);
}

void AnalyzerLayer::post(IDXGIFactory4EnumWarpAdapterCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppvAdapter_.key);
  analyzerService_.addParent(c.ppvAdapter_.key, c.object_.key);
}

void AnalyzerLayer::post(IDXGIOutput5DuplicateOutput1Command& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.pDevice_.key);
  analyzerService_.notifyObject(c.ppOutputDuplication_.key);
  analyzerService_.addParent(c.ppOutputDuplication_.key, c.object_.key);
}

void AnalyzerLayer::post(IDXGIFactory6EnumAdapterByGpuPreferenceCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppvAdapter_.key);
  analyzerService_.addParent(c.ppvAdapter_.key, c.object_.key);
}

void AnalyzerLayer::post(IDMLDeviceCompileOperatorCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.op_.key);
  analyzerService_.notifyObject(c.ppv_.key);
  analyzerService_.addParent(c.ppv_.key, c.object_.key);
  analyzerService_.addParent(c.ppv_.key, c.op_.key);
}

void AnalyzerLayer::post(IDMLDevice1CompileGraphCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppv_.key);
  analyzerService_.notifyObjects(c.desc_.operatorKeys);
  analyzerService_.addParent(c.ppv_.key, c.object_.key);
  for (unsigned key : c.desc_.operatorKeys) {
    analyzerService_.addParent(c.ppv_.key, key);
  }
}

void AnalyzerLayer::post(IDStorageFactoryOpenFileCommand& c) {
  if (analyzerService_.afterRange()) {
    return;
  }
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppv_.key);
  analyzerService_.addParent(c.ppv_.key, c.object_.key);
}

} // namespace DirectX
} // namespace gits

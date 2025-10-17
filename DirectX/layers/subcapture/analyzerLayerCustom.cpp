// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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
      analyzerService_(subcaptureRange, commandListService_, raytracingService_),
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
                         rootSignatureService_),
      executeIndirectService_(gpuAddressService_, raytracingService_, commandListService_) {
  optimize_ = Configurator::Get().directx.features.subcapture.optimize;
  optimizeRaytracing_ = Configurator::Get().directx.features.subcapture.optimizeRaytracing;
  if (optimize_) {
    shaderIdentifierService_.enablePlayerIdentifierLookup();
  }
}

void AnalyzerLayer::post(IDXGISwapChainPresentCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  if (c.Flags_.value & DXGI_PRESENT_TEST) {
    return;
  }
  analyzerService_.present(c.key, c.object_.key);
}

void AnalyzerLayer::post(IDXGISwapChain1Present1Command& c) {
  analyzerService_.notifyObject(c.object_.key);
  if (c.PresentFlags_.value & DXGI_PRESENT_TEST) {
    return;
  }
  analyzerService_.present(c.key, c.object_.key);
}

void AnalyzerLayer::post(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObjects(c.ppCommandLists_.keys);
  analyzerService_.executeCommandLists(c.key, c.object_.key, c.ppCommandLists_.keys);
  if (optimize_ || optimizeRaytracing_) {
    raytracingService_.executeCommandLists(c.key, c.object_.key, c.object_.value,
                                           c.ppCommandLists_.value, c.NumCommandLists_.value);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListResetCommand& c) {
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
  analyzerService_.notifyObject(c.object_.key);
  subcaptureRange_.executionEnd();
  analyzerService_.executionEnd();
}

void AnalyzerLayer::post(ID3D12CommandQueueWaitCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.pFence_.key);
  if (c.result_.value != S_OK) {
    return;
  }
  analyzerService_.commandQueueWait(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
  if (optimize_ || optimizeRaytracing_) {
    raytracingService_.commandQueueWait(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
  }
}

void AnalyzerLayer::post(ID3D12CommandQueueSignalCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.pFence_.key);
  if (c.result_.value != S_OK) {
    return;
  }
  analyzerService_.commandQueueSignal(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
  if (optimize_ || optimizeRaytracing_) {
    raytracingService_.commandQueueSignal(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
  }
}

void AnalyzerLayer::post(ID3D12FenceSignalCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  if (c.result_.value != S_OK) {
    return;
  }
  analyzerService_.fenceSignal(c.key, c.object_.key, c.Value_.value);
  if (optimize_ || optimizeRaytracing_) {
    raytracingService_.fenceSignal(c.key, c.object_.key, c.Value_.value);
  }
}

void AnalyzerLayer::post(ID3D12DeviceCreateFenceCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppFence_.key);
  if (c.result_.value != S_OK) {
    return;
  }
  analyzerService_.fenceSignal(c.key, c.ppFence_.key, c.InitialValue_.value);
  if (optimize_ || optimizeRaytracing_) {
    raytracingService_.fenceSignal(c.key, c.ppFence_.key, c.InitialValue_.value);
  }
}

void AnalyzerLayer::post(ID3D12Device3EnqueueMakeResidentCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObjects(c.ppObjects_.keys);
  analyzerService_.notifyObject(c.pFenceToSignal_.key);
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
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetGraphicsRootSignatureCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListIASetIndexBufferCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListIASetVertexBuffersCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSOSetTargetsCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListClearDepthStencilViewCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListClearRenderTargetViewCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12DeviceCopyDescriptorsSimpleCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  if (optimize_) {
    descriptorService_.copyDescriptors(c);
    commandListService_.copyDescriptors(c);
  }
}

void AnalyzerLayer::post(ID3D12DeviceCopyDescriptorsCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  if (optimize_) {
    descriptorService_.copyDescriptors(c);
    commandListService_.copyDescriptors(c);
  }
}

void AnalyzerLayer::post(ID3D12DeviceCreateRenderTargetViewCommand& c) {
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
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.pResource_.key);
  analyzerService_.notifyObject(c.DestDescriptor_.interfaceKey);
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
    state->bufferLocationKey = c.pDesc_.bufferLocationKey;
    state->bufferLocationOffset = c.pDesc_.bufferLocationOffset;
    state->destDescriptor = c.DestDescriptor_.value;
    state->destDescriptorKey = c.DestDescriptor_.interfaceKey;
    state->destDescriptorIndex = c.DestDescriptor_.index;
    descriptorService_.storeState(state);
  }
}

void AnalyzerLayer::post(ID3D12DeviceCreateSamplerCommand& c) {
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

void AnalyzerLayer::post(ID3D12DeviceCreateDescriptorHeapCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppvHeap_.key);
  if (c.result_.value != S_OK) {
    return;
  }
  if (optimize_) {
    commandListService_.createDescriptorHeap(c);
  }
}

void AnalyzerLayer::post(ID3D12DeviceCreateRootSignatureCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppvRootSignature_.key);
  if (c.result_.value != S_OK) {
    return;
  }
  if (optimize_) {
    rootSignatureService_.createRootSignature(c);
  }
}

void AnalyzerLayer::post(ID3D12Device5CreateStateObjectCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppStateObject_.key);
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

void AnalyzerLayer::post(ID3D12Device7AddToStateObjectCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppNewStateObject_.key);
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

void AnalyzerLayer::pre(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& c) {
  CapturePlayerShaderIdentifierService::ShaderIdentifier shaderIdentifier;
  memcpy(shaderIdentifier.data(), c.result_.value, shaderIdentifier.size());
  shaderIdentifierService_.addCaptureShaderIdentifier(c.key, shaderIdentifier,
                                                      c.pExportName_.value);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandList4SetPipelineState1Command& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetDescriptorHeapsCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::pre(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (optimize_ || optimizeRaytracing_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::pre(ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) {
  if (optimize_ || optimizeRaytracing_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::pre(
    ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoCommand& c) {
  if (optimize_ || optimizeRaytracing_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::pre(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) {
  if (optimize_ || optimizeRaytracing_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::pre(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) {
  if (optimize_ || optimizeRaytracing_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::pre(ID3D12GraphicsCommandList4DispatchRaysCommand& c) {
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12DeviceCreateCommandSignatureCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppvCommandSignature_.key);
  if (c.result_.value != S_OK) {
    return;
  }
  if (optimize_) {
    executeIndirectService_.createCommandSignature(c);
  }
}

void AnalyzerLayer::pre(ID3D12GraphicsCommandListExecuteIndirectCommand& c) {
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::pre(ID3D12GraphicsCommandListDispatchCommand& c) {
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::pre(ID3D12GraphicsCommandListDrawIndexedInstancedCommand& c) {
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::pre(ID3D12GraphicsCommandListDrawInstancedCommand& c) {
  if (optimize_) {
    commandListService_.command(c);
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
    analyzerService_.notifyObject(c.object_.key);
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
    analyzerService_.notifyObject(c.object_.key);
    descriptorHandleService_.addPlayerHandle(c.object_.key, c.result_.value);
  }
}

void AnalyzerLayer::post(IUnknownReleaseCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  if (optimize_ || optimizeRaytracing_) {
    if (c.result_.value == 0) {
      gpuAddressService_.destroyInterface(c.object_.key);
    }
  }
}

void AnalyzerLayer::post(IUnknownQueryInterfaceCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
}

void AnalyzerLayer::post(IUnknownAddRefCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
}

void AnalyzerLayer::post(ID3D12DeviceCreateCommittedResourceCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppvResource_.key);
  if (optimize_ || optimizeRaytracing_) {
    if (c.InitialResourceState_.value == D3D12_RESOURCE_STATE_GENERIC_READ) {
      raytracingService_.genericReadResource(c.ppvResource_.key);
    }
  }
}

void AnalyzerLayer::post(ID3D12Device4CreateCommittedResource1Command& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppvResource_.key);
  if (optimize_ || optimizeRaytracing_) {
    if (c.InitialResourceState_.value == D3D12_RESOURCE_STATE_GENERIC_READ) {
      raytracingService_.genericReadResource(c.ppvResource_.key);
    }
  }
}

void AnalyzerLayer::post(ID3D12Device8CreateCommittedResource2Command& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppvResource_.key);
  if (optimize_ || optimizeRaytracing_) {
    if (c.InitialResourceState_.value == D3D12_RESOURCE_STATE_GENERIC_READ) {
      raytracingService_.genericReadResource(c.ppvResource_.key);
    }
  }
}

void AnalyzerLayer::post(ID3D12Device10CreateCommittedResource3Command& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppvResource_.key);
  if (optimize_ || optimizeRaytracing_) {
    if (c.InitialLayout_.value == D3D12_BARRIER_LAYOUT_GENERIC_READ) {
      raytracingService_.genericReadResource(c.ppvResource_.key);
    }
  }
}

void AnalyzerLayer::post(ID3D12DeviceCreatePlacedResourceCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.pHeap_.key);
  analyzerService_.notifyObject(c.ppvResource_.key);
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
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.pHeap_.key);
  analyzerService_.notifyObject(c.ppvResource_.key);
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
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.pHeap_.key);
  analyzerService_.notifyObject(c.ppvResource_.key);
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
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListCopyBufferRegionCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListCopyTextureRegionCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListCopyResourceCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListCopyTilesCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListResolveSubresourceCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListDiscardResourceCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetPipelineStateCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListResourceBarrierCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandList1ResolveSubresourceRegionCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandList3SetProtectedResourceSessionCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandList4InitializeMetaCommandCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandList4ExecuteMetaCommandCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(MappedDataMetaCommand& c) {
  analyzerService_.notifyObject(c.resource_.key);
  if (optimize_) {
    analyzerService_.mappedDataMeta(c.resource_.key);
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandList7BarrierCommand& c) {
  analyzerService_.commandListCommand(c.object_.key);
  if (optimize_) {
    commandListService_.command(c);
  }
}

void AnalyzerLayer::post(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) {
  analyzerService_.commandListCommand(c.pCommandList_.key);
}

void AnalyzerLayer::post(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) {
  analyzerService_.commandListCommand(c.pCommandList_.key);
}

void AnalyzerLayer::post(xessD3D12CreateContextCommand& c) {
  analyzerService_.notifyObject(c.pDevice_.key);
  analyzerService_.notifyObject(c.phContext_.key);
  if (optimize_) {
    analyzerService_.createXessContext(c);
  }
}

void AnalyzerLayer::post(IDXGIDeviceCreateSurfaceCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppSurface_.key);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListClearStateCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.commandListCommand(c.object_.key);
  commandListService_.command(c);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListExecuteBundleCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.commandListCommand(c.object_.key);
  commandListService_.command(c);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListBeginQueryCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.commandListCommand(c.object_.key);
  commandListService_.command(c);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListEndQueryCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.commandListCommand(c.object_.key);
  commandListService_.command(c);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListResolveQueryDataCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.commandListCommand(c.object_.key);
  commandListService_.command(c);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListSetPredicationCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.commandListCommand(c.object_.key);
  commandListService_.command(c);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandList1AtomicCopyBufferUINTCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.commandListCommand(c.object_.key);
  commandListService_.command(c);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandList1AtomicCopyBufferUINT64Command& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.commandListCommand(c.object_.key);
  commandListService_.command(c);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandList4BeginRenderPassCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.commandListCommand(c.object_.key);
  commandListService_.command(c);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandList5RSSetShadingRateImageCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.commandListCommand(c.object_.key);
  commandListService_.command(c);
}

void AnalyzerLayer::post(ID3D12GraphicsCommandList6DispatchMeshCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.commandListCommand(c.object_.key);
  commandListService_.command(c);
}

void AnalyzerLayer::post(ID3D12DeviceCreateGraphicsPipelineStateCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppPipelineState_.key);
  analyzerService_.notifyObject(c.pDesc_.rootSignatureKey);
}

void AnalyzerLayer::post(ID3D12DeviceCreateComputePipelineStateCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppPipelineState_.key);
  analyzerService_.notifyObject(c.pDesc_.rootSignatureKey);
}

void AnalyzerLayer::post(ID3D12PipelineLibraryLoadGraphicsPipelineCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppPipelineState_.key);
  analyzerService_.notifyObject(c.pDesc_.rootSignatureKey);
}

void AnalyzerLayer::post(ID3D12PipelineLibraryLoadComputePipelineCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppPipelineState_.key);
  analyzerService_.notifyObject(c.pDesc_.rootSignatureKey);
}

void AnalyzerLayer::post(ID3D12PipelineLibrary1LoadPipelineCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppPipelineState_.key);
  analyzerService_.notifyObject(c.pDesc_.rootSignatureKey);
}

void AnalyzerLayer::post(ID3D12Device2CreatePipelineStateCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppPipelineState_.key);
  analyzerService_.notifyObject(c.pDesc_.rootSignatureKey);
}

void AnalyzerLayer::post(IDMLDeviceCreateBindingTableCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppv_.key);
  analyzerService_.notifyObject(c.desc_.data.cpuDescHandleKey);
  analyzerService_.notifyObject(c.desc_.data.gpuDescHandleKey);
}

void AnalyzerLayer::post(IDMLBindingTableResetCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.desc_.data.cpuDescHandleKey);
  analyzerService_.notifyObject(c.desc_.data.gpuDescHandleKey);
}

void AnalyzerLayer::post(IDMLBindingTableBindInputsCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObjects(c.bindings_.resourceKeys);
}

void AnalyzerLayer::post(IDMLBindingTableBindOutputsCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObjects(c.bindings_.resourceKeys);
}

void AnalyzerLayer::post(IDMLDevice1CompileGraphCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppv_.key);
  analyzerService_.notifyObjects(c.desc_.operatorKeys);
}

void AnalyzerLayer::post(xessD3D12InitCommand& c) {
  analyzerService_.notifyObject(c.hContext_.key);
  analyzerService_.notifyObject(c.pInitParams_.key);
  analyzerService_.notifyObject(c.pInitParams_.tempBufferHeapKey);
  analyzerService_.notifyObject(c.pInitParams_.tempTextureHeapKey);
  analyzerService_.notifyObject(c.pInitParams_.pipelineLibraryKey);
}

void AnalyzerLayer::post(xessD3D12GetInitParamsCommand& c) {
  analyzerService_.notifyObject(c.hContext_.key);
  analyzerService_.notifyObject(c.pInitParams_.key);
  analyzerService_.notifyObject(c.pInitParams_.tempBufferHeapKey);
  analyzerService_.notifyObject(c.pInitParams_.tempTextureHeapKey);
  analyzerService_.notifyObject(c.pInitParams_.pipelineLibraryKey);
}

void AnalyzerLayer::post(xessD3D12ExecuteCommand& c) {
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

void AnalyzerLayer::post(IDStorageFactoryCreateQueueCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.ppv_.key);
  analyzerService_.notifyObject(c.desc_.deviceKey);
}

void AnalyzerLayer::post(IDStorageQueueEnqueueRequestCommand& c) {
  analyzerService_.notifyObject(c.object_.key);
  analyzerService_.notifyObject(c.request_.fileKey);
  analyzerService_.notifyObject(c.request_.resourceKey);
}

void AnalyzerLayer::post(INTC_D3D12_GetSupportedVersionsCommand& c) {
  analyzerService_.notifyObject(c.pDevice_.key);
}

void AnalyzerLayer::post(INTC_D3D12_CreateDeviceExtensionContextCommand& c) {
  analyzerService_.notifyObject(c.pDevice_.key);
  analyzerService_.notifyObject(c.ppExtensionContext_.key);
  analyzerService_.createDeviceExtensionContext(c);
}

void AnalyzerLayer::post(INTC_D3D12_CreateDeviceExtensionContext1Command& c) {
  analyzerService_.notifyObject(c.pDevice_.key);
  analyzerService_.notifyObject(c.ppExtensionContext_.key);
  analyzerService_.createDeviceExtensionContext(c);
}

void AnalyzerLayer::post(INTC_DestroyDeviceExtensionContextCommand& c) {
  analyzerService_.notifyObject(c.ppExtensionContext_.key);
}

void AnalyzerLayer::post(INTC_D3D12_CheckFeatureSupportCommand& c) {
  analyzerService_.notifyObject(c.pExtensionContext_.key);
}

void AnalyzerLayer::post(INTC_D3D12_CreateCommandQueueCommand& c) {
  analyzerService_.notifyObject(c.pExtensionContext_.key);
  analyzerService_.notifyObject(c.ppCommandQueue_.key);
}

void AnalyzerLayer::post(INTC_D3D12_CreateReservedResourceCommand& c) {
  analyzerService_.notifyObject(c.pExtensionContext_.key);
  analyzerService_.notifyObject(c.ppvResource_.key);
}

void AnalyzerLayer::post(INTC_D3D12_SetFeatureSupportCommand& c) {
  analyzerService_.notifyObject(c.pExtensionContext_.key);
}

void AnalyzerLayer::post(INTC_D3D12_GetResourceAllocationInfoCommand& c) {
  analyzerService_.notifyObject(c.pExtensionContext_.key);
}

void AnalyzerLayer::post(INTC_D3D12_CreateComputePipelineStateCommand& c) {
  analyzerService_.notifyObject(c.pExtensionContext_.key);
  analyzerService_.notifyObject(c.ppPipelineState_.key);
  analyzerService_.notifyObject(c.pDesc_.rootSignatureKey);
}

void AnalyzerLayer::post(INTC_D3D12_CreatePlacedResourceCommand& c) {
  analyzerService_.notifyObject(c.pExtensionContext_.key);
  analyzerService_.notifyObject(c.pHeap_.key);
  analyzerService_.notifyObject(c.ppvResource_.key);
}

void AnalyzerLayer::post(INTC_D3D12_CreateCommittedResourceCommand& c) {
  analyzerService_.notifyObject(c.pExtensionContext_.key);
  analyzerService_.notifyObject(c.ppvResource_.key);
}

void AnalyzerLayer::post(INTC_D3D12_CreateHeapCommand& c) {
  analyzerService_.notifyObject(c.pExtensionContext_.key);
  analyzerService_.notifyObject(c.ppvHeap_.key);
}

void AnalyzerLayer::post(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& c) {
  analyzerService_.notifyObject(c.pDevice_.key);
}

void AnalyzerLayer::post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& c) {
  analyzerService_.notifyObject(c.pDev_.key);
}

void AnalyzerLayer::post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& c) {
  analyzerService_.notifyObject(c.pDev_.key);
}

void AnalyzerLayer::post(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& c) {
  analyzerService_.notifyObject(c.pCommandList_.key);
}

} // namespace DirectX
} // namespace gits

// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "analyzerLayerAuto.h"
#include "log.h"
#include "configurationLib.h"

#include <fstream>
#include <sstream>

namespace gits {
namespace DirectX {

AnalyzerLayer::AnalyzerLayer(SubcaptureRange& subcaptureRange)
    : Layer("Analyzer"),
      m_SubcaptureRange(subcaptureRange),
      m_AnalyzerService(
          subcaptureRange, m_CommandListService, m_RaytracingService, m_ExecuteIndirectService),
      m_CommandListService(m_AnalyzerService,
                           m_DescriptorService,
                           m_RootSignatureService,
                           m_RaytracingService,
                           m_ExecuteIndirectService,
                           m_SubcaptureRange.CommandListSubcapture()),
      m_RaytracingService(m_DescriptorService,
                          m_GpuAddressService,
                          m_DescriptorHandleService,
                          m_ShaderIdentifierService,
                          m_CommandListService,
                          m_RootSignatureService,
                          m_ResourceStateTracker),
      m_ExecuteIndirectService(m_GpuAddressService, m_RaytracingService, m_CommandListService) {
  m_Optimize = Configurator::Get().directx.features.subcapture.optimize;
  m_OptimizeRaytracing = Configurator::Get().directx.features.subcapture.optimizeRaytracing;
  if (m_Optimize) {
    m_ShaderIdentifierService.EnablePlayerIdentifierLookup();
  }
}

void AnalyzerLayer::Post(IDXGISwapChainPresentCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  if (c.m_Flags.Value & DXGI_PRESENT_TEST) {
    return;
  }
  m_AnalyzerService.Present(c.Key, c.m_Object.Key);
  m_CommandListService.Present();
}

void AnalyzerLayer::Post(IDXGISwapChain1Present1Command& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  if (c.m_PresentFlags.Value & DXGI_PRESENT_TEST) {
    return;
  }
  m_AnalyzerService.Present(c.Key, c.m_Object.Key);
  m_CommandListService.Present();
}

void AnalyzerLayer::Post(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObjects(c.m_ppCommandLists.Keys);
  m_AnalyzerService.ExecuteCommandLists(c.Key, c.m_Object.Key, c.m_ppCommandLists.Keys);
  if (m_Optimize || m_OptimizeRaytracing) {
    m_ResourceStateTracker.ExecuteCommandLists(
        reinterpret_cast<ID3D12GraphicsCommandList**>(c.m_ppCommandLists.Value),
        c.m_NumCommandLists.Value);
    m_RaytracingService.ExecuteCommandLists(c.Key, c.m_Object.Key, c.m_Object.Value,
                                            c.m_ppCommandLists.Value, c.m_NumCommandLists.Value);
    m_ExecuteIndirectService.ExecuteCommandLists(c.Key, c.m_Object.Key, c.m_Object.Value,
                                                 c.m_ppCommandLists.Value,
                                                 c.m_NumCommandLists.Value);
  }
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandListResetCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_pAllocator.Key);
  m_AnalyzerService.NotifyObject(c.m_pInitialState.Key);
  if (c.m_Result.Value != S_OK) {
    return;
  }
  m_AnalyzerService.ExecutionStart();
  m_SubcaptureRange.ExecutionStart();
  m_AnalyzerService.CommandListReset(c.m_Object.Key, c.m_pAllocator.Key, c.m_pInitialState.Key);
  if (m_Optimize || m_OptimizeRaytracing) {
    m_CommandListService.CommandListReset(c);
  }
  if (m_Optimize) {
    m_CommandListService.CommandListReset(c);
  }
}

void AnalyzerLayer::Post(ID3D12FenceGetCompletedValueCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_SubcaptureRange.ExecutionEnd();
  m_AnalyzerService.ExecutionEnd();
}

void AnalyzerLayer::Post(ID3D12CommandQueueWaitCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_pFence.Key);
  if (c.m_Result.Value != S_OK) {
    return;
  }
  m_AnalyzerService.CommandQueueWait(c.Key, c.m_Object.Key, c.m_pFence.Key, c.m_Value.Value);
  if (m_Optimize || m_OptimizeRaytracing) {
    m_RaytracingService.CommandQueueWait(c.Key, c.m_Object.Key, c.m_pFence.Key, c.m_Value.Value);
    m_ExecuteIndirectService.CommandQueueWait(c.Key, c.m_Object.Key, c.m_pFence.Key,
                                              c.m_Value.Value);
  }
}

void AnalyzerLayer::Post(ID3D12CommandQueueSignalCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_pFence.Key);
  if (c.m_Result.Value != S_OK) {
    return;
  }
  m_AnalyzerService.CommandQueueSignal(c.Key, c.m_Object.Key, c.m_pFence.Key, c.m_Value.Value);
  if (m_Optimize || m_OptimizeRaytracing) {
    m_RaytracingService.CommandQueueSignal(c.Key, c.m_Object.Key, c.m_pFence.Key, c.m_Value.Value);
    m_ExecuteIndirectService.CommandQueueSignal(c.Key, c.m_Object.Key, c.m_pFence.Key,
                                                c.m_Value.Value);
  }
}

void AnalyzerLayer::Post(ID3D12FenceSignalCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  if (c.m_Result.Value != S_OK) {
    return;
  }
  m_AnalyzerService.FenceSignal(c.Key, c.m_Object.Key, c.m_Value.Value);
  if (m_Optimize || m_OptimizeRaytracing) {
    m_RaytracingService.FenceSignal(c.Key, c.m_Object.Key, c.m_Value.Value);
    m_ExecuteIndirectService.FenceSignal(c.Key, c.m_Object.Key, c.m_Value.Value);
  }
}

void AnalyzerLayer::Post(ID3D12Device3EnqueueMakeResidentCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObjects(c.m_ppObjects.Keys);
  m_AnalyzerService.NotifyObject(c.m_pFenceToSignal.Key);
  if (c.m_Result.Value != S_OK) {
    return;
  }
  m_AnalyzerService.FenceSignal(c.Key, c.m_pFenceToSignal.Key, c.m_FenceValueToSignal.Value);
  if (m_Optimize || m_OptimizeRaytracing) {
    m_RaytracingService.FenceSignal(c.Key, c.m_pFenceToSignal.Key, c.m_FenceValueToSignal.Value);
    m_ExecuteIndirectService.FenceSignal(c.Key, c.m_pFenceToSignal.Key,
                                         c.m_FenceValueToSignal.Value);
  }
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandListSetComputeRootSignatureCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  if (m_Optimize) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandListSetGraphicsRootSignatureCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  if (m_Optimize) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  if (m_Optimize) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  if (m_Optimize) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  if (m_Optimize) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  if (m_Optimize) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  if (m_Optimize) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  if (m_Optimize) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  if (m_Optimize) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  if (m_Optimize) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandListIASetIndexBufferCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  if (m_Optimize) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandListIASetVertexBuffersCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  if (m_Optimize) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandListSOSetTargetsCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  if (m_Optimize) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  if (m_Optimize) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandListClearDepthStencilViewCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  if (m_Optimize) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandListClearRenderTargetViewCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  if (m_Optimize) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  if (m_Optimize) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  if (m_Optimize) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Post(ID3D12DeviceCopyDescriptorsSimpleCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  if (m_Optimize) {
    m_DescriptorService.CopyDescriptors(c);
    m_CommandListService.CopyDescriptors(c);
  }
}

void AnalyzerLayer::Post(ID3D12DeviceCopyDescriptorsCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  if (m_Optimize) {
    m_DescriptorService.CopyDescriptors(c);
    m_CommandListService.CopyDescriptors(c);
  }
}

void AnalyzerLayer::Post(ID3D12DeviceCreateRenderTargetViewCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_pResource.Key);
  m_AnalyzerService.NotifyObject(c.m_DestDescriptor.InterfaceKey);
  if (m_Optimize) {
    D3D12RenderTargetViewState* state = new D3D12RenderTargetViewState();
    state->DeviceKey = c.m_Object.Key;
    state->ResourceKey = c.m_pResource.Key;
    if (state->IsDesc = c.m_pDesc.Value ? true : false) {
      state->Desc = *c.m_pDesc.Value;
    }
    state->DestDescriptor = c.m_DestDescriptor.Value;
    state->DestDescriptorKey = c.m_DestDescriptor.InterfaceKey;
    state->DestDescriptorIndex = c.m_DestDescriptor.Index;
    m_DescriptorService.StoreState(state);
  }
}

void AnalyzerLayer::Post(ID3D12DeviceCreateDepthStencilViewCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_pResource.Key);
  m_AnalyzerService.NotifyObject(c.m_DestDescriptor.InterfaceKey);
  if (m_Optimize) {
    D3D12DepthStencilViewState* state = new D3D12DepthStencilViewState();
    state->DeviceKey = c.m_Object.Key;
    state->ResourceKey = c.m_pResource.Key;
    if (state->IsDesc = c.m_pDesc.Value ? true : false) {
      state->Desc = *c.m_pDesc.Value;
    }
    state->DestDescriptor = c.m_DestDescriptor.Value;
    state->DestDescriptorKey = c.m_DestDescriptor.InterfaceKey;
    state->DestDescriptorIndex = c.m_DestDescriptor.Index;
    m_DescriptorService.StoreState(state);
  }
}

void AnalyzerLayer::Post(ID3D12DeviceCreateShaderResourceViewCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_pResource.Key);
  m_AnalyzerService.NotifyObject(c.m_DestDescriptor.InterfaceKey);
  if (m_Optimize) {
    D3D12ShaderResourceViewState* state = new D3D12ShaderResourceViewState();
    state->DeviceKey = c.m_Object.Key;
    state->ResourceKey = c.m_pResource.Key;
    if (c.m_pDesc.Value) {
      state->IsDesc = true;
      state->Desc = *c.m_pDesc.Value;
      if (c.m_pDesc.Value->ViewDimension == D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE) {
        state->ResourceKey = c.m_pDesc.RaytracingLocationKey;
        state->RaytracingLocationOffset = c.m_pDesc.RaytracingLocationOffset;
      }
    }
    state->DestDescriptor = c.m_DestDescriptor.Value;
    state->DestDescriptorKey = c.m_DestDescriptor.InterfaceKey;
    state->DestDescriptorIndex = c.m_DestDescriptor.Index;
    m_DescriptorService.StoreState(state);
  }
}

void AnalyzerLayer::Post(ID3D12DeviceCreateUnorderedAccessViewCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_pResource.Key);
  m_AnalyzerService.NotifyObject(c.m_pCounterResource.Key);
  m_AnalyzerService.NotifyObject(c.m_DestDescriptor.InterfaceKey);
  if (m_Optimize) {
    D3D12UnorderedAccessViewState* state = new D3D12UnorderedAccessViewState();
    state->DeviceKey = c.m_Object.Key;
    state->ResourceKey = c.m_pResource.Key;
    state->AuxiliaryResourceKey = c.m_pCounterResource.Key;
    if (state->IsDesc = c.m_pDesc.Value ? true : false) {
      state->Desc = *c.m_pDesc.Value;
    }
    state->DestDescriptor = c.m_DestDescriptor.Value;
    state->DestDescriptorKey = c.m_DestDescriptor.InterfaceKey;
    state->DestDescriptorIndex = c.m_DestDescriptor.Index;
    m_DescriptorService.StoreState(state);
  }
}

void AnalyzerLayer::Post(ID3D12DeviceCreateConstantBufferViewCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_pDesc.BufferLocationKey);
  m_AnalyzerService.NotifyObject(c.m_DestDescriptor.InterfaceKey);
  if (m_Optimize) {
    D3D12ConstantBufferViewState* state = new D3D12ConstantBufferViewState();
    state->DeviceKey = c.m_Object.Key;
    if (state->IsDesc = c.m_pDesc.Value ? true : false) {
      state->Desc = *c.m_pDesc.Value;
    }
    state->ResourceKey = c.m_pDesc.BufferLocationKey;
    state->BufferLocationOffset = c.m_pDesc.BufferLocationOffset;
    state->DestDescriptor = c.m_DestDescriptor.Value;
    state->DestDescriptorKey = c.m_DestDescriptor.InterfaceKey;
    state->DestDescriptorIndex = c.m_DestDescriptor.Index;
    m_DescriptorService.StoreState(state);
  }
}

void AnalyzerLayer::Post(ID3D12DeviceCreateSamplerCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_DestDescriptor.InterfaceKey);
  if (m_Optimize) {
    D3D12SamplerState* state = new D3D12SamplerState();
    state->DeviceKey = c.m_Object.Key;
    state->Desc = *c.m_pDesc.Value;
    state->DestDescriptor = c.m_DestDescriptor.Value;
    state->DestDescriptorKey = c.m_DestDescriptor.InterfaceKey;
    state->DestDescriptorIndex = c.m_DestDescriptor.Index;
    m_DescriptorService.StoreState(state);
  }
}

void AnalyzerLayer::Pre(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  CapturePlayerShaderIdentifierService::ShaderIdentifier shaderIdentifier;
  memcpy(shaderIdentifier.data(), c.m_Result.Value, shaderIdentifier.size());
  m_ShaderIdentifierService.AddCaptureShaderIdentifier(c.Key, shaderIdentifier,
                                                       c.m_pExportName.Value);
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandList4SetPipelineState1Command& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  if (m_Optimize) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandListSetDescriptorHeapsCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  if (m_Optimize) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Pre(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  if (m_Optimize || m_OptimizeRaytracing) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Pre(ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  if (m_Optimize || m_OptimizeRaytracing) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Pre(
    ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  if (m_Optimize || m_OptimizeRaytracing) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Pre(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  if (m_Optimize || m_OptimizeRaytracing) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Pre(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  if (m_Optimize || m_OptimizeRaytracing) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Pre(ID3D12GraphicsCommandList4DispatchRaysCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  if (m_Optimize) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Pre(ID3D12GraphicsCommandListExecuteIndirectCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  if (m_Optimize) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Pre(ID3D12GraphicsCommandListDispatchCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  if (m_Optimize) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Pre(ID3D12GraphicsCommandListDrawIndexedInstancedCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  if (m_Optimize) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Pre(ID3D12GraphicsCommandListDrawInstancedCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  if (m_Optimize) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Pre(ID3D12ResourceGetGPUVirtualAddressCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  if (m_Optimize || m_OptimizeRaytracing) {
    m_RaytracingService.GetGPUVirtualAddress(c);
    D3D12_RESOURCE_DESC desc = c.m_Object.Value->GetDesc();
    m_GpuAddressService.AddGpuCaptureAddress(c.m_Object.Value, c.m_Object.Key, desc.Width,
                                             c.m_Result.Value);
  }
}

void AnalyzerLayer::Post(ID3D12ResourceGetGPUVirtualAddressCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  if (m_Optimize || m_OptimizeRaytracing) {
    m_AnalyzerService.NotifyObject(c.m_Object.Key);
    D3D12_RESOURCE_DESC desc = c.m_Object.Value->GetDesc();
    m_GpuAddressService.AddGpuPlayerAddress(c.m_Object.Value, c.m_Object.Key, desc.Width,
                                            c.m_Result.Value);
  }
}

void AnalyzerLayer::Pre(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  if (m_Optimize) {
    m_DescriptorHandleService.AddCaptureHandle(c.m_Object.Value, c.m_Object.Key, c.m_Result.Value);
  }
}

void AnalyzerLayer::Post(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  if (m_Optimize) {
    m_AnalyzerService.NotifyObject(c.m_Object.Key);
    m_DescriptorHandleService.AddPlayerHandle(c.m_Object.Key, c.m_Result.Value);
  }
}

void AnalyzerLayer::Post(IUnknownReleaseCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  if (m_Optimize || m_OptimizeRaytracing) {
    if (c.m_Result.Value == 0) {
      m_GpuAddressService.DestroyInterface(c.m_Object.Key);
    }
  }
}

void AnalyzerLayer::Post(IUnknownQueryInterfaceCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
}

void AnalyzerLayer::Post(IUnknownAddRefCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandList2WriteBufferImmediateCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  if (m_Optimize) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandListCopyBufferRegionCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  if (m_Optimize) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandListCopyTextureRegionCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  if (m_Optimize) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandListCopyResourceCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  if (m_Optimize) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandListCopyTilesCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  if (m_Optimize) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandListResolveSubresourceCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  if (m_Optimize) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandListDiscardResourceCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  if (m_Optimize) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandListSetPipelineStateCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  if (m_Optimize) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandListResourceBarrierCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  if (m_Optimize) {
    m_ResourceStateTracker.ResourceBarrier(c.m_Object.Value, c.m_pBarriers.Value,
                                           c.m_NumBarriers.Value,
                                           c.m_pBarriers.ResourceKeys.data());
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandList1ResolveSubresourceRegionCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  if (m_Optimize) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandList3SetProtectedResourceSessionCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  if (m_Optimize) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandList4InitializeMetaCommandCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  if (m_Optimize) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandList4ExecuteMetaCommandCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  if (m_Optimize) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Post(MappedDataMetaCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_resource.Key);
  if (m_Optimize) {
    m_AnalyzerService.MappedDataMeta(c.m_resource.Key);
  }
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandList7BarrierCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  if (m_Optimize) {
    m_CommandListService.Command(c);
  }
}

void AnalyzerLayer::Post(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.CommandListCommand(c.m_pCommandList.Key);
}

void AnalyzerLayer::Post(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.CommandListCommand(c.m_pCommandList.Key);
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandListClearStateCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  m_CommandListService.Command(c);
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandListExecuteBundleCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  m_CommandListService.Command(c);
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandListBeginQueryCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  m_CommandListService.Command(c);
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandListEndQueryCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  m_CommandListService.Command(c);
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandListResolveQueryDataCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  m_CommandListService.Command(c);
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandListSetPredicationCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  m_CommandListService.Command(c);
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandList1AtomicCopyBufferUINTCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  m_CommandListService.Command(c);
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandList1AtomicCopyBufferUINT64Command& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  m_CommandListService.Command(c);
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandList4BeginRenderPassCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  m_CommandListService.Command(c);
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandList5RSSetShadingRateImageCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  m_CommandListService.Command(c);
}

void AnalyzerLayer::Post(ID3D12GraphicsCommandList6DispatchMeshCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.CommandListCommand(c.m_Object.Key);
  m_CommandListService.Command(c);
}

void AnalyzerLayer::Post(IDMLBindingTableResetCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_desc.TableFields.CpuDescHandleKey);
  m_AnalyzerService.NotifyObject(c.m_desc.TableFields.GpuDescHandleKey);
}

void AnalyzerLayer::Post(IDMLBindingTableBindInputsCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObjects(c.m_bindings.ResourceKeys);
}

void AnalyzerLayer::Post(IDMLBindingTableBindOutputsCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObjects(c.m_bindings.ResourceKeys);
}

void AnalyzerLayer::Post(xessD3D12InitCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_hContext.Key);
  m_AnalyzerService.NotifyObject(c.m_pInitParams.Key);
  m_AnalyzerService.NotifyObject(c.m_pInitParams.TempBufferHeapKey);
  m_AnalyzerService.NotifyObject(c.m_pInitParams.TempTextureHeapKey);
  m_AnalyzerService.NotifyObject(c.m_pInitParams.PipelineLibraryKey);
}

void AnalyzerLayer::Post(xessD3D12GetInitParamsCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_hContext.Key);
  m_AnalyzerService.NotifyObject(c.m_pInitParams.Key);
  m_AnalyzerService.NotifyObject(c.m_pInitParams.TempBufferHeapKey);
  m_AnalyzerService.NotifyObject(c.m_pInitParams.TempTextureHeapKey);
  m_AnalyzerService.NotifyObject(c.m_pInitParams.PipelineLibraryKey);
}

void AnalyzerLayer::Post(xessD3D12ExecuteCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_hContext.Key);
  m_AnalyzerService.NotifyObject(c.m_pCommandList.Key);
  m_AnalyzerService.NotifyObject(c.m_pExecParams.ColorTextureKey);
  m_AnalyzerService.NotifyObject(c.m_pExecParams.VelocityTextureKey);
  m_AnalyzerService.NotifyObject(c.m_pExecParams.DepthTextureKey);
  m_AnalyzerService.NotifyObject(c.m_pExecParams.ExposureScaleTextureKey);
  m_AnalyzerService.NotifyObject(c.m_pExecParams.ResponsivePixelMaskTextureKey);
  m_AnalyzerService.NotifyObject(c.m_pExecParams.OutputTextureKey);
  m_AnalyzerService.NotifyObject(c.m_pExecParams.DescriptorHeapKey);
}

void AnalyzerLayer::Post(IDStorageQueueEnqueueRequestCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_request.FileKey);
  m_AnalyzerService.NotifyObject(c.m_request.ResourceKey);
}

void AnalyzerLayer::Post(INTC_D3D12_GetSupportedVersionsCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_pDevice.Key);
}

void AnalyzerLayer::Post(INTC_D3D12_CreateDeviceExtensionContextCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_pDevice.Key);
  m_AnalyzerService.NotifyObject(c.m_ppExtensionContext.Key);
  m_AnalyzerService.CreateDeviceExtensionContext(c);
  m_AnalyzerService.AddParent(c.m_ppExtensionContext.Key, c.m_pDevice.Key);
}

void AnalyzerLayer::Post(INTC_D3D12_CreateDeviceExtensionContext1Command& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_pDevice.Key);
  m_AnalyzerService.NotifyObject(c.m_ppExtensionContext.Key);
  m_AnalyzerService.CreateDeviceExtensionContext(c);
  m_AnalyzerService.AddParent(c.m_ppExtensionContext.Key, c.m_pDevice.Key);
}

void AnalyzerLayer::Post(INTC_DestroyDeviceExtensionContextCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_ppExtensionContext.Key);
}

void AnalyzerLayer::Post(INTC_D3D12_CheckFeatureSupportCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_pExtensionContext.Key);
}

void AnalyzerLayer::Post(INTC_D3D12_CreateCommandQueueCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_pExtensionContext.Key);
  m_AnalyzerService.NotifyObject(c.m_ppCommandQueue.Key);
  m_AnalyzerService.AddParent(c.m_ppCommandQueue.Key, c.m_pExtensionContext.Key);
}

void AnalyzerLayer::Post(INTC_D3D12_CreateReservedResourceCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_pExtensionContext.Key);
  m_AnalyzerService.NotifyObject(c.m_ppvResource.Key);
  m_AnalyzerService.AddParent(c.m_ppvResource.Key, c.m_pExtensionContext.Key);
  m_ResourceStateTracker.AddResource(c.m_ppvResource.Key, c.m_InitialState.Value);
}

void AnalyzerLayer::Post(INTC_D3D12_SetFeatureSupportCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_pExtensionContext.Key);
}

void AnalyzerLayer::Post(INTC_D3D12_GetResourceAllocationInfoCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_pExtensionContext.Key);
}

void AnalyzerLayer::Post(INTC_D3D12_CreateComputePipelineStateCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_pExtensionContext.Key);
  m_AnalyzerService.NotifyObject(c.m_ppPipelineState.Key);
  m_AnalyzerService.NotifyObject(c.m_pDesc.RootSignatureKey);
  m_AnalyzerService.AddParent(c.m_ppPipelineState.Key, c.m_pExtensionContext.Key);
  m_AnalyzerService.AddParent(c.m_ppPipelineState.Key, c.m_pDesc.RootSignatureKey);
}

void AnalyzerLayer::Post(INTC_D3D12_CreatePlacedResourceCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_pExtensionContext.Key);
  m_AnalyzerService.NotifyObject(c.m_pHeap.Key);
  m_AnalyzerService.NotifyObject(c.m_ppvResource.Key);
  m_AnalyzerService.AddParent(c.m_ppvResource.Key, c.m_pExtensionContext.Key);
  m_AnalyzerService.AddParent(c.m_ppvResource.Key, c.m_pHeap.Key);
  m_ResourceStateTracker.AddResource(c.m_ppvResource.Key, c.m_InitialState.Value);
}

void AnalyzerLayer::Post(INTC_D3D12_CreateCommittedResourceCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_pExtensionContext.Key);
  m_AnalyzerService.NotifyObject(c.m_ppvResource.Key);
  m_AnalyzerService.AddParent(c.m_ppvResource.Key, c.m_pExtensionContext.Key);
  m_ResourceStateTracker.AddResource(c.m_ppvResource.Key, c.m_InitialResourceState.Value);
}

void AnalyzerLayer::Post(INTC_D3D12_CreateHeapCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_pExtensionContext.Key);
  m_AnalyzerService.NotifyObject(c.m_ppvHeap.Key);
  m_AnalyzerService.AddParent(c.m_ppvHeap.Key, c.m_pExtensionContext.Key);
}

void AnalyzerLayer::Post(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_pDevice.Key);
}

void AnalyzerLayer::Post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_pDev.Key);
}

void AnalyzerLayer::Post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_pDev.Key);
}

void AnalyzerLayer::Post(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_pCommandList.Key);
}

void AnalyzerLayer::Post(D3D12CreateDeviceCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_pAdapter.Key);
  m_AnalyzerService.NotifyObject(c.m_ppDevice.Key);
  m_AnalyzerService.AddParent(c.m_ppDevice.Key, c.m_pAdapter.Key);
}

void AnalyzerLayer::Post(DMLCreateDeviceCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_d3d12Device.Key);
  m_AnalyzerService.NotifyObject(c.m_ppv.Key);
  m_AnalyzerService.AddParent(c.m_ppv.Key, c.m_d3d12Device.Key);
}

void AnalyzerLayer::Post(DMLCreateDevice1Command& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_d3d12Device.Key);
  m_AnalyzerService.NotifyObject(c.m_ppv.Key);
  m_AnalyzerService.AddParent(c.m_ppv.Key, c.m_d3d12Device.Key);
}

void AnalyzerLayer::Post(xessD3D12CreateContextCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_pDevice.Key);
  m_AnalyzerService.NotifyObject(c.m_phContext.Key);
  m_AnalyzerService.AddParent(c.m_phContext.Key, c.m_pDevice.Key);
  if (m_Optimize) {
    m_AnalyzerService.CreateXessContext(c);
  }
}

void AnalyzerLayer::Post(ID3D12DeviceCreateCommandQueueCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppCommandQueue.Key);
  m_AnalyzerService.AddParent(c.m_ppCommandQueue.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(ID3D12DeviceCreateCommandAllocatorCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppCommandAllocator.Key);
  m_AnalyzerService.AddParent(c.m_ppCommandAllocator.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(ID3D12DeviceCreateGraphicsPipelineStateCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppPipelineState.Key);
  m_AnalyzerService.NotifyObject(c.m_pDesc.RootSignatureKey);
  m_AnalyzerService.AddParent(c.m_ppPipelineState.Key, c.m_Object.Key);
  m_AnalyzerService.AddParent(c.m_ppPipelineState.Key, c.m_pDesc.RootSignatureKey);
}

void AnalyzerLayer::Post(ID3D12DeviceCreateComputePipelineStateCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppPipelineState.Key);
  m_AnalyzerService.NotifyObject(c.m_pDesc.RootSignatureKey);
  m_AnalyzerService.AddParent(c.m_ppPipelineState.Key, c.m_Object.Key);
  m_AnalyzerService.AddParent(c.m_ppPipelineState.Key, c.m_pDesc.RootSignatureKey);
}

void AnalyzerLayer::Post(ID3D12DeviceCreateCommandListCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_pCommandAllocator.Key);
  m_AnalyzerService.NotifyObject(c.m_pInitialState.Key);
  m_AnalyzerService.NotifyObject(c.m_ppCommandList.Key);
  m_AnalyzerService.AddParent(c.m_ppCommandList.Key, c.m_Object.Key);
  m_AnalyzerService.AddParent(c.m_ppCommandList.Key, c.m_pInitialState.Key);
  m_AnalyzerService.AddParent(c.m_ppCommandList.Key, c.m_pCommandAllocator.Key);
}

void AnalyzerLayer::Post(ID3D12DeviceCreateDescriptorHeapCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppvHeap.Key);
  m_AnalyzerService.AddParent(c.m_ppvHeap.Key, c.m_Object.Key);
  if (c.m_Result.Value != S_OK) {
    return;
  }
  if (m_Optimize) {
    m_CommandListService.CreateDescriptorHeap(c);
  }
}

void AnalyzerLayer::Post(ID3D12DeviceCreateRootSignatureCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppvRootSignature.Key);
  m_AnalyzerService.AddParent(c.m_ppvRootSignature.Key, c.m_Object.Key);
  if (c.m_Result.Value != S_OK) {
    return;
  }
  if (m_Optimize) {
    m_RootSignatureService.CreateRootSignature(c);
  }
}

void AnalyzerLayer::Post(ID3D12DeviceCreateCommittedResourceCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppvResource.Key);
  m_AnalyzerService.AddParent(c.m_ppvResource.Key, c.m_Object.Key);
  m_ResourceStateTracker.AddResource(c.m_ppvResource.Key, c.m_InitialResourceState.Value);
}

void AnalyzerLayer::Post(ID3D12DeviceCreateHeapCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppvHeap.Key);
  m_AnalyzerService.AddParent(c.m_ppvHeap.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(ID3D12DeviceCreatePlacedResourceCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_pHeap.Key);
  m_AnalyzerService.NotifyObject(c.m_ppvResource.Key);
  m_AnalyzerService.AddParent(c.m_ppvResource.Key, c.m_pHeap.Key);
  if (m_Optimize || m_OptimizeRaytracing) {
    if (c.m_Result.Value == S_OK) {
      m_ResourceStateTracker.AddResource(c.m_ppvResource.Key, c.m_InitialState.Value);
      m_GpuAddressService.CreatePlacedResource(c.m_pHeap.Key, c.m_ppvResource.Key,
                                               c.m_pDesc.Value->Flags);
    }
  }
}

void AnalyzerLayer::Post(ID3D12DeviceCreateReservedResourceCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppvResource.Key);
  m_AnalyzerService.AddParent(c.m_ppvResource.Key, c.m_Object.Key);
  m_ResourceStateTracker.AddResource(c.m_ppvResource.Key, c.m_InitialState.Value);
}

void AnalyzerLayer::Post(ID3D12DeviceCreateSharedHandleCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_pObject.Key);
  m_AnalyzerService.AddParent(c.m_pObject.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(ID3D12DeviceOpenSharedHandleCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppvObj.Key);
  m_AnalyzerService.AddParent(c.m_ppvObj.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(ID3D12DeviceCreateFenceCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppFence.Key);
  m_AnalyzerService.AddParent(c.m_ppFence.Key, c.m_Object.Key);
  if (c.m_Result.Value != S_OK) {
    return;
  }
  m_AnalyzerService.FenceSignal(c.Key, c.m_ppFence.Key, c.m_InitialValue.Value);
  if (m_Optimize || m_OptimizeRaytracing) {
    m_RaytracingService.FenceSignal(c.Key, c.m_ppFence.Key, c.m_InitialValue.Value);
  }
}

void AnalyzerLayer::Post(ID3D12DeviceCreateQueryHeapCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppvHeap.Key);
  m_AnalyzerService.AddParent(c.m_ppvHeap.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(ID3D12DeviceCreateCommandSignatureCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppvCommandSignature.Key);
  m_AnalyzerService.AddParent(c.m_ppvCommandSignature.Key, c.m_Object.Key);
  if (c.m_Result.Value != S_OK) {
    return;
  }
  if (m_Optimize) {
    m_ExecuteIndirectService.CreateCommandSignature(c);
    m_CommandListService.CreateCommandSignature(c);
  }
}

void AnalyzerLayer::Post(ID3D12Device1CreatePipelineLibraryCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppPipelineLibrary.Key);
}

void AnalyzerLayer::Post(ID3D12Device2CreatePipelineStateCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppPipelineState.Key);
  m_AnalyzerService.NotifyObject(c.m_pDesc.RootSignatureKey);
  m_AnalyzerService.AddParent(c.m_ppPipelineState.Key, c.m_Object.Key);
  m_AnalyzerService.AddParent(c.m_ppPipelineState.Key, c.m_pDesc.RootSignatureKey);
}

void AnalyzerLayer::Post(ID3D12Device4CreateCommandList1Command& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppCommandList.Key);
  m_AnalyzerService.AddParent(c.m_ppCommandList.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(ID3D12Device4CreateProtectedResourceSessionCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppSession.Key);
  m_AnalyzerService.AddParent(c.m_ppSession.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(ID3D12Device4CreateCommittedResource1Command& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppvResource.Key);
  m_AnalyzerService.AddParent(c.m_ppvResource.Key, c.m_Object.Key);
  m_ResourceStateTracker.AddResource(c.m_ppvResource.Key, c.m_InitialResourceState.Value);
}

void AnalyzerLayer::Post(ID3D12Device4CreateHeap1Command& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_pProtectedSession.Key);
  m_AnalyzerService.NotifyObject(c.m_ppvHeap.Key);
  m_AnalyzerService.AddParent(c.m_ppvHeap.Key, c.m_Object.Key);
  m_AnalyzerService.AddParent(c.m_ppvHeap.Key, c.m_pProtectedSession.Key);
}

void AnalyzerLayer::Post(ID3D12Device4CreateReservedResource1Command& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_pProtectedSession.Key);
  m_AnalyzerService.NotifyObject(c.m_ppvResource.Key);
  m_AnalyzerService.AddParent(c.m_ppvResource.Key, c.m_Object.Key);
  m_AnalyzerService.AddParent(c.m_ppvResource.Key, c.m_pProtectedSession.Key);
  m_ResourceStateTracker.AddResource(c.m_ppvResource.Key, c.m_InitialState.Value);
}

void AnalyzerLayer::Post(ID3D12Device5CreateLifetimeTrackerCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_pOwner.Key);
  m_AnalyzerService.NotifyObject(c.m_ppvTracker.Key);
  m_AnalyzerService.AddParent(c.m_ppvTracker.Key, c.m_Object.Key);
  m_AnalyzerService.AddParent(c.m_ppvTracker.Key, c.m_pOwner.Key);
}

void AnalyzerLayer::Post(ID3D12Device5CreateMetaCommandCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppMetaCommand.Key);
  m_AnalyzerService.AddParent(c.m_ppMetaCommand.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(ID3D12Device5CreateStateObjectCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppStateObject.Key);
  m_AnalyzerService.AddParent(c.m_ppStateObject.Key, c.m_Object.Key);
  for (auto& it : c.m_pDesc.InterfaceKeysBySubobject) {
    m_AnalyzerService.NotifyObject(it.second);
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  if (m_Optimize) {
    m_RaytracingService.CreateStateObject(c);
  }
}

void AnalyzerLayer::Post(ID3D12Device5GetRaytracingAccelerationStructurePrebuildInfoCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  if (m_Optimize) {
    for (unsigned key : c.m_pDesc.InputKeys) {
      m_AnalyzerService.NotifyObject(key);
    }
  }
}

void AnalyzerLayer::Post(ID3D12Device7CreateProtectedResourceSession1Command& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppSession.Key);
  m_AnalyzerService.AddParent(c.m_ppSession.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(ID3D12Device8CreateCommittedResource2Command& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppvResource.Key);
  m_AnalyzerService.AddParent(c.m_ppvResource.Key, c.m_Object.Key);
  m_ResourceStateTracker.AddResource(c.m_ppvResource.Key, c.m_InitialResourceState.Value);
}

void AnalyzerLayer::Post(ID3D12Device8CreatePlacedResource1Command& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_pHeap.Key);
  m_AnalyzerService.NotifyObject(c.m_ppvResource.Key);
  m_AnalyzerService.AddParent(c.m_ppvResource.Key, c.m_pHeap.Key);
  if (m_Optimize || m_OptimizeRaytracing) {
    if (c.m_Result.Value == S_OK) {
      m_ResourceStateTracker.AddResource(c.m_ppvResource.Key, c.m_InitialState.Value);
      m_GpuAddressService.CreatePlacedResource(c.m_pHeap.Key, c.m_ppvResource.Key,
                                               c.m_pDesc.Value->Flags);
    }
  }
}

void AnalyzerLayer::Post(ID3D12Device9CreateShaderCacheSessionCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppvSession.Key);
  m_AnalyzerService.AddParent(c.m_ppvSession.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(ID3D12Device9CreateCommandQueue1Command& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppCommandQueue.Key);
  m_AnalyzerService.AddParent(c.m_ppCommandQueue.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(ID3D12Device10CreateCommittedResource3Command& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppvResource.Key);
  m_AnalyzerService.AddParent(c.m_ppvResource.Key, c.m_Object.Key);
  m_ResourceStateTracker.AddResource(c.m_ppvResource.Key, c.m_InitialLayout.Value);
}

void AnalyzerLayer::Post(ID3D12Device10CreatePlacedResource2Command& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_pHeap.Key);
  m_AnalyzerService.NotifyObject(c.m_ppvResource.Key);
  m_AnalyzerService.AddParent(c.m_pHeap.Key, c.m_pHeap.Key);
  if (m_Optimize || m_OptimizeRaytracing) {
    if (c.m_Result.Value == S_OK) {
      m_ResourceStateTracker.AddResource(c.m_ppvResource.Key, c.m_InitialLayout.Value);
      m_GpuAddressService.CreatePlacedResource(c.m_pHeap.Key, c.m_ppvResource.Key,
                                               c.m_pDesc.Value->Flags);
    }
  }
}

void AnalyzerLayer::Post(ID3D12Device10CreateReservedResource2Command& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_pProtectedSession.Key);
  m_AnalyzerService.NotifyObject(c.m_ppvResource.Key);
  m_AnalyzerService.AddParent(c.m_ppvResource.Key, c.m_Object.Key);
  m_AnalyzerService.AddParent(c.m_ppvResource.Key, c.m_pProtectedSession.Key);
  m_ResourceStateTracker.AddResource(c.m_ppvResource.Key, c.m_InitialLayout.Value);
}

void AnalyzerLayer::Post(ID3D12Device14CreateRootSignatureFromSubobjectInLibraryCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppvRootSignature.Key);
  m_AnalyzerService.AddParent(c.m_ppvRootSignature.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(ID3D12SDKConfiguration1CreateDeviceFactoryCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppvFactory.Key);
  m_AnalyzerService.AddParent(c.m_ppvFactory.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(ID3D12DSRDeviceFactoryCreateDSRDeviceCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_pD3D12Device.Key);
  m_AnalyzerService.NotifyObject(c.m_ppvDSRDevice.Key);
  m_AnalyzerService.AddParent(c.m_ppvDSRDevice.Key, c.m_Object.Key);
  m_AnalyzerService.AddParent(c.m_ppvDSRDevice.Key, c.m_pD3D12Device.Key);
}

void AnalyzerLayer::Post(IDXGIFactoryCreateSoftwareAdapterCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppAdapter.Key);
  m_AnalyzerService.AddParent(c.m_ppAdapter.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(IDXGIDeviceCreateSurfaceCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppSurface.Key);
  m_AnalyzerService.AddParent(c.m_ppSurface.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(IDXGIResource1CreateSubresourceSurfaceCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppSurface.Key);
  m_AnalyzerService.AddParent(c.m_ppSurface.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(IDXGIFactory2CreateSwapChainForHwndCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_pDevice.Key);
  m_AnalyzerService.NotifyObject(c.m_pRestrictToOutput.Key);
  m_AnalyzerService.NotifyObject(c.m_ppSwapChain.Key);
  m_AnalyzerService.AddParent(c.m_ppSwapChain.Key, c.m_Object.Key);
  m_AnalyzerService.AddParent(c.m_ppSwapChain.Key, c.m_pDevice.Key);
  m_AnalyzerService.AddParent(c.m_ppSwapChain.Key, c.m_pRestrictToOutput.Key);
}

void AnalyzerLayer::Post(IDXGIFactory2CreateSwapChainForCoreWindowCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_pDevice.Key);
  m_AnalyzerService.NotifyObject(c.m_pWindow.Key);
  m_AnalyzerService.NotifyObject(c.m_pRestrictToOutput.Key);
  m_AnalyzerService.NotifyObject(c.m_ppSwapChain.Key);
  m_AnalyzerService.AddParent(c.m_ppSwapChain.Key, c.m_Object.Key);
  m_AnalyzerService.AddParent(c.m_ppSwapChain.Key, c.m_pDevice.Key);
  m_AnalyzerService.AddParent(c.m_ppSwapChain.Key, c.m_pWindow.Key);
  m_AnalyzerService.AddParent(c.m_ppSwapChain.Key, c.m_pRestrictToOutput.Key);
}

void AnalyzerLayer::Post(IDXGIFactory2CreateSwapChainForCompositionCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_pDevice.Key);
  m_AnalyzerService.NotifyObject(c.m_pRestrictToOutput.Key);
  m_AnalyzerService.NotifyObject(c.m_ppSwapChain.Key);
  m_AnalyzerService.AddParent(c.m_ppSwapChain.Key, c.m_Object.Key);
  m_AnalyzerService.AddParent(c.m_ppSwapChain.Key, c.m_pDevice.Key);
  m_AnalyzerService.AddParent(c.m_ppSwapChain.Key, c.m_pRestrictToOutput.Key);
}

void AnalyzerLayer::Post(IDXGIFactoryMediaCreateSwapChainForCompositionSurfaceHandleCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_pDevice.Key);
  m_AnalyzerService.NotifyObject(c.m_pRestrictToOutput.Key);
  m_AnalyzerService.NotifyObject(c.m_ppSwapChain.Key);
  m_AnalyzerService.AddParent(c.m_ppSwapChain.Key, c.m_Object.Key);
  m_AnalyzerService.AddParent(c.m_ppSwapChain.Key, c.m_pDevice.Key);
  m_AnalyzerService.AddParent(c.m_ppSwapChain.Key, c.m_pRestrictToOutput.Key);
}

void AnalyzerLayer::Post(
    IDXGIFactoryMediaCreateDecodeSwapChainForCompositionSurfaceHandleCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_pDevice.Key);
  m_AnalyzerService.NotifyObject(c.m_pYuvDecodeBuffers.Key);
  m_AnalyzerService.NotifyObject(c.m_pRestrictToOutput.Key);
  m_AnalyzerService.NotifyObject(c.m_ppSwapChain.Key);
  m_AnalyzerService.AddParent(c.m_ppSwapChain.Key, c.m_Object.Key);
  m_AnalyzerService.AddParent(c.m_ppSwapChain.Key, c.m_pDevice.Key);
  m_AnalyzerService.AddParent(c.m_ppSwapChain.Key, c.m_pYuvDecodeBuffers.Key);
  m_AnalyzerService.AddParent(c.m_ppSwapChain.Key, c.m_pRestrictToOutput.Key);
}

void AnalyzerLayer::Post(IDMLDeviceCreateOperatorCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppv.Key);
  m_AnalyzerService.AddParent(c.m_ppv.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(IDMLDeviceCreateOperatorInitializerCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObjects(c.m_operators.Keys);
  m_AnalyzerService.NotifyObject(c.m_ppv.Key);
  m_AnalyzerService.AddParent(c.m_ppv.Key, c.m_Object.Key);
  for (unsigned key : c.m_operators.Keys) {
    m_AnalyzerService.AddParent(c.m_ppv.Key, key);
  }
}

void AnalyzerLayer::Post(IDMLDeviceCreateCommandRecorderCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppv.Key);
  m_AnalyzerService.AddParent(c.m_ppv.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(IDMLDeviceCreateBindingTableCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppv.Key);
  m_AnalyzerService.NotifyObject(c.m_desc.TableFields.CpuDescHandleKey);
  m_AnalyzerService.NotifyObject(c.m_desc.TableFields.GpuDescHandleKey);
  m_AnalyzerService.AddParent(c.m_ppv.Key, c.m_Object.Key);
  m_AnalyzerService.AddParent(c.m_ppv.Key, c.m_desc.TableFields.CpuDescHandleKey);
  m_AnalyzerService.AddParent(c.m_ppv.Key, c.m_desc.TableFields.GpuDescHandleKey);
}

void AnalyzerLayer::Post(IDStorageFactoryCreateQueueCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppv.Key);
  m_AnalyzerService.NotifyObject(c.m_desc.DeviceKey);
  m_AnalyzerService.AddParent(c.m_ppv.Key, c.m_Object.Key);
  m_AnalyzerService.AddParent(c.m_ppv.Key, c.m_desc.DeviceKey);
}

void AnalyzerLayer::Post(IDStorageFactoryCreateStatusArrayCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppv.Key);
  m_AnalyzerService.AddParent(c.m_ppv.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(ID3D12PipelineStateGetCachedBlobCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppBlob.Key);
  m_AnalyzerService.AddParent(c.m_ppBlob.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(ID3D12PipelineLibraryLoadGraphicsPipelineCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppPipelineState.Key);
  m_AnalyzerService.NotifyObject(c.m_pDesc.RootSignatureKey);
  m_AnalyzerService.AddParent(c.m_ppPipelineState.Key, c.m_Object.Key);
  m_AnalyzerService.AddParent(c.m_ppPipelineState.Key, c.m_pDesc.RootSignatureKey);
}

void AnalyzerLayer::Post(ID3D12PipelineLibraryLoadComputePipelineCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppPipelineState.Key);
  m_AnalyzerService.NotifyObject(c.m_pDesc.RootSignatureKey);
  m_AnalyzerService.AddParent(c.m_ppPipelineState.Key, c.m_Object.Key);
  m_AnalyzerService.AddParent(c.m_ppPipelineState.Key, c.m_pDesc.RootSignatureKey);
}

void AnalyzerLayer::Post(ID3D12PipelineLibrary1LoadPipelineCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppPipelineState.Key);
  m_AnalyzerService.NotifyObject(c.m_pDesc.RootSignatureKey);
  m_AnalyzerService.AddParent(c.m_ppPipelineState.Key, c.m_Object.Key);
  m_AnalyzerService.AddParent(c.m_ppPipelineState.Key, c.m_pDesc.RootSignatureKey);
}

void AnalyzerLayer::Post(ID3D12Device3OpenExistingHeapFromAddressCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppvHeap.Key);
  m_AnalyzerService.AddParent(c.m_ppvHeap.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(ID3D12Device3OpenExistingHeapFromFileMappingCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppvHeap.Key);
  m_AnalyzerService.AddParent(c.m_ppvHeap.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(ID3D12ProtectedSessionGetStatusFenceCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppFence.Key);
  m_AnalyzerService.AddParent(c.m_ppFence.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(ID3D12SwapChainAssistantGetSwapChainObjectCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppv.Key);
  m_AnalyzerService.AddParent(c.m_ppv.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(ID3D12SwapChainAssistantGetCurrentResourceAndCommandQueueCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppvResource.Key);
  m_AnalyzerService.NotifyObject(c.m_ppvQueue.Key);
  m_AnalyzerService.AddParent(c.m_ppvResource.Key, c.m_Object.Key);
  m_AnalyzerService.AddParent(c.m_ppvQueue.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(ID3D12Device7AddToStateObjectCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_pStateObjectToGrowFrom.Key);
  m_AnalyzerService.NotifyObject(c.m_ppNewStateObject.Key);
  m_AnalyzerService.AddParent(c.m_ppNewStateObject.Key, c.m_Object.Key);
  m_AnalyzerService.AddParent(c.m_ppNewStateObject.Key, c.m_pStateObjectToGrowFrom.Key);
  for (auto& it : c.m_pAddition.InterfaceKeysBySubobject) {
    m_AnalyzerService.NotifyObject(it.second);
  }
  if (c.m_Result.Value != S_OK) {
    return;
  }
  if (m_Optimize) {
    m_RaytracingService.AddToStateObject(c);
  }
}

void AnalyzerLayer::Post(ID3D12Device13OpenExistingHeapFromAddress1Command& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppvHeap.Key);
  m_AnalyzerService.AddParent(c.m_ppvHeap.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(ID3D12DeviceTools1GetApplicationSpecificDriverStateCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppBlob.Key);
  m_AnalyzerService.AddParent(c.m_ppBlob.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(ID3D12DeviceFactoryGetConfigurationInterfaceCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppv.Key);
  m_AnalyzerService.AddParent(c.m_ppv.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(ID3D12DeviceConfigurationSerializeVersionedRootSignatureCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppResult.Key);
  m_AnalyzerService.NotifyObject(c.m_ppError.Key);
  m_AnalyzerService.AddParent(c.m_ppResult.Key, c.m_Object.Key);
  m_AnalyzerService.AddParent(c.m_ppError.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(
    ID3D12DeviceConfigurationCreateVersionedRootSignatureDeserializerCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppvDeserializer.Key);
  m_AnalyzerService.AddParent(c.m_ppvDeserializer.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(
    ID3D12DeviceConfiguration1CreateVersionedRootSignatureDeserializerFromSubobjectInLibraryCommand&
        c) {
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppvDeserializer.Key);
  m_AnalyzerService.AddParent(c.m_ppvDeserializer.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(IDXGIAdapterEnumOutputsCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppOutput.Key);
  m_AnalyzerService.AddParent(c.m_ppOutput.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(IDXGISwapChainGetBufferCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppSurface.Key);
  m_AnalyzerService.AddParent(c.m_ppSurface.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(IDXGISwapChainGetFullscreenStateCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppTarget.Key);
  m_AnalyzerService.AddParent(c.m_ppTarget.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(IDXGISwapChainGetContainingOutputCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppOutput.Key);
  m_AnalyzerService.AddParent(c.m_ppOutput.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(IDXGIFactoryEnumAdaptersCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppAdapter.Key);
  m_AnalyzerService.AddParent(c.m_ppAdapter.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(IDXGIFactory1EnumAdapters1Command& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppAdapter.Key);
  m_AnalyzerService.AddParent(c.m_ppAdapter.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(IDXGIOutputDuplicationAcquireNextFrameCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppDesktopResource.Key);
  m_AnalyzerService.AddParent(c.m_ppDesktopResource.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(IDXGISurface2GetResourceCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppParentResource.Key);
  m_AnalyzerService.AddParent(c.m_ppParentResource.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(IDXGISwapChain1GetCoreWindowCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppUnk.Key);
  m_AnalyzerService.AddParent(c.m_ppUnk.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(IDXGISwapChain1GetRestrictToOutputCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppRestrictToOutput.Key);
  m_AnalyzerService.AddParent(c.m_ppRestrictToOutput.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(IDXGIOutput1DuplicateOutputCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_pDevice.Key);
  m_AnalyzerService.NotifyObject(c.m_ppOutputDuplication.Key);
  m_AnalyzerService.AddParent(c.m_ppOutputDuplication.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(IDXGIFactory4EnumAdapterByLuidCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppvAdapter.Key);
  m_AnalyzerService.AddParent(c.m_ppvAdapter.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(IDXGIFactory4EnumWarpAdapterCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppvAdapter.Key);
  m_AnalyzerService.AddParent(c.m_ppvAdapter.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(IDXGIOutput5DuplicateOutput1Command& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_pDevice.Key);
  m_AnalyzerService.NotifyObject(c.m_ppOutputDuplication.Key);
  m_AnalyzerService.AddParent(c.m_ppOutputDuplication.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(IDXGIFactory6EnumAdapterByGpuPreferenceCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppvAdapter.Key);
  m_AnalyzerService.AddParent(c.m_ppvAdapter.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(IDMLDeviceCompileOperatorCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_op.Key);
  m_AnalyzerService.NotifyObject(c.m_ppv.Key);
  m_AnalyzerService.AddParent(c.m_ppv.Key, c.m_Object.Key);
  m_AnalyzerService.AddParent(c.m_ppv.Key, c.m_op.Key);
}

void AnalyzerLayer::Post(IDMLDevice1CompileGraphCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppv.Key);
  m_AnalyzerService.NotifyObjects(c.m_desc.OperatorKeys);
  m_AnalyzerService.AddParent(c.m_ppv.Key, c.m_Object.Key);
  for (unsigned key : c.m_desc.OperatorKeys) {
    m_AnalyzerService.AddParent(c.m_ppv.Key, key);
  }
}

void AnalyzerLayer::Post(IDStorageFactoryOpenFileCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_Object.Key);
  m_AnalyzerService.NotifyObject(c.m_ppv.Key);
  m_AnalyzerService.AddParent(c.m_ppv.Key, c.m_Object.Key);
}

void AnalyzerLayer::Post(xellD3D12CreateContextCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_device.Key);
  m_AnalyzerService.NotifyObject(c.m_out_context.Key);
  m_AnalyzerService.AddParent(c.m_out_context.Key, c.m_device.Key);
  if (m_Optimize) {
    m_AnalyzerService.CreateXellContext(c);
  }
}

void AnalyzerLayer::Post(xefgSwapChainD3D12CreateContextCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_pDevice.Key);
  m_AnalyzerService.NotifyObject(c.m_phSwapChain.Key);
  m_AnalyzerService.AddParent(c.m_phSwapChain.Key, c.m_pDevice.Key);
  if (m_Optimize) {
    m_AnalyzerService.CreateXefgContext(c);
  }
}

void AnalyzerLayer::Post(xefgSwapChainD3D12InitFromSwapChainCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_hSwapChain.Key);
  m_AnalyzerService.NotifyObject(c.m_pCmdQueue.Key);
  m_AnalyzerService.NotifyObject(c.m_pInitParams.Key);
  // Original swapChain is necessary for proper xefg swapChain initialization.
  m_AnalyzerService.ForceApplicationSwapChainRestore(c.m_pInitParams.ApplicationSwapChainKey);
  m_AnalyzerService.NotifyObject(c.m_pInitParams.TempBufferHeapKey);
  m_AnalyzerService.NotifyObject(c.m_pInitParams.TempTextureHeapKey);
  m_AnalyzerService.NotifyObject(c.m_pInitParams.PipelineLibraryKey);
}

void AnalyzerLayer::Post(xefgSwapChainD3D12InitFromSwapChainDescCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_hSwapChain.Key);
  m_AnalyzerService.NotifyObject(c.m_pCmdQueue.Key);
  m_AnalyzerService.NotifyObject(c.m_pDxgiFactory.Key);
  m_AnalyzerService.NotifyObject(c.m_pInitParams.Key);
  m_AnalyzerService.NotifyObject(c.m_pInitParams.ApplicationSwapChainKey);
  m_AnalyzerService.NotifyObject(c.m_pInitParams.TempBufferHeapKey);
  m_AnalyzerService.NotifyObject(c.m_pInitParams.TempTextureHeapKey);
  m_AnalyzerService.NotifyObject(c.m_pInitParams.PipelineLibraryKey);
}

void AnalyzerLayer::Post(xefgSwapChainD3D12GetSwapChainPtrCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_hSwapChain.Key);
  m_AnalyzerService.NotifyObject(c.m_ppSwapChain.Key);
}

void AnalyzerLayer::Post(xefgSwapChainGetPropertiesCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_hSwapChain.Key);
}

void AnalyzerLayer::Post(xefgSwapChainTagFrameConstantsCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_hSwapChain.Key);
}

void AnalyzerLayer::Post(xefgSwapChainSetEnabledCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_hSwapChain.Key);
}

void AnalyzerLayer::Post(xefgSwapChainSetPresentIdCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_hSwapChain.Key);
}

void AnalyzerLayer::Post(xefgSwapChainGetLastPresentStatusCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_hSwapChain.Key);
}

void AnalyzerLayer::Post(xefgSwapChainSetLoggingCallbackCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_hSwapChain.Key);
}

void AnalyzerLayer::Post(xefgSwapChainDestroyCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_hSwapChain.Key);
}

void AnalyzerLayer::Post(xefgSwapChainSetLatencyReductionCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_hSwapChain.Key);
}

void AnalyzerLayer::Post(xefgSwapChainSetSceneChangeThresholdCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_hSwapChain.Key);
}

void AnalyzerLayer::Post(xefgSwapChainGetPipelineBuildStatusCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_hSwapChain.Key);
}

void AnalyzerLayer::Post(xefgSwapChainEnableDebugFeatureCommand& c) {
  if (m_AnalyzerService.AfterRange()) {
    return;
  }
  m_AnalyzerService.NotifyObject(c.m_hSwapChain.Key);
}

} // namespace DirectX
} // namespace gits
